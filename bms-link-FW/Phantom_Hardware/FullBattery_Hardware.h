/*
 * Battery_Hardware.h
 *
 *  Created on: Jun 29, 2026
 *      Author: tanjo
 *
 *  NOTE ON FIXES:
 *   1. NUMBER_OF_MODULES_SERIES was hardcoded to 4; per the current pack spec there
 *      is 1 module -> updated to 1. Everything below now derives from that
 *      constant instead of independently hardcoding series/parallel counts,
 *      so future changes to module count stay consistent automatically.
 *   2. BATTERY_VOLT_100_FULL / BATTERY_VOLT_0_FULL referenced
 *      CELL_VOLT_100_FULL / CELL_VOLT_0_FULL (missing the _F suffix) ->
 *      fixed, and rebased on MODULE_* voltages since the pack is a series
 *      stack of modules, not raw cells.
 *   3. NUMBER_OF_THERMISTORS (256) collided with an identically-named but
 *      differently-valued macro in BatteryModule.h (64) - if both headers
 *      were ever included together this is a macro redefinition conflict.
 *      Renamed to NUMBER_OF_THERMISTORS_TOTAL, now derived as
 *      (per-module count * NUMBER_OF_MODULES_SERIES) instead of an independent
 *      magic number.
 *   4. This file previously did NOT include BatteryModule.h and instead
 *      redefined cell-derived math directly (NUMBER_OF_CELLS_SERIES=6,
 *      NUMBER_OF_CELLS_PARALLEL=4 as independent literals). That let the
 *      pack-level topology drift out of sync with the module/P_group
 *      definitions (e.g. previously 6 series cells split across 4 modules
 *      is not an integer number of series cells per module). Now the pack
 *      is built from BatteryModule.h so the topology is always consistent.
 *   5. Was missing a pack-level charging-percentage macro entirely (the
 *      module file had one, buggy); added MAX_BATTERY_CHARGING_PERCENTAGE,
 *      correctly left un-scaled since SOC% is intensive.
 */

#ifndef PHANTOM_HARDWARE_FULLBATTERY_HARDWARE_H_
#define PHANTOM_HARDWARE_FULLBATTERY_HARDWARE_H_

//#include "SlaveCommunation_Hardware.h"
#include "BatteryModule.h"

//////////////////////////////////////////////////////////////
// PACK TOPOLOGY: NUMBER_OF_MODULES_SERIES modules connected in SERIES.
//////////////////////////////////////////////////////////////
#define NUMBER_OF_MODULES_SERIES    1   /* Modules connected in SERIES to form the full pack */

#define NUMBER_OF_CELLS_SERIES       (NUMBER_OF_CELLS_SERIES_PER_MODULE * NUMBER_OF_MODULES_SERIES)
#define NUMBER_OF_CELLS_PARALLEL     (NUMBER_OF_CELLS_PARALLEL_PER_MODULE)
#define NUMBER_OF_CELLS_TOTAL        (NUMBER_OF_CELLS_SERIES * NUMBER_OF_CELLS_PARALLEL)

#define NUMBER_OF_THERMISTORS_TOTAL    (NUMBER_OF_THERMISTORS_PER_MODULE * NUMBER_OF_MODULES_SERIES)
#define NUMBER_OF_FANS_TOTAL           (NUMBER_OF_FANS_PER_MODULE * NUMBER_OF_MODULES_SERIES)
#define NUMBER_OF_SLAVE_BOARDS_TOTAL   (NUMBER_OF_SLAVE_BOARDS_PER_MODULE * NUMBER_OF_MODULES_SERIES)

//////////////////////////////////////////////////////////////
// VOLTAGE (series stacking of modules: voltages ADD)
//////////////////////////////////////////////////////////////
#define BATTERY_NOMINAL_VOLT        (NUMBER_OF_MODULES_SERIES * MODULE_NOMINAL_VOLT)
#define BATTERY_VOLT_OVER_F         (NUMBER_OF_MODULES_SERIES * MODULE_VOLT_OVER_F)
#define BATTERY_VOLT_UNDER_F        (NUMBER_OF_MODULES_SERIES * MODULE_VOLT_UNDER_F)

