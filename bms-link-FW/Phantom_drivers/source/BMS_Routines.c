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

     SetBMSFault_bool_HIGH(ErrorFault);
 }
 //----------------------------------------------------------------------------------------------------
 bool MeasureCellVoltage_SubRoutine(){
     uint16_t CellVolt[NUMBER_OF_CELLS_SLAVES];

     bool Vaild = MeasureCellVoltage(CellVolt);
     SetCellVolt(CellVolt);

//     const uint16_t * CellVolt_pre1 = GetCellVoltReadPrt(1);

     return Vaild;
 }

 bool SetBalancePWM(){
     uint8_t * BalanceNibbles = GetBalancePWM_NibblesWritePrt();
     const uint16_t * allVolts = GetCellVoltReadPrt(0);

     GetDrainPWMNibbles(allVolts, BalanceNibbles);

     uint8_t SlavePWMBalanceNibbles[NUMBER_OF_CELLS_SLAVES/2] = {0};
     memset(SlavePWMBalanceNibbles, 0, sizeof(SlavePWMBalanceNibbles));
     memcpy(SlavePWMBalanceNibbles, BalanceNibbles, sizeof(uint8_t) * (NUMBER_OF_CELLS_SERIES+1)/2);

     wakeup_idle();


     bool EQ = WriteThenRead_PWM(SlavePWMBalanceNibbles);
     return EQ;
 }
//----------------------------------------------------------------------------------------------------
void SendChargerControlsRoutine(){
//    if(getBMS_State() != BMS_CHARGING)
//        return;

    battery2Charger_t battery2Charger;
    battery2Charger.actual_Ah = 0;
    battery2Charger.soc = 0;

    battery2Charger.cell_avg_V = GetAvgCellVolt_float();
    battery2Charger.cell_max_V = GetMaxCellVolt_float();
    battery2Charger.cell_min_V = GetMinCellVolt_float();

    battery2Charger.temp_max = GetMaxCellTemp_float();
    battery2Charger.temp_min = GetMinCellTemp_float();

    battery2Charger.charge_current_A = 0;
    battery2Charger.pack_voltage_V = 0;

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
     if(getBMS_State() == BMS_CHARGING){
         SubRoutine_ErrorHandler_Decorator(SetBalancePWM, BAD_SLAVE_CONNECTION_FLAG);
     }

     // TODO: check faults
 }


 void MonitorCellTempRoutine(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
 }

 void SlaveFlagsRoutine(){
 //     const uint32_t CS_pollWaitings = waitSPIFree(1000);
#if !USE_ANILOG_GPIO
     SubRoutine_ErrorHandler_Decorator(MeasureRef2ndVoltage, BAD_SLAVE_CONNECTION_FLAG);
#endif
     SubRoutine_ErrorHandler_Decorator(ReadStatAndGetFlags, BAD_SLAVE_CONNECTION_FLAG);

      const uint32_t Flags = checkStatFlags();
      AddSlaveFaults(Flags);
  }

 void MeasureBatteryVoltage(){
     SubRoutine_ErrorHandler_Decorator(MeasureAndSaveBatteryVoltage, BAD_HV_VOLT_FLAG);
 }

void keepSlavesAwakeRoutine(){
    SendDummyCMD();
}

void SendDataRoutine_Serial(){
    SendCellVoltage_Serial();
//    SendCellTemp_Serial();
//    SendFanData_Serial();
//    SendIMDData_Serial();
//    SendCellRes_Serial();
    SendCellPWM_Serial();
    SendSlaveState_Serial();
//    SendChargerData_Serial();

}

void SendData2VCU(){

//    SubRoutine_ErrorHandler_Decorator(Send2VCU_direct(BatteryVolts, BatterySOC, BatteryCurrent);
}

void MeasureCellResistanceRoutine(){

     uint8_t SlavePWMBalanceNibbles[NUMBER_OF_CELLS_SLAVES/2] = {0};
     uint16_t NoCurrentVolts[NUMBER_OF_CELLS_SLAVES];
     uint16_t CurrentVolts[NUMBER_OF_CELLS_SLAVES];

     const float OG_Charing_current_A = Charger_GetTargetCurrent();
     const float NoCharingCurrent_A = 0;
     const float ControledCharingCurrent_A = 1;

     bool Vaild = true;
     memset(SlavePWMBalanceNibbles, 0x00, sizeof(SlavePWMBalanceNibbles));
     wakeup_idle();
     Vaild &= WriteThenRead_PWM(SlavePWMBalanceNibbles);

     Charger_SetTargetCurrent(NoCharingCurrent_A);

     doUntilTimeLimit(waitBatterySettle_helper, MIN_SEC_MS_US_TICK_2_TICK(0,5,0,0,0));

     Vaild &= MeasureCellVoltage(NoCurrentVolts);
     //------------------------------------------------------------------------
     memset(SlavePWMBalanceNibbles, 0xFF, sizeof(SlavePWMBalanceNibbles));
     Vaild &= WriteThenRead_PWM(SlavePWMBalanceNibbles);

     Charger_SetTargetCurrent(ControledCharingCurrent_A);

     doUntilTimeLimit(waitBatterySettle_helper, MIN_SEC_MS_US_TICK_2_TICK(0,5,0,0,0));

     Vaild &= MeasureCellVoltage(CurrentVolts);

     const float Idiff = Charger_GetOutputCurrent() - NoCharingCurrent_A;
     //------------------------------------------------------------------------
     // reset slaves and charger
     Charger_SetTargetCurrent(OG_Charing_current_A);
     Vaild &= WriteThenRead_PWM(GetBalancePWM_NibblesWritePrt());
     //------------------------------------------------------------------------
     float res_f[NUMBER_OF_CELLS_SERIES];
     int i;
     for(i=0;i<NUMBER_OF_CELLS_SERIES;i++){
         const float Vdiff =  SLAVE_ADC2VOLTS(NoCurrentVolts[i] - CurrentVolts[i]);
         res_f[i] = Vdiff / Idiff;
     }

}
