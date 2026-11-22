/*
 * Charger_DataSheetSpec.h
 *
 *  Raw values from the HK-MF series 3.3 kW on-board charger hardware spec
 *  and the "BMS: CAN BUS COMMUNICATION SPECIFICATION" (doc 0010, v0A).
 *  Nothing in this file is derived, assumed, or tuned. If a value is not
 *  printed in the spec, it does NOT belong here (see Charger_Hardware.h).
 *
 *  All macros use the CHARGER_DS_ prefix so they never collide with the
 *  application-level CHARGER_ macros in Charger_Hardware.h.
 *  Only Charger_Hardware.h should include this file.
 *
 *  Where the source documents contradict each other, the value is NOT
 *  guessed here: see the "OPEN QUESTIONS" block at the end.
 */

#ifndef PHANTOM_HARDWARE_CHARGER_DATASHEETSPEC_H_
#define PHANTOM_HARDWARE_CHARGER_DATASHEETSPEC_H_

/* ---- Identification ---- */
#define CHARGER_DS_SERIES_NAME                  "HK-MF"

/* ---- Models: output voltage range (V) and max output current (A) ---- */
#define CHARGER_DS_HK_MF_48_40_VOUT_MIN_V       (35.0f)
#define CHARGER_DS_HK_MF_48_40_VOUT_MAX_V       (70.0f)
#define CHARGER_DS_HK_MF_48_40_IOUT_MAX_A       (40.0f)

#define CHARGER_DS_HK_MF_72_40_VOUT_MIN_V       (50.0f)
#define CHARGER_DS_HK_MF_72_40_VOUT_MAX_V       (107.0f)
#define CHARGER_DS_HK_MF_72_40_IOUT_MAX_A       (40.0f)

#define CHARGER_DS_HK_MF_108_32_VOUT_MIN_V      (80.0f)
#define CHARGER_DS_HK_MF_108_32_VOUT_MAX_V      (161.0f)
#define CHARGER_DS_HK_MF_108_32_IOUT_MAX_A      (32.0f)

/* ---- Output (DC to pack) ---- */
#define CHARGER_DS_OUT_POWER_MAX_220VAC_W       (3300.0f)
#define CHARGER_DS_OUT_POWER_MAX_110VAC_W       (1650.0f)
#define CHARGER_DS_OUT_CV_ACCURACY_PERCENT      (1.0f)     /* +/- */
#define CHARGER_DS_OUT_CC_ACCURACY_PERCENT      (2.0f)     /* +/- */
#define CHARGER_DS_OUT_RIPPLE_PERCENT           (5.0f)     /* +/- */
#define CHARGER_DS_OUT_RISE_TIME_MS_MAX         (5000U)
#define CHARGER_DS_OUT_OVERSHOOT_PERCENT_MAX    (10.0f)
#define CHARGER_DS_SHUTOFF_CURRENT_10PCT_MS     (300U)     /* current < 10% within this time */
#define CHARGER_DS_SHUTOFF_CURRENT_ZERO_MS      (500U)     /* current = 0 within this time   */
#define CHARGER_DS_EFFICIENCY_OUTPUT_TABLE_PERCENT_MIN  (94.0f)
#define CHARGER_DS_EFFICIENCY_INPUT_TABLE_PERCENT_MIN   (93.0f)  /* full load; see OPEN QUESTIONS */

/* ---- Input (AC) ---- */
#define CHARGER_DS_IN_VOLTAGE_AC_MIN_V          (90.0f)
#define CHARGER_DS_IN_VOLTAGE_AC_MAX_V          (265.0f)
#define CHARGER_DS_IN_FREQ_MIN_HZ               (47.0f)
#define CHARGER_DS_IN_FREQ_MAX_HZ               (63.0f)
#define CHARGER_DS_IN_CURRENT_MAX_A             (16.0f)
#define CHARGER_DS_IN_POWER_FACTOR_MIN          (0.98f)    /* above 1650 W */
#define CHARGER_DS_IN_STANDBY_POWER_W_MAX       (5.0f)
#define CHARGER_DS_IN_INRUSH_CURRENT_A_MAX      (24.0f)

/* ---- Auxiliary 12 V output ---- */
#define CHARGER_DS_LV_VOLTAGE_V                 (12.0f)
#define CHARGER_DS_LV_CURRENT_NOMINAL_A         (5.5f)
#define CHARGER_DS_LV_POWER_W_MAX               (66.0f)
#define CHARGER_DS_LV_CV_ACCURACY_PERCENT       (2.0f)     /* +/- */
#define CHARGER_DS_LV_RIPPLE_PERCENT_MAX        (1.0f)

/* ---- Control interface ---- */
#define CHARGER_DS_WAKEUP_INPUT_CURRENT_MA_MAX  (10.0f)
#define CHARGER_DS_WAKEUP_OUTPUT_CURRENT_A_MAX  (0.2f)
#define CHARGER_DS_SLEEP_CURRENT_MA_MAX         (1.0f)
#define CHARGER_DS_SLEEP_PEAK_CURRENT_A_MAX     (5.0f)     /* old header said 5 mA; see OPEN QUESTIONS */

