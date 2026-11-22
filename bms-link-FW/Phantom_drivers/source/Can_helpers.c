/*
 * Can_helpers.c
 *
 *  Replaces Phantom_Can.c
 */

#include <string.h>
#include "can.h"
#include "reg_can.h"
#include "PhantomAlgorithms.h"
#include "PhantomTimers.h"
#include "Can_helpers.h"

#ifndef CAN_NODE
#define CAN_NODE                canREG1
#endif
#define CAN_TX_TIMEOUT_TICKS    MS_TO_TICKS(CAN_TX_TIMEOUT_MS)
#define CAN_RX_GAP_TICKS        MS_TO_TICKS(CAN_RX_GAP_MS)

//---------------------------------------------------------------------------------------------------------
// Box table, one row per CanBox_t in the same order as the enum.
//   hw     HALCoGen message box number
//   tx     true = we send on it, false = we receive on it
//   dlc    bytes per frame, keep equal to the box in HALCoGen (longer data is cut, shorter is padded)
//   frames frames in one message (1 = plain message)
//   reply  TX box that can_reply() uses for this RX box
//---------------------------------------------------------------------------------------------------------
#pragma WEAK(BMS2ALL_FAULT_FullRoutine)
void BMS2ALL_FAULT_FullRoutine(void)        { /* placeholder */ }
#pragma WEAK(BMS2VCU_Data_FullRoutine)
void BMS2VCU_Data_FullRoutine(void)         { /* placeholder */ }
#pragma WEAK(VCU2BMS_DATA_FullRoutine)
void VCU2BMS_DATA_FullRoutine(void)         { /* placeholder */ }
#pragma WEAK(BMS2CHARGER_DATA_FullRoutine)
void BMS2CHARGER_DATA_FullRoutine(void)     { /* placeholder */ }
#pragma WEAK(CHARGER2BMS_DATA_FullRoutine)
void CHARGER2BMS_DATA_FullRoutine(void)     { /* placeholder */ }

typedef struct {
    uint32_t   hw;
    bool       tx;
    uint8_t    dlc;
    uint8_t    frames;
    CanBox_t   reply;
    VoidFunc_t onDone;
} CanBoxDef_t;

static const CanBoxDef_t canBoxes[CAN_BOX_COUNT] = {
    // hw box            tx     dlc  frames  reply box          callback
    { canMESSAGE_BOX1,   true,  8U,  1U,     CAN_NO_BOX,        BMS2ALL_FAULT_FullRoutine    },
    { canMESSAGE_BOX3,   true,  8U,  1U,     CAN_NO_BOX,        BMS2VCU_Data_FullRoutine     },
    { canMESSAGE_BOX4,   false, 8U,  1U,     BMS2VCU_DATA,      VCU2BMS_DATA_FullRoutine     },
    { canMESSAGE_BOX6,   true,  8U,  1U,     CAN_NO_BOX,        BMS2CHARGER_DATA_FullRoutine },
    { canMESSAGE_BOX7,   false, 8U,  1U,     BMS2CHARGER_DATA,  CHARGER2BMS_DATA_FullRoutine }
};

#define BOX_COUNT   ((size_t)CAN_BOX_COUNT)

//---------------------------------------------------------------------------------------------------------
// State
//---------------------------------------------------------------------------------------------------------
typedef struct {
    volatile bool     busOff;
    volatile uint32_t level;
} CanBus_t;

typedef struct {
    uint8_t       data[CAN_MAX_FRAMES * CAN_MAX_DLC];
    size_t        bytes;            // size of the finished message
    ticks_t       last;             // when the last frame came in
    uint8_t       frame;            // frames collected so far
    volatile bool ready;            // finished, waiting for can_receive_multi
    volatile bool lost;             // something was dropped, reported by the next read
} CanRxMsg_t;

static CanBus_t   bus = { false, 0U };
static CanRxMsg_t rxMsg[CAN_BOX_COUNT];

//---------------------------------------------------------------------------------------------------------
// Hardware wrapper: the only code that calls can.h or touches the CAN registers
//---------------------------------------------------------------------------------------------------------
void canHw_init(void)
{
    canInit();
    canEnableErrorNotification(CAN_NODE);       // bus-off, warning and parity interrupts
}

bool canHw_isBoxValid(const uint32_t hwBox)
{
    return canIsMessageBoxValid(CAN_NODE, hwBox) != 0U;
}

// 1 = request set, 0 = box still sending. It sends the dlc set in HALCoGen.
bool canHw_tx(const uint32_t hwBox, const uint8_t frame[CAN_MAX_DLC])
{
    return canTransmit(CAN_NODE, hwBox, frame) != 0U;
}

