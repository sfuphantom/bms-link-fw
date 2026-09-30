/*
 * BatteryCell_P_Group_Hardware.h
 *
 *  Created on: Sep 7, 2026
 *      Author: tanjo
 *
 *  NOTE ON FIXES:
 *   1. P_GROUP_CAPACITY_NOMINAL_MAH multiplied by NUMBER_OF_CELLS_PARALLEL,
 *      which was never defined in this file (only NUMBER_OF_CELLS_PER_P_GROUP
 *      was) -> fixed to use NUMBER_OF_CELLS_PER_P_GROUP.
 *   2. P_GROUP_CAPACITY_ACTUAL_MAH referenced CELL_CAPACITY_ACTUAL_MAH, which
 *      was only commented out in BatteryCell_Hardware.h -> that macro is now
 *      defined for real (defaults to nominal capacity) and this file scales
 *      it by the parallel cell count, same as the nominal value.
 *   3. MAX_DISCHARGE_CURRENT_AMPS was hardcoded to a single cell's rating
 *      (35A) instead of the parallel-combined rating for the whole P_group
 *      -> fixed and renamed to P_GROUP_MAX_DISCHARGE_CURRENT_AMPS. No
 *      backward-compatibility alias is kept.
 */

#ifndef PHANTOM_HARDWARE_BATTERYCELL_P_GROUP_HARDWARE_H_
#define PHANTOM_HARDWARE_BATTERYCELL_P_GROUP_HARDWARE_H_

#include "BatteryCell_Hardware.h"

//////////////////////////////////////////////////////////////
// P_GROUP TOPOLOGY
//////////////////////////////////////////////////////////////
#define NUMBER_OF_CELLS_PER_P_GROUP    4   /* Cells connected in PARALLEL within one P_group */

//////////////////////////////////////////////////////////////
// CAPACITY / ENERGY / MASS
// Parallel connection: capacity, energy, current, and mass ADD across cells.
// Voltage is UNCHANGED (all cells in a P_group sit at the same terminal voltage).
//////////////////////////////////////////////////////////////
#define P_GROUP_CAPACITY_NOMINAL_MAH               (CELL_CAPACITY_NOMINAL_MAH * NUMBER_OF_CELLS_PER_P_GROUP)
#define P_GROUP_CAPACITY_NOMINAL_AH                (P_GROUP_CAPACITY_NOMINAL_MAH/1000.0f)
#define P_GROUP_CAPACITY_ACTUAL_MAH                (CELL_CAPACITY_ACTUAL_MAH * NUMBER_OF_CELLS_PER_P_GROUP)
#define P_GROUP_CAPACITY_ACTUAL_AH                 (P_GROUP_CAPACITY_ACTUAL_MAH/1000.0f)

#define P_GROUP_ENERGY_NOMINAL_WH                  (P_GROUP_CAPACITY_NOMINAL_AH * CELL_NOMINAL_VOLT)
#define P_GROUP_MASS_GRAMS                         (CELL_MASS_GRAMS * NUMBER_OF_CELLS_PER_P_GROUP)
#define P_GROUP_VOLUME_CM3                         (CELL_VOLUME_CM3 * NUMBER_OF_CELLS_PER_P_GROUP)

//////////////////////////////////////////////////////////////
// VOLTAGE (unchanged by parallel connection - same as a single cell)
//////////////////////////////////////////////////////////////
#define P_GROUP_NOMINAL_VOLT              (CELL_NOMINAL_VOLT)
#define P_GROUP_VOLT_OVER_F               (CELL_VOLT_OVER_F)
#define P_GROUP_VOLT_UNDER_F              (CELL_VOLT_UNDER_F)
#define P_GROUP_VOLT_100_FULL_F           (CELL_VOLT_100_FULL_F)
#define P_GROUP_VOLT_0_FULL_F             (CELL_VOLT_0_FULL_F)
#define P_GROUP_VOLT_OP_RANGE_F           (CELL_VOLT_OP_RANGE_F)

//////////////////////////////////////////////////////////////
// CURRENT (parallel cells: current ratings ADD, assuming ideal current sharing
// across the group - real-world sharing depends on interconnect/busbar resistance
// matching, which is not modeled here)
//////////////////////////////////////////////////////////////
#define P_GROUP_MAX_CHARGE_CURRENT_AMPS             (NUMBER_OF_CELLS_PER_P_GROUP * CELL_MAX_CHARGE_CURRENT_AMPS)
#define P_GROUP_MAX_DISCHARGE_CURRENT_AMPS          (NUMBER_OF_CELLS_PER_P_GROUP * CELL_MAX_DISCHARGE_CONTINUOUS_CURRENT_AMPS)
#define P_GROUP_MAX_DISCHARGE_PULSE_CURRENT_AMPS    (NUMBER_OF_CELLS_PER_P_GROUP * CELL_MAX_DISCHARGE_PULSE_CURRENT_AMPS)

#define P_GROUP_CC_CHARGING_CURRENT_AMPS     (NUMBER_OF_CELLS_PER_P_GROUP * CELL_CC_CHARGING_CURRENT_AMPS)
#define P_GROUP_EOC_CHARGING_CURRENT_AMPS    (NUMBER_OF_CELLS_PER_P_GROUP * CELL_EOC_CHARGING_CURRENT_AMPS)

/* C-rate is intensive (same as a single cell's) - the whole group charges/discharges at the same rate */
#define P_GROUP_MAX_CHARGE_CRATE                    (CELL_MAX_CHARGE_CRATE)
#define P_GROUP_MAX_DISCHARGE_CRATE                 (CELL_MAX_DISCHARGE_CRATE)

//////////////////////////////////////////////////////////////
// INTERNAL RESISTANCE
// Parallel cells: resistances combine as 1/(1/R1+1/R2+...) = R_cell/N for N identical cells
//////////////////////////////////////////////////////////////
#define P_GROUP_INTERNAL_RES_MOHM                   (CELL_MAX_INTERNAL_RES_MOHM / NUMBER_OF_CELLS_PER_P_GROUP)
#define P_GROUP_VOLT_SAG_ESTIMATE_V(AMPS)           ((( AMPS ) * (P_GROUP_INTERNAL_RES_MOHM/1000.0f)))

//////////////////////////////////////////////////////////////
// SOC (intensive - identical to a single cell, since all cells in the group sit at the same voltage)
//////////////////////////////////////////////////////////////
#define P_GROUP_VOLT_TO_SOC_PERCENT(V)               (CELL_VOLT_TO_SOC_PERCENT(V))
#define P_GROUP_SOC_PERCENT_TO_VOLT(SOC)             (CELL_SOC_PERCENT_TO_VOLT(SOC))

#endif /* PHANTOM_HARDWARE_BATTERYCELL_P_GROUP_HARDWARE_H_ */
