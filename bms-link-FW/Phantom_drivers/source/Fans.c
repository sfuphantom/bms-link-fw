/*
 * Fans.c
 *
 *  Created on: Jun 30, 2026
 *      Author: tanjo
 */

#include "het.h"
#include "Fans.h"

void StopAllFans(){
    int i;
    for(i=0;i<NUMBER_OF_FANS;i++){
        pwmStop(FAN_HET_RAM, i);
    }
}
void StartAllFans(){
    int i;
    for(i=0;i<NUMBER_OF_FANS;i++){
        pwmStop(FAN_HET_RAM, i);
    }
}
void SetAllFansDuty(const uint32_t duty){
    int i;
    for(i=0;i<NUMBER_OF_FANS;i++){
        pwmSetDuty(FAN_HET_RAM, i, duty);
    }
}
void SetAllFansSignal(const hetSIGNAL_t signal){
    int i;
    for(i=0;i<NUMBER_OF_FANS;i++){
        pwmSetSignal(FAN_HET_RAM, i, signal);
    }
}

void init_fans(){
    hetInit();
    const hetSIGNAL_t signal = {0,0};
    SetAllFansSignal(signal);
    StopAllFans();
}

