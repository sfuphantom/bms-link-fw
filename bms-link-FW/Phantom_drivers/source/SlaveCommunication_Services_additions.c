/*
 * Additions to SlaveCommunation_Services.c
 *
 * Also note these bugs in the original file:
 *
 * 1. wakeup_idle() - missing braces around loop body; only setCS(LOW) is inside the for-loop.
 *    Fix:
 *      for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
 *          setCS(LOW);
 *          SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
 *          setCS(HIGH);
 *      }
 *
 * 2. ClearSlaveRegs() - same brace bug; only setCS(LOW) is inside the for-loop.
 *    Fix:
 *      for (i = 0; i < NUMBER_OF_CLEAR_CMDS; i++) {
 *          setCS(LOW);
 *          SendCmdAndPec2Slave(All_Cear_CMDs[i]);
 *          setCS(HIGH);
 *      }
 *
 * 3. GetValueConfigReg_A() - missing break statements in switch; every case falls through.
 *
 * 4. Write_CFGR_A() - wrapped in #ifndef BYTE_LENGTH but BYTE_LENGTH is never defined,
 *    so the byte-based path is always compiled and the word-based #else is dead code.
 *    Decide which path you want and remove the conditional, or define BYTE_LENGTH.
 */

#include "spi.h"
#include <stdint.h>
#include <stdbool.h>
#include "ltc6811_commands.h"
#include "SlaveCommunication.h"
#include "SlaveCommunication_Services.h"
#include "PhantomHelpers.h"



float Slave_SC2Volt(uint16_t SC_raw) {
    return (float)SC_raw * 20.0f * 100e-6f;
}

/*
 * Convert raw ITMP ADC word to degrees Celsius.
 * Formula from datasheet: T(°C) = ITMP * 100µV / 7.5mV/°C - 273
 */
float Slave_ITMP2Celsius(uint16_t ITMP_raw) {
    return ((float)ITMP_raw * 100e-6f / 7.5e-3f) - 273.0f;
}



/*
 * Read Status Register Group B into a StatusReg_B struct array.
 *
 * The OV/UV flags are packed two per byte across STBR2-STBR4.
 * Each cell has one OV bit and one UV bit; we unpack them into
 * two separate 12-bit fields (bits 0-11 = cells 1-12).
 *
 * Returns a bitmask where bit i is set if the PEC for board i matched.
 */


//bool MeasureInternalParams(uint8_t MD, uint8_t CHST,
//                           struct StatusReg_A *sta_out,
//                           struct StatusReg_B *stb_out) {
//    uint16_t CHST_bits = CHST & 0x07;
//    uint16_t MD_bits   = (MD & 0x03) << 7;
//    uint16_t cmd       = LTC6811_ADSTAT | MD_bits | 0x0040 | CHST_bits;
//    /* bit 6 = 1 selects ADSTAT (vs ADSTATD) */
//
//    bool done = SendCMD2Slave_and_Poll(cmd);
//    if (done) {
//        Read_STATA(sta_out);
//        Read_STATB(stb_out);
//    }
//    return done;
//}

//bool RunMuxDiagnostic(void) {
//    return SendCMD2Slave_and_Poll(LTC6811_DIAGN);
//}

bool MeasureOpenWire(bool PUP, uint8_t MD, bool DCP) {
    uint16_t PUP_bits = PUP  ? 0x0040 : 0x0000;
    uint16_t DCP_bits = DCP  ? 0x0010 : 0x0000;
    uint16_t MD_bits  = (MD & 0x03) << 7;
    uint16_t cmd = LTC6811_ADOW | MD_bits | PUP_bits | 0x0020 | DCP_bits;
    return SendCMD2Slave_and_Poll(cmd);
}

