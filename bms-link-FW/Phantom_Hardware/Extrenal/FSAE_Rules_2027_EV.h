/*
 * FSAE_Rules_2027_EV.h
 *
 *  Numeric limits from the FSAE Rules 2027 (V1.0, 1 Sept 2026), taken from
 *  04_FSAE_Rules_2027_EV_handoff.md. Rule IDs are in the comments so the
 *  original can be checked. Rule wording is paraphrased in the source.
 *
 *  Only numbers that drive the electrical system live here. Nothing is
 *  derived or tuned: team margins go in the *_hardware.h files.
 *
 *  All macros use the RULE_ prefix. Units are in the macro name.
 *  Only *_hardware.h files should include this file.
 */

#ifndef PHANTOM_HARDWARE_FSAE_RULES_2027_EV_H_
#define PHANTOM_HARDWARE_FSAE_RULES_2027_EV_H_

/* ---- Power and voltage ---- */
#define RULE_POWER_MAX_KW                   (80.0f)     /* EV.3.3.1 Energy Meter                    */
#define RULE_TS_VOLT_MAX_V                  (600.0f)    /* EV.3.3.2 between any two points, DC      */
#define RULE_HV_DC_ABOVE_V                  (60.0f)     /* T.9.1.1 HV is above this (LV at or below) */
#define RULE_HV_AC_RMS_ABOVE_V              (25.0f)
#define RULE_REGEN_OFF_BELOW_KMH            (5.0f)      /* EV.3.3.3 no regen from 0 to 5 km/h       */

/* ---- Energy Meter violation (EV.3.4.1) ---- */
#define RULE_EM_VIOLATION_TIME_MS           (100.0f)    /* Sustained this long, or ...              */
#define RULE_EM_AVG_WINDOW_MS               (500.0f)    /* ... above the moving average of this     */

/* ---- Modules (EV.5.1.1) ---- */
#define RULE_MODULE_VOLT_MAX_V              (120.0f)    /* Static                                   */
#define RULE_MODULE_ENERGY_MAX_MJ           (6.0f)      /* Max stack voltage x nominal capacity     */
#define RULE_MODULE_MASS_MAX_KG             (12.0f)
#define RULE_MODULE_G_LONGITUDINAL          (40.0f)     /* F.10.3 cells must not be crushed         */
#define RULE_MODULE_G_LATERAL               (40.0f)
#define RULE_MODULE_G_VERTICAL              (20.0f)

/* ---- Shutdown, precharge, discharge ---- */
#define RULE_TS_SAFE_VOLT_V                 (60.0f)     /* EV.8.2.2c, EV.9.4.2b TS must be this low ... */
#define RULE_TS_DISCHARGE_MAX_SEC           (5.0f)      /* ... within this long after shutdown          */
#define RULE_PRECHARGE_TARGET_PERCENT       (90.0f)     /* EV.5.6.1 of TS voltage before 2nd IR closes  */
#define RULE_DISCHARGE_RATED_SEC            (15.0f)     /* EV.5.6.3d at max TS voltage                  */
#define RULE_IR_HOLDUP_CAP_MAX_MS           (250.0f)    /* EV.5.4.5                                     */
#define RULE_VOLT_INDICATOR_ON_V            (60.0f)     /* EV.5.7.3 ON at or above ...                  */
#define RULE_VOLT_INDICATOR_OFF_V           (50.0f)     /* ... OFF below                                */

/* ---- BMS ---- */
#define RULE_CELL_TEMP_MAX_C                (60.0f)     /* EV.8.5.2 or cell datasheet max, whichever is lower */
#define RULE_TEMP_SENSE_MIN_PERCENT         (20.0f)     /* EV.8.5.5 lithium cells with a sensor, evenly spread */
#define RULE_TEMP_SENSE_MAX_DIST_MM         (10.0f)     /* EV.8.5.3 from weld/clamp on negative terminal */

/* ---- IMD ---- */
#define RULE_IMD_MIN_OHM_PER_V              (500.0f)    /* EV.8.6.3 response value, of max TS voltage   */
#define RULE_IMD_TRIP_MAX_SEC               (30.0f)     /* IN.10.1 shutdown within this ...             */
#define RULE_IMD_TRIP_RES_BELOW_PERCENT     (50.0f)     /* ... at fault resistance up to this far below response value */
#define RULE_IMD_TEST_VOLT_LOW_V            (250.0f)    /* IN.10.2 insulation measured at 250 V or 500 V */
#define RULE_IMD_TEST_VOLT_HIGH_V           (500.0f)

