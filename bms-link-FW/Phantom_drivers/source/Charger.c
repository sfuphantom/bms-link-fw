/*
 * Charger.c
 *
 *  Created on: Jun 17, 2026
 *      Author: tanjo
 */


#include <stdint.h>
#include <stdbool.h>
#include "Charger.h"
#include "charger_can.h"
#include "can.h"
#include "PhantomHelpers.h"


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

ChargerState_t CheckChargerState(const bool start_charging){
    //TODO: check rising edge of gpio pin or cans

    ChargerState_t NewChargerStart;
    const ChargerState_t currentChargerState = ChargerData.ChargerState;

    switch (currentChargerState){
    case NOT_CHARGING   : NewChargerStart = start_charging ? Start_CHARGING : NOT_CHARGING; break;
    case Start_CHARGING : NewChargerStart = start_charging ? Start_CHARGING : NOT_CHARGING; break;
    case CHARGING       : NewChargerStart = start_charging ? CHARGING       : NOT_CHARGING; break;
    case DONE_CHARGING  : NewChargerStart = start_charging ? DONE_CHARGING  : NOT_CHARGING; break;
    case CHARGER_FALUT  : NewChargerStart = start_charging ? CHARGER_FALUT  : NOT_CHARGING; break;
    default             : NewChargerStart = start_charging ? currentChargerState  : NOT_CHARGING; break;
    }

    return NewChargerStart;

}
//----------------------------------------------------------------------------------------------------
bool sendCmd2ChargerWraper(){
    ChargerCmd_t ChargerDateWrite;


    if(ChargerData.ChargerState == Start_CHARGING || ChargerData.ChargerState == CHARGING){
        ChargerDateWrite.charge_enable = ChargerData.enable;
        ChargerDateWrite.max_voltage_dV = ChargerData.TargetChargerVoltage;
        ChargerDateWrite.max_current_dA = ChargerData.TargetChargerCurrent;

        ChargerData.ChargerState = CHARGING;
    }
    else{
        ChargerDateWrite.charge_enable = false;
        ChargerDateWrite.max_voltage_dV = 0;
        ChargerDateWrite.max_current_dA = 0;

        ChargerData.TargetChargerCurrent = 0;
        ChargerData.TargetChargerVoltage = 0;
        ChargerData.enable = false;
    }

    (void)Charger_SendCmd(&ChargerDateWrite);


    return true;
}
void GetChargerStateWraper(){
    ChargerStatus_t ChargerDateRead;
    (void)Charger_GetStatus(&ChargerDateRead);

    ChargerData.OutputChargerVoltage = ChargerDateRead.output_voltage_dV;
    ChargerData.OutputChargerCurrent = ChargerDateRead.output_current_dA;
    ChargerData.status = ChargerDateRead.status_flags;
}
//----------------------------------------------------------------------------------------------------

uint16_t GetChargerFaluts();
void SetChargerFaluts(const uint16_t Faluts);
//----------------------------------------------------------------------------------------------------
// TODO: use cans to talk with chager
uint16_t GetChargerTargetVoltage(){
    return ChargerData.TargetChargerVoltage;
}
uint16_t GetChargerTargetCurrent(){
    return ChargerData.TargetChargerCurrent;
}
uint16_t GetChargerOutputVoltage(){
    return ChargerData.OutputChargerVoltage;
}
uint16_t GetChargerOutputCurrent(){
    return ChargerData.OutputChargerCurrent;
}
uint16_t GetChargerStatus(){
    return ChargerData.status;
}
//----------------------------------------------------------------------------------------------------

bool changeChargeState(const uint16_t Volt, const uint16_t current, const bool enable){
    ChargerData.TargetChargerVoltage = Volt;
    ChargerData.TargetChargerCurrent = current;
    ChargerData.enable = enable;
    return sendCmd2ChargerWraper();
}
bool SetChargerVoltage(const uint16_t Volt){
    ChargerData.TargetChargerVoltage = Volt;
    return sendCmd2ChargerWraper();
}
bool SetChargerCurrent(const uint16_t current){
    ChargerData.TargetChargerCurrent = current;
    return sendCmd2ChargerWraper();
}


bool TurnChargerOn(){
    ChargerData.enable = true;
    return sendCmd2ChargerWraper();
}
bool TurnChargerOff(){
    ChargerData.enable = false;
    return sendCmd2ChargerWraper();
}
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
    ChargerData.ChargerState = CHARGING;
    ChargerData.EstimateTimeDone = 0;
    ChargerData.OutputChargerCurrent=0;
    ChargerData.OutputChargerVoltage=0;
    ChargerData.TargetChargerCurrent=0;
    ChargerData.TargetChargerVoltage=0;
    ChargerData.enable=true;
    ChargerData.status=0;

}
//----------------------------------------------------------------------------------------------------

ChargerCmd_t ChargerDateWrite;
ChargerCmd_t ChargerDateRead;