void CheckOpenWires(uint8_t MD, uint16_t *cell_voltages, uint16_t *open_wire_mask) {
    uint16_t cell_pu[12] = {0};
    uint16_t cell_pd[12] = {0};

    /* Two pull-up passes */
    MeasureOpenWire(TRUE, MD, FALSE);
    MeasureOpenWire(TRUE, MD, FALSE);
    GetVoltageReadings(cell_pu);   /* last reading is what counts */

    /* Two pull-down passes */
    MeasureOpenWire(FALSE, MD, FALSE);
    MeasureOpenWire(FALSE, MD, FALSE);
    GetVoltageReadings(cell_pd);

    *open_wire_mask = 0x0000;

    /* C0 open: pull-up reading for cell 1 (index 0) == 0 */
    if (cell_pu[0] == 0x0000)
        *open_wire_mask |= (1 << 0);

    /* C12 open: pull-down reading for cell 12 (index 11) == 0 */
    if (cell_pd[11] == 0x0000)
        *open_wire_mask |= (1 << 12);

    /* C(n) open: CELL_delta(n+1) = cell_pu[n] - cell_pd[n] < -400mV
     * -400mV in ADC counts = -4000 counts (100µV per count)
     * Since these are unsigned, we check if cell_pd > cell_pu by > 4000 */
    int n;
    for (n = 0; n < 11; n++) {
        int32_t delta = (int32_t)cell_pu[n + 1] - (int32_t)cell_pd[n + 1];
        if (delta < -4000)
            *open_wire_mask |= (1 << (n + 1));
    }

    /* Return the pull-up readings as reference voltages */
    int k;
    for (k = 0; k < 12; k++)
        cell_voltages[k] = cell_pu[k];
}

/* ============================================================
 *  ADC SELF-TEST  (CVST / AXST / STATST)
 * ============================================================ */

typedef enum {
    SELF_TEST_1 = 1,   /* expects 0x9555 in 7kHz/normal mode */
    SELF_TEST_2 = 2    /* expects 0x6AAA in 7kHz/normal mode */
} SelfTestMode;

bool RunCellVoltageSelfTest(uint8_t MD, SelfTestMode ST) {
    uint16_t ST_bits = ((uint16_t)ST & 0x03) << 5;
    uint16_t MD_bits = ((uint16_t)MD & 0x03) << 7;
    uint16_t cmd = LTC6811_CVST | MD_bits | ST_bits | 0x0007;
    return SendCMD2Slave_and_Poll(cmd);
}

bool RunAuxSelfTest(uint8_t MD, SelfTestMode ST) {
    uint16_t ST_bits = ((uint16_t)ST & 0x03) << 5;
    uint16_t MD_bits = ((uint16_t)MD & 0x03) << 7;
    uint16_t cmd = LTC6811_AXST | MD_bits | ST_bits | 0x0007;
    return SendCMD2Slave_and_Poll(cmd);
}

bool RunStatusSelfTest(uint8_t MD, SelfTestMode ST) {
    uint16_t ST_bits = ((uint16_t)ST & 0x03) << 5;
    uint16_t MD_bits = ((uint16_t)MD & 0x03) << 7;
    uint16_t cmd = LTC6811_STATST | MD_bits | ST_bits | 0x000F;
    return SendCMD2Slave_and_Poll(cmd);
}

/*
 * Validate a cell voltage register readback against the expected
 * self-test pattern for a given ADC mode.
 *
 * Returns true if all 12 cell readings match the expected pattern.
 *
 * Expected patterns (Table 12):
 *   ST1, 27kHz/14kHz: 0x9565/0x9553
 *   ST1, all others:  0x9555
 *   ST2, 27kHz/14kHz: 0x6A9A/0x6AAC
 *   ST2, all others:  0x6AAA
 */
