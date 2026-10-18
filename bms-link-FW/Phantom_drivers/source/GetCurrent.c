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


void MeasureAndSaveCurrent(){
#warning This i cooked, we are not measuring, it is set to a 0, FIX IT!!!
    const float current = 0;//GetChargerCurrent() - GetInverterCurrent() - CalcFanCurrent_Estimte();
    Pack_SetCurrent(current);
}

float GetCurrent_f(){
    const float current = Pack_GetCurrent();
    return current;
}


#warning this is not a measurement, get a measurment
