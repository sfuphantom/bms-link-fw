/*
 * Balance.c
 *
 *  Created on: Aug 31, 2026
 *      Author: tanjo
 */
#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include "SlaveCommunation_Hardware.h"
#include "BatteryCell_Hardware.h"
#include "FullBattery_Hardware.h"
#include "PhantomHelpers.h"

#include "BatteryData.h"
#include "Balance.h"


//static inline uint8_t Percent2Nibble(uint8_t){
//
//}
//////////////////////////////////////////////////////////////////////////////////////////
static inline uint8_t CalcTargetPWMNibble_SlowDrain(const uint16_t cellVolt){
    const uint16_t offset = cellVolt - CELL_CHARGING_TARGET_SLAVE_ADC;
    const uint16_t scaled = offset << 4;
    const uint8_t Nibble = scaled / SLOW_DRAIN_BAND_SLAVE_ADC;

    return MIN_VAL(Nibble, 0xF);
}
static uint8_t CalcTargetPWMNibble(const uint16_t cellVolt){

    if(cellVolt < CELL_CHARGING_TARGET_SLAVE_ADC)
        return 0x0;


    else if(cellVolt > FULL_DRAIN_THESHOLD_SLAVE_ADC)
        return 0xF;

    else
        return CalcTargetPWMNibble_SlowDrain(cellVolt);
}
//----------------------------------------------------------------------------------
static inline uint8_t CalcBalancePWMNibble_SlowDrain(const uint16_t V_diff){
    const uint16_t offset = V_diff - BALANCE_NO_DRAIN_THESHOLD_SLAVE_ADC_DIFF;
    const uint16_t scaled = offset << 4;
    const uint8_t Nibble = scaled / BALANCE_SLOW_DRAIN_BAND_SLAVE_ADC_DIFF;

    return MIN_VAL(Nibble, 0xF);
}

static uint8_t CalcBalancePWMNibble(const uint16_t V_diff, const uint8_t Pre_PWMNibble){

    if(V_diff < BALANCE_NO_DRAIN_THESHOLD_SLAVE_ADC_DIFF)
        return 0x0;

    else if(V_diff > BALANCE_FULL_DRAIN_THESHOLD_SLAVE_ADC_DIFF)
        return 0xF;

    else if(Pre_PWMNibble == 0 && V_diff > BALANCE_SLOW_DRAIN_THESHOLD_SLAVE_ADC_DIFF)
        return 0x0;

    else
        return CalcBalancePWMNibble_SlowDrain(V_diff);
}
//----------------------------------------------------------------------------------
 void GetDrainPWMNibbles(const uint16_t allVolts[NUMBER_OF_CELLS_SERIES], uint8_t * PWMNibbles){
      uint8_t i, j;
      const uint16_t min = array16_min(allVolts, NUMBER_OF_CELLS_SERIES);

      for(i=0; i<NUMBER_OF_CELLS_SERIES/2; i++){

          uint8_t byte = 0;
          uint8_t * const PWMByte_prt = PWMNibbles++;

          for(j=0; j<NIBBLE2BYTES; j++){

              const uint16_t cellVolt = allVolts[i*NIBBLE2BYTES + j];

              const uint8_t targetDrainNibble = CalcTargetPWMNibble(cellVolt);

#if BALANCE_WHILE_CHARGING
              const uint8_t Pre_PWMNibble = ((*PWMByte_prt) >> (j*4)) & 0xF;
              const uint8_t balanceDrainNibble = CalcBalancePWMNibble(cellVolt - min, Pre_PWMNibble);

              const uint8_t Nibble = MAX_VAL(targetDrainNibble, balanceDrainNibble);
#else
              const uint8_t Nibble = targetDrainNibble;
#endif

//              MIN_VAL(Nibble, 0xF);
              byte |= Nibble<<(j*4);
          }
          *PWMByte_prt = byte;
      }

//----------------------------------------------------------------------------------
#if NUMBER_OF_CELLS_SERIES%2 == 1

  uint8_t byte = 0;
  uint8_t * const PWMByte_prt = PWMNibbles++;
  j=0;

  const uint16_t cellVolt = allVolts[i*NIBBLE2BYTES + j];

  const uint8_t targetDrainNibble = CalcTargetPWMNibble(cellVolt);
#if BALANCE_WHILE_CHARGING
  const uint8_t Pre_PWMNibble = ((*PWMByte_prt)) & 0xF;
  const uint8_t balanceDrainNibble = CalcBalancePWMNibble(cellVolt - min, Pre_PWMNibble);

  const uint8_t Nibble = MAX_VAL(targetDrainNibble, balanceDrainNibble);
#else
  const uint8_t Nibble = targetDrainNibble;
#endif

//              MIN_VAL(Nibble, 0xF);
  byte |= Nibble<<(j*4);

  *PWMByte_prt = byte;

#endif
  }
