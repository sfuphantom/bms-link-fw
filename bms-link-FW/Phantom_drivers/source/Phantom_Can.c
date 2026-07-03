/*
 * Phantom_Can.c
 *
 *  Created on: Jul 1, 2026
 *      Author: tanjo
 */


#include "can.h"
#include "reg_can.h"
#include "Phantom_Can.h"
#include <string.h>
#include "PhantomHelpers.h"


//----------------------------------------------------------------------------------------
static uint32_t can_transmit_data(const phantomCanMsgBox msgbox, const uint8_t data[], const uint8_t len){
    uint8_t buf[DEFULT_CAN_MSG_SIZE_BYTE];
//    const uint8_t arr_len = sizeof_arr(data);
    memset(buf, 0, sizeof(buf));
    memcpy(buf, data, len*sizeof(uint8_t));

    return canTransmit(CAN_NODE_REG, msgbox, buf);
}
static uint32_t can_receive_data(const phantomCanMsgBox msgbox, uint8_t data[], const uint8_t len){
    uint8_t buf[DEFULT_CAN_MSG_SIZE_BYTE];
    uint32_t return_val = canGetData(CAN_NODE_REG, msgbox, buf);

    memcpy(data, buf, len*sizeof(uint8_t));
    return return_val;
}
bool wasCanVaild(uint32 returnVal){
    if(returnVal == 0) return true;

    else return false;
}
//----------------------------------------------------------------------------------------
uint32_t transmit_BMS2VCU_FAULT(const uint8_t data[]){
    return can_transmit_data(BMS2VCU_FAULT, data, BMS2VCU_BATTERY_VOLT_MSG_LEN_BYTES);
}
uint32_t receive_BMS2VCU_FAULT(uint8_t data[]){
    return can_receive_data(BMS2VCU_FAULT, data, BMS2VCU_BATTERY_VOLT_MSG_LEN_BYTES);
}
//---------------------------------------------------------
uint32_t transmit_BMS2VCU_BatteryVoltage(const uint16_t BatteryVoltage){
    uint8_t data[BMS2VCU_BATTERY_VOLT_MSG_LEN_BYTES];
    data[0] = BatteryVoltage>>8U;
    data[1] = BatteryVoltage>>0U;

    return can_transmit_data(BMS2VCU_BATTERY_VOLT, data, BMS2VCU_BATTERY_VOLT_MSG_LEN_BYTES);
}
uint32_t receive_BMS2VCU_BatteryVoltage(uint16_t* const BatteryVoltage){
    uint8_t data[BMS2VCU_BATTERY_VOLT_MSG_LEN_BYTES];
    uint32_t return_val = can_receive_data(BMS2VCU_BATTERY_VOLT, data, BMS2VCU_BATTERY_VOLT_MSG_LEN_BYTES);

    *BatteryVoltage = (uint16_t)(data[0]<<8U) | (data[1]<<0U);
    return return_val;

}
//---------------------------------------------------------
uint32_t transmit_BMS2CHARGER_DATA(const uint8_t data[]){
    return can_transmit_data(BMS2CHARGER_DATA, data, BMS2CHARGER_DATA_MSG_LEN_BYTES);
}
//uint32_t receive_BMS2CHARGER_DATA(uint8_t data[]){
//    uint32_t return_val = can_receive_data(BMS2VCU_BATTERY_VOLT, data, CHARGER2BMS_DATA_MSG_LEN_BYTES);
//
//    return return_val;
//}
//---------------------------------------------------------
//uint32_t transmit_CHARGER2BMS_DATA(const uint8_t data[]){
//    return can_transmit_data(BMS2CHARGER_DATA, data, BMS2CHARGER_DATA_MSG_LEN_BYTES);
//}
uint32_t receive_CHARGER2BMS_DATA(uint8_t data[]){
    uint32_t return_val = can_receive_data(BMS2VCU_BATTERY_VOLT, data, CHARGER2BMS_DATA_MSG_LEN_BYTES);

    return return_val;
}
//---------------------------------------------------------

#pragma WEAK(BMS2VCU_FAULT_FullRoutine)
void BMS2VCU_FAULT_FullRoutine()
{
    //Do Nothing, PlaceHolder
}
#pragma WEAK(BMS2VCU_BatteryVoltage_FullRoutine)
void BMS2VCU_BatteryVoltage_FullRoutine()
{
    //Do Nothing, PlaceHolder
}
#pragma WEAK(VCU2BMS_DATA_FullRoutine)
void VCU2BMS_DATA_FullRoutine()
{
    //Do Nothing, PlaceHolder
}
#pragma WEAK(BMS2CHARGER_DATA_FullRoutine)
void BMS2CHARGER_DATA_FullRoutine()
{
    //Do Nothing, PlaceHolder
}
#pragma WEAK(CHARGER2BMS_DATA_FullRoutine)
void CHARGER2BMS_DATA_FullRoutine()
{
    //Do Nothing, PlaceHolder
}
void canMessageNotification(canBASE_t *node, uint32 messageBox){
    const phantomCanMsgBox msgBox = (phantomCanMsgBox)messageBox;

    if(node == canREG1){
        switch(msgBox){
            case    BMS2VCU_FAULT        : BMS2VCU_FAULT_FullRoutine()          ; break;
            case    BMS2VCU_BATTERY_VOLT : BMS2VCU_BatteryVoltage_FullRoutine() ; break;
            case    VCU2BMS_DATA         : VCU2BMS_DATA_FullRoutine()           ; break;

            case    BMS2CHARGER_DATA     : BMS2CHARGER_DATA_FullRoutine()       ; break;
            case    CHARGER2BMS_DATA     : CHARGER2BMS_DATA_FullRoutine()       ; break;

            default : break;

        }
    }
}
