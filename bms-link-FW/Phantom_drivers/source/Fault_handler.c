/*
 * Fault_handler.c
 *
 *  Created on: Jun 23, 2026
 *      Author: tanjo
 */


#include "gio.h"
#include "can.h"
#include "sci.h"
#include "het.h"
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
     if(NewLevel){
         hetREG1->DOUT |= (1 << bit);   // set high
     }
     else {
         hetREG1->DOUT &= ~(1 << bit);   // set high

     }

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

void Debug_GIO_Notification(){
    BMSFaultsData.BMS_Faults |= 1U << DEBUG_FLAG;
    Fault_Handler();
}
void IMD_FAULT_GIO_Notification(){
    BMSFaultsData.IMD_Faults = 0xF ;
    BMSFaultsData.Isolation_Faults = 0xF ;
    Fault_Handler();
}
void BMS_FAULT_GIO_Notification(){

}
void START_CHARGING_GIO_Notification(){

}
void gioNotification(gioPORT_t *port, uint32 bit){
    if(port != gioPORTA){return;}

    switch(bit){
        case GIO_DEGUBING_BIT1      : Debug_GIO_Notification(); break;
        case GIO_IMD_FAULT_BIT      : IMD_FAULT_GIO_Notification(); break;
        case GIO_BMS_FAULT_BIT      : BMS_FAULT_GIO_Notification(); break;
        case GIO_START_CHARGING_BIT : START_CHARGING_GIO_Notification(); break;
    }
}
//----------------------------------------------------------------------------------------------------
//bool SendFault_Cans(const BMSFaultsData_t* const FaultsFromBMS){
//    uint8_t data[BMS2VCU_FAULT_MSG_LEN_BYTES];
//    data[0] = FaultsFromBMS->IMD_Faults;
//    data[1] = FaultsFromBMS->Isolation_Faults;
//    data[2] = FaultsFromBMS->BMS_Faults;
//    data[3] = FaultsFromBMS->Charger_Faults;
//    data[4] = FaultsFromBMS->Slave_Faults>>8U;
//    data[5] = FaultsFromBMS->Slave_Faults>>0U;
//
//    return transmit_BMS2VCU_FAULT(data);
//}
//bool GetFault_Cans(BMSFaultsData_t* const FaultsFromBMS){
//    uint8_t data[BMS2VCU_FAULT_MSG_LEN_BYTES];
//    bool returnVal = receive_BMS2VCU_FAULT(data);
//
//    FaultsFromBMS->IMD_Faults       = data[0];
//    FaultsFromBMS->Isolation_Faults  = data[1];
//    FaultsFromBMS->BMS_Faults       = data[2];
//    FaultsFromBMS->Charger_Faults   = data[3];
//    FaultsFromBMS->Slave_Faults     = ((uint16_t)data[4]<<8U) | data[5];
//
//    return returnVal;
//}

bool SendFault_Cans(const BMSFaultsData_t* const data){
    const uint32_t returnval =  can_transmit_data(BMS2ALL_FAULT, data, sizeof(*data));
    return returnval;
}
bool GetFault_Cans(BMSFaultsData_t* const data){
    const uint32_t returnval =  can_receive_data(BMS2ALL_FAULT, data, sizeof(*data));
    return returnval;
}
//----------------------------------------------------------------------------------------------------
bool AnyBMSFaults(){
    return (BMSFaultsData.BMS_Faults != BMS_NO_FALUT_VAL);
}
bool AnySlaveFaults(){
    return (BMSFaultsData.Slave_Faults != SLAVE_NO_FAULT_VAL);
}
bool AnyChargerFaults(){
    return (BMSFaultsData.Charger_Faults != CHARGER_NO_FALUT_VAL);
}
bool AnyIMDFaults(){
    return (BMSFaultsData.IMD_Faults != IMD_NO_FAULT_VAL);
}
bool AnyIsolationFaults(){
    return (BMSFaultsData.Isolation_Faults != ISOLATION_NO_FAULT_VAL);
}

