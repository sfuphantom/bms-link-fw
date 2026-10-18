/*
 * BatteryCell_Hardware.h
 *
 *  Created on: Aug 30, 2026
 *      Author: tanjo
 *
 *  Cell definition used by BatteryCell_P_Group_Hardware.h (its only includer).
 *  Raw datasheet numbers live in BatteryCell_DataSheetSpec.h (CELL_DS_*).
 *
 *  Layout:
 *    1. USER PLACEHOLDERS - every buffer / hysteresis / factor / assumption.
 *                           Tune everything here; nothing below needs editing.
 *    2. CAPACITY, VOLTAGE, CURRENT, TEMPERATURE, RESISTANCE - datasheet value
 *       combined with the placeholders above.
 *    3. GRAPH DATA - approximate values read off the datasheet charts.
 */

#ifndef PHANTOM_HARDWARE_BATTERYCELL_HARDWARE_H_
#define PHANTOM_HARDWARE_BATTERYCELL_HARDWARE_H_

#include "BatteryCell_DataSheetSpec.h"

//////////////////////////////////////////////////////////////
// 1. USER PLACEHOLDERS (none of these are on the datasheet)
//////////////////////////////////////////////////////////////

/* ---- Voltage buffers / hysteresis (V) ---- */
#define CELL_VOLT_SAFETY_BUF                (0.200f)   /* Fault limit pulled in from the datasheet limit   */
#define CELL_VOLT_FAULT_BUF                 (0.100f)   /* Extra margin between fault limit and 0%/100% SOC */
#define CELL_VOLT_RECOVERY_HYST             (0.050f)   /* Hysteresis before clearing an OV/UV fault        */

/* ---- Temperature buffers / hysteresis (C) ---- */
#define CELL_TEMP_SAFETY_BUF                (5.0f)     /* Operating window pulled in from the datasheet limit */
#define CELL_TEMP_RECOVERY_HYST             (2.0f)     /* Hysteresis before clearing an OT/UT fault           */
#define CELL_TEMP_CHARGE_DERATE_BAND_WIDTH_C (10.0f)   /* Reduced charge current this far above the charge min */

/* ---- Safety factors (1.0 = use the datasheet value unchanged) ---- */
#define CELL_CAPACITY_ACTUAL_FACTOR         (0.95f)    /* Datasheet capacity is "typical"; plan on slightly less */
#define CELL_CHARGE_CURRENT_DERATE_FACTOR   (0.90f)    /* Applied to datasheet max charge current     */
#define CELL_DISCHARGE_CURRENT_DERATE_FACTOR (0.90f)   /* Applied to datasheet max discharge current  */
#define CELL_DISCHARGE_PULSE_FACTOR         (1.00f)    /* Pulse = continuous x this; datasheet gives no separate pulse rating */
#define CELL_INTERNAL_RES_MARGIN_FACTOR     (1.25f)    /* Allowance for resistance growth with age/temperature */

/* ---- Other user-chosen values ---- */
#define CELL_DISCHARGE_PULSE_DURATION_SEC   (10.0f)    /* Not specified on datasheet */
#define CELL_CC_CHARGING_CURRENT_AMPS       (3.0f)     /* Our constant-current charge setpoint */

/* Charge target: false = use the voltage below (SOC is calculated),
 *                true  = use the SOC % below (voltage is calculated)   */
#define DEFINE_MAX_CELL_CHARGING_PERCENTAGE_N_VOLTAGE_TARGET    false
#define CELL_CHARGE_TARGET_VOLT_USER        (3.6f)
#define CELL_CHARGE_TARGET_PERCENT_USER     (90.0f)

/* Balancing */
#define CELL_BALANCE_START_BELOW_FULL_V     (0.050f)   /* Start balancing this far below the 100% voltage */
#define CELL_BALANCE_DELTA_TRIGGER_V        (0.020f)   /* Cell mismatch that starts balancing             */
#define CELL_BALANCE_TARGET_DELTA_V         (0.005f)   /* Cell mismatch that stops balancing              */

