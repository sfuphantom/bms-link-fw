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
bool Send2VCU_direct(const uint16_t BatteryVolts, const uint16_t BatterySOC, const uint16_t BatteryCurrent){
    const BMS2VCU_t VCU2BMS = {BatteryVolts, BatterySOC, BatteryCurrent};

    return can_transmit_data(VCU2BMS_DATA, &VCU2BMS, sizeof(BMS2VCU_t));
}

void VCU2BMS_DATA_FullRoutine(){
    VCU2BMS_t VCU2BMS;
    can_receive_data(VCU2BMS_DATA, &VCU2BMS, sizeof(VCU2BMS_t));
}

