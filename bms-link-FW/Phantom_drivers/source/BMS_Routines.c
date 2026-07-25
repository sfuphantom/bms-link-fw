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

 //---------------------------------------------------------------------------------------------------------
 uint32_t checkStatFlags(){
      bool flag;
      uint32_t Flags = 0;

      struct StatusReg* current_Slave = GetStatusRegData();
      int i;
      for(i=0;i<NUMBER_OF_SLAVE_BOARDS; i++, current_Slave++){

          flag = current_Slave->OV_flags == 0;
          Flags |= (uint32_t)flag<<BAD_OV_flags;
          flag = current_Slave->UV_flags == 0;
          Flags |= (uint32_t)flag<<BAD_UV_flags;
          flag = !current_Slave->THSD;
          Flags |= (uint32_t)flag<<BAD_THSD;
          flag = !current_Slave->MUXFAIL;
          Flags |= (uint32_t)flag<<BAD_MUXFAIL;

          flag = current_Slave->ITMP > MAX_INTERNAL_DIE_TEMPERATURE_FLAG;
          Flags |= (uint32_t)flag<<BAD_ITMP;
          flag = current_Slave->ITMP < MIN_INTERNAL_DIE_TEMPERATURE_FLAG;
          Flags |= (uint32_t)flag<<BAD_ITMP;
          flag = current_Slave->VA > MAX_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG;
          Flags |= (uint32_t)flag<<BAD_VA;
          flag = current_Slave->VA < MIN_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG;
          Flags |= (uint32_t)flag<<BAD_VA;
          flag = current_Slave->VD > MAX_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG;
          Flags |= (uint32_t)flag<<BAD_VD;
          flag = current_Slave->VD < MIN_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG;
          Flags |= (uint32_t)flag<<BAD_VD;
      }

      AddSlaveFaults(Flags);

      return Flags;
 }
 //---------------------------------------------------------------------------------------------------------

 bool MeasureRef2ndVoltageSubRoutine_NoErrorHandling() {
       uint32_t cmdDone = MeasureAUXCmd(ADC_MEASURE_MODE, 6);

       uint16_t AUXDataOut[NUMBER_OF_AUCILIARY];
       GetGPIOReadings_Analog(AUXDataOut);

       uint16_t* Ref2nd_DataOut = GetRefVolt2ndWritePrt();

       int i,j;
       int idx = 0;
       bool flag_2ndRef;
       for(i=0;i<NUMBER_OF_SLAVE_BOARDS; i++){
           idx+=GPIOS_PER_SLAVE_BOARD;
           for(j=0;j<REF_2ND_PER_SLAVE_BOARD; j++){
               flag_2ndRef |= (MAX_2ND_REFERENCE_VOLTAGE_FLAG < AUXDataOut[idx]) | (MIN_2ND_REFERENCE_VOLTAGE_FLAG > AUXDataOut[idx]);
               *Ref2nd_DataOut = AUXDataOut[idx++];
                Ref2nd_DataOut++;
           }
       }

       bool AllValid = !array16_eq_any(Ref2nd_DataOut, SPI_DUMMY_DATA_WORD, NUMBER_OF_REF_2ND);

       if(!((cmdDone != 0) && AllValid)){
           return false;
       }

       if(flag_2ndRef){
           GetSlaveFault_bool(BAD_REF2ND);
       }

       return true;
 }

 bool MeasureAUXVoltageSubRoutine_NoErrorHandling(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     uint32_t cmdDone = MeasureAUXCmd(ADC_MEASURE_MODE, 0);

     uint16_t AUXDataOut[NUMBER_OF_AUCILIARY];
     GetGPIOReadings_Analog(AUXDataOut);

     bool AllValid = !array16_eq_any(AUXDataOut, SPI_DUMMY_DATA_WORD, NUMBER_OF_AUCILIARY);

     if(!((cmdDone != 0) && AllValid)){
         return false;
     }
     ///////////////////////////////////////////////////////////
     uint16_t* GPIO_DataOut = GetCellTempWritePrt();
     uint16_t* Ref2nd_DataOut = GetRefVolt2ndWritePrt();

     int i,j;
     int idx = 0;
     bool flag_2ndRef = FALSE;
     bool flag_Temp = FALSE;
     for(i=0;i<NUMBER_OF_SLAVE_BOARDS; i++){
         for(j=0;j<GPIOS_PER_SLAVE_BOARD; j++){
             flag_Temp |= MAX_CELL_TEMPERATURE_FLAG < AUXDataOut[idx];
             *GPIO_DataOut = AUXDataOut[idx++];
              GPIO_DataOut++;
         }

         for(j=0;j<REF_2ND_PER_SLAVE_BOARD; j++){
             flag_2ndRef |= (MAX_2ND_REFERENCE_VOLTAGE_FLAG < AUXDataOut[idx]) | (MIN_2ND_REFERENCE_VOLTAGE_FLAG > AUXDataOut[idx]);
             *Ref2nd_DataOut = AUXDataOut[idx++];

              Ref2nd_DataOut++;
         }
     }
     if(flag_2ndRef){
         SetSlaveFault_bool_HIGH(BAD_REF2ND);
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
     wakeup_sleep();

     uint16_t* VoltDataOut = GetCellVoltWritePrt();

//     uint16_t DCC[NUMBER_OF_CELLS];
//     memset(DCC,0,NUMBER_OF_CELLS * sizeof(uint16_t));
//     SetConfig_DCC(DCC);
//     Write_CFGR();

     uint32_t cmdDone = MeasureCellsCmd(ADC_MEASURE_MODE, ADC_MEASURE_DISCHARGE_PERMITED, 0);

     wakeup_sleep();
     GetVoltageReadings(VoltDataOut);

     bool AllValid = !array16_eq_any(VoltDataOut, SPI_DUMMY_DATA_WORD, NUMBER_OF_CELLS);;

     return (cmdDone != 0) && AllValid;
 }
 bool BalanceCellsSubRoutine_NoErrorHandling(){
     wakeup_sleep();

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
     wakeup_sleep();

     const uint32_t cmdDone = MeasureSTATCmd(ADC_MEASURE_MODE,0x0);

     const bool Stat_Valid = Read_STAT();

#if !USE_ANILOG_GPIO
     const bool Config_Valid = Read_CFGR();
#else
     const bool Config_Valid = TRUE;
#endif

     const bool Valid = (cmdDone != 0) && Stat_Valid && Config_Valid;
     if(!Valid){
         return false;
     }

     uint32_t Flags = checkStatFlags()<<1;

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

    AddSlaveFaults(Flags);
    return true;
 }
 //---------------------------------------------------------------------------------------------------------
 bool HV_DataSubRoutine_NoErrorHandling(){
     const uint16_t HV_DataRaw = Get_HV_Data_Raw();

     if(HV_DataRaw == SPI_DUMMY_DATA_WORD){return false;}

     const uint16_t HV_DataShifted = HV_DataRaw>>ADS7044_CODE_SHIFT;

     const float HV_Volts = HV_ADC2VOLTS(HV_DataShifted);

 //    Set_HV_Voltage(HV_Volts);


     BatteryData.HV_Voltage = HV_DataShifted;

//     bool can_vaild = transmit_BMS2VCU_Data(HV_DataShifted, getIMDResistance());

 //    if(HV_Volts > 410.0f || HV_Volts < 260.0f){
 //        SetBMSFault_bool_HIGH(BAD_HV_VOLT_FLAG);
 //    }

     return true;
 }
 //---------------------------------------------------------------------------------------------------------
 void SubRoutine_ErrorHandler(bool (*SubRoutine_prt)(void)){
     int repeat_idx;
     bool Valid;

     for(repeat_idx=0; repeat_idx<NUMBER_OF_FAILS_ALLOWED;repeat_idx++){
//         if(GetSlaveFault_bool(BAD_SLAVE_CONNECTION_FLAG)){
//            return;
//         }

//         wakeup_sleep();
         Valid = SubRoutine_prt();

         if(Valid){return;}
     }

     SetSlaveFault_bool_HIGH(BAD_SLAVE_CONNECTION_FLAG);
 }
 //---------------------------------------------------------------------------------------------------------
 void MeasureGPIOVoltageSubRoutine(){
     SubRoutine_ErrorHandler(MeasureAUXVoltageSubRoutine_NoErrorHandling);
 }
 void MeasureRef2ndVoltageSubRoutine(){
     SubRoutine_ErrorHandler(MeasureRef2ndVoltageSubRoutine_NoErrorHandling);
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
 void HV_DataSubRoutine(){
     SubRoutine_ErrorHandler(HV_DataSubRoutine_NoErrorHandling);
 }
//----------------------------------------------------------------------------------------------------
 void CellVoltageControlRoutine(){
//     const uint32_t CS_pollWaitings = waitSPIFree(1000);
     MeasureCellVoltageSubRoutine();

     uint16_t Avg_Volt_16 = GetAvgCellVolt();



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

      ReadStatAndGetFlagsSubRoutine();
#if !USE_ANILOG_GPIO
      MeasureRef2ndVoltageSubRoutine();
#endif

  }
 void HV_DataRoutine(){
     HV_DataSubRoutine();
 }
