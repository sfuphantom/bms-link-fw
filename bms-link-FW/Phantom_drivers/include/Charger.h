/* Charger.h
 *
 *  NOTE ON FIXES (from original revision):
 *   1. SINGLE_CELL_MAX_PROTECTION_V / SINGLE_CELL_MIN_PROTECTION_V /
 *      SINGLE_CELL_CHARGING_TARGET_V / SINGLE_CELL_TARGET_MAX_DELTA_V all
 *      ran a plain float VOLT value through CHARGER_CAN_TO_SIGNAL_VOLT
 *      (which DECODES a CAN integer back to volts, i.e. multiplies by
 *      0.001) instead of CHARGER_CELL_VOLT_TO_CAN (which ENCODES volts
 *      into the 1mV/bit CAN representation, i.e. divides by 0.001). This
 *      made every one of these thresholds evaluate to ~0 once cast to an
 *      integer, so cell-level OV/UV protection and the charge-complete
 *      voltage window were effectively meaningless. Fixed to use
 *      CHARGER_CELL_VOLT_TO_CAN throughout, and SINGLE_CELL_TARGET_MAX_DELTA_V
 *      now reuses CELL_BALANCE_TARGET_DELTA_V from BatteryCell_Hardware.h
 *      instead of an arbitrary 0.001 (which is a volt, not a delta-band).
 *   2. CV_CHARGING_TRIGGER passed CHARGER_TARGET_VOLTAGE (already a CAN
 *      integer, 0.1V/bit) back through CHARGER_BATTERY_VOLTAGE_TO_CAN,
 *      i.e. it re-encoded an already-encoded value, inflating it ~10x.
 *      Since s_battery.pack_voltage_V can never reach that inflated value,
 *      CC_CHARGING could never transition to CV_CHARGING. Fixed to a
 *      plain subtraction in CAN units.
 *   3. CELL_MIN/MAX_CHARGING_OVERSHOOT_TIGGER/TRIGGER were built from
 *      CHARGER_TARGET_VOLTAGE, which is a PACK-level value in 0.1V/bit
 *      units (~200-ish), but are compared against s_battery.cell_max_V,
 *      which is a single-CELL value in 1mV/bit units (~3000-4200). The
 *      trigger was therefore always far below any real cell reading, so
 *      CHARGER_STATE_OVERCHARGED_CELL would latch almost immediately on
 *      any charge attempt and could then never clear (its own clear
 *      threshold suffers the same problem). Fixed to derive both from
 *      SINGLE_CELL_CHARGING_TARGET_V (now correctly cell-scaled, see #1).
 *   4. CELL_CHARGING_OVERHEATED_HBAND ran a plain 10 degC delta through
 *      CHARGER_TEMP_TO_CAN, which bakes in the fixed +100 offset used for
 *      absolute temperatures. Subtracting that from
 *      CELL_MAX_CHARGING_OVERHEATED_TRIGGER produced a "clear" threshold
 *      that decodes to -50 degC, i.e. CHARGER_STATE_OVERHEATED_CELL could
 *      practically never clear. Fixed to a plain (unscaled) 10.
 *   5. ChargerCmdMsg13_t.temp_max / temp_min were declared int8_t, but the
 *      wire value is CHARGER_TEMP_TO_CAN(C) = C + 100 as an unsigned byte
 *      (valid data range 60-160 decimal, which does not fit in int8_t).
 *      Changed to uint8_t to match CHARGER_TEMP_TO_CAN's actual range and
 *      s_battery's own (uint8_t) storage of the same value.
 *   6. Added named bit masks for the Charger_UpdateStateMachine() "flags"
 *      argument and a CHARGER_STATUS_FAULT_MASK for s_chargerStatus so the
 *      magic numbers 0x01/0x02/0x04 in Charger.c have a documented meaning,
 *      and so a genuinely benign status bit (CHARGER_STATUS_STARTING) does
 *      not force a fault shutdown every time the charger reports it (see
 *      fixes list in Charger.c for how this is used).
 *   7. Declared Charger_GetTargetVoltage/Current and Charger_IsOutputEnabled
 *      to match the getters that were missing an implementation, and added
 *      Charger_UpdateOutputsFromState() (formerly the un-prototyped, un-
 *      prefixed file-local "changeChargerControl") so callers outside
 *      Charger.c can actually invoke it - see Charger.c fix notes for why
 *      this matters.
 *   8. Removed the duplicate CHARGER_CONSTANT_CURRENT_A definition and the
 *      value-less CHARGER_PIN_OTHERS macro (a #define with no replacement
 *      text is not usable as a value; left as a plain comment instead).
 *
 *  STILL FLAGGED (not fixed here - needs input this file doesn't have):
 *   - All CAN payload fields in the BMS->Charger spec are Big-Endian
 *     ("High Byte" first). Charger_BuildMessageXX() in Charger.c used to
 *     memory-cast a struct straight onto the wire buffer; on any
 *     little-endian target (virtually every Cortex-M/x86 part) that sends
 *     every multi-byte field byte-swapped. Rewritten in Charger.c to pack
 *     bytes explicitly - but Charger_ReceiveStatus()'s *input* (however
 *     the RX path turns a raw CAN frame into a ChargerStatusBroadcast_t)
 *     is outside this file and needs the same check.
 *   - It isn't clear from the supplied datasheets whether the physical
 *     ElCon HK-MF charger actually implements the newer, paged Message
 *     10-14 protocol used here, or only the older single-frame Message 1
 *     (ID 0x1806E5F4, no page byte). Confirm against the charger vendor
 *     before relying on this on real hardware.
 */
