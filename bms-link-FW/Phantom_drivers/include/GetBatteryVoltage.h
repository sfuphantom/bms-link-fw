/*
 * GetBatteryVoltage.h
 *
 *  Created on: Sep 13, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_GETBATTERYVOLTAGE_H_
#define PHANTOM_DRIVERS_INCLUDE_GETBATTERYVOLTAGE_H_


#include "FullBattery_Hardware.h"
//////////////////////////////////////////////////////////////////
#define USE_HV_BOARD false
//////////////////////////////////////////////////////////////////
//SPI
#define HV_ADQ_TIME_ns          200

#define ADS7044_CODE_SHIFT      2u
//#define ADS7044_CODE_MASK       0x0FFFu
//#define ADS7044_SIGN_BIT        0x0800u
//#define ADS7044_SIGN_EXTEND     0xF000u


#define HV_ADC2VOLTS_OFFSET     3.3f
#define HV_ADC2VOLTS_SCALE      3.3f

#define HV_OVER_VOLT_FLAG      41000
#define HV_UNDER_VOLT_FLAG     26000
///////////////////////////////////////////////////////////////////////
#define VOLT2SAVED_SCALE    (100.0f)

///////////////////////////////////////////////////////////////////////

bool MeasureAndSaveBatteryVoltage();
float GetBatteryVoltage();

#endif /* PHANTOM_DRIVERS_INCLUDE_GETBATTERYVOLTAGE_H_ */
