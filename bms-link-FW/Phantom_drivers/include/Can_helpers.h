/*
 * Can_helpers.h
 *
 * High-level CAN communication helpers.
 * Hardware independent: delegates physical CAN access to CAN_stubs.
 */

#ifndef CAN_HELPERS_H
#define CAN_HELPERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "CAN_stubs.h"

#define CAN_MAX_FRAMES      8U      /* most frames in one multi-frame message */

#ifndef CAN_TX_TIMEOUT_MS
#define CAN_TX_TIMEOUT_MS   10U     /* can_send_multi: wait for a free box, per frame */
#endif
#ifndef CAN_RX_GAP_MS
#define CAN_RX_GAP_MS       50U     /* multi-frame receive: longest pause between two frames */
#endif

/* Called from the interrupt: RX box = a full message arrived, TX box = a frame was sent */
extern void BMS2ALL_FAULT_FullRoutine(void);
extern void BMS2VCU_Data_FullRoutine(void);
extern void VCU2BMS_DATA_FullRoutine(void);
extern void BMS2CHARGER_DATA_FullRoutine(void);
extern void CHARGER2BMS_DATA_FullRoutine(void);

#define can_ok(status)  ((status) == CAN_OK)

/* Setup and bus state */
void     can_init(void);            /* canHw_init() + error interrupt on, do not call canInit() as well */
void     can_service(void);         /* main loop: restarts the node after bus-off */
bool     can_isBusOff(void);
uint32_t can_errorLevel(void);
uint32_t can_lastError(void);

/* One frame, straight to / from the hardware, never blocks. */
/* Too long: cut to the box dlc. Too short: padded with 0. */
CanStatus_t can_send(const CanTxBox_t box, const uint8_t * const data, const size_t len);
CanStatus_t can_receive(const CanRxBox_t box, uint8_t * const data, const size_t len);

/* Whole message, several frames with the same id. data is already in frame order. */
CanStatus_t can_send_multi(const CanTxBox_t box, const uint8_t * const data, const size_t len);
CanStatus_t can_receive_multi(const CanRxBox_t box, uint8_t * const data, const size_t len);

CanStatus_t can_reply(const CanRxBox_t rxBox, const uint8_t * const data, const size_t len);
CanStatus_t can_request(const CanRxBox_t box);

/* Any type (struct, union with bit fields, float, ...). Sends memory order. */
CanStatus_t can_send_any(const CanTxBox_t box, const void * const src, const size_t size);
CanStatus_t can_receive_any(const CanRxBox_t box, void * const dst, const size_t size);

#define CAN_SEND_VAR(box, var)      can_send_any((box), &(var), sizeof(var))
#define CAN_RECEIVE_VAR(box, var)   can_receive_any((box), &(var), sizeof(var))

#endif /* CAN_HELPERS_H */
