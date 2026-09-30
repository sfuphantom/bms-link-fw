/*
 * Fans.c
 *
 *  Created on: Jun 30, 2026
 *      Author: tanjo
 */

#include "het.h"
#include "Fans.h"
#include "string.h"
#include "BatteryModule.h"
#include "FullBattery_Hardware.h"
///////////////////////////////////////////////////////////
static uint8_t FanDuty[NUMBER_OF_FANS_TOTAL] = {0};
///////////////////////////////////////////////////////////
void setFanDuty(const uint8_t fanID, const uint8_t duty){
    pwmSetDuty(FAN_HET_RAM, fanID, duty);
    FanDuty[fanID] = duty;
}
uint8_t getFanDuty(const uint8_t fanID){
    return FanDuty[fanID];
}
///////////////////////////////////////////////////////////
void SetModuleFansDuty(const uint8_t ModuleID, const uint8_t duty){
    int i;
    for(i=0;i<NUMBER_OF_FANS_PER_MODULE;i++){
        setFanDuty(i+ModuleID, duty);
    }
}
void GetModuleFansDuty(uint8_t * fanDutyCpy, const uint8_t ModuleID){
    int i;
    for(i=0;i<NUMBER_OF_FANS_PER_MODULE;i++){
        fanDutyCpy[i] = getFanDuty(i+ModuleID);
    }
}
///////////////////////////////////////////////////////////
void SetAllFansDuty(const uint8_t duty){
    int i;
    for(i=0;i<NUMBER_OF_FANS_TOTAL;i++){
        setFanDuty(i, duty);
    }
}
void GetAllFansDuty(uint8_t * fanDutyCpy){
    memcpy(fanDutyCpy, FanDuty, sizeof(FanDuty));
}
const uint8_t* GetFansDuty_ReadPrt(){
    return FanDuty;
}
///////////////////////////////////////////////////////////

void init_fans(){
    const uint8_t startDuty = 95;
    const hetSIGNAL_t signal = {startDuty, 40};

    int i;
    for(i=0;i<NUMBER_OF_FANS_TOTAL;i++){
        pwmSetSignal(FAN_HET_RAM, i, signal);
        pwmStart(FAN_HET_RAM, i);
    }

    memset(FanDuty, startDuty, sizeof(FanDuty));
}

void SetFaultFan(){
    SetAllFansDuty(FAN_MAX_DUTY);
}
///////////////////////////////////////////////////////////
float CalcFanCurrent_Estimte(){
    int i;

    float currentSum_Amp = 0;
    for(i=0;i<NUMBER_OF_FANS_TOTAL;i++){
        currentSum_Amp += SIGNAL_FAN_CURRENT_100 * FanDuty[i];
    }

    return currentSum_Amp;
}

