/*
 * Charger.c
 *
 *  Created on: Jun 17, 2026
 *      Author: tanjo
 */


#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "Charger.h"
//#include "charger_can.h"
#include "can.h"
#include "Phantom_Can.h"

#include "PhantomHelpers.h"

#include "BatteryData.h"

//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
//bool SendFault_Cans(const BMSFaultsData_t* const data){
//    const uint32_t returnval =  can_transmit_data(BMS2VCU_FAULT, data, sizeof(*data));
//    return returnval;
//}
//bool GetFault_Cans(BMSFaultsData_t* const data){
//    const uint32_t returnval =  can_receive_data(BMS2VCU_FAULT, data, sizeof(*data));
//    return returnval;
//}

//static uint32 Charger_SendCmd(const ChargerCmd_t *cmd){
//    uint8 data[BMS2CHARGER_DATA_MSG_LEN_BYTES] = {0};
//
//    uint16 v = swap_word_bytes(cmd->output_voltage_dV);
//    uint16 i = swap_word_bytes(cmd->output_current_dA);
//
//    memcpy(&data[0], &v, 2);
//    memcpy(&data[2], &i, 2);
//    data[4] = cmd->status_flags ? 1 : 0;
//
//    return transmit_BMS2CHARGER_DATA(data);
//}
//static uint32 Charger_GetStatus(ChargerStatus_t *status){
//    uint8 data[BMS2CHARGER_DATA_MSG_LEN_BYTES];
//    uint16 v, i;
//
//    uint32_t return_val = receive_CHARGER2BMS_DATA(data);
//
//    if (return_val == 0U) return 0U;
//
//    memcpy(&v, &data[0], 2);
//    memcpy(&i, &data[2], 2);
//
//    status->output_voltage_dV = swap_word_bytes(v);
//    status->output_current_dA = swap_word_bytes(i);
//    status->status_flags = data[4];
//
//    return return_val;
//}

#define CHARGER_BIG_ENDINEN TRUE
bool Charger_SendCmd(const ChargerStatus_t* const data){

#if CHARGER_BIG_ENDINEN
    const uint32_t return_val =  can_transmit_data(BMS2CHARGER_DATA, data, sizeof(ChargerStatus_t));
#else
    ChargerStatus_t data_swap;
    data_swap.status_flags      = data->status_flags;
    data_swap.output_voltage_dV = swap_word_bytes(data->output_voltage_dV);
    data_swap.output_current_dA = swap_word_bytes(data->output_current_dA);

    const uint32_t return_val =  can_transmit_data(BMS2CHARGER_DATA, &data_swap, sizeof(data_swap));
#endif
    return return_val;
}
bool Charger_GetStatus(ChargerStatus_t* const data){

    const uint32_t return_val =  can_receive_data(CHARGER2BMS_DATA, data, sizeof(*data));

    if (return_val == 0U) return 0U;

#if !CHARGER_BIG_ENDINEN

    data->output_voltage_dV = swap_word_bytes(data->output_voltage_dV);
    data->output_current_dA = swap_word_bytes(data->output_current_dA);
#endif
    return return_val;
}

bool sendCmd2ChargerWraper(){
    if(ChargerData.ChargerState == Start_CHARGING){
        ChargerData.ChargerState = CHARGING;
    }
    else if (ChargerData.ChargerState != CHARGING){
        ChargerDateWrite->status_flags = false;
        ChargerDateWrite->output_voltage_dV = 0;
        ChargerDateWrite->output_current_dA = 0;
    }

    return Charger_SendCmd(ChargerDateWrite);
}
bool GetChargerStatusWraper(){
    return Charger_GetStatus(ChargerDateRead);
}
//----------------------------------------------------------------------------------------------------
// TODO: use cans to talk with chager
uint16_t GetChargerTargetVoltage(){
    return ChargerDateWrite->output_voltage_dV;
}
uint16_t GetChargerTargetCurrent(){
    return ChargerDateWrite->output_current_dA;
}
uint16_t GetChargerOutputVoltage(){
    return ChargerDateRead->output_voltage_dV;
}
uint16_t GetChargerOutputCurrent(){
    return ChargerDateRead->output_current_dA;
}
bool isChargerEnabled(){
    return ChargerDateWrite->status_flags;
}
uint8_t GetChargerStatus(){
    return ChargerDateRead->status_flags;
}
//----------------------------------------------------------------------------------------------------
bool changeChargeState(const uint16_t Volt, const uint16_t current, const bool enable){
    ChargerDateWrite->output_voltage_dV = Volt;
    ChargerDateWrite->output_current_dA = current;
    ChargerDateWrite->status_flags      = enable;

    return sendCmd2ChargerWraper();
}
bool SetChargerVoltage(const uint16_t Volt){
    ChargerDateWrite->output_voltage_dV = Volt;
    return sendCmd2ChargerWraper();
}
bool SetChargerCurrent(const uint16_t current){
    ChargerDateWrite->output_current_dA = current;
    return sendCmd2ChargerWraper();
}
bool SetChargerLimits(const uint16_t Volt, const uint16_t current){
    ChargerDateWrite->output_voltage_dV = Volt;
    ChargerDateWrite->output_current_dA = current;

    return sendCmd2ChargerWraper();
}
bool TurnChargerOn(){
    ChargerDateWrite->status_flags = true;
    return sendCmd2ChargerWraper();
}
bool TurnChargerOff(){
    ChargerDateWrite->status_flags = false;
    return sendCmd2ChargerWraper();
}
//----------------------------------------------------------------------------------------------------
bool isCharging(){
    return ChargerData.ChargerState == CHARGING;
}
bool isChargingDone(){
    return ChargerData.ChargerState;
}
bool StartCharging(){
    ChargerData.ChargerState = Start_CHARGING;
    return sendCmd2ChargerWraper();
}
bool DoneCharging(){
    ChargerData.ChargerState = DONE_CHARGING;
    return sendCmd2ChargerWraper();
}
bool StopCharging(){
    ChargerData.ChargerState = NOT_CHARGING;
    return sendCmd2ChargerWraper();
}
bool ShutDownCharger_Fault(){
    ChargerData.ChargerState = CHARGER_FALUT;

    ChargerDateWrite->output_voltage_dV = 0;
    ChargerDateWrite->output_current_dA = 0;
    ChargerDateWrite->status_flags  = false;

    return sendCmd2ChargerWraper();
}

