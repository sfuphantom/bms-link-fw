/*
 * SendDataSerial .c
 *
 *  Created on: Jul 31, 2026
 *      Author: tanjo
 */

#include "BatteryData.h"
#include "Fault_handler.h"
#include "spi_helpers.h"
//#include "sci_helpers.h"
#include "strings.h"
#include "SendDataSerial.h"
#include "PhantomTimers.h"
#include "SlaveCommunation_Functions.h"


void StartMsg(const msg_ID_t msg_ID){
#if USE_UART_N_SPI
    sciSendByte(UART_REG, SOH_BYTE);
    sciSendByte(UART_REG, (uint8_t)msg_ID);
#endif
}
void endMsg(){
#if USE_UART_N_SPI
    sciSendByte(UART_REG, EOT_BYTE);
#endif
}
msg_ID_t RecieveMsgID(const uint8_t firstData);
bool RecieveMsgEnd(const uint8_t lastData);
uint16_t msg_ID2data_len(const msg_ID_t msg_ID);

void SendDataSerial(const void * data, const uint16_t data_len){
    uint8_t data8[MAX_BUF_SIZE];

    const uint32_t len = data_len < MAX_BUF_SIZE?data_len:MAX_BUF_SIZE;

    memcpy(data8, data, len);
    sciSend(UART_REG, len, data8);
}
//void RecieveDataSerial(uint8_t * data, const uint16_t data_len);
//void SendRecieveDataSerial(const uint8_t * data_Tx, uint8_t * data_Rx, const uint16_t data_len);


//------------------------------------------------------------------------------------
void SendMsgSerial_DL(const msg_ID_t msg_ID, const void * data, const uint16_t data_len){
    StartMsg(msg_ID);
    SendDataSerial(data, data_len);
    endMsg();
}

//void PrecessRecieveMsgSerial(msg_ID_t* const msg_ID, uint8_t * data){
//    *msg_ID = RecieveMsgID(data[0]);
//    const uint16_t data_len = msg_ID2data_len(*msg_ID);
//
//    RecieveDataSerial(data, data_len);
////    bool RecieveMsgEnd(const uint8_t lastData);
//}

//---------------------------------------------------------------------------------------------------------
void SendCellVoltage_Serial(){
   const uint16_t * CellVolt_prt =  GetCellVoltReadPrt();
   SendMsgSerial_DL(SEND_CELL_VOLT, CellVolt_prt, CELL_VOLTAGE_MSG_LEN);
}
void SendCellTemp_Serial(){

}
void SendFanData_Serial(){

}
void SendIMDData_Serial(){
    const ecapIMDData_t EcapIMDData = GetEcapIMDData();
    SendMsgSerial_DL(SEND_IMD_DATA, &EcapIMDData, IMD_ECAP_DATA_MSG_LEN);
}
void SendChargerData_Serial(){

}
void SendCellRes_Serial(){

}
void SendCellDCC_Serial(){
   int i,k;
   const uint16_t* DCC_Val = GetCellDCCReadPrt();
   uint8_t DCC_compressed[3];

   StartMsg(SEND_DCC_DATA);

   for(i=0,k=0; i<(NUMBER_OF_SLAVE_BOARDS+1)/2; i+=2,k=0){
       DCC_compressed[k++] = (uint8_t)DCC_Val[i];

       #if NUMBER_OF_SLAVE_BOARDS%2 == 0
       DCC_compressed[k++]   = (  ((uint8_t)(DCC_Val[i+1]  <<4) & 0xF0)
                               |   (uint8_t)(DCC_Val[i]    >>8));
       DCC_compressed[k++]  = (uint8_t)(DCC_Val[i+1]>>4);


       #else
       if(NUMBER_OF_SLAVE_BOARDS > i+1){
           DCC_compressed[k++]   = (  ((uint8_t)(DCC_Val[i+1]  <<4) & 0xF0)
                                   |   (uint8_t)(DCC_Val[i]    >>8));
           DCC_compressed[k++]  = (uint8_t)(DCC_Val[i+1]>>4);
       }
       else{
           DCC_compressed[k++]   = (uint8_t)(DCC_Val[i]>>8);
       }
       #endif

       SendDataSerial(DCC_compressed, sizeof(DCC_compressed));
   }

   endMsg();

   //[0x0123,0x0456,0x0789, 0x0ABC] >> [0x12,0x34,0x56,0x78,0x9A,0xCB]
//   for(i=0,k=0; i<NUMBER_OF_SLAVE_BOARDS; i+=2){
//
//       DCC_compressed[k++] = (uint8_t)DCC_Val[i];
//
//       #if NUMBER_OF_SLAVE_BOARDS%2 == 0
//       DCC_compressed[k++]   = (  ((uint8_t)(DCC_Val[i+1]  <<4) & 0xF0)
//                               |   (uint8_t)(DCC_Val[i]    >>8));
//       DCC_compressed[k++]  = (uint8_t)(DCC_Val[i+1]>>4);
//
//       #else
//        DCC_compressed[k++]   = (uint8_t)(DCC_Val[i]>>8);
//       #endif
//
//   }
//   SendMsgSerial_DL(SEND_DCC_DATA, DCC_compressed, CELL_DCC_MSG_LEN);
}
void SendSlaveState_Serial(){
    StartMsg(SEND_SLAVE_STATE);

    const StatusReg* current_Slave = GetStatusRegData();

    int i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS; i++){

        const uint16_t RefVolt2nd = current_Slave->RefVolt2nd;
        const uint16_t ITMP = current_Slave->ITMP;
        const uint16_t SC = current_Slave->SC;

        SendDataSerial(&RefVolt2nd, sizeof(uint16_t));
        SendDataSerial(&ITMP, sizeof(uint16_t));
        SendDataSerial(&SC, sizeof(uint16_t));

        current_Slave++;
    }

    endMsg();
}

