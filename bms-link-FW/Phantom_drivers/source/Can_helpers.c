/*
 * Can_helpers.c
 *
 * High-level CAN communication helpers.
 * Hardware independent: routes physical operations through CAN_stubs.
 */

#include <string.h>
#include "PhantomAlgorithms.h"
#include "PhantomTimers.h"
#include "Can_helpers.h"

#define CAN_TX_TIMEOUT_TICKS    MS_TO_TICKS(CAN_TX_TIMEOUT_MS)
#define CAN_RX_GAP_TICKS        MS_TO_TICKS(CAN_RX_GAP_MS)

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

/* ------------------------------------------------------------------------- */
/* Logical Box Tables (match CanRxBox_t / CanTxBox_t enum order per R5)      */
/* ------------------------------------------------------------------------- */
typedef struct {
    uint8_t    dlc;
    uint8_t    frames;
    CanTxBox_t reply;
    VoidFunc_t onDone;
} CanRxBoxDef_t;

typedef struct {
    uint8_t    dlc;
    VoidFunc_t onDone;
} CanTxBoxDef_t;

static const CanRxBoxDef_t canRxBoxes[CAN_RX_COUNT] = {
    [CAN_RX_VCU2BMS_DATA]     = { 8U, 1U, CAN_TX_BMS2VCU_DATA,     VCU2BMS_DATA_FullRoutine     },
    [CAN_RX_CHARGER2BMS_DATA] = { 8U, 1U, CAN_TX_BMS2CHARGER_DATA, CHARGER2BMS_DATA_FullRoutine },
};
_Static_assert((sizeof(canRxBoxes) / sizeof(canRxBoxes[0])) == CAN_RX_COUNT, "canRxBoxes count mismatch");

static const CanTxBoxDef_t canTxBoxes[CAN_TX_COUNT] = {
    [CAN_TX_BMS2ALL_FAULT]    = { 8U, BMS2ALL_FAULT_FullRoutine    },
    [CAN_TX_BMS2VCU_DATA]     = { 8U, BMS2VCU_Data_FullRoutine     },
    [CAN_TX_BMS2CHARGER_DATA] = { 8U, BMS2CHARGER_DATA_FullRoutine },
};
_Static_assert((sizeof(canTxBoxes) / sizeof(canTxBoxes[0])) == CAN_TX_COUNT, "canTxBoxes count mismatch");

/* ------------------------------------------------------------------------- */
/* State                                                                     */
/* ------------------------------------------------------------------------- */
typedef struct {
    volatile bool     busOff;
    volatile uint32_t level;
} CanBus_t;

typedef struct {
    uint8_t       data[CAN_MAX_FRAMES * CAN_MAX_DLC];
    size_t        bytes;            /* size of the finished message */
    ticks_t       last;             /* when the last frame came in */
    uint8_t       frame;            /* frames collected so far */
    volatile bool ready;            /* finished, waiting for can_receive_multi */
    volatile bool lost;             /* something was dropped, reported by the next read */
} CanRxMsg_t;

static CanBus_t   bus = { false, 0U };
static CanRxMsg_t rxMsg[CAN_RX_COUNT];

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */
static bool rxBox_ok(const CanRxBox_t box)
{
    return (unsigned int)box < (unsigned int)CAN_RX_COUNT;
}

static bool txBox_ok(const CanTxBox_t box)
{
    return (unsigned int)box < (unsigned int)CAN_TX_COUNT;
}

static CanStatus_t checkRx(const CanRxBox_t box, const void * const data, const size_t len)
{
    if (!rxBox_ok(box))                 { return CAN_ERR_BAD_BOX; }
    if (data == NULL && len != 0U)      { return CAN_ERR_NULL; }
    if (!canHw_isRxBoxValid(box))       { return CAN_ERR_NO_CFG; }
    return CAN_OK;
}

static CanStatus_t checkTx(const CanTxBox_t box, const void * const data, const size_t len)
{
    if (!txBox_ok(box))                 { return CAN_ERR_BAD_BOX; }
    if (data == NULL && len != 0U)      { return CAN_ERR_NULL; }
    if (!canHw_isTxBoxValid(box))       { return CAN_ERR_NO_CFG; }
    if (bus.busOff)                     { return CAN_ERR_BUS_OFF; }
    return CAN_OK;
}

/* too long: cut, too short: 0 */
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

static CanStatus_t txFrame(const CanTxBox_t box, const uint8_t frame[CAN_MAX_DLC])
{
    return canHw_tx(box, frame) ? CAN_OK : CAN_ERR_BUSY;
}

static CanStatus_t txFrameWait(const CanTxBox_t box, const uint8_t frame[CAN_MAX_DLC])
{
    Timeout_t t;
    timeout_start(&t, CAN_TX_TIMEOUT_TICKS);

    while (txFrame(box, frame) == CAN_ERR_BUSY) {
        if (bus.busOff)             { return CAN_ERR_BUS_OFF; }
        if (timeout_expired(&t))    { return CAN_ERR_TIMEOUT; }
    }
    return CAN_OK;
}

