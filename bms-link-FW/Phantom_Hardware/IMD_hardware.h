/*
 * IMD_hardware.h
 *
 *  Created on: Oct 25, 2026
 *      Author: tanjo
 *
 *  IMD definition used by IMD.h / IMD.c.
 *  Raw datasheet numbers live in IMD_DataSheetSpec.h (IMD_DS_*).
 *  Rule limits live in FSAE_Rules_2027_EV.h (RULE_*).
 *
 *  Layout:
 *    1. USER PLACEHOLDERS - every tolerance / fault limit / fit. Tune here.
 *    2. DERIVED VALUES    - datasheet and rule values combined with the placeholders.
 */

#ifndef PHANTOM_HARDWARE_IMD_HARDWARE_H_
#define PHANTOM_HARDWARE_IMD_HARDWARE_H_

#include <IMD_DataSheetSpec.h>
#include <FSAE_Rules_2027_EV.h>
#include "hal_stdtypes.h"

//--------------------------------------------------------------------------
// IMD message names are taken from the IMD datasheet
typedef enum {
    Normal_Condition,
    Short_Circuit,
    Undervoltage_Condition,
    Speed_Start_Measurement_Good,
    Speed_Start_Measurement_Bad,
    Device_Error,
    Connection_Fault_Earth,
    Bad_Info,
    Undefined_Fault
} IMDStateEnum;

#define ISOLATION_UNKNOWN   0xFFu

//////////////////////////////////////////////////////////////
// 1. USER PLACEHOLDERS (none of these are on the datasheet)
//////////////////////////////////////////////////////////////

/* ---- Decode tolerance ---- */
#define IMD_DUTY_TOL_PERCENT                (0.5f)      /* Widens every datasheet duty window on both sides */

/* ---- Fault limits ---- */
#define NORMAL_IMD_RESISTANCE_kOHM          (1E3f)      /* Expected healthy insulation, 1 MOhm (reference value) */
#define MIN_IMD_RESISTANCE_kOHM             (1.0f)      /* kOhm PER VOLT of max TS voltage. Rule minimum is 0.5 (EV.8.6.3) */
#define MIN_IMD_ISOLATION_PERCENT           (75.0f)
#define MAX_IMD_ISOLATION_PERCENT           (100.0f)    /* Also catches ISOLATION_UNKNOWN   */
#define MAX_IMD_DUTY_PERCENT                (50.0f)     /* Duty above this is a fault       */

/* ---- Isolation % (NOT on the datasheet) ----
 * Linear map of duty to percent:  iso% = OFFSET - SLOPE * duty
 * Gives about 95% at duty 5% and about 0.5% at duty 95%.                          */
#define IMD_ISOLATION_FIT_OFFSET            (100.53763f)
#define IMD_ISOLATION_FIT_SLOPE             (1.05287f)

//////////////////////////////////////////////////////////////
// 2. DERIVED VALUES
//////////////////////////////////////////////////////////////

/* ---- Frequency decode: upper bound of each state = its frequency + half a step ---- */
#define IMD_FREQ_STEP_HZ                    (IMD_DS_FREQ_NORMAL_HZ - IMD_DS_FREQ_SHORT_CIRCUIT_HZ)
#define IMD_FREQ_UP_SHORT_HZ                (IMD_DS_FREQ_SHORT_CIRCUIT_HZ + IMD_FREQ_STEP_HZ / 2.0f)   /*  5 Hz */
#define IMD_FREQ_UP_NORMAL_HZ               (IMD_DS_FREQ_NORMAL_HZ        + IMD_FREQ_STEP_HZ / 2.0f)   /* 15 Hz */
#define IMD_FREQ_UP_UNDERV_HZ               (IMD_DS_FREQ_UNDERVOLTAGE_HZ  + IMD_FREQ_STEP_HZ / 2.0f)   /* 25 Hz */
#define IMD_FREQ_UP_SST_HZ                  (IMD_DS_FREQ_SST_HZ           + IMD_FREQ_STEP_HZ / 2.0f)   /* 35 Hz */
#define IMD_FREQ_UP_DEVICE_ERR_HZ           (IMD_DS_FREQ_DEVICE_ERROR_HZ  + IMD_FREQ_STEP_HZ / 2.0f)   /* 45 Hz */
#define IMD_FREQ_UP_EARTH_HZ                (IMD_DS_FREQ_EARTH_FAULT_HZ   + IMD_FREQ_STEP_HZ / 2.0f)   /* 55 Hz */