void Send_BMSFaultsData_Serial(){
    BMSFaultsData_t *BMSFaultsData = GetBMSFaultsData();
    StartMsg(SEND_FAULT_DATA);

    SendDataSerial(BMSFaultsData, sizeof(BMSFaultsData_t));

    //
    //


    endMsg();
}

//---------------------------------------------------------------------------------------------------------

static uint32_t PeriodicLastSentTick_arr[NUMBER_OF_PERIODIC_MSG] = {0};

void SendPeriodic(const msg_ID_t msg_ID){
    const uint8_t periodicMsgID = (uint8_t)msg_ID - PERIODIC_MSG_ID_START_IDX;
    const uint32_t now = getNow_tick();

    uint32_t Period;
    switch(msg_ID){
        case SEND_CELL_VOLT:       Period = CELL_VOLTAGE_MSG_PERIOD_TICK;      break;
        case SEND_CELL_TEMP:       Period = CELL_TEMP_MSG_PERIOD_TICK;         break;
        case SEND_DCC_DATA:        Period = CELL_DCC_MSG_PERIOD_TICK;          break;
        case SEND_IMD_DATA:        Period = IMD_ECAP_DATA_MSG_PERIOD_TICK;     break;
        case SEND_CHARGER_DATA:    Period = CHARGER_DATA_MSG_PERIOD_TICK;      break;
        case SEND_CELL_RES:        Period = CELL_RESISTANCE_MSG_PERIOD_TICK;   break;
        case SEND_SLAVE_STATE:     Period = SLAVE_STATUS_DATA_PERIOD_TICK;     break;
        case SEND_FAN_DATA:        Period = FAN_DATA_MSG_PERIOD_TICK;          break;
        default: return;
    }

    if(now < Period + PeriodicLastSentTick_arr[periodicMsgID])
        return;

    PeriodicLastSentTick_arr[periodicMsgID] = now;

    switch(msg_ID){
        case SEND_CELL_VOLT:            SendCellVoltage_Serial() ;      return;
        case SEND_CELL_TEMP:            SendCellTemp_Serial();          return;
        case SEND_DCC_DATA:             SendCellDCC_Serial() ;          return;
        case SEND_IMD_DATA:             SendIMDData_Serial() ;          return;
        case SEND_CHARGER_DATA:         SendChargerData_Serial();       return;
        case SEND_CELL_RES:             SendCellRes_Serial();           return;
        case SEND_SLAVE_STATE:          SendSlaveState_Serial();        return;
        case SEND_FAN_DATA:             SendFanData_Serial() ;          return;
        default: return;
    }


}


