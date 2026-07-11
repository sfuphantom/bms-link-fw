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
bool wasCanVaild(uint32 returnVal){
    if(returnVal == 0) return true;

    else return false;
}
uint32_t can_transmit_data(const phantomCanMsgBox msgbox, const void * const data, const uint8_t len){
    uint8_t buf[DEFULT_CAN_MSG_SIZE_BYTE];
//    const uint8_t arr_len = sizeof_arr(data);
    memset(buf, 0, sizeof(buf));
    memcpy(buf, data, len*sizeof(uint8_t));

    return canTransmit(CAN_NODE_REG, msgbox, buf);
}
uint32_t can_receive_data(const phantomCanMsgBox msgbox, void * const data, const uint8_t len){
    uint8_t buf[DEFULT_CAN_MSG_SIZE_BYTE];
    uint32_t return_val = canGetData(CAN_NODE_REG, msgbox, buf);

    memcpy(data, buf, len*sizeof(uint8_t));
    return return_val;
}

//----------------------------------------------------------------------------------------

#pragma WEAK(BMS2ALL_FAULT_FullRoutine)
void BMS2ALL_FAULT_FullRoutine()
{
    //Do Nothing, PlaceHolder
}
#pragma WEAK(BMS2VCU_Data_FullRoutine)
void BMS2VCU_Data_FullRoutine()
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
//----------------------------------------------------------------------------------------
void canMessageNotification(canBASE_t *node, uint32 messageBox){
    const phantomCanMsgBox msgBox = (phantomCanMsgBox)messageBox;

    if(node == canREG1){
        switch(msgBox){
            case    BMS2ALL_FAULT        : BMS2ALL_FAULT_FullRoutine()          ; break;

            case    BMS2VCU_DATA         : BMS2VCU_Data_FullRoutine()           ; break;
            case    VCU2BMS_DATA         : VCU2BMS_DATA_FullRoutine()           ; break;

            case    BMS2CHARGER_DATA     : BMS2CHARGER_DATA_FullRoutine()       ; break;
            case    CHARGER2BMS_DATA     : CHARGER2BMS_DATA_FullRoutine()       ; break;

            default : break;

        }
    }
}
