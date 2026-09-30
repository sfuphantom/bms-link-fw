/*
 * BatteryCell_Hardware.h
 *
 *  Created on: Aug 30, 2026
 *      Author: tanjo
 *
 *  NOTE ON FIXES (from original revision):
 *   1. CELL_CAPACITY_NOMINAL_AH referenced an undefined symbol
 *      (CAPACITY_NOMINAL_SPEC_MAH) -> fixed to CELL_CAPACITY_NOMINAL_MAH.
 *   2. CELL_VOLT_OP_RANGE_F referenced CELL_VOLT_100_FULL / CELL_VOLT_0_FULL
 *      (missing the _F suffix, so they were undefined) -> fixed.
 *   3. MAX_CELL_CHARGING_PERCENTAGE_F referenced CELL_SOC_RANGE, which was
 *      never defined -> fixed to CELL_VOLT_OP_RANGE_F.
 *   4. CELL_TEMP_OP_RANGE referenced CELL_TEMP_100_FULL / CELL_TEMP_0_FULL,
 *      which do not exist (apparent copy/paste from the voltage block) ->
 *      fixed to (CELL_TEMP_Max - CELL_TEMP_Min).
 *   5. CELL_MAX_INTERNAL_CAPSITANS_MF misspelled "capacitance" and
 *      CELL_NORMINAL_VOLT misspelled "nominal" -> both renamed outright to
 *      CELL_MAX_INTERNAL_CAPACITANCE_MF and CELL_NOMINAL_VOLT. No
 *      backward-compatibility aliases are kept.
 *   6. CELL_TEMP_Min subtracted the safety buffer from CELL_TEMP_UNDER,
 *      which pushes the threshold BELOW the actual minimum rated
 *      temperature instead of narrowing the safe window inward (the
 *      voltage macros do this correctly: CELL_VOLT_UNDER_F + buffer).
 *      Fixed to CELL_TEMP_UNDER + CELL_TEMP_SAFETY_BUF.
 *
 *  UPDATE (based on MOLICEL INR-18650-P26A datasheet):
 *   Cell characteristics, physical dimensions, current ratings, energy
 *   density, and ambient temperature ranges below have been replaced with
 *   real datasheet values. Parameters the datasheet does not specify (cycle
 *   life count, self-discharge rate, calendar life, internal capacitance,
 *   storage voltage, thermal runaway onset, short-circuit/fuse ratings,
 *   BMS safety buffers/hysteresis, pulse discharge rating/duration) remain
 *   generic typical-Li-ion placeholders - see chat response for the full list.
 */

#ifndef PHANTOM_HARDWARE_BATTERYCELL_HARDWARE_H_
#define PHANTOM_HARDWARE_BATTERYCELL_HARDWARE_H_


enum batteryShape{
    CYLINDRICAL_BATTERY_CELL = 0,
    POUCH_BATTERY_CELL,
    PRISMATIC_BATTERY_CELL,
    COIN_BATTERY_CELL,
};

enum batteryChemistry{
    CHEM_LI_NMC = 0,        /* Lithium Nickel Manganese Cobalt Oxide */
    CHEM_LI_NCA,            /* Lithium Nickel Cobalt Aluminum Oxide  */
    CHEM_LI_FEPO4,          /* Lithium Iron Phosphate (LFP)          */
    CHEM_LI_COBALT_OXIDE,   /* LCO                                    */
    CHEM_LI_TITANATE,       /* LTO                                    */
};

/* Selected chemistry / form factor for this cell definition:
 * MOLICEL INR-18650-P26A. Chemistry is NOT stated on the datasheet itself;
 * CHEM_LI_NMC is an inference based on this being a known high-drain
 * Molicel "P"-series 18650 cell, not a confirmed spec value. */
#define CELL_CHEMISTRY                      (CHEM_LI_NMC)
#define CELL_SHAPE                          (CYLINDRICAL_BATTERY_CELL)

//////////////////////////////////////////////////////////////
// PHYSICAL / MECHANICAL PARAMETERS (MOLICEL INR-18650-P26A datasheet, all "Max" dims)
//////////////////////////////////////////////////////////////
#define CELL_FORM_FACTOR_NAME               "18650"
#define CELL_CAN_MATERIAL                   "Steel"
#define CELL_DIAMETER_MM                    (18.6f)
#define CELL_HEIGHT_MM                      (65.2f)
#define CELL_MASS_GRAMS                     (50.0f)
#define CELL_VOLUME_CM3                     (3.14159265f * (CELL_DIAMETER_MM/2.0f/10.0f) * (CELL_DIAMETER_MM/2.0f/10.0f) * (CELL_HEIGHT_MM/10.0f))

