/*
 * IMD_DataSheetSpec.h
 *
 *  Raw values from the BENDER ISOMETER IR155-3203/IR155-3204 data sheet
 *  (IR155-32xx-V004_D00115_05_D_XXEN/06.2024).
 *  Nothing in this file is derived, assumed, or tuned. If a value is not
 *  printed on the datasheet, it does NOT belong here (see IMD_hardware.h).
 *
 *  All macros use the IMD_DS_ prefix so they never collide with the
 *  application-level macros in IMD_hardware.h.
 *  Only IMD_hardware.h should include this file.
 *
 *  Units are in the macro name: _V, _MA, _kOHM, _HZ, _SEC, _PERCENT ...
 */

#ifndef PHANTOM_HARDWARE_IMD_DATASHEETSPEC_H_
#define PHANTOM_HARDWARE_IMD_DATASHEETSPEC_H_

/* ---- Identification ---- */
#define IMD_DS_MODEL_NAME                   "IR155-3203/IR155-3204"
#define IMD_DS_MANUFACTURER                 "Bender"
#define IMD_DS_DOC_VERSION                  "V004 (06.2024)"

/* ---- Supply / power ---- */
#define IMD_DS_SUPPLY_VOLT_MIN_V            (10.0f)
#define IMD_DS_SUPPLY_VOLT_MAX_V            (36.0f)
#define IMD_DS_SUPPLY_CURRENT_MAX_MA        (150.0f)    /* Max operating current Is   */
#define IMD_DS_PEAK_CURRENT_MAX_AMPS        (2.0f)      /* Max current Ik             */
#define IMD_DS_INRUSH_CURRENT_AMPS          (6.0f)
#define IMD_DS_INRUSH_DURATION_MS           (2.0f)
#define IMD_DS_POWER_MAX_W                  (2.0f)
#define IMD_DS_SUPPLY_DROPOUT_MAX_MS        (2.0f)      /* "Voltage failure"          */
#define IMD_DS_LOAD_DUMP_LIMIT_V            (50.0f)     /* No surge/load dump protection above this */

/* ---- Monitored HV system (L+/L-) ---- */
#define IMD_DS_HV_DC_MAX_V                  (1000.0f)
#define IMD_DS_HV_AC_PEAK_MAX_V             (1000.0f)
#define IMD_DS_HV_AC_RMS_MAX_V              (660.0f)    /* 10 Hz...1 kHz */
#define IMD_DS_HV_AC_FREQ_MIN_HZ            (10.0f)
#define IMD_DS_HV_AC_FREQ_MAX_HZ            (1000.0f)
#define IMD_DS_ISOLATION_TEST_VAC           (3500.0f)   /* 1 min, (L+/L-) to everything else */
#define IMD_DS_LEAKAGE_CAP_MAX_UF           (1.0f)      /* Ce; above this range shrinks and time grows */
#define IMD_DS_LEAKAGE_CAP_EXAMPLE_UF       (3.0f)      /* Example: max range 1 MOhm, tan = 68 s       */
#define IMD_DS_LEAKAGE_CAP_EXAMPLE_TAN_SEC  (68.0f)

/* ---- Measuring circuit ---- */
#define IMD_DS_MEAS_VOLT_V                  (40.0f)     /* +/- */
#define IMD_DS_MEAS_CURRENT_UA              (33.0f)     /* +/-, at RF = 0 */
#define IMD_DS_IMPEDANCE_50HZ_MIN_MOHM      (1.2f)
#define IMD_DS_INTERNAL_RES_MIN_MOHM        (1.2f)

/* ---- Response values ---- */
#define IMD_DS_RAN_MIN_kOHM                 (100.0f)    /* Response value Ran, customer setting */
#define IMD_DS_RAN_MAX_kOHM                 (1000.0f)
#define IMD_DS_RAN_DEFAULT_kOHM             (100.0f)
#define IMD_DS_RAN_HYSTERESIS_PERCENT       (25.0f)     /* DCP */
#define IMD_DS_UV_DETECT_MIN_V              (0.0f)
#define IMD_DS_UV_DETECT_MAX_V              (500.0f)
#define IMD_DS_UV_DETECT_DEFAULT_3204_V     (0.0f)      /* 0 = inactive */
#define IMD_DS_UV_DETECT_DEFAULT_3203_V     (300.0f)
#define IMD_DS_FAVE_MIN                     (1)         /* Factor averaging, output M */
#define IMD_DS_FAVE_MAX                     (10)
#define IMD_DS_FAVE_FACTORY                 (10)        /* Recommended for EV/hybrid  */

