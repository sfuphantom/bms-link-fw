/*
 * Battery_Hardware.h
 *
 *  Created on: Jun 29, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_HARDWARE_BATTERY_HARDWARE_H_
#define PHANTOM_HARDWARE_BATTERY_HARDWARE_H_

#define NUMBER_OF_CELLS_SERIES 96
#define NUMBER_OF_CELLS_PARALLEL 5
//////////////////////////////////////////////////////////////
#define CELL_VOLT_100_FULL  41000
#define CELL_VOLT_0_FULL    33000
#define CELL_SOC_RANGE      (CELL_VOLT_100_FULL - CELL_VOLT_0_FULL)

#define CELL_VOLT_SAFETY_RANGE 50
#define CELL_VOLT_OVER      (CELL_VOLT_100_FULL + CELL_VOLT_SAFETY_RANGE)
#define CELL_VOLT_UNDER     (CELL_VOLT_0_FULL   - CELL_VOLT_SAFETY_RANGE)

//////////////////////////////////////////////////////////////
#define MAX_CELL_CHARGING_PERCENTAGE        (70.0f / 100)
#define MAX_CELL_CHARGING_VOLTAGE_TARGET    ((MAX_CELL_CHARGING_PERCENTAGE * CELL_SOC_RANGE) + CELL_VOLT_0_FULL)//38000
#define CELL_CHARGING_SOC_TARGET_TOLORENCES_PERCENTAGE    (02.0f / 100)
/////////////////////////////////////////////////////////////////

#endif /* PHANTOM_HARDWARE_BATTERY_HARDWARE_H_ */
