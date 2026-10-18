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

#define PWM_NIBBLE_MAX 0xF   // LTC6811: duty = nibble / 15
#define DRAIN_OHMS     (RES_DRAIN_OHMS + RES_ON_OHMS)

static uint8_t GetNibble(const uint8_t pwm[], const uint8_t cell){
    return (pwm[cell / 2] >> ((cell % 2) * 4)) & 0xF;
}

static void SetNibble(uint8_t pwm[], const uint8_t cell, const uint8_t nibble){
    const uint8_t shift = (cell % 2) * 4;
    pwm[cell / 2] = (uint8_t)((pwm[cell / 2] & ~(0xF << shift)) | (nibble << shift));
}

// ratio 0..1 -> nibble 0..15. Out of range is clamped, NaN gives 0.
static uint8_t RatioToNibble(const float ratio){
    if(!(ratio > 0.0f))
        return 0;
    if(ratio >= 1.0f)
        return PWM_NIBBLE_MAX;
    return (uint8_t)lroundf(ratio * PWM_NIBBLE_MAX);
}

// Ramps 0 -> 15 from startVolts to fullVolts. Once running it keeps the lowest duty
// until the value is below startVolts - hysteresis, so it doesn't flicker on/off.
static uint8_t RampNibble(const float value, const float startVolts, const float fullVolts,
                          const float hysteresis, const uint8_t prevNibble){

    const uint8_t nibble = RatioToNibble((value - startVolts) / (fullVolts - startVolts));
    const bool holding = (prevNibble > 0) && (value > startVolts - hysteresis);   // NaN -> false

    return (nibble == 0 && holding) ? 1 : nibble;
}

// 1.0 = full drain, 0.0 = none. Unknown temperature means no drain.
static float TempDerate(const float dieTempC){
    if(isnan(dieTempC))
        return 0.0f;
    const float derate = mapRange_f(dieTempC, DIE_TEMP_DERATE_START_C, DIE_TEMP_DERATE_STOP_C, 1.0f, 0.0f);
    return (float)clampValue(derate, 0.0, 1.0);
}

//////////////////////////////////////////////////////////////////////////////////////////
void Balance_GetPWM(const float cellVolts[NUMBER_OF_CELLS_SERIES],
                    const float minCellVolt,
                    const float dieTempC[NUMBER_OF_SLAVE_BOARDS_TOTAL],
                    uint8_t pwm[NUMBER_OF_CELLS_SERIES_PAIRS]){
    uint8_t slave;
    for(slave = 0; slave < NUMBER_OF_SLAVE_BOARDS_TOTAL; slave++){

        const float derate = TempDerate(dieTempC[slave]);
        uint8_t i;
        for(i = 0; i < CELLS_PER_SLAVE_BOARD; i++){

            const uint8_t cell = slave * CELLS_PER_SLAVE_BOARD + i;
            if(cell >= NUMBER_OF_CELLS_SERIES)
                break;

            const float cellVolt = cellVolts[cell];
            const uint8_t prevNibble = GetNibble(pwm, cell);

            uint8_t nibble = RampNibble(cellVolt, DRAIN_START_VOLTS, DRAIN_FULL_VOLTS,
                                        DRAIN_HYSTERESIS_VOLTS, prevNibble);

            if(BALANCE_WHILE_CHARGING){
                const uint8_t balanceNibble = RampNibble(cellVolt - minCellVolt, BALANCE_START_DIFF_VOLTS,
                                                         BALANCE_FULL_DIFF_VOLTS, BALANCE_HYSTERESIS_VOLTS, prevNibble);
                nibble = MAX_VAL(nibble, balanceNibble);
            }

            SetNibble(pwm, cell, (uint8_t)lroundf(nibble * derate));
        }
    }
}

void Balance_GetDCC(const uint8_t pwm[NUMBER_OF_CELLS_SERIES_PAIRS], uint16_t dcc[NUMBER_OF_SLAVE_BOARDS_TOTAL]){
    uint8_t slave;
    for(slave = 0; slave < NUMBER_OF_SLAVE_BOARDS_TOTAL; slave++){

        dcc[slave] = 0;
        uint8_t i;
        for(i = 0; i < CELLS_PER_SLAVE_BOARD; i++){

            const uint8_t cell = slave * CELLS_PER_SLAVE_BOARD + i;
            if(cell >= NUMBER_OF_CELLS_SERIES)
                break;

            if(GetNibble(pwm, cell) > 0)
                BIT_SET(dcc[slave], i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
float Balance_GetDrainAmps(const float cellVolts[NUMBER_OF_CELLS_SERIES],
                           const uint8_t pwm[NUMBER_OF_CELLS_SERIES_PAIRS],
                           float cellAmps[NUMBER_OF_CELLS_SERIES]){

    float totalAmps = 0.0f;
    uint8_t cell;
    for(cell = 0; cell < NUMBER_OF_CELLS_SERIES; cell++){
        const float duty = (float)GetNibble(pwm, cell) / PWM_NIBBLE_MAX;
        cellAmps[cell] = duty * cellVolts[cell] / DRAIN_OHMS;
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