/* Protection hardware (generic placeholders) */
#define CELL_SHORT_CIRCUIT_CURRENT_THRESHOLD_AMPS   (80.0f)
#define CELL_SHORT_CIRCUIT_RESPONSE_TIME_US         (200.0f)
#define CELL_FUSE_RATED_CURRENT_AMPS                (60.0f)

//////////////////////////////////////////////////////////////
// 2. CELL VALUES
//////////////////////////////////////////////////////////////

/* ---- Physical ---- */
#define CELL_MASS_GRAMS                     (CELL_DS_MASS_GRAMS_MAX)
#define CELL_VOLUME_CM3                     (3.14159265f * (CELL_DS_DIAMETER_MM_MAX/20.0f) * (CELL_DS_DIAMETER_MM_MAX/20.0f) * (CELL_DS_HEIGHT_MM_MAX/10.0f))

/* ---- Capacity ---- */
#define CELL_NOMINAL_VOLT                   (CELL_DS_VOLT_NOMINAL)
#define CELL_CAPACITY_NOMINAL_MAH           (CELL_DS_CAPACITY_TYPICAL_MAH)
#define CELL_CAPACITY_ACTUAL_MAH            (CELL_CAPACITY_NOMINAL_MAH * CELL_CAPACITY_ACTUAL_FACTOR)   /* Override with a measured value if available */

/* ---- Voltage limits ---- */
#define CELL_MAX_CHARGING_VOLT              (CELL_DS_VOLT_CHARGE)
#define CELL_DISCHARGE_CUT_OFF_VOLT         (CELL_DS_VOLT_DISCHARGE_CUTOFF)

#define CELL_VOLT_OVER_F                    (CELL_MAX_CHARGING_VOLT      - CELL_VOLT_SAFETY_BUF)   /* 4.0V */
#define CELL_VOLT_UNDER_F                   (CELL_DISCHARGE_CUT_OFF_VOLT + CELL_VOLT_SAFETY_BUF)   /* 2.7V */

#define CELL_VOLT_OV_FAULT_SET_F            (CELL_VOLT_OVER_F)
#define CELL_VOLT_OV_FAULT_CLEAR_F          (CELL_VOLT_OVER_F  - CELL_VOLT_RECOVERY_HYST)
#define CELL_VOLT_UV_FAULT_SET_F            (CELL_VOLT_UNDER_F)
#define CELL_VOLT_UV_FAULT_CLEAR_F          (CELL_VOLT_UNDER_F + CELL_VOLT_RECOVERY_HYST)

/* Usable window for SOC */
#define CELL_VOLT_100_FULL_F                (CELL_VOLT_OVER_F  - CELL_VOLT_FAULT_BUF)
#define CELL_VOLT_0_FULL_F                  (CELL_VOLT_UNDER_F + CELL_VOLT_FAULT_BUF)
#define CELL_VOLT_OP_RANGE_F                (CELL_VOLT_100_FULL_F - CELL_VOLT_0_FULL_F)

/* SOC (%) <-> resting voltage, linear approximation (used by the P_group header) */
#define CELL_VOLT_TO_SOC_PERCENT(V)         ((((V) - CELL_VOLT_0_FULL_F) / CELL_VOLT_OP_RANGE_F) * 100.0f)
#define CELL_SOC_PERCENT_TO_VOLT(SOC)       (CELL_VOLT_0_FULL_F + (((SOC) / 100.0f) * CELL_VOLT_OP_RANGE_F))

