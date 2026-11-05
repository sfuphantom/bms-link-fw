/*
 * BMS_VCU_Communication.c
 *
 *  Created on: Jul 9, 2026
 *      Author: tanjo
 */


#include "Can_Helpers.h"
#include "BMS_Data.h"
#include "Fault_handler.h"
#include "VCU_BMS_Communication.h"

#warning This is unfinished
bool Send2VCU_direct(const uint16_t BatteryVolts, const uint16_t BatterySOC, const uint16_t BatteryCurrent){
    const BMS2VCU_buf_t VCU2BMS = {BatteryVolts, BatterySOC, BatteryCurrent};

    return can_send(VCU2BMS_DATA, VCU2BMS.buf, sizeof(BMS2VCU_buf_t));
}

void VCU2BMS_DATA_FullRoutine(){
    VCU2BMS_buf_t VCU2BMS;
    can_receive(VCU2BMS_DATA, VCU2BMS.buf, sizeof(VCU2BMS_buf_t));
}

