/*
 * BatteryData.c
 *
 *  Created on: Jun 8, 2026
 *      Author: tanjo
 */


#include <stdint.h>
#include <stdbool.h>
#include "string.h"

#include "SlaveCommunation_Hardware.h"
//#include "BatteryCell_Hardware.h"
#include "FullBattery_Hardware.h"

#include "PhantomHelpers.h"
#include "BatteryData.h"
#include "RowBuf.h"

  struct BatteryData_t BatteryData;
//----------------------------------------------------------------------------------------------------
//float Slave_ADC2Volt(const uint16_t ADC_Word){
//    VOLTS2SLAVE_ADC(ADC_Word);
//}
//inline uint16_t Slave_Volt2ADC(const float ADC_Volt){
//    uint16_t ADC_Value = (ADC_Volt - ADC_OFFSET_VOLTS)/ADC2VOLTS;
//    return ADC_Value;
//}
//inline uint16_t Slave_Volt2ADC(const float ADC_Volt){
//    uint16_t ADC_Value = (ADC_Volt - ADC_OFFSET_VOLTS)/ADC2VOLTS;
//    return ADC_Value;
//}
//inline float Slave_ADC2Volt(const uint16_t ADC_Word){
//    uint16_t ADC_Round = round16(ADC_Word, 16-ADC_RESOLUTION_BIT);
//    float Volt = ADC_Round * ADC2VOLTS + ADC_OFFSET_VOLTS;
//    return Volt;
//}
//inline float Slave_ADC2Celcius(const uint16_t ADC){
//    float Volts = Slave_ADC2Volt(ADC);
//    float Kelvin = Volts / ITMP_MILLI_VOLTS_2_CELCIUS * 1000;
//    float Celcius = Kelvin - ITMP_KELVIN_2_CELCIUS;
//    return Celcius;
//}
//void Slave_ADC2Volt_arr(const uint16_t* ADC_Words, float* Volts, const uint16_t len){
//    int i;
//    for(i=0; i<len; i++)
//        Volts[i] = Slave_ADC2Volt(ADC_Words[i]);
//}

//inline uint16_t BatteryCurrent2Voltage_16(const uint16_t current){
//    const uint16_t voltage = current;
//    return voltage;
//}
//inline uint16_t BatteryCurrent2Voltage_f(const uint16_t current){
//    const uint16_t voltage_16 = BatteryCurrent2Voltage_16(current);
//    const uint16_t voltage_f = voltage_16;
//
//    return voltage_f;
//}
//inline float CellVolts2SoC(const uint16_t CellVolt){
//    const uint16_t CellVolt_offset = CellVolt - CELL_VOLT_0_FULL;
//    const float CellSOC = CellVolt_offset /(CELL_SOC_RANGE) * 100;
//    return CellSOC;
//}
 //----------------------------------------------------------------------------------------------------