/* Low-voltage connector pins (all other pins NC) */
#define CHARGER_DS_LV_PIN_12V                   (10U)
#define CHARGER_DS_LV_PIN_CAN_H                 (11U)
#define CHARGER_DS_LV_PIN_CAN_L                 (12U)
#define CHARGER_DS_LV_PIN_12V_GND               (17U)
/* AC input connector */
#define CHARGER_DS_AC_PIN_N                     (1U)
#define CHARGER_DS_AC_PIN_PE                    (2U)
#define CHARGER_DS_AC_PIN_L                     (3U)
/* DC output connector */
#define CHARGER_DS_DC_PIN_POSITIVE              (1U)
#define CHARGER_DS_DC_PIN_NEGATIVE              (2U)

/* ---- Protections ---- */
#define CHARGER_DS_PROT_IN_OVP_VAC              (270.0f)   /* +/-5 V */
#define CHARGER_DS_PROT_IN_UVP_VAC              (85.0f)    /* +/-5 V */
#define CHARGER_DS_PROT_OUT_VOLTAGE_MARGIN_V    (5.0f)     /* stops above model max + this, or below model min - this */
#define CHARGER_DS_PROT_TEMP_DERATE_START_C     (85.0f)
#define CHARGER_DS_PROT_TEMP_SHUTDOWN_C         (90.0f)
#define CHARGER_DS_PROT_GROUND_RESISTANCE_MOHM_MAX  (100.0f)

/* ---- Environment ---- */
#define CHARGER_DS_TEMP_OPERATING_MIN_C         (-40.0f)
#define CHARGER_DS_TEMP_OPERATING_MAX_C         (60.0f)
#define CHARGER_DS_TEMP_STORAGE_MIN_C           (-40.0f)
#define CHARGER_DS_TEMP_STORAGE_MAX_C           (105.0f)
#define CHARGER_DS_HUMIDITY_MIN_PERCENT         (5.0f)
#define CHARGER_DS_HUMIDITY_MAX_PERCENT         (95.0f)    /* non-condensing */
#define CHARGER_DS_ALTITUDE_MAX_M               (5000U)
#define CHARGER_DS_NOISE_MAX_DB                 (65.0f)
#define CHARGER_DS_IP_RATING                    "IP67"
#define CHARGER_DS_MTBF_HOURS                   (150000U)

/* ---- Mechanical ---- */
#define CHARGER_DS_LENGTH_MM                    (264.5f)   /* +/-3 mm */
#define CHARGER_DS_WIDTH_MM                     (252.0f)   /* +/-3 mm */
#define CHARGER_DS_HEIGHT_MM                    (100.0f)   /* +/-3 mm */
#define CHARGER_DS_MASS_KG_MAX                  (5.0f)

/* ---- CAN protocol (BMS <-> charger) ---- */
#define CHARGER_DS_CAN_BAUD_SPEC                (250000U)  /* CAN 2.0B, 29-bit IDs, J1939 style */
#define CHARGER_DS_CAN_PERIOD_MS                (1000U)    /* every message, 8 data bytes        */
#define CHARGER_DS_CAN_TIMEOUT_MS               (5000U)    /* no message this long -> charger closes output */

/* Source addresses */
#define CHARGER_DS_CAN_SA_BMS                   (0xF4U)
#define CHARGER_DS_CAN_SA_CHARGER               (0xE5U)
#define CHARGER_DS_CAN_SA_CHARGER2              (0xE6U)    /* charging-station charger */
#define CHARGER_DS_CAN_SA_BROADCAST             (0x50U)

/* IDs, for reference/checking. The real IDs are set in the HALCoGen message boxes. */
#define CHARGER_DS_CAN_ID_MSG1                  (0x1806E5F4UL)  /* BMS -> charger, V/I/control, NO page byte */
#define CHARGER_DS_CAN_ID_MSG10_14              (0x1806E6F4UL)  /* BMS -> charger, byte 8 = page 1..5       */
#define CHARGER_DS_CAN_ID_MSG2                  (0x18FF50E5UL)  /* charger -> broadcast, status             */

/*
 * OPEN QUESTIONS (source documents contradict each other - verify on the real unit)
 *  - Baud rate:   CAN spec 250 kbps; hardware doc says 125/250/500 selectable in one
 *                 place and only 250/500 in another.
 *  - Termination: hardware doc lists a 120 ohm terminal resistance in one place and
 *                 "no terminating resistor" in another. Measure CAN_H to CAN_L.
 *  - Efficiency:  94% in the output table, 93% in the input table.
 *  - Sleep peak:  hardware doc handoff says "peak <= 5 A"; the old Charger.h said 5 mA.
 *  - Pin 10:      doc does not say whether +12 V is an input (control supply) or the
 *                 auxiliary output. No wake-up pin is listed.
 *  - IDs:         the HK-MF doc never names message IDs. That it speaks this exact
 *                 protocol is an assumption. A plain on-board charger may only
 *                 understand Message 1 (ID 0x1806E5F4, no page byte).
 */

#endif /* PHANTOM_HARDWARE_CHARGER_DATASHEETSPEC_H_ */
