/*
 * Can_helpers.h
 *
 *  Replaces Phantom_Can.h
 */

#ifndef CAN_HELPERS_H
#define CAN_HELPERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define CAN_MAX_DLC         8U      // bytes in one CAN frame
#define CAN_MAX_FRAMES      8U      // most frames in one multi-frame message

#ifndef CAN_TX_TIMEOUT_MS
#define CAN_TX_TIMEOUT_MS   10U     // can_send_multi: wait for a free box, per frame
#endif
#ifndef CAN_RX_GAP_MS
#define CAN_RX_GAP_MS       50U     // multi-frame receive: longest pause between two frames
#endif

//---------------------------------------------------------------------------------------------------------
// Message boxes. Same order as the table in Can_helpers.c
//---------------------------------------------------------------------------------------------------------
typedef enum {
    BMS2ALL_FAULT,
    BMS2VCU_DATA,
    VCU2BMS_DATA,
    BMS2CHARGER_DATA,
    CHARGER2BMS_DATA,
    CAN_BOX_COUNT
} CanBox_t;

#define CAN_NO_BOX  ((CanBox_t)CAN_BOX_COUNT)

// Called from the interrupt: RX box = a full message arrived, TX box = a frame was sent
extern void BMS2ALL_FAULT_FullRoutine(void);
extern void BMS2VCU_Data_FullRoutine(void);
extern void VCU2BMS_DATA_FullRoutine(void);
extern void BMS2CHARGER_DATA_FullRoutine(void);
extern void CHARGER2BMS_DATA_FullRoutine(void);

//---------------------------------------------------------------------------------------------------------
// Status
//---------------------------------------------------------------------------------------------------------
typedef enum {
    CAN_OK = 0,
    CAN_ERR_BAD_BOX,        // box is not in the table
    CAN_ERR_NO_CFG,         // box is not enabled in HALCoGen
    CAN_ERR_WRONG_DIR,      // sending on an RX box, or receiving on a TX box
    CAN_ERR_NULL,           // data is NULL but len is not 0
    CAN_ERR_BUS_OFF,        // node is bus-off, call can_service() in the main loop
    CAN_ERR_BUSY,           // TX box still holds the last frame, nothing was sent
    CAN_ERR_TIMEOUT,        // can_send_multi gave up waiting for the box
    CAN_ERR_NO_DATA,        // nothing received
    CAN_ERR_OVERRUN         // data is valid, but a message was lost before it
} CanStatus_t;

#define can_ok(status)  ((status) == CAN_OK)

//---------------------------------------------------------------------------------------------------------
// Setup and bus state
//---------------------------------------------------------------------------------------------------------
void     can_init(void);            // canInit() + error interrupt on, do not call canInit() as well
void     can_service(void);         // main loop: restarts the node after bus-off
bool     can_isBusOff(void);
uint32_t can_errorLevel(void);      // canLEVEL_* of the last error interrupt
uint32_t can_lastError(void);       // canERROR_*. Reading it clears the status flags

//---------------------------------------------------------------------------------------------------------
// One frame, straight to / from the hardware, never blocks.
// Too long: cut to the box dlc. Too short: padded with 0.
//---------------------------------------------------------------------------------------------------------
CanStatus_t can_send(const CanBox_t box, const uint8_t * const data, const size_t len);
CanStatus_t can_receive(const CanBox_t box, uint8_t * const data, const size_t len);

//---------------------------------------------------------------------------------------------------------
// Whole message, several frames with the same id. data is already in frame order.
//
// can_send_multi:    sends every frame, waits for the box between frames. Blocks, not for interrupts.
// can_receive_multi: gives the message the interrupt collected. The frame count is in the box table.
//                    CAN_ERR_NO_DATA until it is complete. Do not mix with can_receive on one box.
//---------------------------------------------------------------------------------------------------------
CanStatus_t can_send_multi(const CanBox_t box, const uint8_t * const data, const size_t len);
CanStatus_t can_receive_multi(const CanBox_t box, uint8_t * const data, const size_t len);

CanStatus_t can_reply(const CanBox_t rxBox, const uint8_t * const data, const size_t len);  // one frame on the box paired with rxBox
CanStatus_t can_request(const CanBox_t box);                                                // remote frame, RX box only

//---------------------------------------------------------------------------------------------------------
// Any type (struct, union with bit fields, float, ...). Bytes go in memory order, so the layout only
// matches between processors with the same endianness and packing.
//     CAN_SEND_VAR(BMS2VCU_DATA, status);
//---------------------------------------------------------------------------------------------------------
CanStatus_t can_send_any(const CanBox_t box, const void * const src, const size_t size);
CanStatus_t can_receive_any(const CanBox_t box, void * const dst, const size_t size);

#define CAN_SEND_VAR(box, var)      can_send_any((box), &(var), sizeof(var))
#define CAN_RECEIVE_VAR(box, var)   can_receive_any((box), &(var), sizeof(var))

//---------------------------------------------------------------------------------------------------------
// Interrupt entry points. hwBox = HALCoGen box number
//---------------------------------------------------------------------------------------------------------
void can_handle_message(const uint32_t hwBox);
void can_handle_error(const uint32_t level);

//---------------------------------------------------------------------------------------------------------
// Hardware wrapper: the only code that calls can.h or touches the CAN registers
//---------------------------------------------------------------------------------------------------------
void        canHw_init(void);
bool        canHw_isBoxValid(const uint32_t hwBox);
bool        canHw_tx(const uint32_t hwBox, const uint8_t frame[CAN_MAX_DLC]);    // false = box busy
CanStatus_t canHw_rx(const uint32_t hwBox, uint8_t frame[CAN_MAX_DLC]);          // OK, NO_DATA or OVERRUN
bool        canHw_remote(const uint32_t hwBox);
void        canHw_restart(void);
uint32_t    canHw_lastError(void);

#endif /* CAN_HELPERS_H */