bool AnyFaults(){
    return AnyBMSFaults() || AnySlaveFaults() || AnyIMDFaults() || AnyIsolationFaults() || AnyChargerFaults();
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
}
void ClearAllBMSFaults(){
    BMSFaultsData.BMS_Faults = BMS_NO_FALUT_VAL;
}
void ClearIMDFault(){
    BMSFaultsData.IMD_Faults = IMD_NO_FAULT_VAL;
}
void ClearAllFaults(){
    ClearIMDFault();
    ClearAllSlaveFaults();
    ClearAllBMSFaults();
    gioSetBitHelper(GIO_BMS_FAULT_BIT , GIO_HIGH);

}
//----------------------------------------------------------------------------------------------------
void Fault_Handler(){
    if(!AnyFaults()){
        return;
    }
    gioSetBitHelper(GIO_BMS_FAULT_BIT , GIO_LOW);

    uint16_t DCC[NUMBER_OF_CELLS];
    memset(DCC,0,NUMBER_OF_CELLS * sizeof(uint16_t));
    SetConfig_DCC(DCC);
    Write_CFGR();

    ShutDownCharger_Fault();
}
//void IMD_Fault_Handler(){
//    const bool hasFault = AnyIMDFaults();
//    Fault_Handler();
//}
//void Slave_Fault_Handler(){
//    const bool hasFault = AnySlaveFaults();
//    Fault_Handler();
//}
//void BMS_Fault_Handler(){
//    const bool hasFault = AnyBMSFaults();
//    Fault_Handler();
//}
//----------------------------------------------------------------------------------------------------
 void SetAllSlaveFaults(const uint16_t NewSlaveFaults){
     BMSFaultsData.Slave_Faults = NewSlaveFaults;

     Fault_Handler();
 }
 void AddSlaveFaults(const uint16_t NewSlaveFaults){
     BMSFaultsData.Slave_Faults |= NewSlaveFaults;

     Fault_Handler();
 }
 void SetSlaveFault_bool(const bool Val, const Slave_Faults Fault){
     BMSFaultsData.Slave_Faults &= ~(1U << Fault);
     BMSFaultsData.Slave_Faults |= (uint32_t)Val << Fault;

     Fault_Handler();
 }
 void SetSlaveFault_bool_HIGH(const Slave_Faults Fault){
     BMSFaultsData.Slave_Faults |= 1U << Fault;

     Fault_Handler();
 }
  //----------------------------------------------------------------------------------------------------
  void SetBMSFault(const uint8_t Val, const uint8_t bitSize, const BMS_Faults Fault){
 //     const uint8_t mask = (1U<<bitSize)-1;
      const uint8_t mask = MINUS1(bitSize);
      uint8_t Val_mask   = Val & mask;

      BMSFaultsData.BMS_Faults &= ~((uint32_t)mask    << Fault);
      BMSFaultsData.BMS_Faults |=  (uint32_t)Val_mask << Fault;

      Fault_Handler();
  }
  void AddBMSFaults(const uint16_t NewBMSFaults){
      BMSFaultsData.BMS_Faults |= NewBMSFaults;

      Fault_Handler();
  }
   void SetBMSFault_bool(const bool Val, const BMS_Faults Fault){
      BMSFaultsData.BMS_Faults &= ~(1U << Fault);
      BMSFaultsData.BMS_Faults |= (uint32_t)Val << Fault;

      Fault_Handler();
  }
   void SetBMSFault_bool_HIGH(const BMS_Faults Fault){
      BMSFaultsData.BMS_Faults |= 1U << Fault;

      Fault_Handler();
  }
 //----------------------------------------------------------------------------------------------------
  void SetIMDFaults(const uint8_t IMDState, const uint8_t IsolationState){
      BMSFaultsData.IMD_Faults = IMDState ;
      BMSFaultsData.Isolation_Faults = IsolationState ;

      Fault_Handler();
  }
  //----------------------------------------------------------------------------------------------------


void init_BMS_Faults(){
    Gio_State_t IMD_F = gioGetBitHelper(GIO_IMD_FAULT_BIT);
    if ((IMD_F & 1) == 0){
        IMD_FAULT_GIO_Notification();
    }
    ClearAllFaults();
}
