/*
 * SendDataSerial.h
 *
 *  Created on: Jul 31, 2026
 *      Author: tanjo
 *
 *  Frame (big endian on the wire, whatever the processor is):
 *      [SOH][id][len hi][len lo][tick u64][body ...][crc16 hi][crc16 lo][EOT]
 *      len = tick + body. The CRC covers id .. end of body and is only there if SERIAL_USE_CRC.
 *  Every frame carries the "now" tick. The Python side uses it as the time of the row.
 *  The frame is sent in pieces, as it is built, so there is no big buffer.
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_SENDDATASERIAL_H_
#define PHANTOM_DRIVERS_INCLUDE_SENDDATASERIAL_H_

#include <stdint.h>
#include <stdbool.h>

#include "SlaveCommunation_Hardware.h"
#include "BatteryCell_Hardware.h"
#include "FullBattery_Hardware.h"

#include "BatteryData.h"
#include "PhantomHelpers.h"
#include "PhantomTimers.h"
#include "Phantom_Can.h"

//---------------------------------------------------------------------------------------------------------
// Transport. Only Transport_Send() in the .c depends on this.
#define SERIAL_TRANSPORT_SCI    0
#define SERIAL_TRANSPORT_SPI    1
#define SERIAL_TRANSPORT        SERIAL_TRANSPORT_SCI

#if SERIAL_TRANSPORT == SERIAL_TRANSPORT_SCI
#include "sci.h"
#include "reg_sci.h"

#define USE_USB_N_PINS 1
#if USE_USB_N_PINS
    #define UART_REG        sciREG
#else
    #define UART_REG        scilinREG
#endif
#endif

//---------------------------------------------------------------------------------------------------------
#define SOH_BYTE            0x01
#define EOT_BYTE            0x04

// The sender and the receiver have to agree on these, nothing about them is sent.
#define SERIAL_USE_CRC      1           // same as USE_CRC in uart_receiver.py

// Sizes. The ones the compiler can check are checked in the .c
#define SERIAL_SECTION_MAX  64          // biggest data section, in bytes
#define SERIAL_LOG_MAX      128         // log text buffer. Text is cut to SERIAL_LOG_MAX - 1 (max 256)
#define SERIAL_RAW_MAX      1024        // raw bytes in one frame, more is cut
#define SERIAL_LOG_CAN      1           // 0 = Log_Can does nothing

#define PWM_NIBBLE_BYTES    ((NUMBER_OF_CELLS_SERIES + 1) / 2)

// Which sections are sent at start up (1 = on). Serial_EnableSection() changes it while running.
#define SERIAL_EN_CELL_VOLT     1
#define SERIAL_EN_CELL_TEMP     0
#define SERIAL_EN_CELL_RES      0
#define SERIAL_EN_CELL_SOC      0
#define SERIAL_EN_PACK          0
#define SERIAL_EN_IMD           0
#define SERIAL_EN_CHARGER       0
#define SERIAL_EN_SLAVE_STATE   1
#define SERIAL_EN_FANS          0
#define SERIAL_EN_PWM           1
#define SERIAL_EN_BMS_STATE     0

// float -> integer scales used by the Compress_ functions. Keep them the same in uart_receiver.py
#define SCALE_CELL_VOLT     10000.0f
#define SCALE_TEMP          100.0f      // signed
#define SCALE_RES           10000.0f
#define SCALE_SOC           100.0f
#define SCALE_PACK_VOLT     100.0f
#define SCALE_CURRENT       10.0f       // signed

//---------------------------------------------------------------------------------------------------------
typedef enum {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_OFF                       // as LOG_MIN_LEVEL: no logs at all
} LogLevel_t;

// Logs below this level are not sent, and the LOG_xxx macros below don't even evaluate their arguments.
#ifndef LOG_MIN_LEVEL
#define LOG_MIN_LEVEL       LOG_LEVEL_DEBUG
#endif

typedef enum {
    MSG_LOG = 1,        // [level][text_len u8][text]
    MSG_LOG_RAW,        // [level][text_len u8][text][raw bytes]
    MSG_RAW,            // [raw bytes]
    MSG_FAULT,          // [fault id u16][text_len u8][text]
    MSG_CAN,            // [msg box][data bytes]
    MSG_DATA            // sections: ([section id][len u16][section data]) ...
} msg_ID_t;

// One block of data inside MSG_DATA. Same numbers in uart_receiver.py (SecId).
typedef enum {
    SEC_CELL_VOLT = 1,
    SEC_CELL_TEMP,
    SEC_CELL_RES,
    SEC_CELL_SOC,
    SEC_PACK,
    SEC_IMD,
    SEC_CHARGER,
    SEC_SLAVE_STATE,
    SEC_FANS,
    SEC_PWM,
    SEC_BMS_STATE,
    SEC_COUNT                           // keep last, has to stay <= 32 (one bit each in the enable mask)
} SectionId_t;

#warning FaultId_t is empty, add the fault IDs here (and the names in FAULT_NAMES in uart_receiver.py)
typedef enum {
    FAULT_ID_NONE = 0,
    FAULT_ID_UNKNOWN
    // add fault IDs here
} FaultId_t;

typedef struct {
    uint32_t framesDropped;     // another send was in progress (ISR), the frame was thrown away
    uint32_t sizeMismatch;      // a Compress_ function wrote another size than the len in its table
    uint8_t  badSection;        // the last section that did
} SerialStats_t;

typedef struct {
    ticks_t last;
    bool    started;
} LogLimiter_t;

//---------------------------------------------------------------------------------------------------------
// Logs. They all use the level filter above, even when called directly.
void Log_Text   (const LogLevel_t level, const char * const text);
void Log_Printf (const LogLevel_t level, const char * const fmt, ...);
void Log_Raw    (const LogLevel_t level, const char * const text, const uint8_t * const data, const uint16_t len);

// "name=value"
void Log_Int    (const LogLevel_t level, const char * const name, const int32_t value);
void Log_UInt   (const LogLevel_t level, const char * const name, const uint32_t value);
void Log_Hex    (const LogLevel_t level, const char * const name, const uint32_t value);
void Log_Float  (const LogLevel_t level, const char * const name, const float value);
void Log_Bool   (const LogLevel_t level, const char * const name, const bool value);

// "name=[a, b, c]"
void Log_ArrayU8   (const LogLevel_t level, const char * const name, const uint8_t  * const arr, const uint16_t count);
void Log_ArrayU16  (const LogLevel_t level, const char * const name, const uint16_t * const arr, const uint16_t count);
void Log_ArrayFloat(const LogLevel_t level, const char * const name, const float    * const arr, const uint16_t count);

// Faults are always sent, no level.
void Log_Fault  (const FaultId_t id, const char * const fmt, ...);

// CAN. msgBox is the message box number. Log_Can takes the length from CanMsgLen() in the .c (8 by default).
void Log_Can    (const phantomCanMsgBox msgBox, const uint8_t * const data);
void Log_CanLen (const phantomCanMsgBox msgBox, const uint8_t * const data, const uint8_t len);

void Send_Raw   (const uint8_t * const data, const uint16_t len);

// true once per period, per limiter. The first call is always true.
bool LogLimiter_Due(LogLimiter_t * const limiter, const ticks_t period);

//---------------------------------------------------------------------------------------------------------
// Macros. LOG_xxx(fmt, ...) is printf style.
#define LOG_PRINTF(level, ...)  do { if ((level) >= LOG_MIN_LEVEL) Log_Printf((level), __VA_ARGS__); } while (0)

#define LOG_TRACE(...)          LOG_PRINTF(LOG_LEVEL_TRACE,    __VA_ARGS__)
#define LOG_DEBUG(...)          LOG_PRINTF(LOG_LEVEL_DEBUG,    __VA_ARGS__)
#define LOG_INFO(...)           LOG_PRINTF(LOG_LEVEL_INFO,     __VA_ARGS__)
#define LOG_WARNING(...)        LOG_PRINTF(LOG_LEVEL_WARNING,  __VA_ARGS__)
#define LOG_ERROR(...)          LOG_PRINTF(LOG_LEVEL_ERROR,    __VA_ARGS__)
#define LOG_CRITICAL(...)       LOG_PRINTF(LOG_LEVEL_CRITICAL, __VA_ARGS__)

#define LOG_RAW(level, text, data, len) \
    do { if ((level) >= LOG_MIN_LEVEL) Log_Raw((level), (text), (data), (len)); } while (0)

// only if cond is true:  LOG_IF(volt > 4.2f, LOG_LEVEL_WARNING, "cell %d high", i);
#define LOG_IF(cond, level, ...) \
    do { if (cond) LOG_PRINTF(level, __VA_ARGS__); } while (0)

// at most once per period, per place in the code:  LOG_EVERY_SEC(LOG_LEVEL_INFO, 5, "alive");
// (MS_TO_TICKS in PhantomTimers.h is not used, it does not compile)
#define LOG_SEC_TICKS(sec)      US_TO_TICKS((ticks_t)(sec) * 1000000U)
#define LOG_MS_TICKS(ms)        US_TO_TICKS((ticks_t)(ms)  * 1000U)

#define LOG_EVERY(level, period_ticks, ...) \
    do { \
        static LogLimiter_t limiter_; \
        if ((level) >= LOG_MIN_LEVEL && LogLimiter_Due(&limiter_, (period_ticks))) \
            Log_Printf((level), __VA_ARGS__); \
    } while (0)

#define LOG_EVERY_SEC(level, sec, ...)  LOG_EVERY(level, LOG_SEC_TICKS(sec), __VA_ARGS__)
#define LOG_EVERY_MS(level, ms, ...)    LOG_EVERY(level, LOG_MS_TICKS(ms),   __VA_ARGS__)

// Log a variable with its own name:  LOG_VAR_INT(count);  sends  count=5
// Level is LOG_VAR_LEVEL (debug unless you define it before including this file).
#ifndef LOG_VAR_LEVEL
#define LOG_VAR_LEVEL           LOG_LEVEL_DEBUG
#endif
#define LOG_VAR_CALL(call)      do { if (LOG_VAR_LEVEL >= LOG_MIN_LEVEL) call; } while (0)

#define LOG_VAR_INT(var)        LOG_VAR_CALL(Log_Int  (LOG_VAR_LEVEL, #var, (var)))
#define LOG_VAR_UINT(var)       LOG_VAR_CALL(Log_UInt (LOG_VAR_LEVEL, #var, (var)))
#define LOG_VAR_HEX(var)        LOG_VAR_CALL(Log_Hex  (LOG_VAR_LEVEL, #var, (var)))
#define LOG_VAR_FLOAT(var)      LOG_VAR_CALL(Log_Float(LOG_VAR_LEVEL, #var, (var)))
#define LOG_VAR_BOOL(var)       LOG_VAR_CALL(Log_Bool (LOG_VAR_LEVEL, #var, (var)))
// real arrays only, not pointers
#define LOG_VAR_ARRAY_U8(arr)       LOG_VAR_CALL(Log_ArrayU8   (LOG_VAR_LEVEL, #arr, (arr), ARRAY_LEN(arr)))
#define LOG_VAR_ARRAY_U16(arr)      LOG_VAR_CALL(Log_ArrayU16  (LOG_VAR_LEVEL, #arr, (arr), ARRAY_LEN(arr)))
#define LOG_VAR_ARRAY_FLOAT(arr)    LOG_VAR_CALL(Log_ArrayFloat(LOG_VAR_LEVEL, #arr, (arr), ARRAY_LEN(arr)))

// Picks the function from the type of the variable. Needs C11 (_Generic).
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define LOG_VAR(var) \
    LOG_VAR_CALL(_Generic((var), \
        bool:           Log_Bool, \
        float:          Log_Float, \
        double:         Log_Float, \
        signed char:    Log_Int,  short:          Log_Int,  int:           Log_Int,  long:           Log_Int, \
        unsigned char:  Log_UInt, unsigned short: Log_UInt, unsigned int:  Log_UInt, unsigned long:  Log_UInt \
    )(LOG_VAR_LEVEL, #var, (var)))
#endif

//---------------------------------------------------------------------------------------------------------
// Data. Sends one MSG_DATA with every enabled section, in the order of the sections[] table in the .c
void SendDataRoutine_Serial(void);
void Serial_EnableSection(const SectionId_t id, const bool enable);

const SerialStats_t * Serial_GetStats(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_SENDDATASERIAL_H_ */