/* Takes one frame from the hardware. True when the whole message is ready. */
static bool rxCollect(const CanRxBox_t box)
{
    CanRxMsg_t * const m = &rxMsg[box];
    const size_t dlc     = canRxBoxes[box].dlc;
    const size_t frames  = MIN_VAL((size_t)canRxBoxes[box].frames, (size_t)CAN_MAX_FRAMES);
    uint8_t frame[CAN_MAX_DLC];
    const CanStatus_t status = canHw_rx(box, frame);

    if (status != CAN_OK && status != CAN_ERR_OVERRUN) { return false; }

    if (m->ready) { m->lost = true; return false; }     /* last message not read yet, drop this one */

    if (status == CAN_ERR_OVERRUN) {
        m->lost = true;
        if (frames > 1U) { m->frame = 0U; return false; }   /* a frame is missing, start over */
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

/* ------------------------------------------------------------------------- */
/* Setup and bus state                                                       */
/* ------------------------------------------------------------------------- */
void can_init(void)
{
    bus.busOff = false;
    bus.level  = 0U;
    memset(rxMsg, 0, sizeof(rxMsg));
    canHw_init();
}

/* Bus-off is flagged by the error interrupt, this starts the recovery */
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

/* ------------------------------------------------------------------------- */
/* Send and receive                                                          */
/* ------------------------------------------------------------------------- */
CanStatus_t can_send(const CanTxBox_t box, const uint8_t * const data, const size_t len)
{
    uint8_t frame[CAN_MAX_DLC];
    const CanStatus_t status = checkTx(box, data, len);

    if (status != CAN_OK) { return status; }
    packFrame(frame, data, len, canTxBoxes[box].dlc);
    return txFrame(box, frame);
}

CanStatus_t can_receive(const CanRxBox_t box, uint8_t * const data, const size_t len)
{
    uint8_t frame[CAN_MAX_DLC];
    CanStatus_t status = checkRx(box, data, len);

    if (status != CAN_OK) { return status; }
    status = canHw_rx(box, frame);
    if (status == CAN_OK || status == CAN_ERR_OVERRUN) { copyOut(data, len, frame, canRxBoxes[box].dlc); }
    return status;
}

CanStatus_t can_send_multi(const CanTxBox_t box, const uint8_t * const data, const size_t len)
{
    uint8_t frame[CAN_MAX_DLC];
    size_t  offset = 0U;
    CanStatus_t status = checkTx(box, data, len);

    if (status != CAN_OK) { return status; }
    if (canTxBoxes[box].dlc == 0U) { return can_send(box, data, len); }

    do {
        packFrame(frame, data + offset, len - offset, canTxBoxes[box].dlc);
        status = txFrameWait(box, frame);
        offset += canTxBoxes[box].dlc;
    } while (status == CAN_OK && offset < len);

    return status;
}

CanStatus_t can_receive_multi(const CanRxBox_t box, uint8_t * const data, const size_t len)
{
    CanRxMsg_t *m;
    CanStatus_t status = checkRx(box, data, len);

    if (status != CAN_OK) { return status; }
    m = &rxMsg[box];
    if (!m->ready) { return CAN_ERR_NO_DATA; }

    copyOut(data, len, m->data, m->bytes);
    status   = m->lost ? CAN_ERR_OVERRUN : CAN_OK;
    m->lost  = false;
    m->ready = false;       /* last: the interrupt drops new messages while this is set */
    return status;
}

CanStatus_t can_reply(const CanRxBox_t rxBox, const uint8_t * const data, const size_t len)
{
    if (!rxBox_ok(rxBox) || !txBox_ok(canRxBoxes[rxBox].reply)) { return CAN_ERR_BAD_BOX; }
    return can_send(canRxBoxes[rxBox].reply, data, len);
}

CanStatus_t can_request(const CanRxBox_t box)
{
    const CanStatus_t status = checkRx(box, NULL, 0U);

    if (status != CAN_OK) { return status; }
    return canHw_remote(box) ? CAN_OK : CAN_ERR_BUSY;
}

CanStatus_t can_send_any(const CanTxBox_t box, const void * const src, const size_t size)
{
    return can_send_multi(box, (const uint8_t *)src, size);
}

CanStatus_t can_receive_any(const CanRxBox_t box, void * const dst, const size_t size)
{
    return can_receive_multi(box, (uint8_t *)dst, size);
}

/* ------------------------------------------------------------------------- */
/* Interrupt side                                                            */
/* ------------------------------------------------------------------------- */
void can_handle_rx(const CanRxBox_t box)
{
    if (!rxBox_ok(box)) { return; }
    if (!rxCollect(box)) { return; }
    if (canRxBoxes[box].onDone != NULL) { canRxBoxes[box].onDone(); }
}

void can_handle_tx(const CanTxBox_t box)
{
    if (!txBox_ok(box)) { return; }
    if (canTxBoxes[box].onDone != NULL) { canTxBoxes[box].onDone(); }
}

void can_handle_error(const uint32_t level)
{
    bus.level = level;
    if (canHw_isBusOffLevel(level)) { bus.busOff = true; }
}
