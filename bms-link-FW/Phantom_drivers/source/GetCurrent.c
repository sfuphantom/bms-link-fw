/*
 * GetCurrent.c
 *
 *  Created on: Sep 15, 2026
 *      Author: tanjo
 */

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "GetCurrent.h"
#include "FullBattery_Hardware.h"
#include "BatteryData.h"

#warning Compressed current range is only +/-65.5 A at this scale. Check it against the maximum pack current.

float MeasureCurrent(void){
#if USE_CURRENT_SENSOR
    return ReadCurrentSensor_A();
#else
#warning Current is ESTIMATED, not measured. Switch USE_CURRENT_SENSOR to 1 once a sensor exists.
#warning The three functions below were commented out in the old code. Confirm they exist (CalcFanCurrent_Estimte may be a typo) and the sign convention is right.
    return GetChargerCurrent() - GetInverterCurrent() - CalcFanCurrent_Estimte();
#endif
}

int16_t CompressCurrent(const float amps){
    if(isnan(amps)) return 0;

    const float scaled = roundf(amps * CURRENT2SAVED_SCALE);
    if(scaled <= (float)INT16_MIN) return INT16_MIN;
    if(scaled >= (float)INT16_MAX) return INT16_MAX;
    return (int16_t)scaled;
}

float GetCurrent_f(void){
    return Pack_GetCurrent();
}

int16_t GetCurrent_i16(void){
    return CompressCurrent(Pack_GetCurrent());
}
