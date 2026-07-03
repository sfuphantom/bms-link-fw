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


#define NUMBER_OF_FANS 5




void StopAllFans();
void StartAllFans();
void SetAllFansDuty(const uint32_t duty);
void SetAllFansSignal(const hetSIGNAL_t signal);

void init_fans();

#endif /* PHANTOM_DRIVERS_INCLUDE_FANS_H_ */