//inline void SetChargingStatus(const bool NewStat){
//     BatteryData.Charging = NewStat;
// }
//inline bool GetChargingStatus(){
//     return BatteryData.Charging;
// }
//// void CheckChargingSatusTask(){
////     SetChargingStatus(TRUE);
//// }
//----------------------------------------------------------------------------------------------------
inline void SetEcapIMDData(const ecapIMDData_t ecapIMDData){
    BatteryData.ecapIMDData = ecapIMDData;
}
inline ecapIMDData_t GetEcapIMDData(){
    return BatteryData.ecapIMDData;
}
 //----------------------------------------------------------------------------------------------------
 inline const uint16_t* GetCellVoltReadPrt(const uint8_t index){
     return (const uint16_t*)BatteryData.CellVolt[index];
 }
 inline const uint16_t* GetCellTempReadPrt(const uint8_t index){
     return (const uint16_t*)BatteryData.CellTemp[index];
 }
 inline const uint16_t* GetCellResReadPrt(const uint8_t index){
     return (const uint16_t*)BatteryData.CellRes[index];
 }
 inline const uint16_t* GetCellSOCReadPrt(const uint8_t index){
     return (const uint16_t*)BatteryData.CellSOC[index];
 }
 inline const uint8_t* GetBalancePWM_NibblesReadPrt(){
     return (const uint8_t*)BatteryData.BalancePWM_Nibbles;
 }

 //----------------------------------------------------------------------------------------------------
 inline uint16_t* GetCellVoltWritePrt(){
     shiftRowBufElements(BatteryData.CellVolt, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_VOLT_SAMPLES_SAVED);
     return BatteryData.CellVolt[0];
 }
 inline uint16_t* GetCellTempWritePrt(){
     shiftRowBufElements(BatteryData.CellTemp, sizeof(uint16_t)*NUMBER_OF_THERMISTORS_TOTAL, NUMBER_OF_TEMP_SAMPLES_SAVED);
     return BatteryData.CellTemp[0];
 }
 inline uint8_t* GetBalancePWM_NibblesWritePrt(){
     return BatteryData.BalancePWM_Nibbles;
 }
 inline uint16_t* GetCellResWritePrt(){
     shiftRowBufElements(BatteryData.CellRes, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_RES_SAMPLES_SAVED);
     return BatteryData.CellRes[0];
 }
 inline uint16_t* GetCellSOCWritePrt(){
     shiftRowBufElements(BatteryData.CellSOC, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_SOC_SAMPLES_SAVED);
     return BatteryData.CellSOC[0];
 }
 //----------------------------------------------------------------------------------------------------
 inline void SetCellVolt(const uint16_t* CellVolt){
     prePendRowIntoBuf(BatteryData.CellVolt, CellVolt, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_VOLT_SAMPLES_SAVED);

     // check falut
 }
 inline void SetCellTemp(const uint16_t* CellTemp){
     prePendRowIntoBuf(BatteryData.CellTemp, CellTemp, sizeof(uint16_t)*NUMBER_OF_THERMISTORS_TOTAL, NUMBER_OF_TEMP_SAMPLES_SAVED);
     // check falut
 }
 inline void SetCellRes(const uint16_t *CellRes){
     prePendRowIntoBuf(BatteryData.CellRes, CellRes, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_RES_SAMPLES_SAVED);
 }
 inline void SetCellSOC(const uint16_t *CellSOC){
     prePendRowIntoBuf(BatteryData.CellSOC, CellSOC, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES, NUMBER_OF_SOC_SAMPLES_SAVED);
 }
 //----------------------------------------------------------------------------------------------------
 inline void GetCellVolt(uint16_t* const CellVolt){
     peekRowBuf(BatteryData.CellVolt, CellVolt, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES);
 }
 inline void GetCellTemp(uint16_t* const CellTemp){
     peekRowBuf(BatteryData.CellTemp, CellTemp, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES);
 }
 inline void GetCellRes(uint16_t* const CellRes){
     peekRowBuf(BatteryData.CellRes, CellRes, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES);
 }
 inline void GetCellSOC(uint16_t* const CellSOC){
     peekRowBuf(BatteryData.CellSOC, CellSOC, sizeof(uint16_t)*NUMBER_OF_CELLS_SERIES);
 }
 //----------------------------------------------------------------------------------------------------
void SetBalancePWM_Nibbles(const uint8_t* BalancePWM_Nibbles){
    memcpy(BatteryData.BalancePWM_Nibbles, BalancePWM_Nibbles, NUMBER_OF_CELLS_SERIES/2 * sizeof(uint8_t));
}
void GetBalancePWM_Nibbles(uint8_t* const BalancePWM_Nibbles){
    memcpy(BalancePWM_Nibbles, BatteryData.BalancePWM_Nibbles, NUMBER_OF_CELLS_SERIES/2 * sizeof(uint8_t));
}