/* ---- Duty windows (datasheet window +/- tolerance) ---- */
#define IMD_DUTY_SST_GOOD_MIN               (IMD_DS_PWM_SST_GOOD_MIN_PERCENT - IMD_DUTY_TOL_PERCENT)
#define IMD_DUTY_SST_GOOD_MAX               (IMD_DS_PWM_SST_GOOD_MAX_PERCENT + IMD_DUTY_TOL_PERCENT)
#define IMD_DUTY_SST_BAD_MIN                (IMD_DS_PWM_SST_BAD_MIN_PERCENT  - IMD_DUTY_TOL_PERCENT)
#define IMD_DUTY_SST_BAD_MAX                (IMD_DS_PWM_SST_BAD_MAX_PERCENT  + IMD_DUTY_TOL_PERCENT)
#define IMD_DUTY_ERR_MIN                    (IMD_DS_PWM_ERR_MIN_PERCENT      - IMD_DUTY_TOL_PERCENT)
#define IMD_DUTY_ERR_MAX                    (IMD_DS_PWM_ERR_MAX_PERCENT      + IMD_DUTY_TOL_PERCENT)

/* ---- Duty <-> resistance (kOhm), datasheet equation ---- */
#define IMD_DUTY_TO_RES_kOHM(D)             ((IMD_DS_PWM_R_NUM_PERCENT * IMD_DS_PWM_R_SCALE_kOHM) / ((D) - IMD_DS_PWM_DUTY_MIN_PERCENT) - IMD_DS_PWM_R_SCALE_kOHM)
#define IMD_RES_kOHM_TO_DUTY(R)             (IMD_DS_PWM_DUTY_MIN_PERCENT + (IMD_DS_PWM_R_NUM_PERCENT * IMD_DS_PWM_R_SCALE_kOHM) / ((R) + IMD_DS_PWM_R_SCALE_kOHM))

#define IMD_RESISTANCE_MAX_kOHM             (IMD_DS_RES_INFINITE_kOHM)   /* "Infinite" (duty <= 5%) is reported as this */

/* Duty at points of interest (sanity checks) */
#define IMD_DUTY_AT_RAN_MIN_PERCENT         IMD_RES_kOHM_TO_DUTY(IMD_DS_RAN_MIN_kOHM)       /* 88.08% */
#define IMD_DUTY_AT_RAN_MAX_PERCENT         IMD_RES_kOHM_TO_DUTY(IMD_DS_RAN_MAX_kOHM)       /* 54.09% */
#define IMD_DUTY_AT_RANGE_MAX_PERCENT       IMD_RES_kOHM_TO_DUTY(IMD_DS_MEAS_RANGE_MAX_kOHM)/* 14.64% */

/* ---- Resistance limits from the rules (max TS voltage x Ohm/V) ---- */
#define IMD_RULE_MIN_RESISTANCE_kOHM        (RULE_IMD_MIN_OHM_PER_V * RULE_TS_VOLT_MAX_V / 1000.0f)   /* 300 kOhm: IMD response value Ran must be at or above this */
#define IMD_FAULT_RESISTANCE_kOHM           (MIN_IMD_RESISTANCE_kOHM * RULE_TS_VOLT_MAX_V)            /* 600 kOhm: resistance below this is a fault */

/* ---- Worst-case fault timing (factory Fave, includes one self test) ---- */
#define IMD_FAULT_ON_DELAY_MAX_SEC          (IMD_DS_TAN_DCP_FAVE_10_SEC + IMD_DS_SELFTEST_DURATION_SEC)
#define IMD_FAULT_OFF_DELAY_MAX_SEC         (IMD_DS_TAB_DCP_FAVE_10_SEC + IMD_DS_SELFTEST_DURATION_SEC)

#endif /* PHANTOM_HARDWARE_IMD_HARDWARE_H_ */
