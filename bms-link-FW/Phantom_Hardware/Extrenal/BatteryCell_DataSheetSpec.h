/*
 * BatteryCell_DataSheetSpec.h
 *
 *  Raw values from the MOLICEL INR-18650-P26A product data sheet.
 *  Nothing in this file is derived, assumed, or tuned. If a value is not
 *  printed on the datasheet, it does NOT belong here (see BatteryCell_Hardware.h).
 *
 *  All macros use the CELL_DS_ prefix so they never collide with the
 *  application-level CELL_ macros in BatteryCell_Hardware.h.
 *  Only BatteryCell_Hardware.h should include this file.
 */

#ifndef PHANTOM_HARDWARE_BATTERYCELL_DATASHEETSPEC_H_
#define PHANTOM_HARDWARE_BATTERYCELL_DATASHEETSPEC_H_

/* ---- Identification ---- */
#define CELL_DS_MODEL_NAME                  "INR-18650-P26A"

/* ---- Physical characteristics (all "Max") ---- */
#define CELL_DS_SHAPE_NAME                  "Cylindrical"
#define CELL_DS_CAN_MATERIAL                "Steel"
#define CELL_DS_DIAMETER_MM_MAX             (18.6f)
#define CELL_DS_HEIGHT_MM_MAX               (65.2f)
#define CELL_DS_MASS_GRAMS_MAX              (50.0f)

/* ---- Capacity / energy ---- */
#define CELL_DS_CAPACITY_TYPICAL_MAH        (2600.0f)
#define CELL_DS_CAPACITY_TYPICAL_AH         (CELL_DS_CAPACITY_TYPICAL_MAH/1000.0f)
#define CELL_DS_ENERGY_TYPICAL_WH           (9.5f)
#define CELL_DS_ENERGY_DENSITY_VOLUMETRIC_WHL   (535.0f)
#define CELL_DS_ENERGY_DENSITY_GRAVIMETRIC_WHKG (190.0f)

/* ---- Voltage ---- */
#define CELL_DS_VOLT_NOMINAL                (3.6f)
#define CELL_DS_VOLT_CHARGE                 (4.2f)   /* Charge (max) voltage */
#define CELL_DS_VOLT_DISCHARGE_CUTOFF       (2.5f)

/* ---- Charge ---- */
#define CELL_DS_CHARGE_CURRENT_STANDARD_AMPS    (2.6f)
#define CELL_DS_CHARGE_CURRENT_MAX_AMPS         (6.0f)
#define CELL_DS_CHARGE_TIME_STANDARD_HOURS      (1.5f)
#define CELL_DS_CHARGE_CV_CUTOFF_AMPS           (0.050f)  /* Charge graph: "50mA cut" */

/* ---- Discharge ---- */
#define CELL_DS_DISCHARGE_CURRENT_MAX_AMPS      (35.0f)

/* ---- Internal resistance ---- */
#define CELL_DS_INTERNAL_RES_AC_1KHZ_MOHM_MAX   (20.0f)
#define CELL_DS_INTERNAL_RES_AC_1KHZ_OHM_MAX    (CELL_DS_INTERNAL_RES_AC_1KHZ_MOHM_MAX/1000.0f)

/* ---- Ambient temperature ---- */
#define CELL_DS_TEMP_CHARGE_MIN_C           (0.0f)
#define CELL_DS_TEMP_CHARGE_MAX_C           (60.0f)
#define CELL_DS_TEMP_DISCHARGE_MIN_C        (-40.0f)
#define CELL_DS_TEMP_DISCHARGE_MAX_C        (60.0f)

#endif /* PHANTOM_HARDWARE_BATTERYCELL_DATASHEETSPEC_H_ */
