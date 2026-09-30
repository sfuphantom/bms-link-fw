/*
 * BatteryModule.h
 *
 *  Created on: Sep 6, 2026
 *      Author: tanjo
 *
 *  NOTE ON FIXES:
 *   1. NUMBER_OF_CELLS_SERIES_PER_MODULE was #define'd TWICE: once as a
 *      literal (6), then immediately redefined via a formula referencing
 *      NUMBER_OF_CELLS_SERIES / NUMBER_OF_MODULE - neither of which exists
 *      in this file (they're defined in FullBattery_Hardware.h, which was
 *      never included here). Removed the broken redefinition; the module is
 *      now built up from P_groups (via BatteryCell_P_Group_Hardware.h)
 *      instead of duplicating cell-level math.
 *   2. NUMBER_OF_SLAVE_BOARD_SERIES_PER_MODULE had the same undefined-
 *      external-reference problem (NUMBER_OF_SLAVE_BOARDS, NUMBER_OF_MODULE).
 *      Replaced with a direct NUMBER_OF_SLAVE_BOARDS_PER_MODULE constant.
 *   3. MODULE_VOLT_100_FULL / MODULE_VOLT_0_FULL referenced CELL_VOLT_100_FULL
 *      / CELL_VOLT_0_FULL (missing the _F suffix used by the real macros in
 *      BatteryCell_Hardware.h) -> fixed, and rebased on P_GROUP_* voltages
 *      since a module is a series stack of P_groups, not raw cells.
 *   4. MAX_MODULE_CHARGING_PERCENTAGE multiplied a PERCENTAGE by the series
 *      cell count. SOC% is intensive - 6 cells in series at 90% SOC is still
 *      90% SOC, not 540%. Fixed to not scale with series count.
 *   5. NUMBER_OF_THERMISTORS collided with an identically-named but
 *      differently-valued macro in FullBattery_Hardware.h (64 vs 256) -
 *      renamed to NUMBER_OF_THERMISTORS_PER_MODULE to avoid the clash.
 */

#ifndef PHANTOM_HARDWARE_BATTERYMODULE_H_
#define PHANTOM_HARDWARE_BATTERYMODULE_H_

#include "BatteryCell_P_Group_Hardware.h"
#include "SlaveCommunation_Hardware.h"


//////////////////////////////////////////////////////////////
// MODULE TOPOLOGY: NUMBER_OF_P_GROUPS_SERIES_PER_MODULE P_groups in SERIES,
// each P_group made of NUMBER_OF_CELLS_PER_P_GROUP cells in PARALLEL.
// i.e. this module is a 6S4P pack of 24 cells.
//////////////////////////////////////////////////////////////
#define NUMBER_OF_P_GROUPS_SERIES_PER_MODULE   6   /* P_groups connected in SERIES within one module */

#define NUMBER_OF_CELLS_SERIES_PER_MODULE      (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE)   /* one P_group = one series position */
#define NUMBER_OF_CELLS_PARALLEL_PER_MODULE    (NUMBER_OF_CELLS_PER_P_GROUP)
#define NUMBER_OF_CELLS_TOTAL_PER_MODULE       (NUMBER_OF_CELLS_SERIES_PER_MODULE * NUMBER_OF_CELLS_PARALLEL_PER_MODULE)

#define NUMBER_OF_SLAVE_BOARDS_PER_MODULE      ((NUMBER_OF_P_GROUPS_SERIES_PER_MODULE+CELLS_PER_SLAVE_BOARD-1)/CELLS_PER_SLAVE_BOARD)
#define NUMBER_OF_FANS_PER_MODULE              2
#define NUMBER_OF_THERMISTORS_PER_MODULE       8   /* fixed hardware spec, not derived from cell count */

#define NUMBER_OF_THERMISTORS_PER_CELL          ((float)NUMBER_OF_THERMISTORS_PER_MODULE/((float)(NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * NUMBER_OF_CELLS_PARALLEL_PER_MODULE)))

