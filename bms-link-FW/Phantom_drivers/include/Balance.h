/*
 * Balance.h
 *
 *  Created on: Aug 30, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_BALANCE_H_
#define PHANTOM_DRIVERS_INCLUDE_BALANCE_H_

#include <stdint.h>
#include <stdbool.h>
#include "SlaveCommunation_Hardware.h"
#include "BatteryCell_Hardware.h"
#include "FullBattery_Hardware.h"
#include "PhantomHelpers.h"
#include "PhantomTimers.h"

#define MAX_CHARGE_AMPS 10

///////////////////////////////////////////////////////////////////
// The DCC bits are switched in software: on for (duty * period) of every period.
#define BALANCE_PWM_PERIOD_TICKS    MAKE_TICK(0,10,0,0,0)

// Cell volts are assumed to be open-circuit volts.

// Also bleed cells that are high compared to the lowest cell
#define BALANCE_WHILE_CHARGING      false

#define BALANCE_START_DIFF_VOLTS    0.2f    // starts draining above this difference
#define BALANCE_FULL_DIFF_VOLTS     0.5f    // 100% drain at this difference
#define BALANCE_HYSTERESIS_VOLTS    0.025f  // keeps draining down to START - HYSTERESIS

///////////////////////////////////////////////////////////////////
// Drain cells near the charge target, 0% at START up to 100% at FULL
#define DRAIN_UP_N_DOWN             true   // true: 100% at the target (starts draining before it)
                                            // false: starts at the target, 100% above it
#define DRAIN_BAND_VOLTS            0.01f
#define DRAIN_HYSTERESIS_VOLTS      0.002f  // keep above the ADC error (about 3 mV)

#if DRAIN_UP_N_DOWN
#define DRAIN_START_VOLTS           (MAX_CELL_CHARGING_VOLTAGE_TARGET_F - DRAIN_BAND_VOLTS)
#else
#define DRAIN_START_VOLTS           MAX_CELL_CHARGING_VOLTAGE_TARGET_F
#endif
#define DRAIN_FULL_VOLTS            (DRAIN_START_VOLTS + DRAIN_BAND_VOLTS)

// Lowest duty kept while a cell is inside the hysteresis band
#define HOLD_DUTY                   0.05f

// Slave die temperature (ITMP): full drain below START, no drain at STOP
#define DIE_TEMP_DERATE_START_C     65.0f
#define DIE_TEMP_DERATE_STOP_C      95.0f

///////////////////////////////////////////////////////////////////
// Charge current limit from the highest cell:
// below START no limit, START..FULL ramps down to what the drain can handle, above FULL stays there
#define CHARGE_LIMIT_START_VOLTS    DRAIN_START_VOLTS
#define CHARGE_LIMIT_FULL_VOLTS     DRAIN_FULL_VOLTS
#define CHARGE_LIMIT_HYSTERESIS_VOLTS 0.02f
#define CHARGE_LIMIT_DRAIN_MARGIN   0.8f    // charge at most this part of the drain current

///////////////////////////////////////////////////////////////////
// Call it often (many times per period). pwm[] = duty 0..1 per cell, in = last result (zero it once
// at startup, it is used for hysteresis), out = new result.
// dcc[] = DCC switches right now, bit n of dcc[slave] = DCC n+1.
void Balance_Update(const float cellVolts[NUMBER_OF_CELLS_SERIES],
                    const float minCellVolt,
                    const float dieTempC[NUMBER_OF_SLAVE_BOARDS_TOTAL],
                    float pwm[NUMBER_OF_CELLS_SERIES],
                    uint16_t dcc[NUMBER_OF_SLAVE_BOARDS_TOTAL]);

// Average drain current per cell (duty * V / R) and the total of all cells.
float Balance_GetDrainAmps(const float cellVolts[NUMBER_OF_CELLS_SERIES],
                           const float pwm[NUMBER_OF_CELLS_SERIES],
                           float cellAmps[NUMBER_OF_CELLS_SERIES]);

// Highest charge current the drain can keep up with. prevLimitAmps = the last result
// (start with MAX_CHARGE_AMPS). Lowers right away, only raises after the cell volts dropped by the hysteresis.
float Balance_GetMaxChargeAmps(const float maxCellVolt, const float maxDieTempC, const float prevLimitAmps);

#endif /* PHANTOM_DRIVERS_INCLUDE_BALANCE_H_ */