#ifndef PHANTOM_DRIVERS_INCLUDE_CHARGER_H_
#define PHANTOM_DRIVERS_INCLUDE_CHARGER_H_

#include <stdint.h>
#include <stdbool.h>
#include "Phantom_Can.h"
#include "PhantomTimers.h"
//#include "BatteryCell_Hardware.h"

#include "FullBattery_Hardware.h"

/*----------------------------------------------------------------------------------------------------
 * Platform Selection
 * Define CHARGER_NOMINAL_PLATFORM as 48, 72, or 108 to set voltage/current limits.
 * Default is 48V platform (compatible with our 6S battery).
 *----------------------------------------------------------------------------------------------------*/
#ifndef CHARGER_NOMINAL_PLATFORM
#define CHARGER_NOMINAL_PLATFORM 48   /* 48V, 72V, or 108V */
#endif

#if   CHARGER_NOMINAL_PLATFORM == 48
#define CHARGER_OUTPUT_VOLTAGE_MIN_V  35.0f
#define CHARGER_OUTPUT_VOLTAGE_MAX_V  70.0f
#define CHARGER_OUTPUT_MAX_CURRENT_A  40.0f
#elif CHARGER_NOMINAL_PLATFORM == 72
#define CHARGER_OUTPUT_VOLTAGE_MIN_V  50.0f
#define CHARGER_OUTPUT_VOLTAGE_MAX_V  107.0f
#define CHARGER_OUTPUT_MAX_CURRENT_A  40.0f
#elif CHARGER_NOMINAL_PLATFORM == 108
#define CHARGER_OUTPUT_VOLTAGE_MIN_V  80.0f
#define CHARGER_OUTPUT_VOLTAGE_MAX_V  161.0f
#define CHARGER_OUTPUT_MAX_CURRENT_A  32.0f
#else
#error "CHARGER_NOMINAL_PLATFORM must be 48, 72, or 108"
#endif

/*----------------------------------------------------------------------------------------------------
 * Input Electrical Specifications
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_INPUT_VOLTAGE_AC_MIN    90.0f     /* VAC */
#define CHARGER_INPUT_VOLTAGE_AC_MAX    265.0f    /* VAC */
#define CHARGER_INPUT_FREQ_MIN_HZ       47.0f
#define CHARGER_INPUT_FREQ_MAX_HZ       63.0f
#define CHARGER_INPUT_MAX_CURRENT_A     16.0f     /* AC input current */
#define CHARGER_INPUT_POWER_FACTOR_MIN  0.98f     /* @ >=1650W */
#define CHARGER_EFFICIENCY_MIN_PERCENT  93.0f     /* full load */
#define CHARGER_STANDBY_POWER_W         5.0f      /* max */
#define CHARGER_INRUSH_CURRENT_A        24.0f     /* max starting inrush */

/*----------------------------------------------------------------------------------------------------
 * Output Electrical Specifications
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_OUTPUT_MAX_POWER_W      3300.0f   /* @220VAC; 1650W @110VAC */
#define CHARGER_OUTPUT_RISE_TIME_MS     5000      /* <5s, overshoot <10% */
#define CHARGER_OUTPUT_OVERSHOOT_PERCENT 10.0f
#define CHARGER_OUTPUT_CV_ACCURACY_PERCENT  1.0f   /* +/-1% */
#define CHARGER_OUTPUT_CC_ACCURACY_PERCENT  2.0f   /* +/-2% */
#define CHARGER_OUTPUT_RIPPLE_VOLTAGE_PERCENT 5.0f /* +/-5% */

