/*
 * Balance.c
 *
 *  Created on: Aug 31, 2026
 *      Author: tanjo
 */
#include <stdint.h>
#include <math.h>
#include "Balance.h"
#include "SlaveCommunation_Hardware.h"
#include "PhantomTimers.h"

static ticks_t periodStart = 0;     // shared by all cells, pwmSignalGen moves it to the current period

// ratio -> duty 0..1. Out of range is clamped, NaN gives 0.
static float RatioToDuty(const float ratio){
    if(!(ratio > 0.0f))
        return 0.0f;
    return MIN_VAL(ratio, 1.0f);
}

// Ramps 0 -> 1 from startVolts to fullVolts. Once running it keeps HOLD_DUTY
// until the value is below startVolts - hysteresis, so it doesn't flicker on/off.
static float RampDuty(const float value, const float startVolts, const float fullVolts,
                      const float hysteresis, const float prevDuty){

    const float duty = RatioToDuty((value - startVolts) / (fullVolts - startVolts));
    const bool holding = (prevDuty > 0.0f) && (value > startVolts - hysteresis);   // NaN -> false

    return holding ? MAX_VAL(duty, HOLD_DUTY) : duty;
}

// 1.0 = full drain, 0.0 = none. Unknown temperature means no drain.
static float TempDerate(const float dieTempC){
    if(isnan(dieTempC))
        return 0.0f;
    const float derate = mapRange_f(dieTempC, DIE_TEMP_DERATE_START_C, DIE_TEMP_DERATE_STOP_C, 1.0f, 0.0f);
    return (float)clampValue(derate, 0.0, 1.0);
}

//////////////////////////////////////////////////////////////////////////////////////////
void Balance_Update(const float cellVolts[NUMBER_OF_CELLS_SERIES],
                    const float minCellVolt,
                    const float dieTempC[NUMBER_OF_SLAVE_BOARDS_TOTAL],
                    float pwm[NUMBER_OF_CELLS_SERIES],
                    uint16_t dcc[NUMBER_OF_SLAVE_BOARDS_TOTAL]){


    uint8_t slave;
    for(slave = 0; slave < NUMBER_OF_SLAVE_BOARDS_TOTAL; slave++){

        dcc[slave] = 0;
        const float dieTempC_slave = dieTempC[slave];
        const float derate = TempDerate(dieTempC_slave);
        uint8_t i;
        for(i = 0; i < CELLS_PER_SLAVE_BOARD; i++){

            const uint8_t cell = slave * CELLS_PER_SLAVE_BOARD + i;
            if(cell >= NUMBER_OF_CELLS_SERIES)
                break;

            const float cellVolt = cellVolts[cell];
            const float prevDuty = pwm[cell] / 100.0f;

            float duty = RampDuty(cellVolt, DRAIN_START_VOLTS, DRAIN_FULL_VOLTS,
                                  DRAIN_HYSTERESIS_VOLTS, prevDuty);

            if(BALANCE_WHILE_CHARGING){
                const float balanceDuty = RampDuty(cellVolt - minCellVolt, BALANCE_START_DIFF_VOLTS,
                                                   BALANCE_FULL_DIFF_VOLTS, BALANCE_HYSTERESIS_VOLTS, prevDuty);
                duty = MAX_VAL(duty, balanceDuty);
            }

            pwm[cell] = 100.0f * duty * derate;

            if(pwmSignalGen_duty100(BALANCE_PWM_PERIOD_TICKS, pwm[cell], &periodStart))
                BIT_SET(dcc[slave], i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
float Balance_GetDrainAmps(const float cellVolts[NUMBER_OF_CELLS_SERIES],
                           const float pwm[NUMBER_OF_CELLS_SERIES],
                           float cellAmps[NUMBER_OF_CELLS_SERIES]){

    float totalAmps = 0.0f;
    uint8_t cell;
    for(cell = 0; cell < NUMBER_OF_CELLS_SERIES; cell++){
        cellAmps[cell] = pwm[cell] * cellVolts[cell] / DRAIN_OHMS;
        totalAmps += cellAmps[cell];
    }
    return totalAmps;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Charge limit for one cell voltage. Never goes up when the volts go up.
static float ChargeLimitAt(const float cellVolt, const float drainAmps){
    const float ratio = (cellVolt - CHARGE_LIMIT_START_VOLTS) / (CHARGE_LIMIT_FULL_VOLTS - CHARGE_LIMIT_START_VOLTS);
    return lerp_f(MAX_CHARGE_AMPS, drainAmps, (float)clampValue(ratio, 0.0, 1.0));
}

float Balance_GetMaxChargeAmps(const float maxCellVolt, const float maxDieTempC, const float prevLimitAmps){

    if(isnan(maxCellVolt))
        return 0.0f;    // can't see the cells, don't charge

    // what the drain can take at full duty (hot slaves drain less)
    const float drainAmps = MIN_VAL(MAX_CHARGE_AMPS,
            CHARGE_LIMIT_DRAIN_MARGIN * TempDerate(maxDieTempC) * maxCellVolt / DRAIN_OHMS);

    const float limit = ChargeLimitAt(maxCellVolt, drainAmps);

    // hysteresis: the old limit holds until the volts are HYSTERESIS below where it was set
    const float heldLimit = MAX_VAL(prevLimitAmps, ChargeLimitAt(maxCellVolt + CHARGE_LIMIT_HYSTERESIS_VOLTS, drainAmps));

    return MIN_VAL(limit, heldLimit);
}
