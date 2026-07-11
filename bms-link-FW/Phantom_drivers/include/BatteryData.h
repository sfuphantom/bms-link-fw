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
#include "SlaveCommunation_Hardware.h"
//#include "SlaveCommunation_Functions.h"


#define CELL_VOLT_100_FULL  41000
#define CELL_VOLT_0_FULL    33000
#define CELL_SOC_RANGE      (CELL_VOLT_100_FULL - CELL_VOLT_0_FULL)

#define CELL_VOLT_SAFETY_RANGE 50
#define CELL_VOLT_OVER      (CELL_VOLT_100_FULL + CELL_VOLT_SAFETY_RANGE)
#define CELL_VOLT_UNDER     (CELL_VOLT_0_FULL   - CELL_VOLT_SAFETY_RANGE)

//////////////////////////////////////////////////////////////
#define MAX_CELL_CHARGING_PERCENTAGE        (70.0f / 100)
#define MAX_CELL_CHARGING_VOLTAGE_TARGET    ((MAX_CELL_CHARGING_PERCENTAGE * CELL_SOC_RANGE) + CELL_VOLT_0_FULL)//38000
#define CELL_CHARGING_SOC_TARGET_TOLORENCES_PERCENTAGE    (02.0f / 100)
/////////////////////////////////////////////////////////////////
#define CELL_BALANCE_THESHOLD_VOLTS_ADC 1000 //0.1V
#define CELL_BALANCE_HYSTERSIS_BAND_VOLTS_ADC 200 // 0.05
#define CELL_BALANCE_TRIGGER_HIGH_VOLTS_ADC (CELL_BALANCE_THESHOLD_VOLTS_ADC + CELL_BALANCE_HYSTERSIS_BAND_VOLTS_ADC)
#define CELL_BALANCE_TRIGGER_LOW_VOLTS_ADC (CELL_BALANCE_THESHOLD_VOLTS_ADC - CELL_BALANCE_HYSTERSIS_BAND_VOLTS_ADC)

#define VOLTS_Per_10000_DRAINED_PER_PULSE 100
//////////////////////////////////////////////////////////////
//#define MAX_CHARGER_CURRENT_AMPS 1
//#define CHARGER_CURRENT_AMPS
/////////////////////////////////////////////////////////////////
#define SLAVE_NO_FAULT_VAL 0
#define IMD_NO_FAULT_VAL 0
//#define BMS_NO_FALUT_VAL 0
//////////////////////////////////////////////////////////////
#define NUMBER_OF_VOLT_SAMPLES_SAVED 3
//////////////////////////////////////////////////////////////

 typedef enum {BAD_Temp_HIGH, BAD_OV_flags, BAD_UV_flags, BAD_THSD, BAD_MUXFAIL, BAD_ITMP, BAD_VA, BAD_VD, BAD_REF2ND, BAD_SLAVE_CONNECTION_FLAG}Slave_Faults;
 typedef enum { BMS_RUNNING, BMS_CHARGING, BMS_DONE_CHARGING, BMS_DISCHARGING, BMS_SLEEPING, BMS_IDLE, BMS_FAULT } BMSState_t;
//----------------------------------------------------------------------------------------------------
 struct FaultsData_struct{
     uint8_t  IMD_Fault;
     uint8_t  HV_LV_ISOLATION;
     uint16_t Slave_Faults;
     uint16_t other_Faults;
 };

 struct BatteryData_struct {

   bool StartCharging;
//   bool DoneChaging;
//   uint16_t ChargerCurrent;
   BMSState_t BMS_State;

   uint16_t CellTemp[NUMBER_OF_GPIOS];
   uint16_t CellVolt[NUMBER_OF_CELLS];

//   uint16_t CellVolt[NUMBER_OF_VOLT_SAMPLES_SAVED][NUMBER_OF_CELLS];
//   uint16_t ReadCellVolt[NUMBER_OF_CELLS];

   uint16_t RefVolt2nd[NUMBER_OF_REF_2ND];
   uint16_t current;
   struct FaultsData_struct FaultsData;
 };
 //----------------------------------------------------------------------------------------------------
 inline uint16_t Slave_Volt2ADC(const float ADC_Volt);
 inline float Slave_ADC2Volt(const uint16_t ADC_Word);
 inline float Slave_ADC2Celcius(const uint16_t ADC);
 void Slave_ADC2Volt_arr(const uint16_t* ADC_Words, float* Volts, const uint16_t len);
//----------------------------------------------------------------------------------------------------
  void SetChargingStatus(const bool NewStat);
  bool GetChargingStatus();
 //----------------------------------------------------------------------------------------------------
  inline uint16_t* GetCellVoltReadPrt();
  inline uint16_t* GetCellTempReadPrt();
  inline uint16_t* GetRefVolt2ndReadPrt();
  inline uint16_t* GetCellVoltWritePrt();
  inline uint16_t* GetCellTempWritePrt();
  inline uint16_t* GetRefVolt2ndWritePrt();
 //----------------------------------------------------------------------------------------------------
  void SetAllSlaveFaults(const uint16_t NewSlaveFaults);
  void AddSlaveFaults(const uint16_t NewSlaveFaults);
  void ClearSlaveFaults();
  uint16_t GetAllSlaveFaults();
  bool AnySlaveFaults();
  void SetSlaveFault(const uint8_t Val, const uint8_t bitSize, const Slave_Faults Fault);
  void SetSlaveFault_bool(const bool Val, const Slave_Faults Fault);
  void SetSlaveFault_bool_HIGH(const Slave_Faults Fault);
  bool GetSlaveFault_bool(const Slave_Faults Fault);
//----------------------------------------------------------------------------------------------------
  void SetAllIMDFaults(const uint8_t NewIMDFaluts);
  void ClearIMDFaults();
  uint8_t GetAllIMDFaults();
  bool AnyIMDFaults();
//----------------------------------------------------------------------------------------------------
  void SetBatteryCurrentVal(const uint16_t ADC_Val);
  uint16_t GetBatteryCurrentVal();
//----------------------------------------------------------------------------------------------------
  inline uint16_t GetAvgCellVolt();
  inline float    GetAvgCellVolt_float();
  inline float    GetAvgCellSOC();
  inline uint16_t GetMaxCellVolt();
  inline float    GetMaxCellVolt_float();
  inline float    GetMaxCellSOC();
  inline uint16_t GetMinCellVolt();
  inline float    GetMinCellVolt_float();
  inline float    GetMinCellSOC();
  //---------------------------------------------------------------------------------------------------------
  uint8_t GetBalanceNibbles(uint8_t* BalanceNibbles);
  uint8_t GetBalanceDCC(uint16_t* DCC);
  //---------------------------------------------------------------------------------------------------------
  struct ChargerData_t* GetBatteryDataPrt();
  void initBatteryData();
  //---------------------------------------------------------------------------------------------------------
  struct BatteryData_struct BatteryData;


#endif /* PHANTOM_DRIVERS_INCLUDE_BATTERYDATA_H_ */