/* Shut off response: current drops below 10% in 300ms, to 0 in 500ms */
#define CHARGER_SHUTDOWN_CURRENT_10PCT_TIME_MS  300
#define CHARGER_SHUTDOWN_CURRENT_ZERO_TIME_MS   500

/*----------------------------------------------------------------------------------------------------
 * Low Voltage Output (Auxiliary 12V)
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_LV_OUTPUT_VOLTAGE_V      12.0f
#define CHARGER_LV_OUTPUT_CURRENT_A      5.5f
#define CHARGER_LV_OUTPUT_POWER_W        66.0f
#define CHARGER_LV_CV_ACCURACY_PERCENT   2.0f
#define CHARGER_LV_RIPPLE_PERCENT        1.0f

/*----------------------------------------------------------------------------------------------------
 * Control Interface (Wake up, CAN)
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_WAKEUP_INPUT_CURRENT_MA  10      /* max 10mA */
#define CHARGER_WAKEUP_OUTPUT_CURRENT_A  0.2f    /* max 0.2A */
#define CHARGER_SLEEP_CURRENT_MA         1       /* typical, peak 5mA */
#define CHARGER_SLEEP_PEAK_CURRENT_MA    5

/* CAN Baud rates (select one) */
#define CHARGER_CAN_BAUDRATE_125K        125000U
#define CHARGER_CAN_BAUDRATE_250K        250000U
#define CHARGER_CAN_BAUDRATE_500K        500000U

/* Default baud rate - choose as needed */
#ifndef CHARGER_CAN_BAUDRATE
#define CHARGER_CAN_BAUDRATE             CHARGER_CAN_BAUDRATE_250K
#endif

/* Termination resistor: per spec, no internal termination (120 Ohm optional) */
#define CHARGER_CAN_TERMINATION_OHM      0       /* 0 = none, 120 = external */

/*----------------------------------------------------------------------------------------------------
 * Protection Functions - Thresholds
 *----------------------------------------------------------------------------------------------------*/
/* Input protection */
#define CHARGER_INPUT_OVP_VAC            270.0f  /* +/-5V tolerance, so effective ~265-275 */
#define CHARGER_INPUT_UVP_VAC            85.0f   /* +/-5V, effective ~80-90 */

/* Output protection (relative to platform limits) */
#define CHARGER_OUTPUT_OVP_V             (CHARGER_OUTPUT_VOLTAGE_MAX_V + 5.0f) /* stop if > max+5V */
#define CHARGER_OUTPUT_UVP_V             (CHARGER_OUTPUT_VOLTAGE_MIN_V - 5.0f) /* stop if < min-5V */

/* Temperature protection */
#define CHARGER_TEMP_DERATE_START_C      85.0f   /* power starts decreasing at 85 deg C */
#define CHARGER_TEMP_SHUTDOWN_C          90.0f   /* output shuts off at 90 deg C */

/* Short circuit protection: stops output */
#define CHARGER_SHORT_CIRCUIT_PROTECTION 1

/* Reverse polarity protection: yes */
#define CHARGER_REVERSE_POLARITY_PROTECTION 1

/* Ground resistance limit */
#define CHARGER_GROUND_RESISTANCE_MAX_MOHM  100    /* <=100m Ohm */

/* CAN communication protection: auto stop if fails */
#define CHARGER_CAN_TIMEOUT_MS           5000    /* typical timeout, per BMS spec */

/*----------------------------------------------------------------------------------------------------
 * Environmental Specifications
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_TEMP_OPERATING_MIN_C     -40.0f
#define CHARGER_TEMP_OPERATING_MAX_C     60.0f
#define CHARGER_TEMP_STORAGE_MIN_C       -40.0f
#define CHARGER_TEMP_STORAGE_MAX_C       105.0f
#define CHARGER_HUMIDITY_MIN_PERCENT     5.0f
#define CHARGER_HUMIDITY_MAX_PERCENT     95.0f   /* non condensing */
#define CHARGER_ALTITUDE_MAX_M           5000
#define CHARGER_NOISE_MAX_DB             65.0f   /* Class A */
#define CHARGER_IP_PROTECTION            "IP67"
#define CHARGER_VIBRATION_SWEEP_10_25HZ_MM  1.2f
#define CHARGER_VIBRATION_25_500HZ_MS2     30.0f
#define CHARGER_VIBRATION_DURATION_HOURS    8     /* each axis */
#define CHARGER_MTBF_HOURS               150000

