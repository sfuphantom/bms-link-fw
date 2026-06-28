/*
 * Fault_handler.c
 *
 *  Created on: Jun 23, 2026
 *      Author: tanjo
 */


#include "gio.h"
#include "can.h"
#include "sci.h"
#include "string.h"

#include "Fault_handler.h"
#include "PhantomHelpers.h"

#include "BatteryData.h"
#include "Charger.h"
#include "charger_can.h"
#include "vcu_can.h"
#include "SlaveCommunation_Hardware.h"
#include "SlaveCommunation_Functions.h"



static uint8_t gio_past_level;
//--------------------------------------------------------------------------
Gio_State_t gioGetBitHelper(const uint8_t bit){
//     const typeof(gio_past_level) bitMask = 1U << bit;

     const bool NewLevel = (bool)gioGetBit(GIO_PORT_A, bit);

     bool LastLevel;
     gio_past_level = GetAndInsertBit(gio_past_level, bit, NewLevel, &LastLevel);

     const uint8_t NewState = (uint8_t)NewLevel | ((uint8_t)(NewLevel ^ LastLevel)<<1U);

     return (Gio_State_t)NewState;
}

Gio_State_t gioSetBitHelper(const uint8_t bit, const Gio_State_t NewState){
//     const typeof(gio_past_level) bitMask = 1U << bit;

     const bool NewLevel = (bool)NewState & 1U;
     gioSetBit(GIO_PORT_A, bit, NewLevel);

     bool LastLevel;

     gio_past_level = GetAndInsertBit(gio_past_level, bit, NewLevel, &LastLevel);

     const uint8_t State = (uint8_t)NewLevel | ((uint8_t)(NewLevel ^ LastLevel)<<1U);

     return (Gio_State_t)State;
}
Gio_State_t gioToggleBitHelper(const uint8_t bit){
//     const typeof(gio_past_level) bitMask = 1U << bit;

    gioToggleBit(GIO_PORT_A, bit);

    bool NewLevel;
    gio_past_level = InvertAndInsertBit(gio_past_level, bit, &NewLevel);

     const uint8_t State = (uint8_t)NewLevel | (1U)<<1U;

     return (Gio_State_t)State;
}
//----------------------------------------------------------------------------------------------------
bool AnyBMSFaults(){
    return (FaultsData.BMS_Faults != BMS_NO_FALUT_VAL);
}
bool AnySlaveFaults(){
    return (FaultsData.Slave_Faults != SLAVE_NO_FAULT_VAL);
}
bool AnyIMDFaults(){
    return (FaultsData.IMD_Faults != IMD_NO_FAULT_VAL);
}
//----------------------------------------------------------------------------------------------------
uint16_t GetAllSlaveFaults(){
    return FaultsData.Slave_Faults;
}
uint8_t GetIMDFaults(){
    return FaultsData.IMD_Faults;
}
void GetIMDFaults_slip(uint8_t *const IMDState, uint8_t *const IsolationState){
    *IMDState       = (FaultsData.IMD_Faults>>0) & 0x7;
    *IsolationState = (FaultsData.IMD_Faults>>3) & 0x7;

}
bool GetBMSFault_bool(const BMS_Faults Fault){
   uint16_t Val = FaultsData.BMS_Faults & ~(1U << Fault);
   return (Val != 0);
}
bool GetSlaveFault_bool(const Slave_Faults Fault){
   uint16_t Val = FaultsData.Slave_Faults & ~(1U << Fault);
   return (Val != 0);
}
//----------------------------------------------------------------------------------------------------
void ClearAllSlaveFaults(){
    FaultsData.Slave_Faults = SLAVE_NO_FAULT_VAL;

    gioSetBitHelper(GIO_BMS_FAULT_BIT , GIO_LOW);
}
void ClearAllBMSFaults(){
    FaultsData.BMS_Faults = BMS_NO_FALUT_VAL;

    gioSetBitHelper(GIO_BMS_FAULT_BIT , GIO_LOW);
}
void ClearIMDFault(){
    FaultsData.IMD_Faults = IMD_NO_FAULT_VAL;

    gioSetBitHelper(GIO_IMD_FAULT_BIT , GIO_LOW);

}
void ClearAllFaults(){
    ClearIMDFault();
    ClearAllSlaveFaults();
    ClearAllBMSFaults();
}
//----------------------------------------------------------------------------------------------------
void Fault_Handler(const uint8_t GIO_bit, const bool hasFault){
    if(hasFault){
        return;
    }
    gioSetBitHelper(GIO_bit , GIO_HIGH);
    TurnChargerOff();

    uint16_t DCC[NUMBER_OF_CELLS];
    memset(DCC,0,NUMBER_OF_CELLS * sizeof(uint16_t));
    SetConfig_DCC(DCC);
    Write_CFGR();
}
void IMD_Fault_Handler(){
    const bool hasFault = AnyIMDFaults();
    Fault_Handler(GIO_IMD_FAULT_BIT, hasFault);
}
void Slave_Fault_Handler(){
    const bool hasFault = AnySlaveFaults();
    Fault_Handler(GIO_BMS_FAULT_BIT, hasFault);
}
void BMS_Fault_Handler(){
    const bool hasFault = AnyBMSFaults();
    Fault_Handler(GIO_BMS_FAULT_BIT, hasFault);
}
//----------------------------------------------------------------------------------------------------
 void SetAllSlaveFaults(const uint16_t NewSlaveFaults){
     FaultsData.Slave_Faults = NewSlaveFaults;

     Slave_Fault_Handler();
 }
 void AddSlaveFaults(const uint16_t NewSlaveFaults){
     FaultsData.Slave_Faults |= NewSlaveFaults;

     Slave_Fault_Handler();
 }
 void SetSlaveFault_bool(const bool Val, const Slave_Faults Fault){
     FaultsData.Slave_Faults &= ~(1U << Fault);
     FaultsData.Slave_Faults |= (uint32_t)Val << Fault;

     Slave_Fault_Handler();
 }
 void SetSlaveFault_bool_HIGH(const Slave_Faults Fault){
     FaultsData.Slave_Faults |= 1U << Fault;

     Slave_Fault_Handler();
 }
  //----------------------------------------------------------------------------------------------------
  void SetBMSFault(const uint8_t Val, const uint8_t bitSize, const BMS_Faults Fault){
 //     const uint8_t mask = (1U<<bitSize)-1;
      const uint8_t mask = MINUS1(bitSize);
      uint8_t Val_mask   = Val & mask;

      FaultsData.BMS_Faults &= ~((uint32_t)mask    << Fault);
      FaultsData.BMS_Faults |=  (uint32_t)Val_mask << Fault;

      BMS_Fault_Handler();
  }
  void AddBMSFaults(const uint16_t NewBMSFaults){
      FaultsData.BMS_Faults |= NewBMSFaults;

      BMS_Fault_Handler();
  }
   void SetBMSFault_bool(const bool Val, const BMS_Faults Fault){
      FaultsData.BMS_Faults &= ~(1U << Fault);
      FaultsData.BMS_Faults |= (uint32_t)Val << Fault;

      BMS_Fault_Handler();
  }
   void SetBMSFault_bool_HIGH(const BMS_Faults Fault){
      FaultsData.BMS_Faults |= 1U << Fault;

      BMS_Fault_Handler();
  }
 //----------------------------------------------------------------------------------------------------
  void SetIMDFaults(const uint8_t IMDState, const uint8_t IsolationState){
      const uint8_t fault = (uint8_t)IMDState |  (uint8_t)IsolationState<<3U;
      FaultsData.IMD_Faults = fault ;

      IMD_Fault_Handler();
  }
  //----------------------------------------------------------------------------------------------------


void init_BMS_Faults(){
    ClearAllFaults();
}
