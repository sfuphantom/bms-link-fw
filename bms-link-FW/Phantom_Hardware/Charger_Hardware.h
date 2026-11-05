/*
 * Charger_Hardware.h
 *
 *  Charger definition used by Charger.c (its only includer).
 *  Raw spec numbers live in Charger_DataSheetSpec.h (CHARGER_DS_*).
 *
 *  Layout:
 *    1. USER PLACEHOLDERS - every buffer / hysteresis / factor / timer / assumption.
 *                           Tune everything here; nothing below needs editing.
 *    2. CHARGER LIMITS    - datasheet value for the selected platform.
 *    3. CHARGE TARGETS    - battery values -> charger setpoints (all SI units:
 *                           V, A, degC, Ah). CAN scaling is private to Charger.c.
 *    4. STATE MACHINE THRESHOLDS - set/clear pairs, all derived from 1 and 3.
 *
 *  Every threshold here is a plain float in V / A / degC. There are no CAN-scaled
 *  integers in this file, so nothing can be compared in the wrong unit.
 */

#ifndef PHANTOM_HARDWARE_CHARGER_HARDWARE_H_
#define PHANTOM_HARDWARE_CHARGER_HARDWARE_H_

#include "Charger_DataSheetSpec.h"
#include "FullBattery_Hardware.h"   /* pack + cell values (pulls in BatteryCell_Hardware.h) */
#include "PhantomTimers.h"

//////////////////////////////////////////////////////////////
// 1. USER PLACEHOLDERS (none of these are in the charger spec)
//////////////////////////////////////////////////////////////

/* ---- Which charger model: 48, 72 or 108 ---- */
#ifndef CHARGER_NOMINAL_PLATFORM
#define CHARGER_NOMINAL_PLATFORM            48
#endif

/* ---- CAN (the real baud rate is set in HALCoGen; see OPEN QUESTIONS in the DS file) ---- */
#define CHARGER_CAN_BAUDRATE                (CHARGER_DS_CAN_BAUD_SPEC)
#define CHARGER_CAN_TERMINATION_OHM         (0U)       /* 0 = none, 120 = external */

/* ---- Charge currents ---- */
#define CHARGER_TRICKLE_CURRENT_AMPS        (0.5f)     /* Very low cells, and the EOC hold  */
#define CHARGER_UNBALANCED_CURRENT_AMPS     (0.1f)     /* While cells are far out of balance */
#define CHARGER_RECHARGE_CURRENT_FACTOR     (0.5f)     /* Recharge current = CC current x this */
#define CHARGER_COLD_DERATE_FACTOR          (0.5f)     /* CC current x this in the cold derate band */

/* ---- Charge-cycle voltage thresholds (per cell, V, unless noted) ---- */
#define CHARGER_TRICKLE_CELL_VOLT           (3.0f)     /* Lowest cell below this -> trickle        */
#define CHARGER_CV_ENTRY_MARGIN_V           (0.2f)     /* PACK volts below target that count as "CV reached".
                                                        * Charger CV accuracy is +/-1%, so on a ~50 V pack
                                                        * this can be too tight. Raise it if CC never ends. */
#define CHARGER_RECHARGE_HYST_V             (0.05f)    /* DONE -> RE_CHARGING when this far under target */
#define CHARGER_UNBALANCE_SET_V             (0.200f)   /* max - min above this -> UNBALANCED  */
#define CHARGER_UNBALANCE_CLEAR_V           (0.010f)   /* max - min below this -> clear       */
#define CHARGER_OVERCHARGE_CLEAR_MARGIN_V   (0.05f)    /* Overcharge clears at target + this  */
#define CHARGER_OVERCHARGE_HYST_V           (0.30f)    /* Overcharge sets at clear + this     */

/* ---- Temperature (degC) ---- */
#define CHARGER_OVERTEMP_HYST_C             (10.0f)    /* Over-temp clears this far below its set point */

/* ---- End of charge ---- */
#define CHARGER_EOC_SOC_MARGIN_PERCENT      (5.0f)     /* EOC needs SOC >= (charge-target SOC - this) */

/* ---- Timers ----
 * ASSUMPTION: the first ticks_make() argument is minutes (the old code commented
 * ticks_make(1,0,0,0,0) as "1 min"). If that is wrong, fix it here, once.        */
#define CHARGER_TICKS_MINUTES(m)            ticks_make((m), 0, 0, 0, 0)
#define CHARGER_INHIBIT_TIME_MIN            (1U)       /* cool-down after a failed EOC / recharge */
#define CHARGER_EOC_HOLD_TIME_MIN           (30U)      /* max time in EOC / RE_CHARGING before giving up */

//////////////////////////////////////////////////////////////
// 2. CHARGER LIMITS (selected model)
//////////////////////////////////////////////////////////////

