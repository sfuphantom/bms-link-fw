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
//#include "charger_can.h"
//#include "vcu_can.h"
#include "SlaveCommunation_Hardware.h"
#include "SlaveCommunation_Functions.h"

#include "Phantom_Can.h"




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
bool SendFault_Cans(const BMSFaultsData_t* const FaultsFromBMS){
    uint8_t data[BMS2VCU_FAULT_MSG_LEN_BYTES];
    data[0] = FaultsFromBMS->IMD_Faults;
    data[1] = FaultsFromBMS->IsolationFaults;
    data[2] = FaultsFromBMS->BMS_Faults;
    data[3] = FaultsFromBMS->Charger_Faults;
    data[4] = FaultsFromBMS->Slave_Faults>>8U;
    data[5] = FaultsFromBMS->Slave_Faults>>0U;

    return transmit_BMS2VCU_FAULT(data);
}
bool GetFault_Cans(BMSFaultsData_t* const FaultsFromBMS){
    uint8_t data[BMS2VCU_FAULT_MSG_LEN_BYTES];
    bool returnVal = receive_BMS2VCU_FAULT(data);

    FaultsFromBMS->IMD_Faults       = data[0];
    FaultsFromBMS->IsolationFaults  = data[1];
    FaultsFromBMS->BMS_Faults       = data[2];
    FaultsFromBMS->Charger_Faults   = data[3];
    FaultsFromBMS->Slave_Faults     = ((uint16_t)data[4]<<8U) | data[5];

    return returnVal;
}
//----------------------------------------------------------------------------------------------------
bool AnyBMSFaults(){
    return (BMSFaultsData.BMS_Faults != BMS_NO_FALUT_VAL);
}
bool AnySlaveFaults(){
    return (BMSFaultsData.Slave_Faults != SLAVE_NO_FAULT_VAL);
}
bool AnyIMDFaults(){
    return (BMSFaultsData.IMD_Faults != IMD_NO_FAULT_VAL);
}
bool AnyFaults(){
    return AnyBMSFaults() || AnySlaveFaults() || AnyIMDFaults();
}
//----------------------------------------------------------------------------------------------------
uint16_t GetAllSlaveFaults(){
    return BMSFaultsData.Slave_Faults;
}
uint8_t GetIMDFaults(){
    return BMSFaultsData.IMD_Faults;
}
void GetIMDFaults_slip(uint8_t *const IMDState, uint8_t *const IsolationState){
    *IMDState       = (BMSFaultsData.IMD_Faults>>0) & 0x7;
    *IsolationState = (BMSFaultsData.IMD_Faults>>3) & 0x7;

}
bool GetBMSFault_bool(const BMS_Faults Fault){
   uint16_t Val = BMSFaultsData.BMS_Faults & ~(1U << Fault);
   return (Val != 0);
}
bool GetSlaveFault_bool(const Slave_Faults Fault){
   uint16_t Val = BMSFaultsData.Slave_Faults & ~(1U << Fault);
   return (Val != 0);
}
//----------------------------------------------------------------------------------------------------
void ClearAllSlaveFaults(){
    BMSFaultsData.Slave_Faults = SLAVE_NO_FAULT_VAL;

    gioSetBitHelper(GIO_BMS_FAULT_BIT , GIO_LOW);
}
void ClearAllBMSFaults(){
    BMSFaultsData.BMS_Faults = BMS_NO_FALUT_VAL;

    gioSetBitHelper(GIO_BMS_FAULT_BIT , GIO_LOW);
}
void ClearIMDFault(){
    BMSFaultsData.IMD_Faults = IMD_NO_FAULT_VAL;

    gioSetBitHelper(GIO_IMD_FAULT_BIT , GIO_LOW);

}
void ClearAllFaults(){
    ClearIMDFault();
    ClearAllSlaveFaults();
    ClearAllBMSFaults();
}
//----------------------------------------------------------------------------------------------------
void Fault_Handler(const uint8_t GIO_bit, const bool hasFault){
    if(!hasFault){
        return;
    }
    gioSetBitHelper(GIO_bit , GIO_HIGH);

    uint16_t DCC[NUMBER_OF_CELLS];
    memset(DCC,0,NUMBER_OF_CELLS * sizeof(uint16_t));
    SetConfig_DCC(DCC);
    Write_CFGR();

    ShutDownCharger_Fault();
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
     BMSFaultsData.Slave_Faults = NewSlaveFaults;

     Slave_Fault_Handler();
 }
 void AddSlaveFaults(const uint16_t NewSlaveFaults){
     BMSFaultsData.Slave_Faults |= NewSlaveFaults;

     Slave_Fault_Handler();
 }
 void SetSlaveFault_bool(const bool Val, const Slave_Faults Fault){
     BMSFaultsData.Slave_Faults &= ~(1U << Fault);
     BMSFaultsData.Slave_Faults |= (uint32_t)Val << Fault;

     Slave_Fault_Handler();
 }
 void SetSlaveFault_bool_HIGH(const Slave_Faults Fault){
     BMSFaultsData.Slave_Faults |= 1U << Fault;

     Slave_Fault_Handler();
 }
  //----------------------------------------------------------------------------------------------------
  void SetBMSFault(const uint8_t Val, const uint8_t bitSize, const BMS_Faults Fault){
 //     const uint8_t mask = (1U<<bitSize)-1;
      const uint8_t mask = MINUS1(bitSize);
      uint8_t Val_mask   = Val & mask;

      BMSFaultsData.BMS_Faults &= ~((uint32_t)mask    << Fault);
      BMSFaultsData.BMS_Faults |=  (uint32_t)Val_mask << Fault;

      BMS_Fault_Handler();
  }
  void AddBMSFaults(const uint16_t NewBMSFaults){
      BMSFaultsData.BMS_Faults |= NewBMSFaults;

      BMS_Fault_Handler();
  }
   void SetBMSFault_bool(const bool Val, const BMS_Faults Fault){
      BMSFaultsData.BMS_Faults &= ~(1U << Fault);
      BMSFaultsData.BMS_Faults |= (uint32_t)Val << Fault;

      BMS_Fault_Handler();
  }
   void SetBMSFault_bool_HIGH(const BMS_Faults Fault){
      BMSFaultsData.BMS_Faults |= 1U << Fault;

      BMS_Fault_Handler();
  }
 //----------------------------------------------------------------------------------------------------
  void SetIMDFaults(const uint8_t IMDState, const uint8_t IsolationState){
      const uint8_t fault = (uint8_t)IMDState |  (uint8_t)IsolationState<<3U;
      BMSFaultsData.IMD_Faults = fault ;

      IMD_Fault_Handler();
  }
  //----------------------------------------------------------------------------------------------------


void init_BMS_Faults(){
    gioInit();
    ClearAllFaults();
}