/* ---- Measuring range / accuracy ---- */
#define IMD_DS_MEAS_RANGE_MAX_kOHM          (10000.0f)  /* 0...10 MOhm */
#define IMD_DS_RES_INFINITE_kOHM            (50000.0f)  /* Duty 5% = ">50 MOhm (infinite)" */

/* SST (30 Hz): only good/bad */
#define IMD_DS_SST_GOOD_FACTOR              (2.0f)      /* good if R > 2   x Ran */
#define IMD_DS_SST_BAD_FACTOR               (0.5f)      /* bad  if R < 0.5 x Ran */

/* DCP relative uncertainty (graph breakpoints) */
#define IMD_DS_UNC_ABS_LOW_MAX_kOHM         (85.0f)     /* 0...85 kOhm: +/-20 kOhm absolute */
#define IMD_DS_UNC_ABS_LOW_kOHM             (20.0f)
#define IMD_DS_UNC_REL_BP1_kOHM             (100.0f)
#define IMD_DS_UNC_REL_BP2_kOHM             (1200.0f)
#define IMD_DS_UNC_REL_BP3_kOHM             (10000.0f)
#define IMD_DS_UNC_REL_AT_BP1_PERCENT       (15.0f)     /* 100 kOhm: +/-15%, narrows linearly ... */
#define IMD_DS_UNC_REL_AT_BP2_PERCENT       (7.0f)      /* ... to +/-7% at 1.2 MOhm ...           */
#define IMD_DS_UNC_REL_AT_BP3_PERCENT       (15.0f)     /* ... and widens again to +/-15% at 10 MOhm */
/* Absolute uncertainty graph labels (+/-) */
#define IMD_DS_UNC_ABS_AT_BP1_kOHM          (15.0f)
#define IMD_DS_UNC_ABS_AT_BP2_kOHM          (84.0f)
#define IMD_DS_UNC_ABS_AT_BP3_kOHM          (1500.0f)

#define IMD_DS_UNC_OUTPUT_FREQ_PERCENT      (5.0f)      /* "Output M (fundamental frequency)", each of 10...50 Hz */
#define IMD_DS_UNC_UV_PERCENT_UN_100V       (10.0f)     /* Un >= 100 V */
#define IMD_DS_UNC_UV_PERCENT_UN_300V       (5.0f)      /* Un >= 300 V */

/* ---- Time response ---- */
#define IMD_DS_TAN_SST_MAX_SEC              (2.0f)      /* Typ < 1 s at Un > 100 V */
#define IMD_DS_TAN_SST_TYP_SEC              (1.0f)
#define IMD_DS_DCP_START_DELAY_SEC          (2.0f)      /* DCP starts this long after power on */
#define IMD_DS_FIRST_MEAS_MAX_SEC           (17.5f)     /* First valid DCP result (10 Hz / 20 Hz) */

/* Response time tan (RF 10 MOhm -> Ran/2, Ce = 1 uF, Un = 1000 V) by Fave */
#define IMD_DS_TAN_DCP_FAVE_1_SEC           (5.0f)
#define IMD_DS_TAN_DCP_FAVE_2_SEC           (7.5f)
#define IMD_DS_TAN_DCP_FAVE_3_SEC           (7.5f)
#define IMD_DS_TAN_DCP_FAVE_4_SEC           (10.0f)
#define IMD_DS_TAN_DCP_FAVE_5_SEC           (12.5f)
#define IMD_DS_TAN_DCP_FAVE_6_SEC           (12.5f)
#define IMD_DS_TAN_DCP_FAVE_7_SEC           (15.0f)
#define IMD_DS_TAN_DCP_FAVE_8_SEC           (17.5f)
#define IMD_DS_TAN_DCP_FAVE_9_SEC           (17.5f)
#define IMD_DS_TAN_DCP_FAVE_10_SEC          (20.0f)

/* Switch-off time tab (Ran/2 -> RF 10 MOhm, Ce = 1 uF, Un = 1000 V) by Fave */
#define IMD_DS_TAB_DCP_FAVE_1_SEC           (20.0f)
#define IMD_DS_TAB_DCP_FAVE_2_SEC           (20.0f)
#define IMD_DS_TAB_DCP_FAVE_3_SEC           (26.0f)
#define IMD_DS_TAB_DCP_FAVE_4_SEC           (26.0f)
#define IMD_DS_TAB_DCP_FAVE_5_SEC           (26.0f)
#define IMD_DS_TAB_DCP_FAVE_6_SEC           (33.0f)
#define IMD_DS_TAB_DCP_FAVE_7_SEC           (33.0f)
#define IMD_DS_TAB_DCP_FAVE_8_SEC           (33.0f)
#define IMD_DS_TAB_DCP_FAVE_9_SEC           (40.0f)
#define IMD_DS_TAB_DCP_FAVE_10_SEC          (40.0f)

