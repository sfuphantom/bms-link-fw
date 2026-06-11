#include "spi.h"
#include <stdint.h>
#include <stdbool.h>

#include "ltc6811_commands.h"
#include "SlaveCommunation_Functions.h"
#include "SlaveCommunication_TaskAndRoutines.h"
#include "PhantomHelpers.h"

#include "BatteryData.h"

//struct BatteryData_struct SlaveData;
//---------------------------------------------------------------------------------------------------------
 void ClearSlaveRegs(){
     ClearCellsCMD();
     ClearAUXCMD();
     ClearStatCMD();
 }

 void initLink(){
     init_PEC15_Table();
     //------------------------------------------------
     wakeup_sleep();

     ClearSlaveRegs();
     initConfig();
 }

 //---------------------------------------------------------------------------------------------------------
 bool MeasureGPIOVoltageRoutine(uint16_t* GPIO_DataOut, uint16_t* Ref2nd_DataOut){
     while(!isADCFree()){}

     ClearAUXCMD();
     MeasureAUXCmd(ADC_MEASURE_MODE, 0);
     bool Done = waitConvComplete_ADC_GPIO();

     uint16_t AUXDataOut[NUMBER_OF_AUCILIARY];
     GetGPIOReadings_Analog(AUXDataOut);

     int i,j;
     int idx = 0;
     for(i=0;i<NUMBER_OF_SLAVE_BOARDS; i++){
         for(j=0;j<GPIOS_PER_SLAVE_BOARD; j++){
             *GPIO_DataOut = AUXDataOut[idx++];
             GPIO_DataOut++;
         }

         for(j=0;j<REF_2ND_PER_SLAVE_BOARD; j++){
             *Ref2nd_DataOut = AUXDataOut[idx++];
             Ref2nd_DataOut++;
         }
     }

     return Done;
 }

 bool MeasureCellVoltageRoutine(uint16_t* VoltDataOut){
     waitADCFree();
//     ClearCellsCMD();

     MeasureCellsCmd(ADC_MEASURE_MODE, ADC_Measure_Discharge_Permit, 0);
     bool Done = waitConvComplete_ADC_Cells();

//     GetVoltageReadings(VoltDataOut);
     wakeup_sleep();
     GetVoltageReadings(VoltDataOut);

     return Done;
 }
 bool BalanceCellsRoutine(const uint16_t* VoltInData){
    #define S_CRTL FALSE
    #if S_CRTL
         uint8_t S_CLTR_nibbles[NUMBER_OF_CELLS/2];

         bool Balance = GetBalanceNibbles(VoltInData, S_CLTR_nibbles);
         if(!Balance)
             return TRUE;


         Write_S_CTRL(S_CLTR_nibbles);

         Start_S_CTRL_Pulsing();

         bool Done = waitConvComplete_Cell_Bal();

         int i;
         for(i=0;i<NUMBER_OF_CELLS/2;i++){
             S_CLTR_nibbles[i]=0;
         }

         Write_S_CTRL(S_CLTR_nibbles);

//         Start_S_CTRL_Pulsing();
         return Done;
    #else
         uint16_t DCC_Val[NUMBER_OF_SLAVE_BOARDS]={0};

         bool Balance = GetBalanceDCC(VoltInData, DCC_Val);

         if(!Balance)
             return TRUE;

         SetConfig_DCC(DCC_Val);
         Write_CFGR();
         delay_ms_us(25,0);

         int i;
         for(i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
             DCC_Val[i]=0;
         }
         SetConfig_DCC(DCC_Val);
         Write_CFGR();
         return TRUE;
    #endif

 }

 uint32_t ReadStatAndGetFlagsRoutine(){

     ClearStatCMD();
     CheckSTATCmd(ADC_MEASURE_MODE,  0b01);
     CheckSTATCmd(ADC_MEASURE_MODE,  0b10);
     waitConvComplete_ADC_STAT();

     Read_STAT();
     Read_CFGR();

     uint32_t Flags = checkStatFlags();
     Flags<<=1;
     Flags |= ReadConfig_gpio_allZero();
    return Flags;
 }

//----------------------------------------------------------------------------------------------------
 void CellVoltageControlTask(){
     if(!rtiTimerExpired(0, 1, 0)){
         return;
     }

     int repeat_idx=0;
     bool Done;

     uint16_t* CellVoltPrt = GetCellVoltPrt();

     for(repeat_idx=0; repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){

         Done = MeasureCellVoltageRoutine(CellVoltPrt);

         if(Done){break;}
     }

//     SlaveData.TotalSumVolt = array16_sum(SlaveData.CellVolt, NUMBER_OF_CELLS);
//     SlaveData.Avg_SOC = array16_avg(SlaveData.CellVolt, NUMBER_OF_CELLS);

     if(GetChargingStatus()){
         for(repeat_idx=0;repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){
             Done = BalanceCellsRoutine(CellVoltPrt);

             if(Done){break;}
         }
     }

 }

 void SlaveFlagsTask(){
     if(!rtiTimerExpired(1, 1, 0)){
         return;
     }

     int repeat_idx;
     uint32_t SlaveFaults;

     bool Done;
     for(repeat_idx=0;repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){
         SlaveFaults = ReadStatAndGetFlagsRoutine();
         SetAllSlaveFaults(SlaveFaults);

         if(Done){break;}
     }

 }
 void MonitorCellTempTask(){
     if(!rtiTimerExpired(2, 1, 0)){
         return;
     }

     int repeat_idx;
     bool Done;
     uint16_t* CellTempPrt = GetCellTempPrt();
     uint16_t* RefVolt2ndPrt = GetRefVolt2ndPrt();
     for(repeat_idx=0;repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){

         Done = MeasureGPIOVoltageRoutine(CellTempPrt, RefVolt2ndPrt);

         if(Done){break;}
     }
 }
