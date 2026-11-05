/*
 * LogData.c
 *
 *  A Compress_ function reads the data and writes it compressed with the Put_ functions.
 *  It has to write exactly the len in the sections[] table. The Python side has a decoder
 *  for each section, with the same sizes and scales.
 *  Only the 0th row (newest) of the history arrays is sent.
 */

#include <math.h>
#include <string.h>

#include "SlaveCommunation_Hardware.h"
#include "SlaveCommunation_Functions.h"
#include "BatteryCell_Hardware.h"
#include "FullBattery_Hardware.h"
#include "BMS_Data.h"
#include "Charger.h"

#include "LogData.h"

#define SECTION_HEADER_LEN  3           // id, length (2)

#define PWM_NIBBLE_BYTES    ((NUMBER_OF_CELLS_SERIES + 1) / 2)

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

static SerialStats_t stats;

//------------------------------------------------------------------------------------
// Writer. Put_ calls add to buf. A write that does not fit is skipped and sets overflow.
//------------------------------------------------------------------------------------
typedef struct {
    uint8_t * const buf;
    const uint16_t  cap;
    uint16_t        len;
    bool            overflow;
} ByteWriter_t;

static bool Writer_Reserve(ByteWriter_t * const w, const uint16_t n){
    if (w->overflow || (w->len + n) > w->cap) {
        w->overflow = true;
        return false;
    }
    return true;
}

static void Put_U8(ByteWriter_t * const w, const uint8_t v){
    if (!Writer_Reserve(w, 1)) return;
    w->buf[w->len++] = v;
}

static void Put_U16(ByteWriter_t * const w, const uint16_t v){
    if (!Writer_Reserve(w, 2)) return;
    write_u16_BE(&w->buf[w->len], v);
    w->len += 2;
}

static void Put_I16(ByteWriter_t * const w, const int16_t v){
    Put_U16(w, (uint16_t)v);
}

static void Put_Float(ByteWriter_t * const w, const float v){
    if (!Writer_Reserve(w, 4)) return;
    write_f_BE(&w->buf[w->len], v);
    w->len += 4;
}

static void Put_Bytes(ByteWriter_t * const w, const uint8_t * const src, const uint16_t n){
    if (!Writer_Reserve(w, n)) return;
    memcpy(&w->buf[w->len], src, n);
    w->len += n;
}

// float -> integer with rounding, saturation, NaN = 0
static uint16_t ToU16(const float v, const float scale){
    return isnan(v) ? 0U : (uint16_t)clampValue(roundf(v * scale), 0.0, 65535.0);
}

static int16_t ToI16(const float v, const float scale){
    return isnan(v) ? 0 : (int16_t)clampValue(roundf(v * scale), -32768.0, 32767.0);
}

static void Put_ScaledU16(ByteWriter_t * const w, const float * const src, const uint16_t count, const float scale){
    uint16_t i;
    for (i = 0; i < count; i++) Put_U16(w, ToU16(src[i], scale));
}

static void Put_ScaledI16(ByteWriter_t * const w, const float * const src, const uint16_t count, const float scale){
    uint16_t i;
    for (i = 0; i < count; i++) Put_I16(w, ToI16(src[i], scale));
}

//------------------------------------------------------------------------------------
// Compress
//------------------------------------------------------------------------------------
static void Compress_CellVolt(ByteWriter_t * const w){
#warning Compress_CellVolt: float -> uint16 scale (SCALE_CELL_VOLT) not checked
    Put_ScaledU16(w, Cell_GetVoltReadPtr(0), NUMBER_OF_CELLS_SERIES, SCALE_CELL_VOLT);
}

static void Compress_CellTemp(ByteWriter_t * const w){
#warning Compress_CellTemp: float -> int16 scale (SCALE_TEMP) not checked
    Put_ScaledI16(w, Thermistor_GetReadPtr(0), NUMBER_OF_THERMISTORS_TOTAL, SCALE_TEMP);
}

static void Compress_CellRes(ByteWriter_t * const w){
#warning Compress_CellRes: float -> uint16 scale (SCALE_RES) not checked
    Put_ScaledU16(w, Cell_GetResReadPtr(0), NUMBER_OF_CELLS_SERIES, SCALE_RES);
}

static void Compress_CellSOC(ByteWriter_t * const w){
#warning Compress_CellSOC: float -> uint16 scale (SCALE_SOC) not checked
    Put_ScaledU16(w, Cell_GetSOCReadPtr(0), NUMBER_OF_CELLS_SERIES, SCALE_SOC);
}

static void Compress_Pack(ByteWriter_t * const w){
#warning Compress_Pack: float -> integer scales not checked
    Put_U16(w, ToU16(Pack_GetVolt(),    SCALE_PACK_VOLT));
    Put_I16(w, ToI16(Pack_GetCurrent(), SCALE_CURRENT));
    Put_U16(w, ToU16(Pack_GetSOC(),     SCALE_SOC));
    Put_U16(w, ToU16(Pack_GetRes(),     SCALE_RES));
}