/* ---- Charge target ---- */
//CELL_CHARGING_TARGET_VOLTS
#if DEFINE_MAX_CELL_CHARGING_PERCENTAGE_N_VOLTAGE_TARGET
#define MAX_CELL_CHARGING_PERCENTAGE_F          (CELL_CHARGE_TARGET_PERCENT_USER / 100.0f)
#define MAX_CELL_CHARGING_VOLTAGE_TARGET_F      ((MAX_CELL_CHARGING_PERCENTAGE_F * CELL_VOLT_OP_RANGE_F) + CELL_VOLT_0_FULL_F)
#else
#define MAX_CELL_CHARGING_VOLTAGE_TARGET_F      (CELL_CHARGE_TARGET_VOLT_USER)
#define MAX_CELL_CHARGING_PERCENTAGE_F          ((MAX_CELL_CHARGING_VOLTAGE_TARGET_F - CELL_VOLT_0_FULL_F) / CELL_VOLT_OP_RANGE_F)
#endif

/* ---- Current ---- */
#define CELL_MAX_CHARGE_CURRENT_AMPS                (CELL_DS_CHARGE_CURRENT_MAX_AMPS * CELL_CHARGE_CURRENT_DERATE_FACTOR)
#define CELL_EOC_CHARGING_CURRENT_AMPS              (CELL_DS_CHARGE_CV_CUTOFF_AMPS)
#define CELL_MAX_DISCHARGE_CONTINUOUS_CURRENT_AMPS  (CELL_DS_DISCHARGE_CURRENT_MAX_AMPS * CELL_DISCHARGE_CURRENT_DERATE_FACTOR)
#define CELL_MAX_DISCHARGE_PULSE_CURRENT_AMPS       (CELL_MAX_DISCHARGE_CONTINUOUS_CURRENT_AMPS * CELL_DISCHARGE_PULSE_FACTOR)

#define CELL_MAX_CHARGE_CRATE                       (CELL_MAX_CHARGE_CURRENT_AMPS / (CELL_CAPACITY_NOMINAL_MAH / 1000.0f))
#define CELL_MAX_DISCHARGE_CRATE                    (CELL_MAX_DISCHARGE_CONTINUOUS_CURRENT_AMPS / (CELL_CAPACITY_NOMINAL_MAH / 1000.0f))

/* ---- Temperature ---- */
/* Discharge (overall rated envelope) */
#define CELL_TEMP_OVER                      (CELL_DS_TEMP_DISCHARGE_MAX_C)
#define CELL_TEMP_UNDER                     (CELL_DS_TEMP_DISCHARGE_MIN_C)
#define CELL_TEMP_Max                       (CELL_TEMP_OVER  - CELL_TEMP_SAFETY_BUF)
#define CELL_TEMP_Min                       (CELL_TEMP_UNDER + CELL_TEMP_SAFETY_BUF)
#define CELL_TEMP_OP_RANGE                  (CELL_TEMP_Max - CELL_TEMP_Min)

#define CELL_TEMP_OT_FAULT_SET_C            (CELL_TEMP_OVER)
#define CELL_TEMP_OT_FAULT_CLEAR_C          (CELL_TEMP_OVER  - CELL_TEMP_RECOVERY_HYST)
#define CELL_TEMP_UT_FAULT_SET_C            (CELL_TEMP_UNDER)
#define CELL_TEMP_UT_FAULT_CLEAR_C          (CELL_TEMP_UNDER + CELL_TEMP_RECOVERY_HYST)

/* Charge (tighter: charging below 0C risks lithium plating) */
#define CELL_TEMP_CHARGE_OVER               (CELL_DS_TEMP_CHARGE_MAX_C)
#define CELL_TEMP_CHARGE_UNDER              (CELL_DS_TEMP_CHARGE_MIN_C)
#define CELL_TEMP_CHARGE_Max                (CELL_TEMP_CHARGE_OVER  - CELL_TEMP_SAFETY_BUF)
#define CELL_TEMP_CHARGE_Min                (CELL_TEMP_CHARGE_UNDER + CELL_TEMP_SAFETY_BUF)
#define CELL_TEMP_CHARGE_DERATE_BAND_MAX_C  (CELL_TEMP_CHARGE_UNDER + CELL_TEMP_CHARGE_DERATE_BAND_WIDTH_C)

/* ---- Internal resistance ---- */
#define CELL_MAX_INTERNAL_RES_MOHM          (CELL_DS_INTERNAL_RES_AC_1KHZ_MOHM_MAX * CELL_INTERNAL_RES_MARGIN_FACTOR)

