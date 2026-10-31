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
#include "IMD.h"
#include <time.h>


void waitBatterySettle_helper(){
    KeepAlive();
    Charger_SendAllCommands();
    delay_ms_us(500,0);

}
 //----------------------------------------------------------------------------------------------------
 bool MeasureCellVoltage_Routine(){
//     wakeup_sleep_and_SetConfig();

     float * CellVolt = Cell_GetVoltWritePtr();

     bool Vaild = MeasureCellVoltage(CellVolt, NUMBER_OF_CELLS_SERIES);
//     SetCellVolt(CellVolt);

//     const uint16_t * CellVolt_pre1 = GetCellVoltReadPrt(1);
     if(!Vaild) {return false;}


     //    if(BMS_GetState() == BMS_CHARGING){
     //        SubRoutine_ErrorHandler_Decorator(SetBalanceRate_SubRoutine, BAD_SLAVE_CONNECTION_FLAG);
     //    }
     //    const uint16_t * allVolts = GetCellVoltReadPrt(0);


          // TODO: check faults
     return Vaild;
 }

 bool SetBalanceRate_Routine(){
//     wakeup_sleep_and_SetConfig();

     float* BalancePWMs = Cell_GetBalancePWMsWritePtr();
     const float * allCellVolts = Cell_GetVoltReadPtr(0);
     const float minCellVolt = Cell_GetMinVolt();

     if(BMS_GetState() == BMS_CHARGING){
         uint16_t BalanceDCC[NUMBER_OF_SLAVE_BOARDS_TOTAL];
         float SlaveBoardTemp[NUMBER_OF_SLAVE_BOARDS_TOTAL];

         getStatus_BoardTempC(SlaveBoardTemp);
         Balance_Update(allCellVolts, minCellVolt, SlaveBoardTemp, BalancePWMs, BalanceDCC);

         SetConfig_DCC(BalanceDCC);
     }
     else{
         SetAllConfig_DCC(0);
     }

     Write_CFGR();
     return true;
 }
//----------------------------------------------------------------------------------------------------
bool SendChargerControls_Routine(){

    //BMS_GetState() != BMS_CHARGING
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

    return true;
}

 bool MonitorCellTemp_Routine(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
#warning there is nothing here :(
 }

 bool CheckSlaveFlags_Routine(){
 //     const uint32_t CS_pollWaitings = waitSPIFree(1000);
//     wakeup_sleep_and_SetConfig();

#if !USE_ANILOG_GPIO
     MeasureRef2ndVoltage();
#endif
     uint32_t Flags;
     ReadStatAndGetFlags(&Flags);

      Fault_RaiseSlaveMask(Flags);
      return true;
  }

// bool MeasureBatteryVoltageRoutine(){
//     SubRoutine_ErrorHandler_Decorator(MeasureAndSetVoltage, BAD_HV_VOLT_FLAG);
// }

bool keepSlavesAwakeRoutine(){
    KeepAlive();
}

//bool SendDataRoutine_Serial(){
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

bool SendData2VCU(){

//    SubRoutine_ErrorHandler_Decorator(Send2VCU_direct(BatteryVolts, BatterySOC, BatteryCurrent);
}

bool IMD_MonitorRoutine(){
    const uint32 high = get_IMD_interupt_data_high();
    const uint32 period = get_IMD_interupt_data_high();

    if (peroidFreq_inValid) {return false;}

    const float freq = get_IMD_freq();
    const float duty = get_IMD_duty();

    IMDData_t IMDData;
    IMDData.duty       = duty;
    IMDData.freq       = freq;
    IMDData.resistance = IMD_CalcResistance(duty);
    IMDData.isolation  = IMD_CalcIsolation(duty);
    IMDData.state      = IMD_CalcState(duty, freq);

//    const IMDStateEnum state = IMD_CalcState(duty, freq);

    IMD_SetData(IMDData);


    set_IMD_fault_state(IMDData.isolation, IMDData.state);
    if (IMD_HasFault(IMDData.duty, IMDData.freq, IMDData.resistance, IMDData.isolation, IMDData.state)){
        Fault_RaiseIMD();
    }

    return true;
}
