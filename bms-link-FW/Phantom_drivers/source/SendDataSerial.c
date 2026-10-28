/*
 * SendDataSerial.c
 *
 *  Created on: Jul 31, 2026
 *      Author: tanjo
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "SlaveCommunation_Functions.h"
#include "BatteryData.h"
#include "Fault_handler.h"
#include "spi_helpers.h"
#include "SendDataSerial.h"
#include "PhantomHelpers.h"
#include "PhantomTimers.h"
#include "Phantom_Can.h"
#include "Charger.h"

#define FRAME_HEADER_LEN    4           // SOH, id, length (2)
#define TICK_LEN            8
#define SECTION_HEADER_LEN  3           // id, length (2)
#define CRC_INIT            0xFFFFU

// Compile time check: the build stops with the name of the problem in the error
#define STATIC_ASSERT(cond, name)   typedef char name[(cond) ? 1 : -1]

// A send is only 'atomic' if nothing else can start one halfway. Put the IRQ disable / enable here
// if anything logs from an ISR. They only wrap the busy flag, not the (slow) UART send.
#ifndef SERIAL_CRITICAL_ENTER
#warning SERIAL_CRITICAL_ENTER / EXIT are empty, fill them in if logs can be sent from an ISR
#define SERIAL_CRITICAL_ENTER()     ((void)0)
#define SERIAL_CRITICAL_EXIT()      ((void)0)
#endif

static volatile bool txBusy = false;
static uint16_t      frameCrc;
static SerialStats_t stats;

//------------------------------------------------------------------------------------
// Transport. To send over SPI (or anything else) only change this function.
//------------------------------------------------------------------------------------
static void Transport_Send(const uint8_t * const data, const uint16_t len){
#if SERIAL_TRANSPORT == SERIAL_TRANSPORT_SCI
    sciSend(UART_REG, len, (uint8_t *)data);        // sciSend does not change the data, it just has no const
#elif SERIAL_TRANSPORT == SERIAL_TRANSPORT_SPI
#warning SPI transport is not written, put the SPI send in Transport_Send
    UNUSED(data);
    UNUSED(len);
#endif
}

//------------------------------------------------------------------------------------
// Frames. Start, any number of Frame_Send, End. The length has to be known at the start.
// Only one frame at a time: Frame_Start takes the busy flag, Frame_End gives it back.
//------------------------------------------------------------------------------------
#if SERIAL_USE_CRC
// CRC-16/CCITT, poly 0x1021, init CRC_INIT. Same as crc16_ccitt() but it can continue where it stopped.
static uint16_t Crc16_Update(uint16_t crc, const uint8_t * data, uint16_t len){
    uint8_t bit;

    while (len--) {
        crc ^= (uint16_t)((uint16_t)(*data++) << 8);
        for (bit = 0; bit < 8; bit++) {
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}
#endif

static bool Tx_Lock(void){
    bool idle;

    SERIAL_CRITICAL_ENTER();
    idle = !txBusy;
    txBusy = true;
    SERIAL_CRITICAL_EXIT();
    return idle;
}

static bool Frame_Start(const msg_ID_t id, const uint16_t bodyLen){
    uint8_t hdr[FRAME_HEADER_LEN + TICK_LEN];
    const uint64_t tick = getNow_tick();

    if (!Tx_Lock()) {
        stats.framesDropped++;
        return false;
    }
    hdr[0] = SOH_BYTE;
    hdr[1] = (uint8_t)id;
    write_u16_BE(&hdr[2], (uint16_t)(TICK_LEN + bodyLen));
    write_u32_BE(&hdr[4], (uint32_t)(tick >> 32));
    write_u32_BE(&hdr[8], (uint32_t)tick);

    Transport_Send(hdr, sizeof(hdr));
#if SERIAL_USE_CRC
    frameCrc = Crc16_Update(CRC_INIT, &hdr[1], sizeof(hdr) - 1);
#endif
    return true;
}

static void Frame_Send(const uint8_t * const data, const uint16_t len){
    if (len == 0) return;

    Transport_Send(data, len);
#if SERIAL_USE_CRC
    frameCrc = Crc16_Update(frameCrc, data, len);
#endif
}

static void Frame_End(void){
    uint8_t tail[3];
    uint8_t n = 0;

#if SERIAL_USE_CRC
    write_u16_BE(tail, frameCrc);
    n = 2;
#endif
    tail[n++] = EOT_BYTE;
    Transport_Send(tail, n);
    txBusy = false;
}

// [head][raw]
static void SendRawFrame(const msg_ID_t id, const uint8_t * const head, const uint8_t headLen, const uint8_t * const raw, const uint16_t rawLen){
    const uint16_t len = MIN_VAL(rawLen, SERIAL_RAW_MAX);

    if (!Frame_Start(id, headLen + len)) return;
    Frame_Send(head, headLen);
    Frame_Send(raw, len);
    Frame_End();
}

// [head][text_len][text][raw]
static void SendTextFrame(const msg_ID_t id, const uint8_t * const head, const uint8_t headLen,
                          const char * const text, const uint16_t textLen,
                          const uint8_t * const raw, const uint16_t rawLen){
    const uint8_t  textLen8 = (uint8_t)MIN_VAL(textLen, SERIAL_LOG_MAX - 1);
    const uint16_t len      = MIN_VAL(rawLen, SERIAL_RAW_MAX);

    if (!Frame_Start(id, headLen + 1 + textLen8 + len)) return;
    Frame_Send(head, headLen);
    Frame_Send(&textLen8, 1);
    Frame_Send((const uint8_t *)text, textLen8);
    Frame_Send(raw, len);
    Frame_End();
}

//------------------------------------------------------------------------------------
// Log text. Built in a buffer, so printf and arrays are possible. The text is cut if it is too long.
//------------------------------------------------------------------------------------
STATIC_ASSERT(SERIAL_LOG_MAX >= 2 && SERIAL_LOG_MAX <= 256, SERIAL_LOG_MAX_must_be_2_to_256_so_the_text_len_fits_a_byte);

typedef struct {
    char     text[SERIAL_LOG_MAX];
    uint16_t len;
} LogText_t;

static void LogText_VAppend(LogText_t * const t, const char * const fmt, va_list args){
    int n;

    if (t->len >= SERIAL_LOG_MAX - 1) return;
    n = vsnprintf(&t->text[t->len], SERIAL_LOG_MAX - t->len, fmt, args);
    if (n < 0) return;
    t->len = (uint16_t)MIN_VAL(t->len + n, SERIAL_LOG_MAX - 1);
}

static void LogText_Append(LogText_t * const t, const char * const fmt, ...){
    va_list args;
    va_start(args, fmt);
    LogText_VAppend(t, fmt, args);
    va_end(args);
}

static bool LevelEnabled(const LogLevel_t level){
    return level >= LOG_MIN_LEVEL;
}

static uint16_t TextLen(const char * const text){
    return (uint16_t)MIN_VAL(strlen(text), SERIAL_LOG_MAX - 1);
}

static void SendLog(const LogLevel_t level, const char * const text, const uint16_t textLen, const uint8_t * const raw, const uint16_t rawLen, const msg_ID_t id){
    const uint8_t head = (uint8_t)level;
    SendTextFrame(id, &head, 1, text, textLen, raw, rawLen);
}

//------------------------------------------------------------------------------------
// Logs
//------------------------------------------------------------------------------------
void Log_Text(const LogLevel_t level, const char * const text){
    if (!LevelEnabled(level)) return;
    SendLog(level, text, TextLen(text), NULL, 0, MSG_LOG);
}

void Log_Printf(const LogLevel_t level, const char * const fmt, ...){
    LogText_t t;
    va_list args;
    if (!LevelEnabled(level)) return;

    t.len = 0;
    va_start(args, fmt);
    LogText_VAppend(&t, fmt, args);
    va_end(args);
    SendLog(level, t.text, t.len, NULL, 0, MSG_LOG);
}

void Log_Raw(const LogLevel_t level, const char * const text, const uint8_t * const data, const uint16_t len){
    if (!LevelEnabled(level)) return;
    SendLog(level, text, TextLen(text), data, len, MSG_LOG_RAW);
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
    LogText_t t;
    uint16_t i;
    if (!LevelEnabled(level)) return;

    t.len = 0;
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
    SendLog(level, t.text, t.len, NULL, 0, MSG_LOG);
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
    LogText_t t;
    va_list args;
    uint8_t head[2];

    t.len = 0;
    va_start(args, fmt);
    LogText_VAppend(&t, fmt, args);
    va_end(args);

    write_u16_BE(head, (uint16_t)id);
    SendTextFrame(MSG_FAULT, head, sizeof(head), t.text, t.len, NULL, 0);
}

// Length of the data in a message box. Add a case for a box that is not the default length.
static uint8_t CanMsgLen(const phantomCanMsgBox msgBox){
    switch (msgBox) {
        default: return DEFULT_CAN_MSG_SIZE_BYTE;
    }
}

void Log_CanLen(const phantomCanMsgBox msgBox, const uint8_t * const data, const uint8_t len){
#if SERIAL_LOG_CAN
    SendRawFrame(MSG_CAN, &msgBox, 1, data, len);
#else
    UNUSED(msgBox);
    UNUSED(data);
    UNUSED(len);
#endif
}

void Log_Can(const phantomCanMsgBox msgBox, const uint8_t * const data){
    Log_CanLen(msgBox, data, CanMsgLen(msgBox));
}

void Send_Raw(const uint8_t * const data, const uint16_t len){
    SendRawFrame(MSG_RAW, NULL, 0, data, len);
}

bool LogLimiter_Due(LogLimiter_t * const limiter, const ticks_t period){
    if (limiter->started && !hasTimeElapsed(limiter->last, period)) return false;

    limiter->last    = getNow_tick();
    limiter->started = true;
    return true;
}

//------------------------------------------------------------------------------------
// Data sections. A Compress_ function reads the data, writes it compressed to out and
// returns how many bytes it wrote. That has to be the len in the sections[] table.
// The Python side has a decoder for each, with the same sizes and scales.
// Only the 0th row (newest) of the history arrays is sent.
//------------------------------------------------------------------------------------
#define LEN_CELL_VOLT       (2 * NUMBER_OF_CELLS_SERIES)
#define LEN_CELL_TEMP       (2 * NUMBER_OF_THERMISTORS_TOTAL)
#define LEN_CELL_RES        (2 * NUMBER_OF_CELLS_SERIES)
#define LEN_CELL_SOC        (2 * NUMBER_OF_CELLS_SERIES)
#define LEN_PACK            8
#define LEN_IMD             16
#define LEN_CHARGER         6
#define LEN_SLAVE_STATE     (6 * NUMBER_OF_SLAVE_BOARDS_TOTAL)
#define LEN_FANS            NUMBER_OF_FANS_TOTAL
#define LEN_PWM             PWM_NIBBLE_BYTES
#define LEN_BMS_STATE       4

#define CHECK_LEN(name)     STATIC_ASSERT(LEN_##name <= SERIAL_SECTION_MAX, section_##name##_is_longer_than_SERIAL_SECTION_MAX)
CHECK_LEN(CELL_VOLT);
CHECK_LEN(CELL_TEMP);
CHECK_LEN(CELL_RES);
CHECK_LEN(CELL_SOC);
CHECK_LEN(PACK);
CHECK_LEN(IMD);
CHECK_LEN(CHARGER);
CHECK_LEN(SLAVE_STATE);
CHECK_LEN(FANS);
CHECK_LEN(PWM);
CHECK_LEN(BMS_STATE);
STATIC_ASSERT(SEC_COUNT <= 32, more_than_32_sections_the_enable_mask_is_too_small);

// Each Put_ writes at p and returns where the next one goes
static uint8_t * Put_U8(uint8_t * const p, const uint8_t v){
    p[0] = v;
    return p + 1;
}

static uint8_t * Put_U16(uint8_t * const p, const uint16_t v){
    write_u16_BE(p, v);
    return p + 2;
}

static uint8_t * Put_I16(uint8_t * const p, const int16_t v){
    return Put_U16(p, (uint16_t)v);
}

static uint8_t * Put_Float(uint8_t * const p, const float v){
    write_f_BE(p, v);
    return p + 4;
}

static uint8_t * Put_Bytes(uint8_t * const p, const uint8_t * const src, const uint16_t n){
    memcpy(p, src, n);
    return p + n;
}

// float -> integer with rounding, saturation, NaN = 0
static uint16_t ToU16(const float v, const float scale){
    return isnan(v) ? 0U : (uint16_t)clampValue(roundf(v * scale), 0.0, 65535.0);
}

static int16_t ToI16(const float v, const float scale){
    return isnan(v) ? 0 : (int16_t)clampValue(roundf(v * scale), -32768.0, 32767.0);
}

static uint8_t * Put_ScaledU16(uint8_t * p, const float * const src, const uint16_t count, const float scale){
    uint16_t i;
    for (i = 0; i < count; i++) p = Put_U16(p, ToU16(src[i], scale));
    return p;
}

static uint8_t * Put_ScaledI16(uint8_t * p, const float * const src, const uint16_t count, const float scale){
    uint16_t i;
    for (i = 0; i < count; i++) p = Put_I16(p, ToI16(src[i], scale));
    return p;
}

static uint16_t Compress_CellVolt(uint8_t * const out){
#warning Compress_CellVolt: float -> uint16 scale (SCALE_CELL_VOLT) not checked
    return (uint16_t)(Put_ScaledU16(out, Cell_GetVoltReadPtr(0), NUMBER_OF_CELLS_SERIES, SCALE_CELL_VOLT) - out);
}

static uint16_t Compress_CellTemp(uint8_t * const out){
#warning Compress_CellTemp: float -> int16 scale (SCALE_TEMP) not checked
    return (uint16_t)(Put_ScaledI16(out, Thermistor_GetReadPtr(0), NUMBER_OF_THERMISTORS_TOTAL, SCALE_TEMP) - out);
}

static uint16_t Compress_CellRes(uint8_t * const out){
#warning Compress_CellRes: float -> uint16 scale (SCALE_RES) not checked
    return (uint16_t)(Put_ScaledU16(out, Cell_GetResReadPtr(0), NUMBER_OF_CELLS_SERIES, SCALE_RES) - out);
}

static uint16_t Compress_CellSOC(uint8_t * const out){
#warning Compress_CellSOC: float -> uint16 scale (SCALE_SOC) not checked
    return (uint16_t)(Put_ScaledU16(out, Cell_GetSOCReadPtr(0), NUMBER_OF_CELLS_SERIES, SCALE_SOC) - out);
}

static uint16_t Compress_Pack(uint8_t * const out){
#warning Compress_Pack: float -> integer scales not checked
    uint8_t * p = out;

    p = Put_U16(p, ToU16(Pack_GetVolt(),    SCALE_PACK_VOLT));
    p = Put_I16(p, ToI16(Pack_GetCurrent(), SCALE_CURRENT));
    p = Put_U16(p, ToU16(Pack_GetSOC(),     SCALE_SOC));
    p = Put_U16(p, ToU16(Pack_GetRes(),     SCALE_RES));
    return (uint16_t)(p - out);
}

static uint16_t Compress_Imd(uint8_t * const out){
#warning Compress_Imd: not compressed, the 4 floats are sent as they are
    const IMDData_t imd = IMD_GetData();
    uint8_t * p = out;

    p = Put_Float(p, imd.duty);
    p = Put_Float(p, imd.freq);
    p = Put_Float(p, imd.isolation);
    p = Put_Float(p, imd.resistance);
    return (uint16_t)(p - out);
}

static uint16_t Compress_Charger(uint8_t * const out){
#warning Compress_Charger: units of the voltage / current / status are not checked
    uint8_t * p = out;

    p = Put_U8 (p, (uint8_t)Charger_GetState());
    p = Put_U16(p, Charger_GetOutputVoltage16());
    p = Put_I16(p, Charger_GetOutputCurrent16());
    p = Put_U8 (p, Charger_GetStatusFlags());
    return (uint16_t)(p - out);
}

static uint16_t Compress_SlaveState(uint8_t * const out){
    const StatusReg_t * const slave = GetStatusRegData();
    uint8_t * p = out;
    int i;

    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL; i++) {
        p = Put_U16(p, slave[i].RefVolt2nd);
        p = Put_U16(p, slave[i].ITMP);
        p = Put_U16(p, slave[i].SC);
    }
    return (uint16_t)(p - out);
}

static uint16_t Compress_Fans(uint8_t * const out){
    return (uint16_t)(Put_Bytes(out, GetFansDuty_ReadPrt(), NUMBER_OF_FANS_TOTAL) - out);
}

static uint16_t Compress_Pwm(uint8_t * const out){
    return (uint16_t)(Put_Bytes(out, Cell_GetBalanceNibblesReadPtr(), PWM_NIBBLE_BYTES) - out);    // already 2 cells per byte
}

static uint16_t Compress_BmsState(uint8_t * const out){
    uint8_t * p = out;

    p = Put_U8 (p, (uint8_t)BMS_GetState());
    p = Put_U8 (p, (uint8_t)BMS_GetContactorState());
    p = Put_U16(p, BMS_GetFaultFlags());
    return (uint16_t)(p - out);
}

//------------------------------------------------------------------------------------
// Order here = order on the wire. A new section needs: an id in SectionId_t, a SERIAL_EN_ macro,
// a LEN_ macro, a Compress_ function, a line here and a Section(...) in uart_receiver.py.
//------------------------------------------------------------------------------------
typedef struct {
    SectionId_t id;
    uint16_t    len;                            // bytes the compress function writes
    uint16_t  (*compress)(uint8_t * const out);
} Section_t;

static const Section_t sections[] = {
    { SEC_CELL_VOLT,   LEN_CELL_VOLT,   Compress_CellVolt   },
    { SEC_CELL_TEMP,   LEN_CELL_TEMP,   Compress_CellTemp   },
    { SEC_CELL_RES,    LEN_CELL_RES,    Compress_CellRes    },
    { SEC_CELL_SOC,    LEN_CELL_SOC,    Compress_CellSOC    },
    { SEC_PACK,        LEN_PACK,        Compress_Pack       },
    { SEC_IMD,         LEN_IMD,         Compress_Imd        },
    { SEC_CHARGER,     LEN_CHARGER,     Compress_Charger    },
    { SEC_SLAVE_STATE, LEN_SLAVE_STATE, Compress_SlaveState },
    { SEC_FANS,        LEN_FANS,        Compress_Fans       },
    { SEC_PWM,         LEN_PWM,         Compress_Pwm        },
    { SEC_BMS_STATE,   LEN_BMS_STATE,   Compress_BmsState   },
};

// One bit per section id
#define SEC_MASK(id, enabled)   ((enabled) ? BIT(id) : 0UL)

static volatile uint32_t enabledMask =
    SEC_MASK(SEC_CELL_VOLT,   SERIAL_EN_CELL_VOLT)   |
    SEC_MASK(SEC_CELL_TEMP,   SERIAL_EN_CELL_TEMP)   |
    SEC_MASK(SEC_CELL_RES,    SERIAL_EN_CELL_RES)    |
    SEC_MASK(SEC_CELL_SOC,    SERIAL_EN_CELL_SOC)    |
    SEC_MASK(SEC_PACK,        SERIAL_EN_PACK)        |
    SEC_MASK(SEC_IMD,         SERIAL_EN_IMD)         |
    SEC_MASK(SEC_CHARGER,     SERIAL_EN_CHARGER)     |
    SEC_MASK(SEC_SLAVE_STATE, SERIAL_EN_SLAVE_STATE) |
    SEC_MASK(SEC_FANS,        SERIAL_EN_FANS)        |
    SEC_MASK(SEC_PWM,         SERIAL_EN_PWM)         |
    SEC_MASK(SEC_BMS_STATE,   SERIAL_EN_BMS_STATE);

// Gets the data, compresses it and puts [id][len][data] in buf. Returns the number of bytes in buf.
static uint16_t Section_Put(const Section_t * const s, uint8_t * const buf){
    uint8_t * const data = &buf[SECTION_HEADER_LEN];

    buf[0] = (uint8_t)s->id;
    write_u16_BE(&buf[1], s->len);

    if (s->compress(data) != s->len) {          // the frame length was already sent, so keep the size and zero it
        memset(data, 0, s->len);
        stats.sizeMismatch++;
        stats.badSection = (uint8_t)s->id;
    }
    return (uint16_t)(SECTION_HEADER_LEN + s->len);
}

void SendDataRoutine_Serial(void){
    const uint32_t mask = enabledMask;          // one copy, so the length and the sections agree
    const uint32_t mismatchBefore = stats.sizeMismatch;
    uint8_t buf[SECTION_HEADER_LEN + SERIAL_SECTION_MAX];
    uint16_t bodyLen = 0;
    size_t i;

    for (i = 0; i < ARRAY_LEN(sections); i++) {
        if (mask & BIT(sections[i].id)) bodyLen += SECTION_HEADER_LEN + sections[i].len;
    }

    if (!Frame_Start(MSG_DATA, bodyLen)) return;
    for (i = 0; i < ARRAY_LEN(sections); i++) {
        if (mask & BIT(sections[i].id)) Frame_Send(buf, Section_Put(&sections[i], buf));
    }
    Frame_End();

    if (stats.sizeMismatch != mismatchBefore) {
        LOG_EVERY_SEC(LOG_LEVEL_ERROR, 5, "section %u: Compress_ size is not the len in sections[]", (unsigned)stats.badSection);
    }
}

void Serial_EnableSection(const SectionId_t id, const bool enable){
    SERIAL_CRITICAL_ENTER();
    if (enable) enabledMask |= BIT(id);
    else        enabledMask &= ~BIT(id);
    SERIAL_CRITICAL_EXIT();
}

const SerialStats_t * Serial_GetStats(void){
    return &stats;
}
