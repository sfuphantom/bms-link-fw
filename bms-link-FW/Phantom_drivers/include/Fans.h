/*
 * Fans.h
 *
 *  Created on: Jun 30, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_FANS_H_
#define PHANTOM_DRIVERS_INCLUDE_FANS_H_

#include "het.h"
#include "reg_het.h"
#include "BatteryModule.h"
#include "FullBattery_Hardware.h"

#define USEING_HET  1

#if USEING_HET == 1
#define FAN_HET_REG  hetREG1
#define FAN_HET_RAM  hetRAM1
#define FAN_HET_PORT hetPORT1
#endif
#if USEING_HET == 2
    #define FAN_HET_REG  hetREG2
    #define FAN_HET_RAM  hetRAM2
    #define FAN_HET_PORT hetPORT2
#endif


//#define NUMBER_OF_FANS 5


#define FAN_MAX_DUTY (95)
#define FAN_MIN_DUTY (5)

#define SIGNAL_FAN_CURRENT_100 (1.0f)

///////////////////////////////////////////////////////////
void setFanDuty(const uint8_t fanID, const uint8_t duty);
uint8_t getFanDuty(const uint8_t fanID);
///////////////////////////////////////////////////////////
void SetModuleFansDuty(const uint8_t ModuleID, const uint8_t duty);
void GetModuleFansDuty(uint8_t * fanDutyCpy, const uint8_t ModuleID);
///////////////////////////////////////////////////////////
void SetAllFansDuty(const uint8_t duty);
void GetAllFansDuty(uint8_t * fanDutyCpy);
const uint8_t* GetFansDuty_ReadPrt();
///////////////////////////////////////////////////////////
void init_fans();
void SetFaultFan();
///////////////////////////////////////////////////////////
float CalcFanCurrent_Estimte();

#endif /* PHANTOM_DRIVERS_INCLUDE_FANS_H_ */
