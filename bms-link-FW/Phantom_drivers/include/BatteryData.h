/*
 * BatteryData.h
 *
 *  Created on: Jun 8, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_BATTERYDATA_H_
#define PHANTOM_DRIVERS_INCLUDE_BATTERYDATA_H_
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Functions.h"

#define CELL_SOC_UINT16_100     OVER_VOLTAGE_FLAG
#define CELL_SOC_UINT16_0       UNDER_VOLTAGE_FLAG
#define CELL_SOC_RANGE          CELL_SOC_UINT16_100 - CELL_SOC_UINT16_0
//////////////////////////////////////////////////////////////
struct BatteryData_struct {

  bool Charging;

  uint16_t CellTemp[NUMBER_OF_GPIOS];
  uint16_t CellVolt[NUMBER_OF_CELLS];

  uint16_t RefVolt2nd[NUMBER_OF_REF_2ND];

  uint8_t IMD_Fault;
  uint32_t Slave_Faults;
  uint16_t current;
};
//----------------------------------------------------------------------------------------------------
typedef enum {Temp_HIGH, OV_flags, UV_flags, THSD, MUXFAIL, ITMP_HIGH, ITMP_LOW, VA_HIGH, VA_LOW, VD_HIGH, VD_LOW}Slave_Faults;
//----------------------------------------------------------------------------------------------------
uint16_t Slave_Volt2ADC(const float ADC_Volt);
float Slave_ADC2Volt(const uint16_t ADC_Word);
float Slave_ADC2Celcius(const uint16_t ADC);
void Slave_ADC2Volt_arr(const uint16_t* ADC_Words, float* Volts, const uint16_t len);
//----------------------------------------------------------------------------------------------------
 void SetChargingStatus(const bool NewStat);
 bool GetChargingStatus();
 //----------------------------------------------------------------------------------------------------
 uint16_t* GetCellVoltPrt();
 uint16_t* GetCellTempPrt();
 uint16_t* GetRefVolt2ndPrt();
 //----------------------------------------------------------------------------------------------------
 void SetAllSlaveFaults(const uint32_t NewSlaveFaults);
 void ClearSlaveFaults();
 uint32_t GetAllSlaveFaults();
 bool AnySlaveFaults();
 void SetSlaveFault(const uint8_t Val, const uint8_t bitSize, const Slave_Faults Fault);
  void SetSlaveFault_bool(const bool Val, const Slave_Faults Fault);
//----------------------------------------------------------------------------------------------------
  void SetAllIMDFaults(const uint8_t NewIMDFaluts);
  void ClearIMDFaults();
  uint8_t GetAllIMDFaults();
  bool AnyIMDFaults();
  //----------------------------------------------------------------------------------------------------
  void SetBatteryCurrentVal(const uint16_t ADC_Val);
  uint16_t GetBatteryCurrentVal();

  //----------------------------------------------------------------------------------------------------
  uint16_t GetAvgCellVolt();
  uint16_t GetAvgCellVolt_float();
  float GetAvgCellSOC();
  uint16_t GetMaxCellVolt();
  uint16_t GetMaxCellVolt_float();
  float GetMaxCellSOC();
  uint16_t GetMinCellVolt();
  uint16_t GetMinCellVolt_float();
  float GetMinCellSOC();

#endif /* PHANTOM_DRIVERS_INCLUDE_BATTERYDATA_H_ */