#define BATTERY_VOLT_100_FULL_F     (NUMBER_OF_MODULES_SERIES * MODULE_VOLT_100_FULL_F)
#define BATTERY_VOLT_0_FULL_F       (NUMBER_OF_MODULES_SERIES * MODULE_VOLT_0_FULL_F)
#define BATTERY_VOLT_OP_RANGE_F     (NUMBER_OF_MODULES_SERIES * MODULE_VOLT_OP_RANGE_F)

#define MAX_BATTERY_CHARGING_VOLTAGE_TARGET    (NUMBER_OF_MODULES_SERIES * MAX_MODULE_CHARGING_VOLTAGE_TARGET)
#define MAX_BATTERY_CHARGING_PERCENTAGE         (MAX_CELL_CHARGING_PERCENTAGE_F)  /* intensive - does not scale with series count */

/* SOC (%) estimate from a resting full-pack voltage */
#define BATTERY_VOLT_TO_SOC_PERCENT(V)          ((((( V ) / (float)NUMBER_OF_MODULES_SERIES) - MODULE_VOLT_0_FULL_F) / MODULE_VOLT_OP_RANGE_F) * 100.0f)
#define BATTERY_SOC_PERCENT_TO_VOLT(SOC)        (NUMBER_OF_MODULES_SERIES * MODULE_SOC_PERCENT_TO_VOLT(SOC))

//////////////////////////////////////////////////////////////
// CAPACITY / ENERGY / MASS
// Series stacking does not change capacity/current (bounded by one module);
// mass/volume/energy scale with the number of modules stacked.
//////////////////////////////////////////////////////////////
#define BATTERY_CAPACITY_NOMINAL_AH   (MODULE_CAPACITY_NOMINAL_AH)
#define BATTERY_CAPACITY_ACTUAL_AH    (MODULE_CAPACITY_ACTUAL_AH)
#define BATTERY_ENERGY_NOMINAL_WH     (BATTERY_CAPACITY_NOMINAL_AH * BATTERY_NOMINAL_VOLT)
#define BATTERY_MASS_GRAMS            (MODULE_MASS_GRAMS * NUMBER_OF_MODULES_SERIES)
#define BATTERY_VOLUME_CM3            (MODULE_VOLUME_CM3 * NUMBER_OF_MODULES_SERIES)
#define BATTERY_GRAVIMETRIC_ENERGY_DENSITY_WHKG  (BATTERY_ENERGY_NOMINAL_WH / (BATTERY_MASS_GRAMS/1000.0f))
#define BATTERY_VOLUMETRIC_ENERGY_DENSITY_WHL     (BATTERY_ENERGY_NOMINAL_WH / (BATTERY_VOLUME_CM3/1000.0f))

//////////////////////////////////////////////////////////////
// CURRENT (bounded by the weakest series link, i.e. same as a single module)
//////////////////////////////////////////////////////////////
#define BATTERY_MAX_CHARGE_CURRENT_AMPS       (MODULE_MAX_CHARGE_CURRENT_AMPS)
#define BATTERY_MAX_DISCHARGE_CURRENT_AMPS    (MODULE_MAX_DISCHARGE_CURRENT_AMPS)
#define BATTERY_MAX_DISCHARGE_PULSE_CURRENT_AMPS (MODULE_MAX_DISCHARGE_PULSE_CURRENT_AMPS)

#define BATTERY_CC_CHARGING_CURRENT_AMPS     (MODULE_CC_CHARGING_CURRENT_AMPS)
#define BATTERY_EOC_CHARGING_CURRENT_AMPS    (MODULE_EOC_CHARGING_CURRENT_AMPS)
//////////////////////////////////////////////////////////////
// INTERNAL RESISTANCE (series: resistances of the stacked modules ADD)
//////////////////////////////////////////////////////////////
#define BATTERY_INTERNAL_RES_MOHM     (NUMBER_OF_MODULES_SERIES * MODULE_INTERNAL_RES_MOHM)
#define BATTERY_VOLT_SAG_ESTIMATE_V(AMPS)   ((( AMPS ) * (BATTERY_INTERNAL_RES_MOHM/1000.0f)))

/////////////////////////////////////////////////////////////////

#endif /* PHANTOM_HARDWARE_FULLBATTERY_HARDWARE_H_ */