//////////////////////////////////////////////////////////////
// VOLTAGE PARAMETERS
//////////////////////////////////////////////////////////////
#define CELL_NOMINAL_VOLT                  (3.6f)
#define CELL_MAX_CHARGING_VOLT             (4.2f)
#define CELL_DISCHARGE_CUT_OFF_VOLT        (2.5f)
#define CELL_STORAGE_VOLT_RECOMMENDED       (3.8f)              /* Generic Li-ion recommendation - not specified on this datasheet */

#define CELL_CAPACITY_NOMINAL_MAH           (2600.0f)
#define CELL_CAPACITY_NOMINAL_AH            (CELL_CAPACITY_NOMINAL_MAH/1000.0f)
#define CELL_CAPACITY_ACTUAL_MAH             (CELL_CAPACITY_NOMINAL_MAH) /* Defaults to nominal; override with a BMS-learned/measured value if available */
#define CELL_CAPACITY_ACTUAL_AH              (CELL_CAPACITY_ACTUAL_MAH/1000.0f)

//////////////////////////////////////////////////////////////
// ENERGY / DENSITY PARAMETERS
//////////////////////////////////////////////////////////////
#define CELL_ENERGY_NOMINAL_WH               (CELL_CAPACITY_NOMINAL_AH * CELL_NOMINAL_VOLT)
#define CELL_GRAVIMETRIC_ENERGY_DENSITY_WHKG  (CELL_ENERGY_NOMINAL_WH / (CELL_MASS_GRAMS/1000.0f))
#define CELL_VOLUMETRIC_ENERGY_DENSITY_WHL     (CELL_ENERGY_NOMINAL_WH / (CELL_VOLUME_CM3/1000.0f))
#define CELL_SPECIFIC_POWER_WKG                (CELL_MAX_DISCHARGE_CONTINUOUS_CURRENT_AMPS * CELL_NOMINAL_VOLT / (CELL_MASS_GRAMS/1000.0f))

//////////////////////////////////////////////////////////////
// SOH / CYCLE / CALENDAR LIFE
// NONE of these are stated as a rated spec on the datasheet - it only shows
// a capacity-vs-cycle-count GRAPH (out to 500 cycles at 10A/20A discharge,
// ~2.6A CC-CV charge) with no numeric "rated cycle life". Values below are
// generic typical-Li-ion placeholders, unchanged from before.
//////////////////////////////////////////////////////////////
#define CELL_MIN_SOH_ALLOWED_PERCENT        (80.0f)
#define CELL_CYCLE_LIFE                     (300)
#define CELL_CALENDAR_LIFE_YEARS            (5.0f)
#define CELL_SELF_DISCHARGE_PERCENT_PER_MONTH (2.5f)
#define CELL_EOL_CAPACITY_FADE_PERCENT       (100.0f - CELL_MIN_SOH_ALLOWED_PERCENT)


//////////////////////////////////////////////////////////////
#define CELL_C2E_RATING(C)                             (C * CELL_NOMINAL_VOLT)
#define CELL_E2C_RATING(E)                             (E / CELL_NOMINAL_VOLT)
/* C-rate <-> current helpers: current (A) needed for a given C-rate, and vice versa */
#define CELL_CRATE_TO_AMPS(C_RATE)                     (C_RATE * CELL_CAPACITY_NOMINAL_AH)
#define CELL_AMPS_TO_CRATE(AMPS)                       (AMPS / CELL_CAPACITY_NOMINAL_AH)
/* Time (in hours) to charge/discharge at a constant current */
#define CELL_TIME_HOURS_AT_CURRENT(AMPS)                (CELL_CAPACITY_NOMINAL_AH / (AMPS))

//////////////////////////////////////////////////////////////
#define CELL_VOLT_SAFETY_BUF    (0.200f)
#define CELL_VOLT_FAULT_BUF     (0.100f)
#define CELL_VOLT_RECOVERY_HYST (0.050f)   /* Hysteresis before clearing an OV/UV fault */
#define CELL_TEMP_SAFETY_BUF    (5.0f)
#define CELL_TEMP_RECOVERY_HYST (2.0f)     /* Hysteresis before clearing an OT/UT fault */

//////////////////////////////////////////////////////////////
#define CELL_VOLT_OVER_F          (CELL_MAX_CHARGING_VOLT      - CELL_VOLT_SAFETY_BUF)//4.0V
#define CELL_VOLT_UNDER_F         (CELL_DISCHARGE_CUT_OFF_VOLT + CELL_VOLT_SAFETY_BUF)//2.7V

#define CELL_VOLT_100_FULL_F      (CELL_VOLT_OVER_F - CELL_VOLT_FAULT_BUF)
#define CELL_VOLT_50_FULL_F       (CELL_NOMINAL_VOLT)
#define CELL_VOLT_0_FULL_F        (CELL_VOLT_UNDER_F + CELL_VOLT_FAULT_BUF)
#define CELL_VOLT_OP_RANGE_F      (CELL_VOLT_100_FULL_F - CELL_VOLT_0_FULL_F)