/* ---- Balancing ---- */
//#define CELL_BALANCE_START_VOLT_F           (CELL_VOLT_100_FULL_F - CELL_BALANCE_START_BELOW_FULL_V)

//////////////////////////////////////////////////////////////
// 3. GRAPH DATA
// Approximate values READ OFF the datasheet charts by eye (roughly +/-5%).
// They are not tabulated on the datasheet - use for modelling/sanity checks only.
//////////////////////////////////////////////////////////////

/* ---- Charge characteristics (CC-CV to 4.2V, 50mA cut, 23C) ---- */
#define CELL_GRAPH_CHARGE_2P6A_CC_END_HOURS             (1.0f)    /* Time the cell reaches 4.2V and CV phase starts */
#define CELL_GRAPH_CHARGE_2P6A_DONE_HOURS               (1.3f)    /* Current tapers to the 50mA cut                 */
#define CELL_GRAPH_CHARGE_5P2A_CC_END_HOURS             (0.45f)
#define CELL_GRAPH_CHARGE_5P2A_DONE_HOURS               (0.6f)

/* ---- Discharge rate characteristics (23C, cut off at 2.5V) ---- */
/* Capacity delivered down to the cut-off voltage */
#define CELL_GRAPH_DISCHARGE_0P52A_CAPACITY_MAH         (2600.0f)
#define CELL_GRAPH_DISCHARGE_2P6A_CAPACITY_MAH          (2550.0f)
#define CELL_GRAPH_DISCHARGE_10A_CAPACITY_MAH           (2500.0f)
#define CELL_GRAPH_DISCHARGE_20A_CAPACITY_MAH           (2450.0f)
/* Terminal voltage shortly after the load is applied */
#define CELL_GRAPH_DISCHARGE_0P52A_START_VOLT           (4.15f)
#define CELL_GRAPH_DISCHARGE_2P6A_START_VOLT            (4.10f)
#define CELL_GRAPH_DISCHARGE_10A_START_VOLT             (3.95f)
#define CELL_GRAPH_DISCHARGE_20A_START_VOLT             (3.70f)

/* ---- Discharge temperature characteristics (2.6A, cut off at 2.5V, charged at 23C) ---- */
/* Capacity delivered down to the cut-off voltage */
#define CELL_GRAPH_TEMP_M40C_CAPACITY_MAH               (2100.0f)
#define CELL_GRAPH_TEMP_M30C_CAPACITY_MAH               (2250.0f)
#define CELL_GRAPH_TEMP_M20C_CAPACITY_MAH               (2350.0f)
#define CELL_GRAPH_TEMP_0C_CAPACITY_MAH                 (2450.0f)
#define CELL_GRAPH_TEMP_23C_CAPACITY_MAH                (2550.0f)
#define CELL_GRAPH_TEMP_45C_CAPACITY_MAH                (2600.0f)
#define CELL_GRAPH_TEMP_60C_CAPACITY_MAH                (2600.0f)

/* ---- Cycle characteristics (23C, charge 2.6A CC-CV 4.2V/50mA) ---- */
/* Capacity retention (%) for both the 10A and 20A discharge curves, which nearly overlap */
#define CELL_GRAPH_CYCLE_TESTED_CYCLES                  (500)
#define CELL_GRAPH_CYCLE_RETENTION_AT_100_PERCENT       (97.0f)
#define CELL_GRAPH_CYCLE_RETENTION_AT_200_PERCENT       (95.0f)
#define CELL_GRAPH_CYCLE_RETENTION_AT_300_PERCENT       (93.0f)
#define CELL_GRAPH_CYCLE_RETENTION_AT_400_PERCENT       (91.5f)
#define CELL_GRAPH_CYCLE_RETENTION_AT_500_PERCENT       (90.0f)

#endif /* PHANTOM_HARDWARE_BATTERYCELL_HARDWARE_H_ */