// 0 = nothing new, 1 = ok, 3 = ok but an earlier frame was lost
CanStatus_t canHw_rx(const uint32_t hwBox, uint8_t frame[CAN_MAX_DLC])
{
    switch (canGetData(CAN_NODE, hwBox, frame)) {
        case 1U:  return CAN_OK;
        case 3U:  return CAN_ERR_OVERRUN;
        default:  return CAN_ERR_NO_DATA;
    }
}

bool canHw_remote(const uint32_t hwBox)
{
    return canSendRemoteFrame(CAN_NODE, hwBox) != 0U;
}

// Bus-off leaves the node in Init. Clearing Init starts the recovery (129 x 11 recessive bits)
void canHw_restart(void)
{
    CAN_NODE->CTL &= ~(uint32)1U;
}

uint32_t canHw_lastError(void)
{
    return canGetLastError(CAN_NODE);
}

// HALCoGen callbacks
void canMessageNotification(canBASE_t *node, uint32 messageBox)
{
    if (node == CAN_NODE) { can_handle_message(messageBox); }
}

void canErrorNotification(canBASE_t *node, uint32 notification)
{
    if (node == CAN_NODE) { can_handle_error(notification); }
}

//---------------------------------------------------------------------------------------------------------
// Helpers
//---------------------------------------------------------------------------------------------------------
static bool box_ok(const CanBox_t box)
{
    return (unsigned int)box < (unsigned int)CAN_BOX_COUNT;
}

static CanBox_t findBox(const uint32_t hwBox)
{
    size_t i;

    for (i = 0U; i < BOX_COUNT; i++) {
        if (canBoxes[i].hw == hwBox) { return (CanBox_t)i; }
    }
    return CAN_NO_BOX;
}

static CanStatus_t check(const CanBox_t box, const bool wantTx, const void * const data, const size_t len)
{
    if (!box_ok(box))                           { return CAN_ERR_BAD_BOX; }
    if (data == NULL && len != 0U)              { return CAN_ERR_NULL; }
    if (canBoxes[box].tx != wantTx)             { return CAN_ERR_WRONG_DIR; }
    if (!canHw_isBoxValid(canBoxes[box].hw))    { return CAN_ERR_NO_CFG; }
    if (wantTx && bus.busOff)                   { return CAN_ERR_BUS_OFF; }
    return CAN_OK;
}

// too long: cut, too short: 0
static void packFrame(uint8_t frame[CAN_MAX_DLC], const uint8_t * const src, const size_t len, const uint8_t dlc)
{
    const size_t n = MIN_VAL(len, (size_t)dlc);

    memset(frame, 0, CAN_MAX_DLC);
    if (n > 0U) { memcpy(frame, src, n); }
}

static void copyOut(uint8_t * const dst, const size_t len, const uint8_t * const src, const size_t srcLen)
{
    const size_t n = MIN_VAL(len, srcLen);

    if (len == 0U) { return; }
    memset(dst, 0, len);
    if (n > 0U) { memcpy(dst, src, n); }
}

static CanStatus_t txFrame(const CanBox_t box, const uint8_t frame[CAN_MAX_DLC])
{
    return canHw_tx(canBoxes[box].hw, frame) ? CAN_OK : CAN_ERR_BUSY;
}

static CanStatus_t txFrameWait(const CanBox_t box, const uint8_t frame[CAN_MAX_DLC])
{
    Timeout_t t;
    timeout_start(&t, CAN_TX_TIMEOUT_TICKS);

    while (txFrame(box, frame) == CAN_ERR_BUSY) {
        if (bus.busOff)             { return CAN_ERR_BUS_OFF; }
        if (timeout_expired(&t))    { return CAN_ERR_TIMEOUT; }
    }
    return CAN_OK;
}

// Takes one frame from the hardware. True when the whole message is ready.
static bool rxCollect(const CanBox_t box)
{
    CanRxMsg_t * const m = &rxMsg[box];
    const size_t dlc     = canBoxes[box].dlc;
    const size_t frames  = MIN_VAL((size_t)canBoxes[box].frames, (size_t)CAN_MAX_FRAMES);
    uint8_t frame[CAN_MAX_DLC];
    const CanStatus_t status = canHw_rx(canBoxes[box].hw, frame);

    if (status != CAN_OK && status != CAN_ERR_OVERRUN) { return false; }

    if (m->ready) { m->lost = true; return false; }     // last message not read yet, drop this one

    if (status == CAN_ERR_OVERRUN) {
        m->lost = true;
        if (frames > 1U) { m->frame = 0U; return false; }   // a frame is missing, start over
    }
    if (m->frame != 0U && hasTimeElapsed(m->last, CAN_RX_GAP_TICKS)) { m->frame = 0U; }

    memcpy(&m->data[m->frame * dlc], frame, dlc);
    m->last = getNow_tick();
    m->frame++;
    if (m->frame < frames) { return false; }

    m->frame = 0U;
    m->bytes = frames * dlc;
    m->ready = true;
    return true;
}