static void Compress_Imd(ByteWriter_t * const w){
#warning Compress_Imd: not compressed, the 4 floats are sent as they are
    const IMDData_t imd = IMD_GetData();

    Put_Float(w, imd.duty);
    Put_Float(w, imd.freq);
    Put_Float(w, imd.isolation);
    Put_Float(w, imd.resistance);
}

static void Compress_Charger(ByteWriter_t * const w){
#warning Compress_Charger: units of the voltage / current / status are not checked
    Put_U8 (w, (uint8_t)Charger_GetState());
    Put_U16(w, Charger_GetOutputVoltage16());
    Put_I16(w, Charger_GetOutputCurrent16());
    Put_U8 (w, Charger_GetStatusFlags());
}

static void Compress_SlaveState(ByteWriter_t * const w){
    const StatusReg_t * const slave = GetStatusRegData();

    Put_Bytes(w, slave->bytes, sizeof(slave->bytes));
}

static void Compress_Fans(ByteWriter_t * const w){
//    Put_Bytes(w, GetFansDuty_ReadPrt(), NUMBER_OF_FANS_TOTAL);
}

static void Compress_Pwm(ByteWriter_t * const w){
//    Put_Bytes(w, Cell_GetBalanceNibblesReadPtr(), PWM_NIBBLE_BYTES);    // already 2 cells per byte
}

static void Compress_BmsState(ByteWriter_t * const w){
    Put_U8 (w, (uint8_t)BMS_GetState());
    Put_U8 (w, (uint8_t)BMS_GetContactorState());
    Put_U16(w, BMS_GetFaultFlags());
}

//------------------------------------------------------------------------------------
// Order here = order on the wire. enabled is the start up value, Serial_EnableSection() changes it.
// A new section needs: an id in SectionId_t, a LEN_ macro, a Compress_ function, a line here
// and a Section(...) in uart_receiver.py.
//------------------------------------------------------------------------------------
typedef struct {
    const SectionId_t id;
    volatile bool     enabled;
    const uint16_t    len;                              // bytes the compress function writes
    void            (*const compress)(ByteWriter_t * const w);
} Section_t;

static Section_t sections[] = {
    { SEC_CELL_VOLT,   true,  LEN_CELL_VOLT,   Compress_CellVolt   },
    { SEC_CELL_TEMP,   false, LEN_CELL_TEMP,   Compress_CellTemp   },
    { SEC_CELL_RES,    false, LEN_CELL_RES,    Compress_CellRes    },
    { SEC_CELL_SOC,    false, LEN_CELL_SOC,    Compress_CellSOC    },
    { SEC_PACK,        false, LEN_PACK,        Compress_Pack       },
    { SEC_IMD,         false, LEN_IMD,         Compress_Imd        },
    { SEC_CHARGER,     false, LEN_CHARGER,     Compress_Charger    },
    { SEC_SLAVE_STATE, true,  LEN_SLAVE_STATE, Compress_SlaveState },
    { SEC_FANS,        false, LEN_FANS,        Compress_Fans       },
    { SEC_PWM,         true,  LEN_PWM,         Compress_Pwm        },
    { SEC_BMS_STATE,   false, LEN_BMS_STATE,   Compress_BmsState   },
};

// Puts [id][len][data] in buf. Returns the number of bytes in buf.
static uint16_t Section_Build(const Section_t * const s, uint8_t * const buf){
    uint8_t * const data = &buf[SECTION_HEADER_LEN];
    ByteWriter_t w = { data, s->len, 0, false };

    buf[0] = (uint8_t)s->id;
    write_u16_BE(&buf[1], s->len);
    s->compress(&w);

    if (w.overflow || w.len != s->len) {        // the frame length was already sent, so keep the size and zero it
        memset(data, 0, s->len);
        stats.sizeMismatch++;
        stats.badSection = (uint8_t)s->id;
    }
    return (uint16_t)(SECTION_HEADER_LEN + s->len);
}

void SendDataRoutine_Serial(void){
    bool     send[ARRAY_LEN(sections)];         // one copy, so the length and the sections agree
    uint8_t  buf[SECTION_HEADER_LEN + SERIAL_SECTION_MAX];
    uint16_t bodyLen = 0;
    size_t   i;

    for (i = 0; i < ARRAY_LEN(sections); i++) {
        send[i] = sections[i].enabled;
        if (send[i]) bodyLen += SECTION_HEADER_LEN + sections[i].len;
    }

    if (!Serial_FrameStart(MSG_DATA, bodyLen)) return;
    for (i = 0; i < ARRAY_LEN(sections); i++) {
        if (send[i]) Serial_FrameSend(buf, Section_Build(&sections[i], buf));
    }
    Serial_FrameEnd();
}

void Serial_EnableSection(const SectionId_t id, const bool enable){
    size_t i;

    for (i = 0; i < ARRAY_LEN(sections); i++) {
        if (sections[i].id == id) sections[i].enabled = enable;
    }
}

const SerialStats_t * Serial_GetStats(void){
    stats.framesDropped = Serial_GetDroppedFrames();
    return &stats;
}
