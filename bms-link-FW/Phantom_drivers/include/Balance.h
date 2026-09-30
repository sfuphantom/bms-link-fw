/*
 * CellBalance.h
 *
 *  Created on: Aug 30, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_BALANCE_H_
#define PHANTOM_DRIVERS_INCLUDE_BALANCE_H_

#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include "SlaveCommunation_Hardware.h"
#include "PhantomHelpers.h"

#include "SlaveCommunation_Hardware.h"
#include "BatteryCell_Hardware.h"
#include "FullBattery_Hardware.h"

///////////////////////////////////////////////////////////////////
#define BALANCE_WHILE_CHARGING false
///////////////////////////////////////////////////////////////////
#define BALANCE_SLOW_NO_DRAIN_HYSTERSIS_BAND_SLAVE_ADC              VOLTS2SLAVE_ADC(0.1f)

#define BALANCE_SLOW_DRAIN_THESHOLD_SLAVE_ADC_DIFF                  VOLTS2SLAVE_ADC(0.2f)
#define BALANCE_NO_DRAIN_THESHOLD_SLAVE_ADC_DIFF                    (BALANCE_SLOW_DRAIN_THESHOLD_SLAVE_ADC_DIFF - BALANCE_SLOW_NO_DRAIN_HYSTERSIS_BAND_SLAVE_ADC)

#define BALANCE_FULL_DRAIN_THESHOLD_SLAVE_ADC_DIFF                  VOLTS2SLAVE_ADC(0.5f)

#define BALANCE_SLOW_DRAIN_BAND_SLAVE_ADC_DIFF                     (BALANCE_FULL_DRAIN_THESHOLD_SLAVE_ADC_DIFF-BALANCE_NO_DRAIN_THESHOLD_SLAVE_ADC_DIFF)

/////////////////////////////////////////////////////////////////////

#define CELL_CHARGING_TARGET_SLAVE_ADC                  VOLTS2SLAVE_ADC(MAX_CELL_CHARGING_VOLTAGE_TARGET_F)

#define SLOW_DRAIN_BAND_SLAVE_ADC                       VOLTS2SLAVE_ADC(0.2f)
#define FULL_DRAIN_THESHOLD_SLAVE_ADC                   (CELL_CHARGING_TARGET_SLAVE_ADC + SLOW_DRAIN_BAND_SLAVE_ADC)


/////////////////////////////////////////////////////////////////////


 void GetDrainPWMNibbles(const uint16_t allVolts[NUMBER_OF_CELLS_SERIES], uint8_t * PWMNibbles);

#endif /* PHANTOM_DRIVERS_INCLUDE_BALANCE_H_ */
