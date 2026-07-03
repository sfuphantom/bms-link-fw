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
#include "PhantomHelpers.h"

#include "BatteryData.h"



//----------------------------------------------------------------------------------------------------
inline uint16_t Slave_Volt2ADC(const float ADC_Volt){
    uint16_t ADC_Value = (ADC_Volt - ADC_OFFSET_VOLTS)/ADC2VOLTS;
    return ADC_Value;
}
inline float Slave_ADC2Volt(const uint16_t ADC_Word){
    uint16 ADC_Round = round16(ADC_Word, 16-ADC_RESOLUTION_BIT);
    float Volt = ADC_Round * ADC2VOLTS + ADC_OFFSET_VOLTS;
    return Volt;
}
inline float Slave_ADC2Celcius(const uint16_t ADC){
    float Volts = Slave_ADC2Volt(ADC);
    float Kelvin = Volts / ITMP_MILLI_VOLTS_2_CELCIUS * 1000;
    float Celcius = Kelvin - ITMP_KELVIN_2_CELCIUS;
    return Celcius;
}
void Slave_ADC2Volt_arr(const uint16_t* ADC_Words, float* Volts, const uint16_t len){
    int i;
    for(i=0; i<len; i++)
        Volts[i] = Slave_ADC2Volt(ADC_Words[i]);
}

inline uint16_t BatteryCurrent2Voltage_16(const uint16_t current){
    const uint16_t voltage = current;
    return voltage;
}
inline uint16_t BatteryCurrent2Voltage_f(const uint16_t current){
    const uint16_t voltage_16 = BatteryCurrent2Voltage_16(current);
    const uint16_t voltage_f = voltage_16;

    return voltage_f;
}
inline float CellVolts2SoC(const uint16_t CellVolt){
    const uint16_t CellVolt_offset = CellVolt - CELL_VOLT_0_FULL;
    const float CellSOC = CellVolt_offset /(CELL_SOC_RANGE) * 100;
    return CellSOC;
}
 //----------------------------------------------------------------------------------------------------
inline void SetChargingStatus(const bool NewStat){
     BatteryData.Charging = NewStat;
 }
inline bool GetChargingStatus(){
     return BatteryData.Charging;
 }
// void CheckChargingSatusTask(){
//     SetChargingStatus(TRUE);
// }
 //----------------------------------------------------------------------------------------------------
 inline uint16_t* GetCellVoltReadPrt(){
     return BatteryData.CellVolt;
 }
 inline uint16_t* GetCellTempReadPrt(){
     return BatteryData.CellTemp;
 }
 inline uint16_t* GetRefVolt2ndReadPrt(){
     return BatteryData.RefVolt2nd;
 }
 inline uint16_t* GetCellVoltWritePrt(){
     return BatteryData.CellVolt;
 }
 inline uint16_t* GetCellTempWritePrt(){
     return BatteryData.CellTemp;
 }
 inline uint16_t* GetRefVolt2ndWritePrt(){
     return BatteryData.RefVolt2nd;
 }
 //----------------------------------------------------------------------------------------------------
  inline void SetBatteryCurrentVal(const uint16_t ADC_Val){
      BatteryData.current = ADC_Val;
  }
  inline uint16_t GetBatteryCurrentVal(){
      return BatteryData.current ;
  }
  inline uint16_t GetBatteryCurrentVal_f(){
      return GetBatteryCurrentVal();
  }
  inline uint16_t GetBatteryVoltVal_16(){
      return BatteryCurrent2Voltage_16(BatteryData.current);
  }
  inline float GetBatteryVoltVal_f(){
      return BatteryCurrent2Voltage_f(BatteryData.current);
  }
