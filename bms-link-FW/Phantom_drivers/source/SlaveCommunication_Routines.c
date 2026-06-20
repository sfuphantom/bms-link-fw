#include "spi.h"
#include <stdint.h>
#include <stdbool.h>

#include "ltc6811_commands.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Functions.h"
#include "SlaveCommunation_Hardware.h"
#include "SlaveCommunication_Routines.h"

#include "PhantomHelpers.h"

#include "BatteryData.h"
#include "Charger.h"

//struct BatteryData_struct SlaveData;
//---------------------------------------------------------------------------------------------------------
 void initLink(){
     init_PEC15_Table();
     //------------------------------------------------
     wakeup_sleep();

     ClearSlaveRegs();

     SetAllPWM_Regs(0xF);
     initConfig();
 }

 void keepAwake(){
     SendDummyCMD();
 }
 //---------------------------------------------------------------------------------------------------------
 bool MeasureGPIOVoltageSubRoutine_NoErrorHandling(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     uint32_t cmdDone = MeasureAUXCmd(ADC_MEASURE_MODE, 0);

     uint16_t AUXDataOut[NUMBER_OF_AUCILIARY];
     GetGPIOReadings_Analog(AUXDataOut);

     bool AllValid = !array16_eq_any(AUXDataOut, SPI_DUMMY_DATA_WORD, NUMBER_OF_GPIOS);

     if(!((cmdDone != 0) && AllValid)){
         return false;
     }
     ///////////////////////////////////////////////////////////
     uint16_t* GPIO_DataOut = GetCellTempWritePrt();
     uint16_t* Ref2nd_DataOut = GetRefVolt2ndWritePrt();

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
     return true;
 }

 bool MeasureCellVoltageSubRoutine_NoErrorHandling(){
     uint16_t* VoltDataOut = GetCellVoltWritePrt();

     uint32_t cmdDone = MeasureCellsCmd(ADC_MEASURE_MODE, ADC_MEASURE_DISCHARGE_PERMITED, 0);

     wakeup_sleep();
     GetVoltageReadings(VoltDataOut);

     bool AllValid = !array16_eq_any(VoltDataOut, SPI_DUMMY_DATA_WORD, NUMBER_OF_CELLS);;

     return (cmdDone != 0) && AllValid;
 }
 bool BalanceCellsSubRoutine_NoErrorHandling(){
    const uint16_t* VoltInData = GetCellVoltReadPrt();

    #define Bal_IMP 1
    #if Bal_IMP == 0
         uint8_t S_CLTR_nibbles[NUMBER_OF_CELLS/2];

         const uint8_t NumCellsFull = GetBalanceNibbles(VoltInData, S_CLTR_nibbles);

         Write_S_CTRL(S_CLTR_nibbles);

         const uint32_t cmdDone = Start_S_CTRL_Pulsing();
         ClearSCtrlCMD();

//         Start_S_CTRL_Pulsing();
         return cmdDone;
    #elif Bal_IMP == 1
         uint16_t DCC_Val[NUMBER_OF_SLAVE_BOARDS]={0};

         const uint8_t NumCellsFull = GetBalanceDCC(DCC_Val);

         SetConfig_DCC(DCC_Val);
         Write_CFGR();
//         delay_ms_us(25,0);

//         int i;
//         for(i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
//             DCC_Val[i]=0;
//         }
//         SetConfig_DCC(DCC_Val);
//         Write_CFGR();
         return NumCellsFull;
    #else
         uint8_t PWM_nibbles[NUMBER_OF_CELLS/2];

         const uint8_t NumCellsFull = GetBalanceNibbles(PWM_nibbles);

          WriteThenRead_PWM(PWM_nibbles);

          return NumCellsFull;
    #endif

 }

 bool ReadStatAndGetFlagsSubRoutine_NoErrorHandling(){
     const uint32_t cmdDone = MeasureSTATCmd(ADC_MEASURE_MODE,0x0);


     const bool Stat_Valid = Read_STAT();
     const bool Config_Valid = Read_CFGR();

     const bool Valid = (cmdDone != 0) && Stat_Valid && Config_Valid;
     if(!Valid){
         return false;
     }

     uint32_t Flags = checkStatFlags();
     Flags<<=1;
     Flags |= !ReadConfig_gpio_allZero();

     SetAllSlaveFaults(Flags);
    return true;
 }
 //---------------------------------------------------------------------------------------------------------
 void SubRoutine_ErrorHandler(bool (*SubRoutine_prt)(void)){
     int repeat_idx;
     bool Valid;



     for(repeat_idx=0; repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){
         if(GetSlaveFault_bool(BAD_SLAVE_CONNECTION_FLAG)){
            return;
         }

         Valid = SubRoutine_prt();

         if(Valid){return;}
     }

     SetSlaveFault_bool_HIGH(BAD_SLAVE_CONNECTION_FLAG);
 }
 //---------------------------------------------------------------------------------------------------------
 void MeasureGPIOVoltageSubRoutine(){
     SubRoutine_ErrorHandler(MeasureGPIOVoltageSubRoutine_NoErrorHandling);
 }

 void MeasureCellVoltageSubRoutine(){
     SubRoutine_ErrorHandler(MeasureCellVoltageSubRoutine_NoErrorHandling);
 }
 void BalanceCellsSubRoutine(){
     SubRoutine_ErrorHandler(BalanceCellsSubRoutine_NoErrorHandling);
 }

 void ReadStatAndGetFlagsSubRoutine(){
     SubRoutine_ErrorHandler(ReadStatAndGetFlagsSubRoutine_NoErrorHandling);
 }
//----------------------------------------------------------------------------------------------------
 void CellVoltageControlRoutine(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     MeasureCellVoltageSubRoutine();

//     SlaveData.TotalSumVolt = array16_sum(SlaveData.CellVolt, NUMBER_OF_CELLS);
//     SlaveData.Avg_SOC = array16_avg(SlaveData.CellVolt, NUMBER_OF_CELLS);

     if(GetChargingStatus()){
         BalanceCellsSubRoutine();

         // TODO: Get AVG_SOC (maybe MAX_SOC too) and use that to change charger current
     }
 }

 void MonitorCellTempRoutine(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     MeasureGPIOVoltageSubRoutine();
 }

 void SlaveFlagsRoutine(){
 //     const uint32_t CS_pollWaitings = waitSPIFree(1000);
      ReadStatAndGetFlagsSubRoutine();
  }
