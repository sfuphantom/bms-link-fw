/*
 * GetCurrent.c
 *
 *  Created on: Sep 15, 2026
 *      Author: tanjo
 */

#include <stdint.h>
#include <stdbool.h>
#include "GetCurrent.h"
#include "FullBattery_Hardware.h"
#include "BatteryData.h"


int16_t Current2Saved(const float Current){
    const float Current_scaled = Current * CURRENT2SAVED_SCALE;
    return (int16_t)Current_scaled;
}
float Saved2Current(const int16_t Saved){
    const float Current = ((float)Saved) / CURRENT2SAVED_SCALE;
    return Current;
}

void MeasureAndSaveCurrent(){
    const float current = 0;//GetChargerCurrent() - GetInverterCurrent() - CalcFanCurrent_Estimte();
    const int16_t current_SaveVal = Current2Saved(current);
    setFullBatteryData_Current(current_SaveVal);
}

float GetCurrent_f(){
    const uint16_t Saved = getFullBatteryData_Current();
    const float current = Saved2Current(Saved);
    return current;
}


#warning this is not a measurement, get a measurment
