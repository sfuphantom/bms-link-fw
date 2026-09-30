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
#include "FullBattery_Hardware.h"

//////////////////////////////////////////////////////////////
#define KILL_DRIVER FALSE
/////////////////////////////////////////////////////////////////
#define NUMBER_OF_VOLT_SAMPLES_SAVED 2
#define NUMBER_OF_TEMP_SAMPLES_SAVED 2
#define NUMBER_OF_RES_SAMPLES_SAVED 2
#define NUMBER_OF_SOC_SAMPLES_SAVED 2
//////////////////////////////////////////////////////////////

//#define PROTO_CODE TRUE

 typedef enum {BMS_START, BMS_RUNNING, BMS_CHARGING, BMS_DONE_CHARGING, BMS_DISCHARGING, BMS_DONE_DISCHARGING, BMS_SLEEPING, BMS_IDLE, BMS_FAULT } BMSState_t;
//----------------------------------------------------------------------------------------------------
 typedef struct {
     uint8_t duty;
     uint8_t freq;
 //    uint32_t resistance;
 }ecapIMDData_t;

 typedef struct {
     uint16_t Volt;
     int16_t Current;
     uint16_t SOC;
     uint16_t Res;

 }FullBatteryData_t;

 struct BatteryData_t {

//   bool ChargingNDischarging;
//   bool DoneChaging;
//   uint16_t ChargerCurrent;
   BMSState_t BMS_State;

   uint16_t CellVolt[NUMBER_OF_VOLT_SAMPLES_SAVED][NUMBER_OF_CELLS_SERIES];
   uint16_t CellTemp[NUMBER_OF_TEMP_SAMPLES_SAVED][NUMBER_OF_THERMISTORS_TOTAL];
   uint16_t CellRes[NUMBER_OF_RES_SAMPLES_SAVED][NUMBER_OF_CELLS_SERIES];
   uint16_t CellSOC[NUMBER_OF_SOC_SAMPLES_SAVED][NUMBER_OF_CELLS_SERIES];
   uint8_t BalancePWM_Nibbles[(NUMBER_OF_CELLS_SERIES+1)/2];

   FullBatteryData_t FullBatteryData;
   ecapIMDData_t ecapIMDData;

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
  void SetBatteryCurrentVal(const uint16_t ADC_Val);
  uint16_t GetBatteryCurrentVal();
  //----------------------------------------------------------------------------------------------------
  inline const uint16_t* GetCellVoltReadPrt(const uint8_t index);
   inline const uint16_t* GetCellTempReadPrt(const uint8_t index);
   inline const uint16_t* GetCellResReadPrt(const uint8_t index);
   inline const uint16_t* GetCellSOCReadPrt(const uint8_t index);
   inline const uint8_t* GetBalancePWM_NibblesReadPrt();
  //----------------------------------------------------------------------------------------------------
  inline uint16_t* GetCellVoltWritePrt();
  inline uint16_t* GetCellTempWritePrt();
  inline uint8_t* GetBalancePWM_NibblesWritePrt();
  inline uint16_t* GetCellResWritePrt();
  inline uint16_t* GetCellSOCWritePrt();
  //----------------------------------------------------------------------------------------------------
  inline void SetCellVolt(const uint16_t* CellVolt);
  inline void SetCellTemp(const uint16_t* CellTemp);
  inline void SetCellRes(const uint16_t *CellRes);
  inline void SetCellSOC(const uint16_t *CellSOC);
  inline void GetCellVolt(uint16_t* const CellVolt);
  inline void GetCellTemp(uint16_t* const CellTemp);
  inline void GetCellRes(uint16_t* const CellRes);
  inline void GetCellSOC(uint16_t* const CellSOC);
  inline void SetBalancePWM_Nibbles(const uint8_t* BalancePWM_Nibbles);
  inline void GetBalancePWM_Nibbles(uint8_t* const BalancePWM_Nibbles);
  //----------------------------------------------------------------------------------------------------
  inline void SetEcapIMDData(const ecapIMDData_t ecapIMDData);
  inline ecapIMDData_t GetEcapIMDData();
  //---------------------------------------------------------------------------------------------------------
  struct BatteryData_t* GetBatteryDataPrt();
  void initBatteryData();
  BMSState_t getBMS_State();
  //---------------------------------------------------------------------------------------------------------


#endif /* PHANTOM_DRIVERS_INCLUDE_BATTERYDATA_H_ */