bool ValidateSelfTestPattern(uint16_t *cell_readings,
                             uint8_t MD,
                             SelfTestMode ST,
                             bool adcopt) {
    /* Determine expected value based on mode */
    uint16_t expected;
    /* Resolve effective mode index (0=422/1kHz, 1=27k/14k, 2=7k/3k, 3=26Hz/2kHz) */
    /* With adcopt=0: MD01=27kHz, MD10=7kHz; adcopt=1: MD01=14kHz, MD10=3kHz */
    bool is_fast_mode = (!adcopt && MD == 1) || (adcopt && MD == 1); /* 27kHz or 14kHz */

    if (ST == SELF_TEST_1)
        expected = is_fast_mode ? (adcopt ? 0x9553 : 0x9565) : 0x9555;
    else
        expected = is_fast_mode ? (adcopt ? 0x6AAC : 0x6A9A) : 0x6AAA;

    int i;
    for (i = 0; i < 12; i++) {
        if (cell_readings[i] != expected)
            return FALSE;
    }
    return TRUE;
}

void SetUVThreshold(float voltage_V) {
    uint16_t vuv = (uint16_t)(voltage_V / (16.0f * 100e-6f));
    if (vuv > 0) vuv -= 1;
    vuv &= 0x0FFF;

    uint16_t data[NUMBER_OF_SLAVE_BOARDS];
    int i;
    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++)
        data[i] = vuv;
    SetValueConfigReg_A(data, VUV);
}

void SetOVThreshold(float voltage_V) {
    uint16_t vov = (uint16_t)(voltage_V / (16.0f * 100e-6f));
    vov &= 0x0FFF;

    uint16_t data[NUMBER_OF_SLAVE_BOARDS];
    int i;
    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++)
        data[i] = vov;
    SetValueConfigReg_A(data, VOV);
}

void CheckOVUVFlags(struct StatusReg_B *stb,
                    uint32_t *ov_board,
                    uint32_t *uv_board) {
    *ov_board = 0;
    *uv_board = 0;
    int i;
    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
        if (stb[i].OV_flags) *ov_board |= (1 << i);
        if (stb[i].UV_flags) *uv_board |= (1 << i);
    }
}

//void EnableCellDischarge(uint8_t cell_num) {
//    if (cell_num < 1 || cell_num > 12) return;
//    int i;
//    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
//        ConfigRegData_A[i].DCC |= (1 << (cell_num - 1));
//    }
//}

//void DisableCellDischarge(uint8_t cell_num) {
//    if (cell_num < 1 || cell_num > 12) return;
//    int i;
//    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
//        ConfigRegData_A[i].DCC &= ~(1 << (cell_num - 1));
//    }
//}
//void DisableAllDischarge(void) {
//    uint16_t data[NUMBER_OF_SLAVE_BOARDS];
//    int i;
//    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++)
//        data[i] = 0x0000;
//    SetValueConfigReg_A(data, DCC);
//}

/* ============================================================
 *  2ND REFERENCE ACCURACY CHECK
 * ============================================================ */

/*
 * Measure and validate the 2nd reference voltage (VREF2) via ADAX.
 * Reads Auxiliary Register Group B after conversion.
 *
 * vref2_readings : output array of VREF2 raw ADC values, one per board
 *
 * The datasheet states readings outside 2.99V–3.01V indicate the system
 * is out of tolerance. Returns true if all boards are within range.
 *
 * 2.99V = 29900 counts, 3.01V = 30100 counts (100µV per count)
 */
bool CheckVREF2Accuracy(uint16_t *vref2_readings) {
    /* CHG = 0b110 = 6 selects 2nd Reference */
    uint16_t cmd = LTC6811_ADAX | (2 << 7) | 0x0006; /* MD=7kHz, CHG=2nd ref */
    SendCMD2Slave_and_Poll(cmd);

    uint16_t aux_data[NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD *
                      WORDS_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS];
    GetGPIOReadings_Analog(aux_data);

    /* REF value is the 6th measurement in the aux group (index 5 per board) */
    bool all_ok = TRUE;
    int i;
    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
        /* Each board contributes 6 uint16_t values; REF is at offset 5 */
        uint16_t ref = aux_data[i * 6 + 5];
        vref2_readings[i] = ref;
        if (ref < 29900 || ref > 30100)
            all_ok = FALSE;
    }
    return all_ok;
}
