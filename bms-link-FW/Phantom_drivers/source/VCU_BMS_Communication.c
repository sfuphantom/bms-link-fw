/*
 * BMS_VCU_Communication.c
 *
 *  Created on: Jul 9, 2026
 *      Author: tanjo
 */


#include "Phantom_Can.h"
#include "BatteryData.h"
#include "Fault_handler.h"
#include "VCU_BMS_Communication.h"

#warning This is unfinished
BMSState_t VCU2BMS_CalcNewState(const VCU2BMS_t * VCU2BMS){
    //TODO
    const bool hasFault = AnyFaults();
    if (hasFault){
        return BMS_FAULT;
    }

    const BMSState_t currentState = getBMS_State();
    const uint8_t VCU_FLAGS = (VCU2BMS->VCU_FLAGS);

    const bool clearFalutFlag  = (bool) (VCU_FLAGS & (1U<<CLAER_FAULTS));
    const bool clearChargeFlag = (bool) (VCU_FLAGS & (1U<<CHARGE));
    const bool clearStartFlag  = (bool) (VCU_FLAGS & (1U<<START));

    if(currentState == BMS_FAULT && !clearFalutFlag){
        return BMS_FAULT;
    }

    switch(currentState){
        case BMS_RUNNING:{

        }
        case BMS_CHARGING:{}
        case BMS_DONE_CHARGING:{}
        case BMS_DISCHARGING:{}
        case BMS_DONE_DISCHARGING:{}
        case BMS_SLEEPING:{}
        case BMS_IDLE:{}
        case BMS_FAULT:{}

    }
    //TODO finish this, add/remove states (I just randomly added them without thinking). add/remove flags
}

bool Send2VCU_direct(const uint16_t BatteryVolts, const uint16_t BatterySOC, const uint16_t BatteryCurrent){
    const BMS2VCU_t VCU2BMS = {BatteryVolts, BatterySOC, BatteryCurrent};

    return can_transmit_data(VCU2BMS_DATA, &VCU2BMS, sizeof(BMS2VCU_t));
}

void VCU2BMS_DATA_FullRoutine(){
    VCU2BMS_t VCU2BMS;
    can_receive_data(VCU2BMS_DATA, &VCU2BMS, sizeof(VCU2BMS_t));

    const BMSState_t newState = VCU2BMS_CalcNewState(&VCU2BMS);;
    setBMS_State(newState);

}