//----------------------------------------------------------------------------------------------------
  inline uint16_t GetAvgCellVolt_SlaveADC(){
      return array16_avg(GetCellVoltReadPrt(0), NUMBER_OF_CELLS_SERIES);
  }
  inline uint16_t GetMaxCellVolt_SlaveADC(){
      return array16_max(GetCellVoltReadPrt(0), NUMBER_OF_CELLS_SERIES);
  }
  inline uint16_t GetMinCellVolt_SlaveADC(){
      return array16_min(GetCellVoltReadPrt(0), NUMBER_OF_CELLS_SERIES);
  }
  inline float GetAvgCellVolt_float(){
      return SLAVE_ADC2VOLTS(GetAvgCellVolt_SlaveADC());
  }
  inline float GetMaxCellVolt_float(){
      return SLAVE_ADC2VOLTS(GetMaxCellVolt_SlaveADC());
  }
  inline float GetMinCellVolt_float(){
      return SLAVE_ADC2VOLTS(GetMinCellVolt_SlaveADC());
  }
  //----------------------------------------------------------------------------------------------------
  inline uint16_t GetAvgCellTemp_SlaveADC(){
      return array16_avg(GetCellTempReadPrt(0), NUMBER_OF_THERMISTORS_TOTAL);
  }
  inline uint16_t GetMaxCellTemp_SlaveADC(){
      return array16_max(GetCellTempReadPrt(0), NUMBER_OF_THERMISTORS_TOTAL);
  }
  inline uint16_t GetMinCellTemp_SlaveADC(){
      return array16_min(GetCellTempReadPrt(0), NUMBER_OF_THERMISTORS_TOTAL);
  }
  inline float GetAvgCellTemp_float(){
      return 0;
  }
  inline float GetMaxCellTemp_float(){
      return 0;
  }
  inline float GetMinCellTemp_float(){
      return 0;
  }
  //----------------------------------------------------------------------------------------------------
  const FullBatteryData_t* getFullBatteryData(){
    return (const FullBatteryData_t*)&BatteryData.FullBatteryData;
  }
  uint16_t getFullBatteryData_Volts(){
    return BatteryData.FullBatteryData.Volt;
  }
  uint16_t getFullBatteryData_SOC(){
    return BatteryData.FullBatteryData.SOC;
  }
  uint16_t getFullBatteryData_Current(){
    return BatteryData.FullBatteryData.Current;
  }
  uint16_t getFullBatteryData_Res(){
    return BatteryData.FullBatteryData.Res;
  }
  //----------------------------------------------------------------------------------------------------
  void setFullBatteryData_Volts(const uint16_t Volt){
    BatteryData.FullBatteryData.Volt = Volt;
  }
  void setFullBatteryData_SOC(const uint16_t SOC){
    BatteryData.FullBatteryData.SOC = SOC;
  }
  void setFullBatteryData_Current(const uint16_t Current){
    BatteryData.FullBatteryData.Current = Current;
  }
  void setFullBatteryData_Res(const uint16_t Res){
    BatteryData.FullBatteryData.Res = Res;
  }
  void setFullBatteryData(const FullBatteryData_t * FullBatteryData){
    memcpy(&BatteryData.FullBatteryData, FullBatteryData, sizeof(FullBatteryData_t));
  }
  void setFullBatteryData_split(const uint16_t Volt, const uint16_t Current, const uint16_t SOC, const uint16_t Res){
      setFullBatteryData_Volts(Volt);
      setFullBatteryData_SOC(SOC);
      setFullBatteryData_Current(Current);
      setFullBatteryData_Res(Res);
  }

//---------------------------------------------------------------------------
void setBMS_State(const BMSState_t State){
    BatteryData.BMS_State = State;
}
BMSState_t getBMS_State(){
    return BatteryData.BMS_State;
}


void BMS_StateMachine(const bool shouldCharge, const bool clearFault, const bool RunBMS, const bool VCUFault, const bool hasFault){
    //TODO
    const BMSState_t currentState = getBMS_State();
    BMSState_t newState = BMS_RUNNING;

    if (hasFault){
        newState = BMS_FAULT;

    }
    switch(currentState){
        case BMS_RUNNING:{

        }
        case BMS_CHARGING:{}
        case BMS_DONE_CHARGING:{}
        case BMS_DISCHARGING:{}
        case BMS_DONE_DISCHARGING:{}
        case BMS_SLEEPING:{}
        case BMS_IDLE:{}
        case BMS_FAULT:{}

    }
    setBMS_State(newState);
}
  //----------------------------------------------------------------------------------------------------

  struct BatteryData_t* GetBatteryData(){
      return &BatteryData;
  }
  void initBatteryData(){
      BatteryData.BMS_State = BMS_CHARGING;

  }
