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
#include "HV_data.h"
#include "Fault_handler.h"
#include "SendDataSerial.h"



//void SendCellDCC_PWM_Serial(){
//
//}

 //---------------------------------------------------------------------------------------------------------

 bool MeasureRef2ndVoltageSubRoutine_NoErrorHandling() {
       ClearAUXCMD();
       MeasureAUXCmd(ADC_MEASURE_MODE, 6);

       uint16_t AUXDataOut[NUMBER_OF_AUCILIARY];
       GetGPIOReadings_Analog(AUXDataOut);

      int i, idx;
      bool AllValid = TRUE;
      StatusReg* current_Slave = GetStatusRegData();
      for(i=0, idx=0;i<NUMBER_OF_SLAVE_BOARDS; i++){
          idx+=GPIOS_PER_SLAVE_BOARD;

          AllValid = SPI_DUMMY_DATA_WORD != AUXDataOut[idx];

          current_Slave->RefVolt2nd = AUXDataOut[idx++];
          current_Slave++;
       }

//       uint16_t* Ref2nd_DataOut = GetRefVolt2ndWritePrt();

//       int i,j;
//       int idx = 0;
//       bool flag_2ndRef;
//       for(i=0;i<NUMBER_OF_SLAVE_BOARDS; i++){
//           idx+=GPIOS_PER_SLAVE_BOARD;
//           for(j=0;j<REF_2ND_PER_SLAVE_BOARD; j++){
//               flag_2ndRef |= (MAX_2ND_REFERENCE_VOLTAGE_FLAG < AUXDataOut[idx]) | (MIN_2ND_REFERENCE_VOLTAGE_FLAG > AUXDataOut[idx]);
//               *Ref2nd_DataOut = AUXDataOut[idx++];
//                Ref2nd_DataOut++;
//           }
//       }

//       bool AllValid = !array16_eq_any(Ref2nd_DataOut, SPI_DUMMY_DATA_WORD, NUMBER_OF_REF_2ND);

       if(!AllValid){
           return false;
       }


       return true;
 }

 bool MeasureAUXVoltageSubRoutine_NoErrorHandling(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     uint32_t cmdDone = MeasureAUXCmd_All(ADC_MEASURE_MODE);

     uint16_t AUXDataOut[NUMBER_OF_AUCILIARY];
     bool PecEq = GetGPIOReadings_Analog(AUXDataOut);

     bool AllValid = !array16_eq_any(AUXDataOut, SPI_DUMMY_DATA_WORD, NUMBER_OF_AUCILIARY);

     if(!((cmdDone != 0) && AllValid && PecEq)){
         return false;
     }
     ///////////////////////////////////////////////////////////
     uint16_t* GPIO_DataOut = GetCellTempWritePrt();
//     uint16_t* Ref2nd_DataOut = GetRefVolt2ndWritePrt();
     StatusReg* current_Slave = GetStatusRegData();

     int i,j;
     int idx = 0;
     bool flag_Temp = FALSE;
     for(i=0;i<NUMBER_OF_SLAVE_BOARDS; i++){
         for(j=0;j<GPIOS_PER_SLAVE_BOARD; j++){
             flag_Temp |= MAX_CELL_TEMPERATURE_FLAG < AUXDataOut[idx];
             *GPIO_DataOut = AUXDataOut[idx++];
              GPIO_DataOut++;
         }

         current_Slave->RefVolt2nd = AUXDataOut[idx++];
         current_Slave++;

//         for(j=0;j<REF_2ND_PER_SLAVE_BOARD; j++){
//             flag_2ndRef |= (MAX_2ND_REFERENCE_VOLTAGE_FLAG < AUXDataOut[idx]) | (MIN_2ND_REFERENCE_VOLTAGE_FLAG > AUXDataOut[idx]);
//             *Ref2nd_DataOut = AUXDataOut[idx++];
//
//              Ref2nd_DataOut++;
//         }
     }
     if(flag_Temp){
         const hetSIGNAL_t signal = {100,40};
         SetAllFansSignal(signal);
         StartAllFans();
     }
     else{
         StopAllFans();

     }
     return true;
 }

 bool MeasureCellVoltageSubRoutine_NoErrorHandling(){
#if USE_WAKEUP_SLEEP
     wakeup_sleep();
#else
     wakeup_idle();
#endif
     uint16_t* VoltDataOut = GetCellVoltWritePrt();

     bool cmdDone = MeasureCellsCmd_All_NoDis(ADC_MEASURE_MODE);
//     uint32_t cmdDone = MeasureCellsCmd(ADC_MEASURE_MODE, ADC_MEASURE_DISCHARGE_PERMITED, 0);

#if USE_WAKEUP_SLEEP
     wakeup_sleep();
#else
     wakeup_idle();
#endif
     bool PecEQ = GetVoltageReadings(VoltDataOut);

     bool AllValid = !array16_eq_any(VoltDataOut, SPI_DUMMY_DATA_WORD, NUMBER_OF_CELLS);

     return cmdDone && AllValid && PecEQ;
 }

 bool BalanceCellsSubRoutine_NoErrorHandling(){
#if USE_WAKEUP_SLEEP
     wakeup_sleep();
#else
     wakeup_idle();
#endif

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
//         uint16_t DCC_Val[NUMBER_OF_SLAVE_BOARDS]={0};
         uint16_t* DCC_Val = GetCellDCCWritePrt();
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
 bool MeasureCellResistanceSubRoutine_NoErrorHandling(){
#if USE_WAKEUP_SLEEP
     wakeup_sleep();
#else
     wakeup_idle();
#endif

     int i;
     bool PecEQ = TRUE;
     const uint16_t* OpenCellVolts= GetCellVoltReadPrt();
     uint16_t disCellVolts[NUMBER_OF_CELLS];

//     uint16_t DCC[NUMBER_OF_SLAVE_BOARDS];
//     ReadConfig_DCC(DCC);
     const uint16_t* DCC = GetCellDCCReadPrt();

//     SetAllConfig_DCC(0xFFFF);
//     Write_CFGR();
//     bool cmdDone = MeasureCellsCmd(ADC_MEASURE_MODE, TRUE, 0);
//     SetConfig_DCC(DCC);

     const uint16_t Cell_71_bits = 1<<6 + 1<<0;
     ClearCellsCMD();
     for(i=0; i<CELLS_PER_SLAVE_BOARD/2; ){

         SetAllHigh_DCC(Cell_71_bits<<i);
         Write_CFGR();

         MeasureCellsCmd_Dis(ADC_MEASURE_MODE, ++i);

         SetConfig_DCC(DCC);
     }

     Write_CFGR();

#if USE_WAKEUP_SLEEP
     wakeup_sleep();
#else
     wakeup_idle();
#endif

     PecEQ &= GetVoltageReadings(disCellVolts);

     uint16_t Vdiff;
     uint16_t res_mOhm;
     uint16_t CellRes_mOhm[NUMBER_OF_CELLS];

     for(i=0; i<NUMBER_OF_CELLS; i++){
         Vdiff = OpenCellVolts[i] - disCellVolts[i];
         res_mOhm = (1000*res_mOhm) * ((float)Vdiff/disCellVolts[i]);
         CellRes_mOhm[i] = res_mOhm;
     }
     writeCellRes(CellRes_mOhm);

     bool AllValid = !array16_eq_any(disCellVolts, SPI_DUMMY_DATA_WORD, NUMBER_OF_CELLS);;


     return (AllValid && PecEQ);
 }
 bool ReadStatAndGetFlagsSubRoutine_NoErrorHandling(){
#if USE_WAKEUP_SLEEP
     wakeup_sleep();
#else
     wakeup_idle();
#endif
     const uint32_t cmdDone = MeasureSTATCmd_All(ADC_MEASURE_MODE);

     const bool Stat_Valid = Read_STAT();

#if !USE_ANILOG_GPIO
//     const bool Config_Valid = Read_CFGR();
     const bool Config_Valid = TRUE;
#else
     const bool Config_Valid = TRUE;
#endif

     const bool Valid = (cmdDone != 0) && Stat_Valid && Config_Valid;
     if(!Valid){
         return false;
     }

#if !USE_ANILOG_GPIO
     const bool flag_Temp = !ReadConfig_gpio_allZero();
     if(flag_Temp){
         const hetSIGNAL_t signal = {100,40};
         SetAllFansSignal(signal);
         StartAllFans();
     }
     else{
         StopAllFans();

     }
 #endif

    return true;
 }
 //---------------------------------------------------------------------------------------------------------
 bool HV_DataSubRoutine_NoErrorHandling(){
     const uint16_t HV_DataRaw = Get_HV_Data_Raw();

     if(HV_DataRaw == SPI_DUMMY_DATA_WORD){return false;}

     const uint16_t HV_DataShifted = HV_DataRaw>>ADS7044_CODE_SHIFT;

     const float HV_Volts = HV_ADC2VOLTS(HV_DataShifted);

 //    Set_HV_Voltage(HV_Volts);


     Set_HV_Voltage(HV_DataShifted);

//     BatteryData.HV_Voltage = HV_DataShifted;



//     bool can_vaild = transmit_BMS2VCU_Data(HV_DataShifted, getIMDResistance());

 //    if(HV_Volts > 410.0f || HV_Volts < 260.0f){
 //        SetBMSFault_bool_HIGH(BAD_HV_VOLT_FLAG);
 //    }

     return true;
 }
 //---------------------------------------------------------------------------------------------------------
 void SubRoutine_ErrorHandler_Decorator(bool (*SubRoutine_prt)(void), const BMS_Faults ErrorFault){
     int repeat_idx;
     bool Valid;

     for(repeat_idx=0; repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){
//         if(anyFaluts()){
//            return;
//         }

         Valid = SubRoutine_prt();

         if(Valid){return;}
     }

     SetBMSFault_bool_HIGH(ErrorFault);
 }
 //---------------------------------------------------------------------------------------------------------
 inline void MeasureGPIOVoltageSubRoutine(){
     SubRoutine_ErrorHandler_Decorator(MeasureAUXVoltageSubRoutine_NoErrorHandling, BAD_SLAVE_CONNECTION_FLAG);
 }
 inline void MeasureRef2ndVoltageSubRoutine(){
     SubRoutine_ErrorHandler_Decorator(MeasureRef2ndVoltageSubRoutine_NoErrorHandling, BAD_SLAVE_CONNECTION_FLAG);
 }

 inline void MeasureCellVoltageSubRoutine(){
     SubRoutine_ErrorHandler_Decorator(MeasureCellVoltageSubRoutine_NoErrorHandling, BAD_SLAVE_CONNECTION_FLAG);
     SendPeriodic(SEND_CELL_VOLT);
 }
 inline void BalanceCellsSubRoutine(){
     SubRoutine_ErrorHandler_Decorator(BalanceCellsSubRoutine_NoErrorHandling, BAD_SLAVE_CONNECTION_FLAG);
     SendPeriodic(SEND_DCC_DATA);
 }

 inline void ReadStatAndGetFlagsSubRoutine(){
     SubRoutine_ErrorHandler_Decorator(ReadStatAndGetFlagsSubRoutine_NoErrorHandling, BAD_SLAVE_CONNECTION_FLAG);
 }
 inline void HV_DataSubRoutine(){
     SubRoutine_ErrorHandler_Decorator(HV_DataSubRoutine_NoErrorHandling, BAD_HV_VOLT_FLAG);
 }
 inline void MeasureCellResistanceSubRoutine(){
     SubRoutine_ErrorHandler_Decorator(MeasureCellResistanceSubRoutine_NoErrorHandling, BAD_SLAVE_CONNECTION_FLAG);
     SendPeriodic(SEND_CELL_RES);
 }
//----------------------------------------------------------------------------------------------------
 void CellVoltageControlRoutine(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     MeasureCellVoltageSubRoutine();

     const uint16_t Avg_Volt_16 = GetAvgCellVolt();

//     SlaveData.TotalSumVolt = array16_sum(SlaveData.CellVolt, NUMBER_OF_CELLS);
//     SlaveData.Avg_SOC = array16_avg(SlaveData.CellVolt, NUMBER_OF_CELLS);

     if(GetChargingStatus()){
         BalanceCellsSubRoutine();

         const float avg_SOC = GetAvgCellSOC();
         const float max_SOC = GetMaxCellSOC();

         bool vaild = CalcNewCurrentSetting(avg_SOC, max_SOC);


         // TODO: Get AVG_SOC (maybe MAX_SOC too) and use that to change charger current
     }
 }

 void MonitorCellTempRoutine(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     MeasureGPIOVoltageSubRoutine();
 }

 void SlaveFlagsRoutine(){
 //     const uint32_t CS_pollWaitings = waitSPIFree(1000);
#if !USE_ANILOG_GPIO
      MeasureRef2ndVoltageSubRoutine();
#endif
      ReadStatAndGetFlagsSubRoutine();

      checkStatFlags();
      SendPeriodic(SEND_SLAVE_STATE);

  }
 void MonitorFullBatteryDataRoutine(){
     HV_DataSubRoutine();
//     getCurrentSubRoutine();
//     CalcSOC();
 }
void MeasureCellResistanceRoutine(){
    MeasureCellResistanceSubRoutine();
}
void keepSlavesAwakeRoutine(){
    SendDummyCMD();
}
