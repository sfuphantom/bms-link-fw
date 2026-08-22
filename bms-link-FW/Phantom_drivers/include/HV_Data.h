/*
 * HV_Current.h
 *
 *  Created on: Jul 2, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_HV_CURRENT_H_
#define PHANTOM_DRIVERS_INCLUDE_HV_CURRENT_H_

#include "spi.h"
#include "spi_helpers.h"

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
//-----------------------------------------
uint16_t Get_HV_Data_Raw();
float HV_ADC2VOLTS(const uint16_t HV_ADC);

#endif /* PHANTOM_DRIVERS_INCLUDE_HV_CURRENT_H_ */