//////////////////////////////////////////////////////////////
// VOLTAGE (series stacking of P_groups: voltages ADD)
//////////////////////////////////////////////////////////////
#define MODULE_NOMINAL_VOLT       (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * P_GROUP_NOMINAL_VOLT)
#define MODULE_VOLT_OVER_F        (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * P_GROUP_VOLT_OVER_F)
#define MODULE_VOLT_UNDER_F       (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * P_GROUP_VOLT_UNDER_F)

#define MODULE_VOLT_100_FULL_F    (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * P_GROUP_VOLT_100_FULL_F)
#define MODULE_VOLT_0_FULL_F      (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * P_GROUP_VOLT_0_FULL_F)
#define MODULE_VOLT_OP_RANGE_F    (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * P_GROUP_VOLT_OP_RANGE_F)

#define MAX_MODULE_CHARGING_VOLTAGE_TARGET        (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * MAX_CELL_CHARGING_VOLTAGE_TARGET_F)
#define MAX_MODULE_CHARGING_PERCENTAGE            (MAX_CELL_CHARGING_PERCENTAGE_F)  /* intensive - does not scale with series count */

/* SOC (%) estimate from a resting module (stack) voltage */
#define MODULE_VOLT_TO_SOC_PERCENT(V)              ((((( V ) / (float)NUMBER_OF_P_GROUPS_SERIES_PER_MODULE) - P_GROUP_VOLT_0_FULL_F) / P_GROUP_VOLT_OP_RANGE_F) * 100.0f)
#define MODULE_SOC_PERCENT_TO_VOLT(SOC)            (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * P_GROUP_SOC_PERCENT_TO_VOLT(SOC))

//////////////////////////////////////////////////////////////
// CAPACITY / ENERGY / MASS
// Series stacking does not change capacity/current (bounded by the P_group);
// mass/volume/energy scale with the number of P_groups stacked.
//////////////////////////////////////////////////////////////
#define MODULE_CAPACITY_NOMINAL_AH    (P_GROUP_CAPACITY_NOMINAL_AH)
#define MODULE_CAPACITY_ACTUAL_AH     (P_GROUP_CAPACITY_ACTUAL_AH)
#define MODULE_ENERGY_NOMINAL_WH      (MODULE_CAPACITY_NOMINAL_AH * MODULE_NOMINAL_VOLT)
#define MODULE_MASS_GRAMS             (P_GROUP_MASS_GRAMS * NUMBER_OF_P_GROUPS_SERIES_PER_MODULE)
#define MODULE_VOLUME_CM3             (P_GROUP_VOLUME_CM3 * NUMBER_OF_P_GROUPS_SERIES_PER_MODULE)

//////////////////////////////////////////////////////////////
// CURRENT (bounded by the weakest series link, i.e. same as a single P_group)
//////////////////////////////////////////////////////////////
#define MODULE_MAX_CHARGE_CURRENT_AMPS       (P_GROUP_MAX_CHARGE_CURRENT_AMPS)
#define MODULE_MAX_DISCHARGE_CURRENT_AMPS    (P_GROUP_MAX_DISCHARGE_CURRENT_AMPS)
#define MODULE_MAX_DISCHARGE_PULSE_CURRENT_AMPS (P_GROUP_MAX_DISCHARGE_PULSE_CURRENT_AMPS)

#define MODULE_CC_CHARGING_CURRENT_AMPS     (CELL_CC_CHARGING_CURRENT_AMPS)
#define MODULE_EOC_CHARGING_CURRENT_AMPS    (CELL_EOC_CHARGING_CURRENT_AMPS)

//////////////////////////////////////////////////////////////
// INTERNAL RESISTANCE (series: resistances of the stacked P_groups ADD)
//////////////////////////////////////////////////////////////
#define MODULE_INTERNAL_RES_MOHM             (NUMBER_OF_P_GROUPS_SERIES_PER_MODULE * P_GROUP_INTERNAL_RES_MOHM)
#define MODULE_VOLT_SAG_ESTIMATE_V(AMPS)      ((( AMPS ) * (MODULE_INTERNAL_RES_MOHM/1000.0f)))

#endif /* PHANTOM_HARDWARE_BATTERYMODULE_H_ */