/*----------------------------------------------------------------------------------------------------
 * Mechanical Dimensions and Weight
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_LENGTH_MM                264.5f  /* +/-3mm */
#define CHARGER_WIDTH_MM                 252.0f  /* +/-3mm */
#define CHARGER_HEIGHT_MM                100.0f  /* +/-3mm */
#define CHARGER_WEIGHT_KG                5.0f    /* max */

/*----------------------------------------------------------------------------------------------------
 * Connector Pin Definitions (for reference - not typically used in code)
 *----------------------------------------------------------------------------------------------------*/
/* Low Voltage (control) connector - 6 pin AMPSEAL 33472200611 / 33472200617 */
#define CHARGER_PIN_10_12V               1
#define CHARGER_PIN_CAN_H                2
#define CHARGER_PIN_CAN_L                3
#define CHARGER_PIN_12V_GND              4
/* pins 5,6 NC - not usable as a #define with no value, kept as a comment */

/* AC Input connector - Amphenol HVSL633023A */
#define CHARGER_AC_PIN_N                 1
#define CHARGER_AC_PIN_PE                2
#define CHARGER_AC_PIN_L                 3

/* DC Output connector - Amphenol HVSL362022A */
#define CHARGER_DC_PIN_POSITIVE          1
#define CHARGER_DC_PIN_NEGATIVE          2

/*----------------------------------------------------------------------------------------------------
 * Additional helper macros for scaling (if needed)
 *----------------------------------------------------------------------------------------------------*/
//#define CHARGER_VOLTAGE_TO_CAN(V)        ((uint16_t)((V) / 0.1f))   /* 0.1V/bit */
//#define CHARGER_CURRENT_TO_CAN(A)        ((int16_t)((A) / 0.1f))    /* 0.1A/bit, signed */
//#define CHARGER_TEMP_TO_CAN(C)           ((int8_t)((C) + 100))      /* offset 100, 1 deg C/bit */
//#define CHARGER_CAN_TO_VOLTAGE(V)        ((float)(V) * 0.1f)
//#define CHARGER_CAN_TO_CURRENT(A)        ((float)(A) * 0.1f)
//#define CHARGER_CAN_TO_TEMP(T)           ((int8_t)((T) - 100))



/*----------------------------------------------------------------------------------------------------
 * CAN Message IDs (as per Charger.pdf)
 *----------------------------------------------------------------------------------------------------*/
//#define BMS2CHARGER_BASE_ID      0x1806E6F4U   /* BMS -> Charger (multiple pages) */
//#define CHARGER2BMS_BROADCAST_ID 0x18FF50E5U   /* Charger -> BCA (status) */

/* Pages for BMS -> Charger messages */
typedef enum {  CHARGER_MSG_PAGE_1 = 1U,
                CHARGER_MSG_PAGE_2 = 2U,
                CHARGER_MSG_PAGE_3 = 3U,
                CHARGER_MSG_PAGE_4 = 4U,
                CHARGER_MSG_PAGE_5 = 5U,} CHARGER_MSG_PAGE_t;

#if NUMBER_OF_CELLS_SERIES > 0xFF
#define NUMBER_OF_CHARGER_MSG_PAGE  (CHARGER_MSG_PAGE_5)
#else
#define NUMBER_OF_CHARGER_MSG_PAGE  (CHARGER_MSG_PAGE_4)
#endif

//#define CHARGER_MSG_PAGE_1       1U
//#define CHARGER_MSG_PAGE_2       2U
//#define CHARGER_MSG_PAGE_3       3U
//#define CHARGER_MSG_PAGE_4       4U
//#define CHARGER_MSG_PAGE_5       5U

/*----------------------------------------------------------------------------------------------------
 * Scaling constants (matching CAN specs: 0.1V, 0.1A, 1mV, 1 deg C offset 100, etc.)
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_VOLT_SCALE       0.1f    /* V per CAN unit */
#define CHARGER_CURR_SCALE       0.1f    /* A per CAN unit */
#define CHARGER_CAP_SCALE        0.1f    /* AH per CAN unit */
#define CHARGER_TEMP_OFFSET      100     /* offset for temperature (deg C) */
#define CHARGER_SIGNAL_CELL_V_SCALE         0.001f  /* V per CAN unit (1 mV) */


