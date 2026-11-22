/*
 * LogMessage.c
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "LogMessage.h"
#include "PhantomTimers.h"

STATIC_ASSERT(SERIAL_LOG_MAX >= 2 && SERIAL_LOG_MAX <= 256, SERIAL_LOG_MAX_must_be_2_to_256_so_the_text_len_fits_a_byte);

//------------------------------------------------------------------------------------
// Frames. [head][text_len][text][raw]. text == NULL: no text_len and no text.
//------------------------------------------------------------------------------------
static void SendFrame(const msg_ID_t id, const uint8_t * const head, const uint8_t headLen,
                      const char * const text, const uint8_t * const raw, const uint16_t rawLen){
    const uint8_t  textLen = text ? (uint8_t)MIN_VAL(strlen(text), (size_t)(SERIAL_LOG_MAX - 1)) : 0;
    const uint16_t len     = MIN_VAL(rawLen, SERIAL_RAW_MAX);
    const uint16_t bodyLen = (uint16_t)(headLen + (text ? 1 + textLen : 0) + len);

    if (!Serial_FrameStart(id, bodyLen)) return;
    Serial_FrameSend(head, headLen);
    if (text) {
        Serial_FrameSend(&textLen, 1);
        Serial_FrameSend((const uint8_t *)text, textLen);
    }
    Serial_FrameSend(raw, len);
    Serial_FrameEnd();
}

static void SendLog(const msg_ID_t id, const LogLevel_t level, const char * const text, const uint8_t * const raw, const uint16_t rawLen){
    const uint8_t head = (uint8_t)level;
    SendFrame(id, &head, 1, text, raw, rawLen);
}

//------------------------------------------------------------------------------------
// Text is built in a buffer, so printf and arrays are possible. It is cut if it is too long.
//------------------------------------------------------------------------------------
typedef struct {
    char   text[SERIAL_LOG_MAX];
    size_t len;
} LogText_t;

static void LogText_VAppend(LogText_t * const t, const char * const fmt, va_list args){
    int n;

    if (t->len >= SERIAL_LOG_MAX - 1) return;

    n = vsnprintf(&t->text[t->len], SERIAL_LOG_MAX - t->len, fmt, args);
    if (n < 0) {
        t->text[t->len] = '\0';
        return;
    }
    t->len = MIN_VAL(t->len + (size_t)n, (size_t)(SERIAL_LOG_MAX - 1));
}

static void LogText_Append(LogText_t * const t, const char * const fmt, ...){
    va_list args;
    va_start(args, fmt);
    LogText_VAppend(t, fmt, args);
    va_end(args);
}

//------------------------------------------------------------------------------------
// Logs
//------------------------------------------------------------------------------------
void Log_Text(const LogLevel_t level, const char * const text){
    if (level < LOG_MIN_LEVEL) return;
    SendLog(MSG_LOG, level, text, NULL, 0);
}

void Log_Printf(const LogLevel_t level, const char * const fmt, ...){
    LogText_t t = {"", 0};
    va_list args;

    if (level < LOG_MIN_LEVEL) return;

    va_start(args, fmt);
    LogText_VAppend(&t, fmt, args);
    va_end(args);
    SendLog(MSG_LOG, level, t.text, NULL, 0);
}

void Log_Raw(const LogLevel_t level, const char * const text, const uint8_t * const data, const uint16_t len){
    if (level < LOG_MIN_LEVEL) return;
    SendLog(MSG_LOG_RAW, level, text, data, len);
}

void Log_Int(const LogLevel_t level, const char * const name, const int32_t value){
    Log_Printf(level, "%s=%ld", name, (long)value);
}

void Log_UInt(const LogLevel_t level, const char * const name, const uint32_t value){
    Log_Printf(level, "%s=%lu", name, (unsigned long)value);
}

void Log_Hex(const LogLevel_t level, const char * const name, const uint32_t value){
    Log_Printf(level, "%s=0x%08lX", name, (unsigned long)value);
}

void Log_Float(const LogLevel_t level, const char * const name, const float value){
    Log_Printf(level, "%s=%.4f", name, (double)value);
}

void Log_Bool(const LogLevel_t level, const char * const name, const bool value){
    Log_Printf(level, "%s=%s", name, value ? "true" : "false");
}

typedef enum { ELEM_U8, ELEM_U16, ELEM_FLOAT } ElemType_t;

static void LogArray(const LogLevel_t level, const char * const name, const void * const arr, const uint16_t count, const ElemType_t type){
    LogText_t t = {"", 0};
    uint16_t i;

    if (level < LOG_MIN_LEVEL) return;

    LogText_Append(&t, "%s=[", name);
    for (i = 0; i < count; i++) {
        if (i > 0) LogText_Append(&t, ", ");
        switch (type) {
            case ELEM_U8:    LogText_Append(&t, "%u",   (unsigned)((const uint8_t  *)arr)[i]); break;
            case ELEM_U16:   LogText_Append(&t, "%u",   (unsigned)((const uint16_t *)arr)[i]); break;
            case ELEM_FLOAT: LogText_Append(&t, "%.4f", (double)  ((const float    *)arr)[i]); break;
        }
    }
    LogText_Append(&t, "]");
    SendLog(MSG_LOG, level, t.text, NULL, 0);
}

void Log_ArrayU8(const LogLevel_t level, const char * const name, const uint8_t * const arr, const uint16_t count){
    LogArray(level, name, arr, count, ELEM_U8);
}

void Log_ArrayU16(const LogLevel_t level, const char * const name, const uint16_t * const arr, const uint16_t count){
    LogArray(level, name, arr, count, ELEM_U16);
}

void Log_ArrayFloat(const LogLevel_t level, const char * const name, const float * const arr, const uint16_t count){
    LogArray(level, name, arr, count, ELEM_FLOAT);
}

void Log_Fault(const FaultId_t id, const char * const fmt, ...){
    LogText_t t = {"", 0};
    va_list args;
    uint8_t head[2];

    va_start(args, fmt);
    LogText_VAppend(&t, fmt, args);
    va_end(args);

    write_u16_BE(head, (uint16_t)id);
    SendFrame(MSG_FAULT, head, sizeof(head), t.text, NULL, 0);
}

//------------------------------------------------------------------------------------
// CAN and raw
//------------------------------------------------------------------------------------
// Length of the data in a message box. Add a case for a box that is not the default length.
static uint8_t CanMsgLen(const CanBox_t msgBox){
    switch (msgBox) {
        default: return CAN_MAX_DLC;
    }
}

void Log_CanLen(const CanBox_t msgBox, const uint8_t * const data, const uint8_t len){
#if SERIAL_LOG_CAN
    const uint8_t box = (uint8_t)msgBox;        // the enum can be bigger than a byte
    SendFrame(MSG_CAN, &box, 1, NULL, data, len);
#else
    (void)msgBox;
    (void)data;
    (void)len;
#endif
}

void Log_Can(const CanBox_t msgBox, const uint8_t * const data){
    Log_CanLen(msgBox, data, CanMsgLen(msgBox));
}

void Send_Raw(const uint8_t * const data, const uint16_t len){
    SendFrame(MSG_RAW, NULL, 0, NULL, data, len);
}

//------------------------------------------------------------------------------------
bool LogLimiter_Due(LogLimiter_t * const limiter, const ticks_t period){
    if (limiter->started && !hasTimeElapsed(limiter->last, period)) return false;

    limiter->last    = getNow_tick();
    limiter->started = true;
    return true;
}
