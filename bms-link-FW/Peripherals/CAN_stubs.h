/*
 * CAN_stubs.h
 *
 * Hardware-independent interface for CAN hardware operations and interrupt upcalls.
 */

#ifndef PERIPHERALS_CAN_STUBS_H_
#define PERIPHERALS_CAN_STUBS_H_

#include <stdint.h>
#include <stdbool.h>
#include "Hardware_Select.h"
#include "PhantomPinAlloc.h"

#define CAN_MAX_DLC         8U      /* bytes in one CAN frame */

typedef enum {
    CAN_OK = 0,
    CAN_ERR_BAD_BOX,        /* box is not in the table */
    CAN_ERR_NO_CFG,         /* box is not enabled in hardware configuration */
    CAN_ERR_WRONG_DIR,      /* sending on an RX box, or receiving on a TX box */
    CAN_ERR_NULL,           /* data is NULL but len is not 0 */
    CAN_ERR_BUS_OFF,        /* node is bus-off, call can_service() in the main loop */
    CAN_ERR_BUSY,           /* TX box still holds the last frame, nothing was sent */
    CAN_ERR_TIMEOUT,        /* can_send_multi gave up waiting for the box */
    CAN_ERR_NO_DATA,        /* nothing received */
    CAN_ERR_OVERRUN         /* data is valid, but a message was lost before it */
} CanStatus_t;

/* Hardware wrapper functions */
void        canHw_init(void);
bool        canHw_isRxBoxValid(const CanRxBox_t box);
bool        canHw_isTxBoxValid(const CanTxBox_t box);
bool        canHw_tx(const CanTxBox_t box, const uint8_t frame[CAN_MAX_DLC]);
CanStatus_t canHw_rx(const CanRxBox_t box, uint8_t frame[CAN_MAX_DLC]);
bool        canHw_remote(const CanRxBox_t box);
void        canHw_restart(void);
uint32_t    canHw_lastError(void);
bool        canHw_isBusOffLevel(const uint32_t level);

/* Interrupt upcalls from stubs into Can_helpers */
void        can_handle_rx(const CanRxBox_t box);
void        can_handle_tx(const CanTxBox_t box);
void        can_handle_error(const uint32_t level);

#endif /* PERIPHERALS_CAN_STUBS_H_ */