/* Self test: runs every 5 min, interfaces are not affected, but tan/tab grow by its duration */
#define IMD_DS_SELFTEST_PERIOD_SEC          (300.0f)
#define IMD_DS_SELFTEST_DURATION_SEC        (10.0f)

/* ---- Measurement output (M) frequency codes ---- */
#define IMD_DS_FREQ_SHORT_CIRCUIT_HZ        (0.0f)      /* Hi: short to Ub+ (Kl.15); Low: IMD off or short to Kl.31 */
#define IMD_DS_FREQ_NORMAL_HZ               (10.0f)     /* DCP, PWM 5...95% is the resistance   */
#define IMD_DS_FREQ_UNDERVOLTAGE_HZ         (20.0f)     /* DCP, PWM 5...95% is the resistance   */
#define IMD_DS_FREQ_SST_HZ                  (30.0f)     /* Speed start, good/bad only           */
#define IMD_DS_FREQ_DEVICE_ERROR_HZ         (40.0f)
#define IMD_DS_FREQ_EARTH_FAULT_HZ          (50.0f)     /* Connection fault on Kl.31 (E/KE)     */

/* ---- Measurement output (M) PWM ---- */
#define IMD_DS_PWM_DUTY_MIN_PERCENT         (5.0f)      /* Working range 5...95% */
#define IMD_DS_PWM_DUTY_MAX_PERCENT         (95.0f)
#define IMD_DS_PWM_SST_GOOD_MIN_PERCENT     (5.0f)
#define IMD_DS_PWM_SST_GOOD_MAX_PERCENT     (10.0f)
#define IMD_DS_PWM_SST_BAD_MIN_PERCENT      (90.0f)
#define IMD_DS_PWM_SST_BAD_MAX_PERCENT      (95.0f)
#define IMD_DS_PWM_ERR_MIN_PERCENT          (47.5f)     /* 40 Hz and 50 Hz */
#define IMD_DS_PWM_ERR_MAX_PERCENT          (52.5f)

/* RF = (90% x 1200 kOhm) / (dc_meas - 5%) - 1200 kOhm,  dc_meas in % (10/20 Hz only)
 * Printed points: 5% = >50 MOhm, 50% = 1200 kOhm, 95% = 0 kOhm                        */
#define IMD_DS_PWM_R_NUM_PERCENT            (90.0f)
#define IMD_DS_PWM_R_SCALE_kOHM             (1200.0f)

/* ---- Outputs (M and OK) ---- */
#define IMD_DS_OUT_DRIVE_DROP_V             (2.0f)      /* High = Us - 2 V (3204); Low = Kl.31 + 2 V (3203) */
#define IMD_DS_OUT_PULL_RES_kOHM            (2.2f)      /* 3204: pull-down to Kl.31; 3203: pull-up to Kl.15; OK_HS: pull-down */
#define IMD_DS_OUT_LOAD_CURRENT_MA          (80.0f)
#define IMD_DS_OUT_TURN_ON_MAX_US           (125.0f)    /* to 90% Vout */
#define IMD_DS_OUT_TURN_OFF_MAX_US          (175.0f)    /* to 10% Vout */
#define IMD_DS_OUT_SLEW_ON_MAX_V_PER_US     (6.0f)      /* 10...30% Vout */
#define IMD_DS_OUT_SLEW_OFF_MAX_V_PER_US    (8.0f)      /* 70...40% Vout */
/* OK_HS: High = no fault (RF > response value); Low = insulation fault, device error,
 *        earth connection fault, undervoltage or device off                           */

/* ---- Connector XK1A pins ---- */
#define IMD_DS_PIN_KL31_ELECTRONIC          (1)
#define IMD_DS_PIN_KL15_SUPPLY              (2)
#define IMD_DS_PIN_KL31_CHASSIS             (3)
#define IMD_DS_PIN_KL31_CHASSIS_SEPARATE    (4)
#define IMD_DS_PIN_M_HS                     (5)         /* 3204 only */
#define IMD_DS_PIN_M_LS                     (6)         /* 3203 only */
#define IMD_DS_PIN_NC                       (7)
#define IMD_DS_PIN_OK_HS                    (8)

/* ---- General ---- */
#define IMD_DS_TEMP_MIN_C                   (-40.0f)
#define IMD_DS_TEMP_MAX_C                   (105.0f)
#define IMD_DS_MASS_GRAMS                   (52.0f)     /* +/-2 g */
#define IMD_DS_PCB_LENGTH_MM                (140.0f)
#define IMD_DS_PCB_WIDTH_MM                 (60.0f)
#define IMD_DS_PCB_HEIGHT_MM                (15.0f)

#endif /* PHANTOM_HARDWARE_IMD_DATASHEETSPEC_H_ */