/* Conversion macros */
#define CHARGER_BATTERY_VOLTAGE_TO_CAN(V)       ((uint16_t)((V) / CHARGER_VOLT_SCALE))   /* 0.1V/bit */
#define CHARGER_CURRENT_TO_CAN(A)               ((int16_t)((A) / CHARGER_CURR_SCALE))    /* 0.1A/bit, signed */
#define CHARGER_TEMP_TO_CAN(C)                  ((uint8_t)((C) + CHARGER_TEMP_OFFSET))    /* offset 100, 1 deg C/bit */
#define CHARGER_CAPACITY_TO_CAN(AH)             ((uint16_t)((AH) / CHARGER_CAP_SCALE))                 /* 0.1Ah/bit*/
#define CHARGER_CELL_VOLT_TO_CAN(V)           ((uint16_t)((V) / CHARGER_SIGNAL_CELL_V_SCALE))   /* 1mV/bit >> 1/1000V.bit */

#define CHARGER_CAN_TO_BATTERY_VOLTAGE(V)       ((float)(V) * CHARGER_VOLT_SCALE)
#define CHARGER_CAN_TO_CURRENT(A)               ((float)(A) * CHARGER_CURR_SCALE)
#define CHARGER_CAN_TO_TEMP(T)                  ((float)((T) - CHARGER_TEMP_OFFSET))
#define CHARGER_CAN_TO_CAP(AH)                  ((float)(AH) * CHARGER_CAP_SCALE)
#define CHARGER_CAN_TO_SIGNAL_VOLT(V)           ((float)((V) * CHARGER_SIGNAL_CELL_V_SCALE))


//#define VOLTS_TO_CHARGER_VAL(V)  ((uint16_t)((V) / CHARGER_VOLT_SCALE))
//#define AMPS_TO_CHARGER_VAL(A)   ((uint16_t)((A) / CHARGER_CURR_SCALE))
//#define MV_TO_CHARGER_VAL(MV)    ((uint16_t)((MV) / CHARGER_MV_SCALE))  /* input in V */
//
//#define CHARGER_VAL_TO_VOLTS(V)  ((float)((V) * CHARGER_VOLT_SCALE))
//#define CHARGER_VAL_TO_AMPS(A)   ((float)((A) * CHARGER_CURR_SCALE))
//#define CHARGER_VAL_TO_TEMP(T)   ((int8_t)((T) - CHARGER_TEMP_OFFSET))

/*----------------------------------------------------------------------------------------------------
 * Default charging limits (derived from battery specifications)
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_TARGET_VOLTAGE                  CHARGER_BATTERY_VOLTAGE_TO_CAN(MAX_BATTERY_CHARGING_VOLTAGE_TARGET)
#define CHARGER_CONSTANT_CURRENT_VOLTAGE        (CHARGER_TARGET_VOLTAGE + CHARGER_BATTERY_VOLTAGE_TO_CAN(0.0f))
//#define CHARGER_CONSTANT_VOLTAGE                (CHARGER_TARGET_VOLTAGE)

#define CHARGER_CONSTANT_CURRENT_A              CHARGER_CURRENT_TO_CAN(BATTERY_CC_CHARGING_CURRENT_AMPS)        /* Constant current phase */
#define CHARGER_TRICKLE_CURRENT_A               CHARGER_CURRENT_TO_CAN(0.5f)        /* Trickle charge current */
#define CHARGER_UNBALANCED_CURRENT_A            CHARGER_CURRENT_TO_CAN(0.1f)
/* Reduced current used to resume charging from CHARGER_STATE_RE_CHARGING
 * (half of the normal CC current, matching the intent of the dead code this
 * state used to carry in comments). */
#define CHARGER_RECHARGE_CURRENT_A              CHARGER_CURRENT_TO_CAN(BATTERY_CC_CHARGING_CURRENT_AMPS * 0.5f)

#define CHARGER_BATTERY_NOMINAL_CAPACITY_AH     CHARGER_CAPACITY_TO_CAN(BATTERY_CAPACITY_NOMINAL_AH)

/* Cell-level protection/target thresholds, all in the SAME 1mV/bit CAN scale
 * as s_battery.cell_max_V / cell_min_V - see fix #1 above. */
#define SINGLE_CELL_MAX_PROTECTION_V            CHARGER_CELL_VOLT_TO_CAN(P_GROUP_VOLT_100_FULL_F)
#define SINGLE_CELL_MIN_PROTECTION_V            CHARGER_CELL_VOLT_TO_CAN(P_GROUP_VOLT_0_FULL_F)

