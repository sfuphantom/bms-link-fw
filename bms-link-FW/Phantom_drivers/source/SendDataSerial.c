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


typedef struct {
    uint16_t RefVolt2nd;
    uint16_t ITMP;
    uint16_t SC;
}SlaveStateSerialData_t;

typedef struct {
    uint8_t   State;
    uint16_t  OutVolt;
    int16_t   OutAmps;
    uint8_t   Status;
}ChargerSerialData_t;
//------------------------------------------------------------------------------------

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

uint16_t CalcCRC16_DataSerial(const void * data, const uint16_t data_len){

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
   const uint16_t * CellVolt_prt = GetCellVoltReadPrt(0);
   SendMsgSerial_DL(SEND_CELL_VOLT, CellVolt_prt, CELL_VOLTAGE_MSG_LEN);
}
void SendCellTemp_Serial(){
    const uint16_t * CellTemp_prt =  GetCellTempReadPrt(0);
    SendMsgSerial_DL(SEND_CELL_TEMP, CellTemp_prt, CELL_TEMP_MSG_LEN);
}
void SendFanData_Serial(){
    const uint8_t * Fan_prt = GetFansDuty_ReadPrt();
    SendMsgSerial_DL(SEND_FAN_DATA, Fan_prt, CELL_TEMP_MSG_LEN);
}
void SendIMDData_Serial(){
    const ecapIMDData_t EcapIMDData = GetEcapIMDData();
    SendMsgSerial_DL(SEND_IMD_DATA, &EcapIMDData, IMD_ECAP_DATA_MSG_LEN);
}
void SendCellRes_Serial(){
    const uint16_t * CellRes_prt =  GetCellResReadPrt(0);
    SendMsgSerial_DL(SEND_CELL_RES, CellRes_prt, CELL_RESISTANCE_MSG_LEN);
}
void SendCellPWM_Serial(){
    const uint8_t * BalancePWM_Nibbles_prt =  GetBalancePWM_NibblesReadPrt();
    SendMsgSerial_DL(SEND_PWM_DATA, BalancePWM_Nibbles_prt, CELL_PWM_MSG_LEN);
}
void SendSlaveState_Serial(){
    const StatusReg_t* current_Slave = GetStatusRegData();
    int i;

    SlaveStateSerialData_t SlaveStateSerialData[NUMBER_OF_SLAVE_BOARDS_TOTAL];

    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++, current_Slave++){

        SlaveStateSerialData[i].RefVolt2nd = current_Slave->RefVolt2nd;
        SlaveStateSerialData[i].ITMP = current_Slave->ITMP;
        SlaveStateSerialData[i].SC = current_Slave->SC;
    }
    SendMsgSerial_DL(SEND_SLAVE_STATE, SlaveStateSerialData, SLAVE_STATUE_MSG_LEN);
}


void SendChargerData_Serial(){
//    const uint8_t   State   = (uint8_t)Charger_GetState();
//    const uint16_t  OutVolt = Charger_GetOutputVoltage16();
//    const int16_t   OutAmps = Charger_GetOutputCurrent16();
//    const uint8_t   Status  = Charger_GetStatusFlags();
//


    ChargerSerialData_t ChargerSerialData;
    ChargerSerialData.State   = (uint8_t)Charger_GetState();
    ChargerSerialData.OutVolt = Charger_GetOutputVoltage16();
    ChargerSerialData.OutAmps = Charger_GetOutputCurrent16();
    ChargerSerialData.Status  = Charger_GetStatusFlags();

    SendMsgSerial_DL(SEND_CHARGER_DATA, &ChargerSerialData, sizeof(ChargerSerialData));

//    StartMsg(SEND_CHARGER_DATA);
//
//    SendDataSerial(&State,      sizeof(uint8_t));
//    SendDataSerial(&OutVolt,    sizeof(uint16_t));
//    SendDataSerial(&OutAmps,    sizeof(int16_t));
//    SendDataSerial(&Status,     sizeof(uint8_t));
//
//    endMsg();
}

void Send_BMSFaultsData_Serial(){
    BMSFaultsData_t *BMSFaultsData = GetBMSFaultsData();
    StartMsg(SEND_FAULT_DATA);

    SendDataSerial(BMSFaultsData, sizeof(BMSFaultsData_t));

    //
    //


    endMsg();
}


#warning this is not done, race conditions, make better