#if   CHARGER_NOMINAL_PLATFORM == 48
#define CHARGER_OUTPUT_VOLTAGE_MIN_V        (CHARGER_DS_HK_MF_48_40_VOUT_MIN_V)
#define CHARGER_OUTPUT_VOLTAGE_MAX_V        (CHARGER_DS_HK_MF_48_40_VOUT_MAX_V)
#define CHARGER_OUTPUT_MAX_CURRENT_A        (CHARGER_DS_HK_MF_48_40_IOUT_MAX_A)
#elif CHARGER_NOMINAL_PLATFORM == 72
#define CHARGER_OUTPUT_VOLTAGE_MIN_V        (CHARGER_DS_HK_MF_72_40_VOUT_MIN_V)
#define CHARGER_OUTPUT_VOLTAGE_MAX_V        (CHARGER_DS_HK_MF_72_40_VOUT_MAX_V)
#define CHARGER_OUTPUT_MAX_CURRENT_A        (CHARGER_DS_HK_MF_72_40_IOUT_MAX_A)
#elif CHARGER_NOMINAL_PLATFORM == 108
#define CHARGER_OUTPUT_VOLTAGE_MIN_V        (CHARGER_DS_HK_MF_108_32_VOUT_MIN_V)
#define CHARGER_OUTPUT_VOLTAGE_MAX_V        (CHARGER_DS_HK_MF_108_32_VOUT_MAX_V)
#define CHARGER_OUTPUT_MAX_CURRENT_A        (CHARGER_DS_HK_MF_108_32_IOUT_MAX_A)
#else
#error "CHARGER_NOMINAL_PLATFORM must be 48, 72, or 108"
#endif

/* Charger stops output outside its range +/- margin */
#define CHARGER_OUTPUT_OVP_V                (CHARGER_OUTPUT_VOLTAGE_MAX_V + CHARGER_DS_PROT_OUT_VOLTAGE_MARGIN_V)
#define CHARGER_OUTPUT_UVP_V                (CHARGER_OUTPUT_VOLTAGE_MIN_V - CHARGER_DS_PROT_OUT_VOLTAGE_MARGIN_V)

//////////////////////////////////////////////////////////////
// 3. CHARGE TARGETS (battery -> charger setpoints)
//////////////////////////////////////////////////////////////

/* Pack */
#define CHARGER_TARGET_VOLTAGE_V            (MAX_BATTERY_CHARGING_VOLTAGE_TARGET)
#define CHARGER_CC_CURRENT_AMPS             (BATTERY_CC_CHARGING_CURRENT_AMPS)
#define CHARGER_RECHARGE_CURRENT_AMPS       (CHARGER_CC_CURRENT_AMPS * CHARGER_RECHARGE_CURRENT_FACTOR)
#define CHARGER_CV_ENTRY_VOLTAGE_V          (CHARGER_TARGET_VOLTAGE_V - CHARGER_CV_ENTRY_MARGIN_V)
#define CHARGER_EOC_CURRENT_AMPS            (BATTERY_EOC_CHARGING_CURRENT_AMPS)   /* charger reports 0.1 A steps */
#define CHARGER_NOMINAL_CAPACITY_AH         (BATTERY_CAPACITY_NOMINAL_AH)

/* Cell */
#define CHARGER_CELL_TARGET_V               (MAX_CELL_CHARGING_VOLTAGE_TARGET_F)
#define CHARGER_CELL_TARGET_WINDOW_V        (CELL_BALANCE_TARGET_DELTA_V)         /* "at target" = within this */
#define CHARGER_CELL_OVP_V                  (CELL_VOLT_OV_FAULT_SET_F)            /* sent in Msg 11 and Msg 12 */
#define CHARGER_CELL_UVP_V                  (CELL_VOLT_UV_FAULT_SET_F)            /* sent in Msg 12            */

/* EOC SOC follows the charge target: with a 100% target this is the old fixed 95%.
 * (A fixed 95% could never be reached when the target is e.g. 3.6 V = ~73% SOC.)  */
#define CHARGER_EOC_SOC_PERCENT             ((MAX_CELL_CHARGING_PERCENTAGE_F * 100.0f) - CHARGER_EOC_SOC_MARGIN_PERCENT)

//////////////////////////////////////////////////////////////
// 4. STATE MACHINE THRESHOLDS (set / clear pairs)
//////////////////////////////////////////////////////////////

/* Cell imbalance (max - min) */
#define CHARGER_UNBALANCE_SET_DELTA_V       (CHARGER_UNBALANCE_SET_V)
#define CHARGER_UNBALANCE_CLEAR_DELTA_V     (CHARGER_UNBALANCE_CLEAR_V)

/* Cell overcharge (max cell) */
#define CHARGER_OVERCHARGE_CLEAR_V          (CHARGER_CELL_TARGET_V + CHARGER_OVERCHARGE_CLEAR_MARGIN_V)
#define CHARGER_OVERCHARGE_SET_V            (CHARGER_OVERCHARGE_CLEAR_V + CHARGER_OVERCHARGE_HYST_V)

/* Over-temperature while charging (max cell temp). Uses the cell charge window. */
#define CHARGER_OVERTEMP_SET_C              (CELL_TEMP_CHARGE_Max)
#define CHARGER_OVERTEMP_CLEAR_C            (CHARGER_OVERTEMP_SET_C - CHARGER_OVERTEMP_HYST_C)

/* Under-temperature while charging (min cell temp): lithium plating risk */
#define CHARGER_UNDERTEMP_SET_C             (CELL_TEMP_CHARGE_Min)
#define CHARGER_UNDERTEMP_CLEAR_C           (CHARGER_UNDERTEMP_SET_C + CELL_TEMP_RECOVERY_HYST)

/* Reduced charge current below this min cell temp */
#define CHARGER_COLD_DERATE_BELOW_C         (CELL_TEMP_CHARGE_DERATE_BAND_MAX_C)

#endif /* PHANTOM_HARDWARE_CHARGER_HARDWARE_H_ */