//---------------------------------------------------------------------------------------------------------
// Setup and bus state
//---------------------------------------------------------------------------------------------------------
void can_init(void)
{
    bus.busOff = false;
    bus.level  = 0U;
    memset(rxMsg, 0, sizeof(rxMsg));
    canHw_init();
}

// Bus-off is flagged by the error interrupt, this starts the recovery
void can_service(void)
{
    if (bus.busOff) {
        bus.busOff = false;
        canHw_restart();
    }
}

bool     can_isBusOff(void)     { return bus.busOff; }
uint32_t can_errorLevel(void)   { return bus.level; }
uint32_t can_lastError(void)    { return canHw_lastError(); }

//---------------------------------------------------------------------------------------------------------
// Send and receive
//---------------------------------------------------------------------------------------------------------
CanStatus_t can_send(const CanBox_t box, const uint8_t * const data, const size_t len)
{
    uint8_t frame[CAN_MAX_DLC];
    const CanStatus_t status = check(box, true, data, len);

    if (status != CAN_OK) { return status; }
    packFrame(frame, data, len, canBoxes[box].dlc);
    return txFrame(box, frame);
}

CanStatus_t can_receive(const CanBox_t box, uint8_t * const data, const size_t len)
{
    uint8_t frame[CAN_MAX_DLC];
    CanStatus_t status = check(box, false, data, len);

    if (status != CAN_OK) { return status; }
    status = canHw_rx(canBoxes[box].hw, frame);
    if (status == CAN_OK || status == CAN_ERR_OVERRUN) { copyOut(data, len, frame, canBoxes[box].dlc); }
    return status;
}

CanStatus_t can_send_multi(const CanBox_t box, const uint8_t * const data, const size_t len)
{
    uint8_t frame[CAN_MAX_DLC];
    size_t  offset = 0U;
    CanStatus_t status = check(box, true, data, len);

    if (status != CAN_OK) { return status; }
    if (canBoxes[box].dlc == 0U) { return can_send(box, data, len); }

    do {
        packFrame(frame, data + offset, len - offset, canBoxes[box].dlc);
        status = txFrameWait(box, frame);
        offset += canBoxes[box].dlc;
    } while (status == CAN_OK && offset < len);

    return status;
}

CanStatus_t can_receive_multi(const CanBox_t box, uint8_t * const data, const size_t len)
{
    CanRxMsg_t *m;
    CanStatus_t status = check(box, false, data, len);

    if (status != CAN_OK) { return status; }
    m = &rxMsg[box];
    if (!m->ready) { return CAN_ERR_NO_DATA; }

    copyOut(data, len, m->data, m->bytes);
    status   = m->lost ? CAN_ERR_OVERRUN : CAN_OK;
    m->lost  = false;
    m->ready = false;       // last: the interrupt drops new messages while this is set
    return status;
}

CanStatus_t can_reply(const CanBox_t rxBox, const uint8_t * const data, const size_t len)
{
    if (!box_ok(rxBox) || !box_ok(canBoxes[rxBox].reply)) { return CAN_ERR_BAD_BOX; }
    return can_send(canBoxes[rxBox].reply, data, len);
}

CanStatus_t can_request(const CanBox_t box)
{
    const CanStatus_t status = check(box, false, NULL, 0U);

    if (status != CAN_OK) { return status; }
    return canHw_remote(canBoxes[box].hw) ? CAN_OK : CAN_ERR_BUSY;
}

CanStatus_t can_send_any(const CanBox_t box, const void * const src, const size_t size)
{
    return can_send_multi(box, (const uint8_t *)src, size);
}

CanStatus_t can_receive_any(const CanBox_t box, void * const dst, const size_t size)
{
    return can_receive_multi(box, (uint8_t *)dst, size);
}

//---------------------------------------------------------------------------------------------------------
// Interrupt side
//---------------------------------------------------------------------------------------------------------
void can_handle_message(const uint32_t hwBox)
{
    const CanBox_t box = findBox(hwBox);

    if (!box_ok(box)) { return; }
    if (!canBoxes[box].tx && !rxCollect(box)) { return; }
    if (canBoxes[box].onDone != NULL) { canBoxes[box].onDone(); }
}

void can_handle_error(const uint32_t level)
{
    bus.level = level;
    if ((level & canLEVEL_BUS_OFF) != 0U) { bus.busOff = true; }
}