/* Voltage-based fault thresholds with recovery hysteresis */
#define CELL_VOLT_OV_FAULT_SET_F       (CELL_VOLT_OVER_F)
#define CELL_VOLT_OV_FAULT_CLEAR_F     (CELL_VOLT_OVER_F  - CELL_VOLT_RECOVERY_HYST)
#define CELL_VOLT_UV_FAULT_SET_F       (CELL_VOLT_UNDER_F)
#define CELL_VOLT_UV_FAULT_CLEAR_F     (CELL_VOLT_UNDER_F + CELL_VOLT_RECOVERY_HYST)

/* Cell balancing thresholds (for multi-cell pack voltage matching) */
#define CELL_BALANCE_START_VOLT_F         (CELL_VOLT_100_FULL_F - 0.050f)
#define CELL_BALANCE_DELTA_TRIGGER_V       (0.020f)   /* Max allowed mismatch between cells before balancing kicks in */
#define CELL_BALANCE_TARGET_DELTA_V        (0.005f)   /* Target mismatch to stop balancing */

//////////////////////////////////////////////////////////////
#define DEFINE_MAX_CELL_CHARGING_PERCENTAGE_N_VOLTAGE_TARGET    false
#if DEFINE_MAX_CELL_CHARGING_PERCENTAGE_N_VOLTAGE_TARGET
#define MAX_CELL_CHARGING_PERCENTAGE_F                          (90.0f / 100)
#define MAX_CELL_CHARGING_VOLTAGE_TARGET_F                      ((MAX_CELL_CHARGING_PERCENTAGE_F * CELL_VOLT_OP_RANGE_F) + CELL_VOLT_0_FULL_F)
#else
#define MAX_CELL_CHARGING_VOLTAGE_TARGET_F                      (3.6f)
#define MAX_CELL_CHARGING_PERCENTAGE_F                          ((MAX_CELL_CHARGING_VOLTAGE_TARGET_F - CELL_VOLT_0_FULL_F) / CELL_VOLT_OP_RANGE_F)
#endif

/* SOC (%) estimate from a resting terminal voltage, linear approximation between 0% and 100% points */
#define CELL_VOLT_TO_SOC_PERCENT(V)         (((( V ) - CELL_VOLT_0_FULL_F) / CELL_VOLT_OP_RANGE_F) * 100.0f)
/* Inverse: resting terminal voltage estimate from a SOC (%) value */
#define CELL_SOC_PERCENT_TO_VOLT(SOC)       (CELL_VOLT_0_FULL_F + ((( SOC ) / 100.0f) * CELL_VOLT_OP_RANGE_F))


#define CELL_MAX_CHARGE_CURRENT_AMPS                   (6.0f)    /* Datasheet "Charge Current: Maximum" */
#define CELL_MAX_CHARGE_CRATE                          (CELL_AMPS_TO_CRATE(CELL_MAX_CHARGE_CURRENT_AMPS))
#define CELL_STANDARD_CHARGE_CURRENT_AMPS               (2.6f)   /* Datasheet "Charge Current: Standard" (~1C) */
#define CELL_STANDARD_CHARGE_TIME_HOURS                 (1.5f)   /* Datasheet "Charge Time: Standard" */
#define CELL_CHARGE_TAPER_CURRENT_CUTOFF_AMPS           (0.050f) /* Datasheet CV-phase cutoff: "50mA cut" */

#define CELL_CC_CHARGING_CURRENT_AMPS     (3.0f)
#define CELL_EOC_CHARGING_CURRENT_AMPS    (50.0f/1000.0f)

//////////////////////////////////////////////////////////////
#define CELL_MAX_DISCHARGE_CONTINUOUS_CURRENT_AMPS     (35.0f)   /* Datasheet "Discharge Current: Maximum" */
/* Datasheet gives a single "Maximum 35A" figure with no separate pulse rating
 * or duration - assumed equal to the continuous max since nothing higher is
 * specified. Duration below is an unconfirmed placeholder. */
