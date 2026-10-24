#include "spi.h"
#include <stdint.h>
#include <stdbool.h>

//#include "ltc6811_commands.h"
#include "spi_helpers.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Functions.h"
#include "SlaveCommunation_Hardware.h"
#include "BMS_Routines.h"

#include "Phantom_Can.h"

#include "PhantomHelpers.h"

#include "BatteryData.h"
#include "Charger.h"
#include "Fans.h"
#include "GetBatteryVoltage.h"
#include "Fault_handler.h"
#include "SendDataSerial.h"
#include "Balance.h"
#include <time.h>


void waitBatterySettle_helper(){
    SendDummyCMD();
    Charger_SendAllCommands();
    delay_ms_us(500,0);
}
 //---------------------------------------------------------------------------------------------------------
 void SubRoutine_ErrorHandler_Decorator(bool (*SubRoutine_prt)(void), const BMS_Faults ErrorFault){
     int repeat_idx;
     bool Valid;

     for(repeat_idx=0; repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){
#warning add logic if there is already a fault
//         if(anyFaluts()){
//            return;
//         }

         Valid = SubRoutine_prt();

         if(Valid){return;}
     }

     Fault_RaiseBMS(ErrorFault);
 }
 //----------------------------------------------------------------------------------------------------
 bool MeasureCellVoltage_SubRoutine(){
     wakeup_sleep_and_SetConfig();

     float * CellVolt = Cell_GetVoltWritePtr();

     bool Vaild = MeasureCellVoltage(CellVolt, NUMBER_OF_CELLS_SERIES);
//     SetCellVolt(CellVolt);

//     const uint16_t * CellVolt_pre1 = GetCellVoltReadPrt(1);

     return Vaild;
 }

 bool SetBalanceRate_SubRoutine(){
     wakeup_sleep_and_SetConfig();

     uint8_t * BalanceNibbles = Cell_GetBalanceNibblesWritePtr();
     uint16_t* BalanceDCC = Cell_GetBalanceDCCWritePtr();
     const float * allCellVolts = Cell_GetVoltReadPtr(0);
     const float minCellVolt = Cell_GetMinVolt();
     float SlaveBoardTemp[NUMBER_OF_SLAVE_BOARDS_TOTAL];
     getStatus_BoardTempC(SlaveBoardTemp);

     Balance_GetPWM(allCellVolts, minCellVolt, SlaveBoardTemp, BalanceNibbles);
     Balance_GetDCC(BalanceNibbles, BalanceDCC);


     uint8_t SlavePWMBalanceNibbles[NUMBER_OF_CELLS_SLAVES/2] = {0};
     memset(SlavePWMBalanceNibbles, 0, sizeof(SlavePWMBalanceNibbles));
     memcpy(SlavePWMBalanceNibbles, BalanceNibbles, sizeof(uint8_t) * NUMBER_OF_CELLS_SERIES_PAIRS);

     wakeup_idle();

     const bool EQ = WriteThenRead_PWM(SlavePWMBalanceNibbles);
     SetConfig_DCC(BalanceDCC);
     Write_CFGR();
     return EQ;
 }
