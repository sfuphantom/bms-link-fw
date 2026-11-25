/*
 * CAN_stubs.c
 *
 * Hardware-specific CAN implementation. The ONLY CAN file including HALCoGen headers.
 */

#include "Hardware_Select.h"
#include "CAN_stubs.h"
#include "Stub_Pins.h"

#if BOARD == BOARD_TMS570_HW

#include "can.h"
#include "reg_can.h"

#ifndef CAN_NODE
#define CAN_NODE                canREG1
#endif

void canHw_init(void)
{
    canInit();
    canEnableErrorNotification(CAN_NODE);       /* bus-off, warning and parity interrupts */
}

bool canHw_isRxBoxValid(const CanRxBox_t box)
{
    const uint32_t hwBox = alloc_canRxHwBox(box);
    if (hwBox == 0U) { return false; }
    return canIsMessageBoxValid(CAN_NODE, hwBox) != 0U;
}

bool canHw_isTxBoxValid(const CanTxBox_t box)
{
    const uint32_t hwBox = alloc_canTxHwBox(box);
    if (hwBox == 0U) { return false; }
    return canIsMessageBoxValid(CAN_NODE, hwBox) != 0U;
}

/* 1 = request set, 0 = box still sending. It sends the dlc set in HALCoGen. */
bool canHw_tx(const CanTxBox_t box, const uint8_t frame[CAN_MAX_DLC])
{
    const uint32_t hwBox = alloc_canTxHwBox(box);
    if (hwBox == 0U) { return false; }
    return canTransmit(CAN_NODE, hwBox, frame) != 0U;
}

/* 0 = nothing new, 1 = ok, 3 = ok but an earlier frame was lost */
CanStatus_t canHw_rx(const CanRxBox_t box, uint8_t frame[CAN_MAX_DLC])
{
    const uint32_t hwBox = alloc_canRxHwBox(box);
    if (hwBox == 0U) { return CAN_ERR_BAD_BOX; }

    switch (canGetData(CAN_NODE, hwBox, frame)) {
        case 1U: return CAN_OK;
        case 3U: return CAN_ERR_OVERRUN;
        default: return CAN_ERR_NO_DATA;
    }
}

bool canHw_remote(const CanRxBox_t box)
{
    const uint32_t hwBox = alloc_canRxHwBox(box);
    if (hwBox == 0U) { return false; }
    return canSendRemoteFrame(CAN_NODE, hwBox) != 0U;
}

/* Bus-off leaves the node in Init. Clearing Init starts the recovery (129 x 11 recessive bits) */
void canHw_restart(void)
{
    CAN_NODE->CTL &= ~(uint32)1U;
}

uint32_t canHw_lastError(void)
{
    return canGetLastError(CAN_NODE);
}

bool canHw_isBusOffLevel(const uint32_t level)
{
    return (level & canLEVEL_BUS_OFF) != 0U;
}

/* HALCoGen callbacks */
void canMessageNotification(canBASE_t *node, uint32 messageBox)
{
    if (node != CAN_NODE) { return; }

    const CanRxBox_t rxBox = alloc_canRxBox(messageBox);
    if (rxBox != CAN_RX_NONE) {
        can_handle_rx(rxBox);
        return;
    }

    const CanTxBox_t txBox = alloc_canTxBox(messageBox);
    if (txBox != CAN_TX_NONE) {
        can_handle_tx(txBox);
    }
}

void canErrorNotification(canBASE_t *node, uint32 notification)
{
    if (node == CAN_NODE) {
        can_handle_error(notification);
    }
}

#elif BOARD == BOARD_STM32_HW
  #error "STM32 CAN stubs not implemented"
#elif BOARD == BOARD_SIL
  #error "SIL CAN stubs not implemented"
#else
  #error "Unknown BOARD"
#endif
