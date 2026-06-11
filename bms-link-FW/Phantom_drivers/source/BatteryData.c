/*
 * BatteryData.c
 *
 *  Created on: Jun 8, 2026
 *      Author: tanjo
 */


#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Functions.h"

#include "BatteryData.h"


struct BatteryData_struct BatteryData;
 //----------------------------------------------------------------------------------------------------
 void SetChargingStatus(const bool NewStat){
     BatteryData.Charging = NewStat;
 }
 bool GetChargingStatus(){
     return BatteryData.Charging;
 }
// void CheckChargingSatusTask(){
//     SetChargingStatus(TRUE);
// }
 //----------------------------------------------------------------------------------------------------
 uint16_t* GetCellVoltPrt(){
     return BatteryData.CellVolt;
 }
 uint16_t* GetCellTempPrt(){
     return BatteryData.CellTemp;
 }
 uint16_t* GetRefVolt2ndPrt(){
     return BatteryData.RefVolt2nd;
 }
 //----------------------------------------------------------------------------------------------------
 void SetAllSlaveFaults(const uint32_t NewSlaveFaults){
     BatteryData.Slave_Faults = NewSlaveFaults;
 }
 void ClearSlaveFaults(){
     SetAllSlaveFaults(0);
 }
 uint32_t GetAllSlaveFaults(){
     return BatteryData.Slave_Faults;
 }
 bool AnySlaveFaults(){
     return (BatteryData.Slave_Faults != 0);
 }
 void SetSlaveFault(const uint8_t Val, const uint8_t bitSize, const Slave_Faults Fault){
     const uint8_t mask = (1U<<bitSize)-1;
     uint8_t Val_mask   = Val & mask;

     BatteryData.Slave_Faults &= ~((uint32_t)mask   << Fault);
     BatteryData.Slave_Faults |=  (uint32_t)Val_mask << Fault;
 }
  void SetSlaveFault_bool(const bool Val, const Slave_Faults Fault){

     BatteryData.Slave_Faults &= (1U << Fault);
     BatteryData.Slave_Faults |= (uint32_t)Val << Fault;
 }
 //----------------------------------------------------------------------------------------------------
  void SetAllIMDFaults(const uint8_t NewIMDFaluts){
      BatteryData.IMD_Fault = NewIMDFaluts & 0x7;
  }
  void ClearIMDFaults(){
      SetAllIMDFaults(0);
  }
  uint8_t GetAllIMDFaults(){
      return BatteryData.IMD_Fault;
  }
  bool AnyIMDFaults(){
      return (BatteryData.IMD_Fault != 0);
  }
  //----------------------------------------------------------------------------------------------------
  void SetBatteryCurrentVal(const uint16_t ADC_Val){
      BatteryData.current = ADC_Val;
  }
  uint16_t GetBatteryCurrentVal(){
      return BatteryData.current ;
  }

  //----------------------------------------------------------------------------------------------------
  uint16_t GetAvgCellVolt(){
      return array16_avg(GetCellVoltPrt(), NUMBER_OF_CELLS);
  }
  float GetAvgCellSOC(){
      const uint16_t avgVolts_offset = GetAvgCellVolt() - UNDER_VOLTAGE_FLAG;
      const float avgSOC = avgVolts_offset /(CELL_SOC_RANGE) * 100;
      return avgSOC;
  }
  uint16_t GetMaxCellVolt(){
      return array16_max(GetCellVoltPrt(), NUMBER_OF_CELLS);
  }
  float GetMaxCellSOC(){
      const uint16_t maxVolts_offset = GetMaxCellVolt() - UNDER_VOLTAGE_FLAG;
      const float maxSOC = maxVolts_offset /(CELL_SOC_RANGE) * 100;
      return maxSOC;
  }
  uint16_t GetMinCellVolt(){
      return array16_min(GetCellVoltPrt(), NUMBER_OF_CELLS);
  }
  float GetMinCellSOC(){
      const uint16_t minVolts_offset = GetMinCellVolt() - UNDER_VOLTAGE_FLAG;
      const float minSOC = minVolts_offset /(CELL_SOC_RANGE) * 100;
      return minSOC;
  }
  //----------------------------------------------------------------------------------------------------
