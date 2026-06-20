/*
 * Charger.c
 *
 *  Created on: Jun 17, 2026
 *      Author: tanjo
 */


#include <stdint.h>
#include <stdbool.h>
#include "Charger.h"
//#include "can.c"


bool isCharging(){
    return ChargerData.ChargerState == CHARGING;
}
void StartCharging(){
    ChargerData.ChargerState = CHARGING;
}
void DoneCharging(){
    ChargerData.ChargerState = DONE_CHARGING;
}
void ResetCharging(){
    ChargerData.ChargerState = NOT_CHARGING;
}

ChargerState_t CheckChargerState(){

    //TODO: check rising edge of gpio pin

}
//----------------------------------------------------------------------------------------------------
uint16_t GetChargerFaluts();
void SetChargerFaluts(const uint16_t Faluts);
//----------------------------------------------------------------------------------------------------
// TODO: use cans to talk with chager
void SetChargerVoltage(const uint16_t Volt);
void SetChargerCurrent(const uint16_t current);
uint16_t GetChargerVoltage();
uint16_t GetChargerCurrent();

void TurnChargerOn();
void TurnChargerOff();
//----------------------------------------------------------------------------------------------------
uint16_t CalcNewCurrentSetting(const float SOC){
    //TODO: get better equtaion
    if(MAX_OVERSHOOT_CURRENT_PERCENTAGE > SOC){
        return MAX_CHARGER_CURRENT_AMPS;
    }
    if(100 - MAX_OVERSHOOT_CURRENT_PERCENTAGE < SOC){
        return OFF_CHARGER_CURRENT_AMPS;
    }

    float NewCurrent_f = (-SOC + 100)/100 * CHARGER_CURRENT_AMPS_RANGE + MIN_CHARGER_CURRENT_AMPS;
    uint16_t NewCurrent_16 = (uint16_t)NewCurrent_f;
    return NewCurrent_16;
}
//----------------------------------------------------------------------------------------------------
struct ChargerData_t* GetChargerDataPrt(){
    return &ChargerData;
}
void initCharger(){

}