//----------------------------------------------------------------------------------------------------
//void getCellResistance(float* Resistance){
//    int i;
//    float CellVolts;
//    const uint16_t* allVolts = GetCellVoltPrt();
//    const float current = GetBatteryCurrentVal_f();
//
//    for(i=0;i<NUMBER_OF_CELLS; i++, allVolts++){
//        CellVolts = (float)(*allVolts);
//        Resistance[i] = CellVolts/current;
//    }
//}
//uint16_t getBatteryResistance(){
//    const float current = GetBatteryCurrentVal_f();
//    const float Volts = GetBatteryVoltVal_f();
//    const float Resistance = Volts/current;
//    return Resistance;
//}
//----------------------------------------------------------------------------------------------------
  inline uint16_t GetAvgCellVolt(){
      return array16_avg(GetCellVoltReadPrt(), NUMBER_OF_CELLS);
  }
  inline uint16_t GetMaxCellVolt(){
      return array16_max(GetCellVoltReadPrt(), NUMBER_OF_CELLS);
  }
  inline uint16_t GetMinCellVolt(){
      return array16_min(GetCellVoltReadPrt(), NUMBER_OF_CELLS);
  }
  inline float GetAvgCellVolt_float(){
      return Slave_ADC2Volt(GetAvgCellVolt());
  }
  inline float GetMaxCellVolt_float(){
      return Slave_ADC2Volt(GetMaxCellVolt());
  }
  inline float GetMinCellVolt_float(){
      return Slave_ADC2Volt(GetMinCellVolt());
  }
  inline float GetAvgCellSOC(){
      const uint16_t avgVolts   = GetAvgCellVolt();
      const float avgSOC        = CellVolts2SoC(avgVolts);
      return avgSOC;
  }
  inline float GetMaxCellSOC(){
      const uint16_t maxVolts   = GetMaxCellVolt();
      const float maxSOC        = CellVolts2SoC(maxVolts);
      return maxSOC;
  }
  inline float GetMinCellSOC(){
      const uint16_t minVolts   = GetMinCellVolt();
      const float minSOC        = CellVolts2SoC(minVolts);
      return minSOC;
  }

  //----------------------------------------------------------------------------------------------------
  bool GetCellsUnbalanceState(const uint16_t avg, const uint16_t min){
      static bool Balance_Hysteresis = FALSE;

      const uint16_t minAvgDiff = avg-min;

      if (CELL_BALANCE_TRIGGER_HIGH_VOLTS_ADC < minAvgDiff ){
          Balance_Hysteresis = TRUE;
      }
      else if(CELL_BALANCE_TRIGGER_LOW_VOLTS_ADC > minAvgDiff){
          Balance_Hysteresis = FALSE;
      }
      else{
          Balance_Hysteresis = Balance_Hysteresis;
      }
      return Balance_Hysteresis;
  }
  inline uint8_t BalanceCellNibbleVaule(const uint16 cellVolt, const uint16_t min){

      const uint16_t Vdiff = cellVolt - min;

      if(CELL_BALANCE_THESHOLD_VOLTS_ADC > Vdiff){
          return 0;
      }
      #define DRAIN_BINARY true
      #if DRAIN_BINARY
          return 0xF;
      #else
           uint8_t nibble;

           nibble  = Vdiff + VOLTS_Per_10000_DRAINED_PER_PULSE/2;
           nibble /= VOLTS_Per_10000_DRAINED_PER_PULSE;

           if(nibble < 0xF){
               return nibble;
           }
           else{
               return 0xF;
           }
    #endif
   }

  uint8_t GetBalanceNibbles(uint8_t* BalanceNibbles){
      int i, j;
      uint16_t cellVolt;
      uint8_t nibble, byte;
      uint8_t NumCellsFull = 0;

      const uint16_t* allVolts = GetCellVoltReadPrt();
      const uint16_t min = array16_min(allVolts, NUMBER_OF_CELLS);
      const uint16_t avg = array16_avg(allVolts, NUMBER_OF_CELLS);

      const bool UnBalance = GetCellsUnbalanceState(avg, min);

      for(i=0; i<NUMBER_OF_CELLS/NIBBLE2BYTES; i++){
          for(j=0, byte=0; j<NIBBLE2BYTES; j++, allVolts){
              cellVolt = *allVolts;

              if(cellVolt >= MAX_CELL_CHARGING_VOLTAGE_TARGET){
                 nibble  = 0xF;
                 NumCellsFull++;
              }
              else if(!UnBalance){
                  nibble = 0;
              }
              else{
                  nibble = BalanceCellNibbleVaule(cellVolt, min);
              }

              byte |= nibble<<(j*4);
          }
          BalanceNibbles[i] = byte;
      }
      return NumCellsFull;
  }
  uint8_t GetBalanceDCC(uint16_t* DCC){
  //     const uint8_t Nibbles2Bytes = 2;
       int i, j;
       uint16_t cellVolt;
       uint8_t nibble=0;
       uint8_t NumCellsFull = 0;

       const uint16_t* allVolts = GetCellVoltReadPrt();
       const uint16_t min = array16_min(allVolts, NUMBER_OF_CELLS);
       const uint16_t avg = array16_avg(allVolts, NUMBER_OF_CELLS);

       const bool UnBalance = GetCellsUnbalanceState(avg, min);
//       const uint32_t T = MAX_CELL_CHARGING_VOLTAGE_TARGET;

       for(i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
           *DCC = 0;

           for (j=0;j<CELLS_PER_SLAVE_BOARD; j++, allVolts++){
               cellVolt = *allVolts;

               if(cellVolt > MAX_CELL_CHARGING_VOLTAGE_TARGET){
                  nibble  = 0xF;
                  NumCellsFull++;
               }
               else if(!UnBalance){
                   nibble = 0;
               }
               else{
                   nibble = BalanceCellNibbleVaule(cellVolt, min);
               }

               if(nibble){
                   *DCC |= 1U<<j;
               }
           }

           DCC++;
       }
       return NumCellsFull;
   }
  //----------------------------------------------------------------------------------------------------
void setBMS_State(BMSState_t State){
    BatteryData.BMS_State = State;
}
BMSState_t getBMS_State(){
    return BatteryData.BMS_State;
}
  //----------------------------------------------------------------------------------------------------

  struct BatteryData_t* GetBatteryData(){
      return &BatteryData;
  }
  void initBatteryData(){
      BatteryData.BMS_State = BMS_RUNNING;


      memset(BatteryData.CellTemp     , 0, NUMBER_OF_GPIOS  *sizeof (uint16_t));
      memset(BatteryData.CellVolt     , 0, NUMBER_OF_CELLS  *sizeof (uint16_t));
      memset(BatteryData.RefVolt2nd   , 0, NUMBER_OF_REF_2ND*sizeof (uint16_t));

      BatteryData.current = 0;
  }