#define SINGLE_CELL_CHARGING_TARGET_V           CHARGER_CELL_VOLT_TO_CAN(MAX_CELL_CHARGING_VOLTAGE_TARGET_F)
/* Allowed per-cell mismatch band around the charging target before the pack
 * is considered "balanced" (re-uses the hardware-defined balance target
 * instead of an arbitrary, wrongly-scaled 0.001). */
#define SINGLE_CELL_TARGET_MAX_DELTA_V          CHARGER_CELL_VOLT_TO_CAN(CELL_BALANCE_TARGET_DELTA_V)
/* Per-cell voltage the pack must sag below (from the charged target) before
 * CHARGER_STATE_DONE_CHARGING will resume charging. Tunable - 50mV is a
 * starting assumption, not a datasheet value. */
#define CHARGER_RECHARGE_HYSTERESIS_V            CHARGER_CELL_VOLT_TO_CAN(0.05f)


/*----------------------------------------------------------------------------------------------------
 * Charger fault / status bits (from Message 2, byte 5)
 *----------------------------------------------------------------------------------------------------*/
#define CHARGER_STATUS_HW_FAIL          (1U << 0)
#define CHARGER_STATUS_OVERTEMP         (1U << 1)
#define CHARGER_STATUS_INPUT_FAULT      (1U << 2)
#define CHARGER_STATUS_STARTING         (1U << 3)
#define CHARGER_STATUS_COMM_TIMEOUT     (1U << 4)

/* Status bits that represent a genuine fault requiring shutdown.
 * CHARGER_STATUS_STARTING is deliberately excluded: it is asserted by the
 * charger while it is legitimately powering up, not a fault, and treating
 * it as one would force a fault-shutdown on every normal start attempt. */
#define CHARGER_STATUS_FAULT_MASK   (CHARGER_STATUS_HW_FAIL | CHARGER_STATUS_OVERTEMP | \
                                     CHARGER_STATUS_INPUT_FAULT | CHARGER_STATUS_COMM_TIMEOUT)

/* Named bits for the "flags" argument to Charger_UpdateStateMachine(), so the
 * meaning of each control bit is documented instead of a bare 0x01/0x02/0x04.
 * NOTE: this is inferred from how Charger.c used them, not from an external
 * spec - confirm against whatever sets these flags before relying on it. */
#define CHARGER_CTRL_FLAG_EXTERNAL_FAULT   (1U << 0)  /* external/system fault request -> force shutdown */
#define CHARGER_CTRL_FLAG_FAULT_RESET      (1U << 1)  /* operator/system requests a fault-state retry */
#define CHARGER_CTRL_FLAG_CHARGE_ENABLE    (1U << 2)  /* permission to charge is present */

#define STATE_INHIBIT_CHARGING_TIME      MIN_SEC_MS_US_TICK_2_TICK(1,0,0,0,0)

/*----------------------------------------------------------------------------------------------------
 * State machine definitions
 *----------------------------------------------------------------------------------------------------*/
typedef enum {
    CHARGER_STATE_NOT_CHARGING,
    CHARGER_STATE_START_CHARGING,
    CHARGER_STATE_TRICKLE_CHARGING,
    CHARGER_STATE_CC_CHARGING,
    CHARGER_STATE_CV_CHARGING,
    CHARGER_STATE_EOC_CHARGING,
    CHARGER_STATE_INHIBIT_CHARGING,
    CHARGER_STATE_RE_CHARGING,
    CHARGER_STATE_DONE_CHARGING,
    CHARGER_STATE_UNBALANCED_CELLS,
    CHARGER_STATE_OVERCHARGED_CELL,
    CHARGER_STATE_OVERHEATED_CELL,
    CHARGER_STATE_CHARGER_FAULT
} ChargerState_t;

#define CHARGER_TRICKLE_CHARGING_STATE_TRIGGER              CHARGER_CELL_VOLT_TO_CAN(3.0f)

#define CELL_CHARGING_BALANCING_DELTA_HBAND                 CHARGER_CELL_VOLT_TO_CAN(0.19f)
#define CELL_CHARGING_MAX_UNBALANCE_DELTA_TRIGGER           CHARGER_CELL_VOLT_TO_CAN(0.200f)
#define CELL_CHARGING_MIN_UNBALANCE_DELTA_TRIGGER           (CELL_CHARGING_MAX_UNBALANCE_DELTA_TRIGGER - CELL_CHARGING_BALANCING_DELTA_HBAND)