#define CELL_MAX_DISCHARGE_PULSE_CURRENT_AMPS          (CELL_MAX_DISCHARGE_CONTINUOUS_CURRENT_AMPS)
#define CELL_MAX_DISCHARGE_PULSE_DURATION_SEC          (10.0f)   /* Not specified on datasheet - placeholder */
#define CELL_MAX_DISCHARGE_CRATE                       (CELL_AMPS_TO_CRATE(CELL_MAX_DISCHARGE_CONTINUOUS_CURRENT_AMPS))
#define CELL_STANDARD_DISCHARGE_CURRENT_AMPS            (CELL_CRATE_TO_AMPS(1.0f))  /* ~2.6A - matches the datasheet's own charge/cycle test baseline current */
//////////////////////////////////////////////////////////////
// Datasheet gives separate ambient ranges for charge vs. discharge:
//   Charge:    0C to 60C
//   Discharge: -40C to 60C
// CELL_TEMP_OVER/UNDER below represent the overall (widest, discharge) rated
// envelope; CELL_TEMP_CHARGE_OVER/UNDER are the tighter charge-only limits.
//////////////////////////////////////////////////////////////
#define CELL_TEMP_OVER          (60.0f)   /* Datasheet discharge ambient max */
#define CELL_TEMP_UNDER          (-40.0f)  /* Datasheet discharge ambient min */
#define CELL_TEMP_CHARGE_OVER    (60.0f)   /* Datasheet charge ambient max */
#define CELL_TEMP_CHARGE_UNDER   (0.0f)    /* Datasheet charge ambient min - charging below 0C risks lithium plating on graphite anodes */
#define CELL_TEMP_STORAGE_MAX    (25.0f)   /* Recommended max ambient temp for long-term storage - not specified on datasheet */

#define CELL_TEMP_Max               (CELL_TEMP_OVER - CELL_TEMP_SAFETY_BUF)
#define CELL_TEMP_Min               (CELL_TEMP_UNDER + CELL_TEMP_SAFETY_BUF)
#define CELL_TEMP_OP_RANGE          (CELL_TEMP_Max - CELL_TEMP_Min)

/* Low-temperature charge-current derating band: many Li-ion cells (this one
 * included, per the 0C charge floor above) need reduced charge current just
 * above their minimum charge temperature. Width is an engineering assumption,
 * not a datasheet spec. */
#define CELL_TEMP_CHARGE_DERATE_BAND_WIDTH_C  (10.0f)
#define CELL_TEMP_CHARGE_DERATE_BAND_MAX_C    (CELL_TEMP_CHARGE_UNDER + CELL_TEMP_CHARGE_DERATE_BAND_WIDTH_C)

/* Temperature-based fault thresholds with recovery hysteresis */
#define CELL_TEMP_OT_FAULT_SET_C        (CELL_TEMP_OVER)
#define CELL_TEMP_OT_FAULT_CLEAR_C      (CELL_TEMP_OVER  - CELL_TEMP_RECOVERY_HYST)
#define CELL_TEMP_UT_FAULT_SET_C         (CELL_TEMP_UNDER)
#define CELL_TEMP_UT_FAULT_CLEAR_C       (CELL_TEMP_UNDER + CELL_TEMP_RECOVERY_HYST)
/* Thermal runaway / venting onset - informational safety limit, not a controllable
 * setpoint. Not specified on the datasheet - generic Li-ion estimate. */
#define CELL_TEMP_THERMAL_RUNAWAY_ONSET_C  (150.0f)

/* Celsius <-> Fahrenheit convenience conversions */
#define CELL_TEMP_C_TO_F(C)         ((( C ) * 9.0f / 5.0f) + 32.0f)
#define CELL_TEMP_F_TO_C(F)         ((( F ) - 32.0f) * 5.0f / 9.0f)

//////////////////////////////////////////////////////////////
#define CELL_MAX_INTERNAL_RES_MOHM        (20.0f)   /* Datasheet "Internal Resistance: AC (1 kHz) Max" */
#define CELL_MAX_INTERNAL_CAPACITANCE_MF  (20.0f)   /* Not specified on datasheet - generic placeholder */
/* Estimated instantaneous voltage sag under a given discharge current (simple IR-drop model) */
#define CELL_VOLT_SAG_ESTIMATE_V(AMPS)     ((( AMPS ) * (CELL_MAX_INTERNAL_RES_MOHM / 1000.0f)))
#define CELL_OPEN_CIRCUIT_VOLT_ESTIMATE(V_LOADED, AMPS)  ((V_LOADED) + CELL_VOLT_SAG_ESTIMATE_V(AMPS))

//////////////////////////////////////////////////////////////
// PROTECTION IC / FUSE PARAMETERS - none of these are on the datasheet
// (it has no short-circuit or fusing spec at all); generic placeholders.
//////////////////////////////////////////////////////////////
#define CELL_SHORT_CIRCUIT_CURRENT_THRESHOLD_AMPS   (80.0f)
#define CELL_SHORT_CIRCUIT_RESPONSE_TIME_US         (200.0f)
#define CELL_FUSE_RATED_CURRENT_AMPS                (60.0f)

#endif /* PHANTOM_HARDWARE_BATTERYCELL_HARDWARE_H_ */