//ChargerState_t CheckNewChargerState(const bool start_charging){
//    //TODO: check rising edge of gpio pin or cans
//    if(ChargerData.ChargerState == CHARGER_FALUT){
//        return CHARGER_FALUT;
//    }
//    if(!start_charging){
//        ChargerData.ChargerState = NOT_CHARGING;
//        return ChargerData.ChargerState;
//    }
//    switch (ChargerData.ChargerState){
//        case NOT_CHARGING   : ChargerData.ChargerState = Start_CHARGING;  break;
//        case Start_CHARGING : ChargerData.ChargerState = Start_CHARGING;  break;
//        case CHARGING       : break;
//        case DONE_CHARGING  : break;
//        case CHARGER_FALUT  : break;
//        default             : break;
//    }
//
//    return ChargerData.ChargerState;
//}

bool ChangeNewChargerState(const bool start_charging){
    //TODO: check rising edge of gpio pin or cans

    if(ChargerData.ChargerState == CHARGER_FALUT){    }
    else if(!start_charging){
        ChargerData.ChargerState = NOT_CHARGING;
    }
    else{
        switch (ChargerData.ChargerState){
            case NOT_CHARGING   : ChargerData.ChargerState = Start_CHARGING;  break;
            case Start_CHARGING : ChargerData.ChargerState = Start_CHARGING;  break;
            case CHARGING       : break;
            case DONE_CHARGING  : break;
            case CHARGER_FALUT  : break;
            default             : break;
        }
    }

    return sendCmd2ChargerWraper();
}
//----------------------------------------------------------------------------------------------------

bool CalcNewCurrentSetting(const float avg_SOC, const float max_SOC){
    //TODO: get better equtaion
    uint16_t NewCurrent_16;

    if(avg_SOC < CELL_CHARGING_SOC_TARGET_TOLORENCES_PERCENTAGE){
        NewCurrent_16 =  MAX_CHARGER_CURRENT_AMPS;
    }
    else if(avg_SOC > MAX_CELL_CHARGING_PERCENTAGE - CELL_CHARGING_SOC_TARGET_TOLORENCES_PERCENTAGE){
        NewCurrent_16 =  OFF_CHARGER_CURRENT_AMPS;
    }
    else if(max_SOC > MAX_CELL_SOC_OVERSHOOT_PERENTAGE){
        NewCurrent_16 =  OFF_CHARGER_CURRENT_AMPS;
    }
    else{

        #define EQ 0

        #if EQ == 0
            const float NewCurrent_percentage = 1.0 - avg_SOC;
        #endif
        #if EQ == 1
            const float scale = 1
            const float NewCurrent_percentage = expf(-avg_SOC*scale);
        #endif
        #if EQ == 2
            const float scale = 1
            const float NewCurrent_percentage = logf(1-avg_SOC)/scale+1;
        #endif
        #if EQ == 3
            const float shift = 0.5;
            const float scale = 1
            const float NewCurrent_percentage = 1/(1+expf(-scale*(avg_SOC-shift)));
        #endif
        #if EQ == 4
            const float scale = 1
            const float NewCurrent_percentage1 = logf(1-avg_SOC)/scale+1;
            const float NewCurrent_percentage2 = 1-expf(scale*(avg_SOC-1));
            const float NewCurrent_percentage = fmaxf(NewCurrent_percentage1, NewCurrent_percentage2);
        #endif

        const float NewCurrent_f = NewCurrent_percentage * CHARGER_CURRENT_AMPS_RANGE + MIN_CHARGER_CURRENT_AMPS;

        NewCurrent_16 = (uint16_t)NewCurrent_f;
    }

    return SetChargerCurrent(NewCurrent_16);
//    return NewCurrent_16;
}
//----------------------------------------------------------------------------------------------------
ChargerData_t* GetChargerDataPrt(){
    return &ChargerData;
}
void initCharger(){
    ChargerData.ChargerState = NOT_CHARGING;
    sendCmd2ChargerWraper();
//    GetChargerStatusWraper();

}
//----------------------------------------------------------------------------------------------------
void CHARGER2BMS_DATA_FullRoutine(){
    GetChargerStatusWraper();
}