/* Both ends now derive from SINGLE_CELL_CHARGING_TARGET_V (cell-scale,
 * 1mV/bit) instead of the pack-scale CHARGER_TARGET_VOLTAGE (see fix #3
 * above - comparing a ~200-ish pack value against a ~3000-4200 cell reading
 * meant this used to latch almost immediately on any real cell voltage). */
#define CELL_CHARGING_OVERSHOOT_HBAND              CHARGER_CELL_VOLT_TO_CAN(0.3f)
#define CELL_MIN_CHARGING_OVERSHOOT_TIGGER         (SINGLE_CELL_CHARGING_TARGET_V + CHARGER_CELL_VOLT_TO_CAN(0.05f))
#define CELL_MAX_CHARGING_OVERSHOOT_TRIGGER        (CELL_MIN_CHARGING_OVERSHOOT_TIGGER + CELL_CHARGING_OVERSHOOT_HBAND)

/* Plain, unscaled degC delta - do NOT run this through CHARGER_TEMP_TO_CAN,
 * which bakes in the +100 absolute-temperature offset (see fix #4 above:
 * subtracting an offset-encoded "10" from an offset-encoded "60" produced a
 * clear-threshold that decodes to -50 degC). */
#define CELL_CHARGING_OVERHEATED_HBAND              (10U)
#define CELL_MAX_CHARGING_OVERHEATED_TRIGGER        CHARGER_TEMP_TO_CAN(60.0f)
#define CELL_MIN_CHARGING_OVERHEATED_TIGGER         (CELL_MAX_CHARGING_OVERHEATED_TRIGGER - CELL_CHARGING_OVERHEATED_HBAND)

/* CHARGER_TARGET_VOLTAGE is already CAN-encoded (0.1V/bit); do a plain
 * subtraction in that same unit system instead of re-encoding it (see fix #2
 * above - the old version divided by 0.1 a second time and inflated the
 * result ~10x, so CC_CHARGING could never reach this trigger). */
#define CV_CHARGING_TRIGGER                         (CHARGER_TARGET_VOLTAGE - CHARGER_BATTERY_VOLTAGE_TO_CAN(0.2f))

#define CHARGER_EOC_CURRENT_TRIGGER                 CHARGER_CURRENT_TO_CAN(BATTERY_EOC_CHARGING_CURRENT_AMPS)        /* End of charge current threshold */
#define CHARGER_EOC_SOC_TRIGGER                     (95)        /* End of charge SOC threshold */

/*----------------------------------------------------------------------------------------------------
 * State machine Triggers
 *----------------------------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------------
 * Data structures for CAN messages
 *----------------------------------------------------------------------------------------------------*/
/* BMS -> Charger command (contains voltage, current, control) - corresponds to Message 10 */
typedef struct {
    uint16_t voltage_dV;      /* Max allowable terminal voltage (0.1V/bit) */
    uint16_t current_dA;      /* Max allowable charging current (0.1A/bit) */
    uint8_t  control;         /* 0 = enable, 1 = disable (protection) */
    uint8_t  discharge_A;     /* Max discharging current (10A/bit) - optional */
    uint8_t  reserved[1];
    uint8_t  page;            /* always 1 for this message */
} __attribute__((packed)) ChargerCmdMsg10_t;

/* BMS -> Charger Message 11 */
typedef struct {
    uint16_t nominal_Ah;      /* 0.1Ah/bit */
    uint16_t actual_Ah;       /* 0.1Ah/bit */
    uint16_t cell_ovp_mV;     /* Single cell max protection voltage (1mV/bit) */
    uint8_t  battery_num;     /* Number of cells in series */
    uint8_t  page;            /* always 2 */
} __attribute__((packed)) ChargerCmdMsg11_t;

/* BMS -> Charger Message 12 */
typedef struct {
    uint16_t cell_max_mV;     /* 1mV/bit */
    uint16_t cell_min_mV;     /* 1mV/bit */
    uint16_t cell_uvp_mV;     /* Single cell min protection voltage (1mV/bit) */
    uint8_t  battery_state;   /* 0x00 normal, bit0=OV, bit1=UV */
    uint8_t  page;            /* always 3 */
} __attribute__((packed)) ChargerCmdMsg12_t;

/* BMS -> Charger Message 13 */
typedef struct {
    uint16_t pack_voltage_dV; /* 0.1V/bit */
    int16_t  charge_current_dA; /* 0.1A/bit, positive=charging, negative=discharging */
    uint8_t  soc;             /* 0-100% */
    uint8_t  temp_max;        /* offset 100: 1 deg C/bit (unsigned - see fix #5) */
    uint8_t  temp_min;        /* offset 100: 1 deg C/bit (unsigned - see fix #5) */
    uint8_t  page;            /* always 4 */
} __attribute__((packed)) ChargerCmdMsg13_t;

