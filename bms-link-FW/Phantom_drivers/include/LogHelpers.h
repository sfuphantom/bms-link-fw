/*
 * LogHelpers.h
 *
 *  Frame (big endian on the wire, whatever the processor is):
 *      [SOH][id][len hi][len lo][tick u64][body ...][crc16 hi][crc16 lo][EOT]
 *      len = tick + body. The CRC covers id .. end of body and is only there if SERIAL_USE_CRC.
 *  The frame is sent in pieces, as it is built, so there is no big buffer.
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_LOGHELPERS_H_
#define PHANTOM_DRIVERS_INCLUDE_LOGHELPERS_H_

#include <stdint.h>
#include <stdbool.h>

#include "PhantomAlgorithms.h"

// Only Serial_Write() and Serial_ReadByte() in the .c depend on this.
// CUSTOM (other board, SIL, SPI...): write SerialPort_Write() and SerialPort_ReadByte() yourself.
#define SERIAL_TRANSPORT_SCI        0
#define SERIAL_TRANSPORT_CUSTOM     1
#define SERIAL_TRANSPORT            SERIAL_TRANSPORT_SCI

#define SOH_BYTE            0x01
#define EOT_BYTE            0x04
#define SERIAL_USE_CRC      1           // same as USE_CRC in uart_receiver.py
#define SERIAL_RAW_MAX      1024        // raw bytes in one frame, more is cut

// Compile time check: the build stops with the name of the problem in the error
#define STATIC_ASSERT(cond, name)   typedef char name[(cond) ? 1 : -1]

// Same numbers in uart_receiver.py
typedef enum {
    MSG_LOG = 1,        // [level][text_len u8][text]
    MSG_LOG_RAW,        // [level][text_len u8][text][raw bytes]
    MSG_RAW,            // [raw bytes]
    MSG_FAULT,          // [fault id u16][text_len u8][text]
    MSG_CAN,            // [msg box u8][data bytes]
    MSG_DATA            // sections: ([section id][len u16][section data]) ...
} msg_ID_t;

#if SERIAL_TRANSPORT == SERIAL_TRANSPORT_CUSTOM
void SerialPort_Write   (const uint8_t * const data, const uint16_t len);   // has to send everything before it returns
bool SerialPort_ReadByte(uint8_t * const byte);                             // false if nothing received, must not wait
#endif

// Basic send / receive. Nothing else in the logs touches the hardware.
void Serial_Write   (const uint8_t * const data, const uint16_t len);
bool Serial_ReadByte(uint8_t * const byte);

// One frame at a time. Start takes the tx lock, End gives it back.
// bodyLen is the number of bytes the Send calls will add (without the tick). Start returns false if
// another frame is in progress (ISR): the frame is dropped and nothing must be sent.
bool Serial_FrameStart(const msg_ID_t id, const uint16_t bodyLen);
void Serial_FrameSend (const uint8_t * const data, const uint16_t len);
void Serial_FrameEnd  (void);

uint32_t Serial_GetDroppedFrames(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_LOGHELPERS_H_ */