/* ---- BSPD (EV.8.7.2) ---- */
#define RULE_BSPD_POWER_KW                  (5.0f)      /* At nominal voltage, with hard braking */
#define RULE_BSPD_TIME_MS                   (500.0f)    /* Must persist longer than this         */

/* ---- APPS / brake ---- */
#define RULE_APPS_IMPLAUS_PERCENT           (10.0f)     /* T.4.2.4-5 */
#define RULE_APPS_IMPLAUS_TIME_MS           (100.0f)
#define RULE_APPS_OPEN_SHORT_LOW_V          (0.5f)      /* T.4.2 analog example: below this or ... */
#define RULE_APPS_OPEN_SHORT_HIGH_V         (4.5f)      /* ... above this is open/short            */
#define RULE_PLAUS_BRAKE_APPS_PERCENT       (25.0f)     /* EV.4.7 brake pressed and APPS above this */
#define RULE_PLAUS_RELEASE_APPS_PERCENT     (5.0f)      /* motors stay off until APPS below this    */
#define RULE_INERTIA_SWITCH_MIN_G           (8.0f)      /* T.9.4 */
#define RULE_INERTIA_SWITCH_MAX_G           (11.0f)

/* ---- Indicators and sequence ---- */
#define RULE_TS_STATUS_FLASH_MIN_HZ         (2.5f)      /* EV.6.1 red light on BMS/IMD fault */
#define RULE_TS_STATUS_FLASH_MAX_HZ         (5.0f)
#define RULE_RTM_FLASH_MIN_HZ               (0.66f)     /* EV.6.2 Ready-to-Move, TS >= 60 V outside container */
#define RULE_RTM_FLASH_MAX_HZ               (1.33f)
#define RULE_LAMP_CHECK_START_MAX_SEC       (1.0f)      /* EV.6.3 after GLVMS on */
#define RULE_LAMP_CHECK_MIN_SEC             (2.0f)
#define RULE_LAMP_CHECK_MAX_SEC             (6.0f)
#define RULE_RTD_SOUND_MIN_SEC              (1.0f)      /* EV.10 */
#define RULE_RTD_SOUND_MAX_SEC              (3.0f)
#define RULE_RTD_SOUND_MIN_DBA              (80.0f)

/* ---- Wiring and grounding ---- */
#define RULE_WIRE_TEMP_MIN_C                (90.0f)     /* EV.7 insulation and TS wire rating       */
#define RULE_TS_WIRE_ANCHOR_N               (200.0f)
#define RULE_GROUND_NEAR_TS_MAX_MOHM        (300.0f)    /* EV.7.7 at 1 A, parts within 100 mm of TS */
#define RULE_GROUND_CONDUCTIVE_MAX_OHM      (5.0f)      /* Parts that may become conductive         */
#define RULE_TSMP_RES_UP_TO_200V_kOHM       (5.0f)      /* EV.5.8 series resistor per TSMP          */
#define RULE_TSMP_RES_UP_TO_400V_kOHM       (10.0f)
#define RULE_TSMP_RES_UP_TO_600V_kOHM       (15.0f)

/* ---- TS/GLV spacing in a shared enclosure (EV.7.5) ---- */
#define RULE_SPACING_BELOW_100V_MM          (10.0f)
#define RULE_SPACING_100_TO_200V_MM         (20.0f)
#define RULE_SPACING_ABOVE_200V_MM          (30.0f)

/* ---- TS/GLV spacing on a shared PCB (EV.7.5.7): clearance / creepage / creepage under conformal coat ---- */
#define RULE_PCB_0_50V_CLEAR_MM             (1.0f)
#define RULE_PCB_0_50V_CREEP_MM             (4.0f)
#define RULE_PCB_0_50V_COAT_MM              (1.0f)
#define RULE_PCB_50_150V_CLEAR_MM           (1.0f)
#define RULE_PCB_50_150V_CREEP_MM           (5.0f)
#define RULE_PCB_50_150V_COAT_MM            (1.0f)
#define RULE_PCB_150_300V_CLEAR_MM          (1.5f)
#define RULE_PCB_150_300V_CREEP_MM          (10.0f)
#define RULE_PCB_150_300V_COAT_MM           (2.0f)
#define RULE_PCB_300_600V_CLEAR_MM          (3.0f)
#define RULE_PCB_300_600V_CREEP_MM          (20.0f)
#define RULE_PCB_300_600V_COAT_MM           (4.0f)

/* ---- Events ---- */
#define RULE_ENDURANCE_KM                   (22.0f)     /* D.12 about */
#define RULE_CO2_KG_PER_KWH                 (0.65f)     /* D.13 */

#endif /* PHANTOM_HARDWARE_FSAE_RULES_2027_EV_H_ */