//----------------------------------------------------------------------------------------------------
void SendChargerControlsRoutine(){
//    if(BMS_GetState() != BMS_CHARGING)
//        return;

    battery2Charger_t battery2Charger;
#warning need actual_Ah function and calc
    battery2Charger.actual_Ah = 0;
    battery2Charger.soc = Pack_GetSOC();

    battery2Charger.cell_avg_V = Cell_GetAvgVolt();
    battery2Charger.cell_max_V = Cell_GetMaxVolt();
    battery2Charger.cell_min_V = Cell_GetMinVolt();

    battery2Charger.temp_max = Thermistor_GetMaxTemp();
    battery2Charger.temp_min = Thermistor_GetMinTemp();

    battery2Charger.charge_current_A = Pack_GetCurrent();
    battery2Charger.pack_voltage_V = Pack_GetVolt();

#warning do not be 0, add values
    uint8_t chargerStateFlag = 0;

    Charger_UpdateBatteryData(&battery2Charger);
    Charger_UpdateStateMachine(chargerStateFlag);
    Charger_UpdateOutputsFromState();
    Charger_SendAllCommands();
//    setChargerState(minCellV, AvgCellV, MaxCellV, BatteryVolt);

//    SubRoutine_ErrorHandler_Decorator(SetChargerSettings, CHARGER_COMMS_FAULT);
}

 void CellVoltageControlRoutine(){
    SubRoutine_ErrorHandler_Decorator(MeasureCellVoltage_SubRoutine, BAD_SLAVE_CONNECTION_FLAG);

//    const uint16_t * allVolts = GetCellVoltReadPrt(0);


     // TODO: check faults
 }

 void SetCellBalancersRoutine(){
     if(BMS_GetState() == BMS_CHARGING){
         SubRoutine_ErrorHandler_Decorator(SetBalanceRate_SubRoutine, BAD_SLAVE_CONNECTION_FLAG);
     }
 }


 void MonitorCellTempRoutine(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
#warning there is nothing here :(
 }

 void SlaveFlagsRoutine(){
 //     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     wakeup_sleep_and_SetConfig();

#if !USE_ANILOG_GPIO
     SubRoutine_ErrorHandler_Decorator(MeasureRef2ndVoltage, BAD_SLAVE_CONNECTION_FLAG);
#endif
     SubRoutine_ErrorHandler_Decorator(ReadStatAndGetFlags, BAD_SLAVE_CONNECTION_FLAG);

      const uint32_t Flags = checkStatFlags();
      Fault_RaiseSlave(Flags);
  }

// void MeasureBatteryVoltageRoutine(){
//     SubRoutine_ErrorHandler_Decorator(MeasureAndSetVoltage, BAD_HV_VOLT_FLAG);
// }

//void keepSlavesAwakeRoutine(){
//    SendDummyCMD();
//}

//void SendDataRoutine_Serial(){
//    SendCellVoltage_Serial();
////    SendCellTemp_Serial();
////    SendFanData_Serial();
////    SendIMDData_Serial();
////    SendCellRes_Serial();
//    SendCellPWM_Serial();
//    SendSlaveState_Serial();
////    SendChargerData_Serial();
//
//}

void SendData2VCU(){

//    SubRoutine_ErrorHandler_Decorator(Send2VCU_direct(BatteryVolts, BatterySOC, BatteryCurrent);
}

void MeasureCellResistanceRoutine(){

     uint8_t SlavePWMBalanceNibbles[NUMBER_OF_CELLS_SLAVES/2] = {0};
     float NoCurrentVolts[NUMBER_OF_CELLS_SLAVES];
     float CurrentVolts[NUMBER_OF_CELLS_SLAVES];

     const float OG_Charing_current_A = Charger_GetTargetCurrent();
     const float NoCharingCurrent_A = 0;
     const float ControledCharingCurrent_A = 1;

     bool Vaild = true;
     memset(SlavePWMBalanceNibbles, 0x00, sizeof(SlavePWMBalanceNibbles));
     wakeup_idle();
     Vaild &= WriteThenRead_PWM(SlavePWMBalanceNibbles);

     Charger_SetTargetCurrent(NoCharingCurrent_A);

     doUntilTimeLimit(waitBatterySettle_helper, ticks_make(0,5,0,0,0));

     Vaild &= MeasureCellVoltage(NoCurrentVolts, NUMBER_OF_CELLS_SERIES);
     //------------------------------------------------------------------------
     memset(SlavePWMBalanceNibbles, 0xFF, sizeof(SlavePWMBalanceNibbles));
     Vaild &= WriteThenRead_PWM(SlavePWMBalanceNibbles);

     Charger_SetTargetCurrent(ControledCharingCurrent_A);

     doUntilTimeLimit(waitBatterySettle_helper, ticks_make(0,5,0,0,0));

     Vaild &= MeasureCellVoltage(CurrentVolts, NUMBER_OF_CELLS_SERIES);

     const float Idiff = Charger_GetOutputCurrent() - NoCharingCurrent_A;
     //------------------------------------------------------------------------
     // reset slaves and charger
     Charger_SetTargetCurrent(OG_Charing_current_A);
     Vaild &= WriteThenRead_PWM(Cell_GetBalanceNibblesReadPtr());
     //------------------------------------------------------------------------
     float * res_f = Cell_GetResWritePtr();
     int i;
     for(i=0;i<NUMBER_OF_CELLS_SERIES;i++){
         const float Vdiff =  NoCurrentVolts[i] - CurrentVolts[i];
         res_f[i] = Vdiff / Idiff;
     }


}