/* BMS -> Charger Message 14 (extended battery numbers) - can be used if needed */
typedef struct {
    uint16_t battery_num;     /* number of cells (LSB first) */
    uint8_t  reserved[5];
    uint8_t  page;            /* always 5 */
} __attribute__((packed)) ChargerCmdMsg14_t;

/* Charger -> BMS broadcast (Message 2) */
typedef struct {
    uint16_t output_voltage_dV; /* 0.1V/bit */
    int16_t  output_current_dA; /* 0.1A/bit, positive=charging, negative=discharging */
    uint8_t  status_flags;      /* fault bits as defined above */
    uint8_t  reserved[3];
} __attribute__((packed)) ChargerStatusBroadcast_t;

/* For convenience, we keep the original ChargerStatus_t for internal use */
typedef struct {
    uint16_t output_voltage_dV;
    int16_t  output_current_dA; /* Signed: positive=charging, negative=discharging */
    uint8_t  status_flags;
} ChargerStatus_t;


/*----------------------------------------------------------------------------------------------------
 * External API
 *----------------------------------------------------------------------------------------------------*/
typedef struct {
//    float nominal_Ah;
    float actual_Ah;
    float pack_voltage_V;
    float charge_current_A;
    uint8_t soc;
    float  temp_max;
    float  temp_min;
    float cell_max_V;
    float cell_min_V;
    float cell_avg_V;
//    float cell_ovp_V;
//    float cell_uvp_V;
//    uint16_t cell_count;
}battery2Charger_t;
/*----------------------------------------------------------------------------------------------------
 * External API
 *----------------------------------------------------------------------------------------------------*/

/* Initialisation */
void Charger_Init(void);

/* Periodically called to send all BMS -> charger messages (should be called every 1 second) */
void Charger_SendAllCommands(void);

/* Read the charger broadcast status (should be called when a new frame arrives) */
bool Charger_ReceiveStatus(const ChargerStatusBroadcast_t *status);

/* Update the internal state machine and adjust target voltage/current accordingly.
 * This should be called periodically (e.g., every 100 ms) with the latest battery data.
 */
void Charger_UpdateBatteryData(const battery2Charger_t * battery2Charger);
void Charger_UpdateStateMachine(const uint8_t flags);

/* Translate the current state machine state into target voltage/current/
 * enable and load them into the outgoing command (formerly the file-local,
 * un-prefixed, un-prototyped "changeChargerControl" - see fix #7). Call this
 * after Charger_UpdateStateMachine() and before Charger_SendAllCommands() /
 * Charger_ApplyTargetSettings() so the newly-decided state is what actually
 * gets sent. */
void Charger_UpdateOutputsFromState(void);

/* Send the current target settings (voltage, current, enable) to the charger.
 * Usually called after state machine update or when parameters change.
 */
bool Charger_ApplyTargetSettings(void);

/* Direct setting of target parameters (will be used by state machine) */
void Charger_SetTargetVoltage(float voltage_V);
void Charger_SetTargetCurrent(float current_A);
void Charger_EnableOutput(bool enable);
void Charger_DisableOutput(void);   /* equivalent to enable = false */

/* Get current target / status */
float Charger_GetTargetVoltage(void);
float Charger_GetTargetCurrent(void);
bool  Charger_IsOutputEnabled(void);
float Charger_GetOutputVoltage(void);
float Charger_GetOutputCurrent(void);
uint8_t Charger_GetStatusFlags(void);
ChargerState_t Charger_GetState(void);

/* Query helpers */
bool Charger_IsCharging(void);      /* true if state is CC, CV, or trickle */
bool Charger_IsChargeComplete(void);

/* Fault handling */
void Charger_FaultShutdown(void);   /* disables output and sets target to zero */

/*----------------------------------------------------------------------------------------------------
 * Internal (exposed for testing)
 *----------------------------------------------------------------------------------------------------*/
void Charger_BuildMessage10(uint8_t *buffer);
void Charger_BuildMessage11(uint8_t *buffer);
void Charger_BuildMessage12(uint8_t *buffer);
void Charger_BuildMessage13(uint8_t *buffer);
void Charger_BuildMessage14(uint8_t *buffer);

#endif /* PHANTOM_DRIVERS_INCLUDE_CHARGER_H_ */
