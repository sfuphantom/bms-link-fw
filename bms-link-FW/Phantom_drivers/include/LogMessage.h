/*
 * LogMessage.h
 *
 *  Messages for the user: text logs, faults, CAN and raw bytes.
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_LOGMESSAGE_H_
#define PHANTOM_DRIVERS_INCLUDE_LOGMESSAGE_H_

#include <stdint.h>
#include <stdbool.h>

#include "LogHelpers.h"
#include "PhantomTimers.h"
#include "Can_Helpers.h"

#define SERIAL_LOG_MAX      256         // log text buffer. Text is cut to SERIAL_LOG_MAX - 1 (max 256)
#define SERIAL_LOG_CAN      1           // 0 = Log_Can does nothing

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

#warning FaultId_t is empty, add the fault IDs here (and the names in FAULT_NAMES in uart_receiver.py)
typedef enum {
    FAULT_ID_NONE = 0,
    FAULT_ID_UNKNOWN
    // add fault IDs here
} FaultId_t;

typedef struct {
    ticks_t last;
    bool    started;
} LogLimiter_t;

//---------------------------------------------------------------------------------------------------------
// They all use the level filter, even when called directly.
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
void Log_Can    (const CanBox_t msgBox, const uint8_t * const data);
void Log_CanLen (const CanBox_t msgBox, const uint8_t * const data, const uint8_t len);

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

#endif /* PHANTOM_DRIVERS_INCLUDE_LOGMESSAGE_H_ */
