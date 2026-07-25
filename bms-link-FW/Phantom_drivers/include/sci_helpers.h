/** @file sci_helper.h
*   @brief UART/SCI Helper Function Definitions
*
*   Convenience wrappers around the TI HALCoGen SCI driver (sci.h) for
*   sending and receiving common data types over UART using a fixed
*   module handle, UART_REG.
*/

#ifndef __SCI_HELPER_H__
#define __SCI_HELPER_H__

#include <stdint.h>
#include <stdbool.h>
#include "sci.h"
#include "reg_sci.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------
 * UART_REG is the sciBASE_t* used by every helper function below.
 * Point it at your actual module (e.g. sciREG, scilinREG) during init:
 *
 *      UART_REG = sciREG;
 * ------------------------------------------------------------------------ */
/* Assign this to your actual SCI module during system init, e.g.:
 *      UART_REG = sciREG;
 * Left NULL here so a missing init is easy to spot/debug. */
#define UART_REG scilinREG


/* Optional: character appended by the "line" send helpers */
#define UART_DEFAULT_END_CHAR   ((uint8)'\n')

/* Optional: character sent by UART_sendStartCharID() to mark the start
 * of a framed packet (STX - Start of Text) */
#define UART_DEFAULT_START_CHAR ((uint8)0x02U)

/* ==========================================================================
 *  PACKET ID ENUM
 * ========================================================================== */

/** @enum sciPacketID_t
*   @brief Example packet/message identifiers.
*
*   Customize this list for your protocol - add/remove/rename entries as
*   needed. UART_sendStartCharID() sends the ID as a single byte, so keep
*   the enum to 256 values or fewer (0x00-0xFF). If you need more, change
*   UART_sendStartCharID/UART_receiveStartCharID to use UART_sendUint16 /
*   UART_receiveUint16 instead.
*/
typedef enum
{
    PKT_ID_UNKNOWN = 0,
    PKT_ID_STATUS,
    PKT_ID_SENSOR_DATA,
    PKT_ID_COMMAND,
    PKT_ID_ACK,
    PKT_ID_NACK
} sciPacketID_t;

/* ==========================================================================
 *  SEND FUNCTIONS
 * ========================================================================== */

/* Single byte / raw array (thin wrappers over sciSendByte / sciSend) */
void UART_sendByte(uint8 data);
void UART_sendArray(uint8 *data, uint32 length);

/* Fixed-width integers */
void UART_sendUint16(uint16 data);
void UART_sendUint32(uint32 data);

/* Arrays of fixed-width integers */
void UART_sendUint16Array(uint16 *data, uint32 length);
void UART_sendUint32Array(uint32 *data, uint32 length);

/* Floating point */
void UART_sendFloat(float32 data);
void UART_sendFloatArray(float32 *data, uint32 length);

/* Strings */
void UART_sendString(char *str);                 /* sends chars, no terminator   */
void UART_sendStringLine(char *str);              /* sends chars + UART_DEFAULT_END_CHAR */

/* End / terminator character, useful to flag end-of-packet to a receiver */
void UART_sendEndChar(void);
void UART_sendChar(uint8 endChar);

/* Sends UART_DEFAULT_START_CHAR followed by the packet ID byte, e.g.:
 *      UART_sendStartCharID(PKT_ID_SENSOR_DATA);
 * Use to mark the start of a framed packet before sending its payload. */
void UART_sendStartCharID(sciPacketID_t id);

/* ==========================================================================
 *  RECEIVE ("RESERVE") FUNCTIONS
 * ========================================================================== */

/* Single byte / raw array (thin wrappers over sciReceiveByte / sciReceive) */
uint8 UART_receiveByte(void);
void  UART_receiveArray(uint8 *data, uint32 length);

/* Fixed-width integers */
uint16 UART_receiveUint16(void);
uint32 UART_receiveUint32(void);

/* Arrays of fixed-width integers */
void UART_receiveUint16Array(uint16 *data, uint32 length);
void UART_receiveUint32Array(uint32 *data, uint32 length);

/* Floating point */
float32 UART_receiveFloat(void);
void    UART_receiveFloatArray(float32 *data, uint32 length);

/* Strings - reads bytes into buffer until endChar is seen or maxLength-1
 * bytes have been read, then null-terminates. Returns number of chars read
 * (excluding the null terminator). */
uint32 UART_receiveString(char *buffer, uint32 maxLength);
uint32 UART_receiveUntilChar(char *buffer, uint32 maxLength, uint8 endChar);

/* Blocks until UART_DEFAULT_START_CHAR is seen, then reads and returns the
 * following packet ID byte as a sciPacketID_t. Any bytes received before
 * the start char are discarded. */
sciPacketID_t UART_receiveStartCharID(void);

#ifdef __cplusplus
}
#endif

#endif /* __SCI_HELPER_H__ */
