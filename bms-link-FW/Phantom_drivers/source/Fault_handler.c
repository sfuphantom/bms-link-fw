/*
 * Fault_handler.c
 *
 *  Created on: Jun 23, 2026
 *      Author: tanjo
 */


//#include "gio.h"
//#include "can.h"
//#include "sci.h"
#include "het.h"
#include "string.h"
#include "spi_helpers.h"
#include "sci_helpers.h"
#include "GIO_helpers.h"


#include "Fault_handler.h"
#include "PhantomHelpers.h"

#include "BatteryData.h"
#include "Charger.h"
#include "Fans.h"
//#include "charger_can.h"
//#include "vcu_can.h"
#include "SlaveCommunation_Hardware.h"
#include "SlaveCommunation_Functions.h"

#include "Phantom_Can.h"
////----------------------------------------------------------------------------------------------------
void Debug_GIO_Notification(){
    BMSFaultsData.BMS_Faults |= 1U << DEBUG_FLAG;
    Fault_Handler();
}
void IMD_FAULT_GIO_Notification(){
    BMSFaultsData.IMD_Faults.IMDState= Undefined_Fault;
    Fault_Handler();
}
////----------------------------------------------------------------------------------------------------

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

bool AnyIMDFaults(){
    if (BMSFaultsData.IMD_Faults.IMDState != IMD_NO_FAULT_VAL){
        return TRUE;
    }
    if (BMSFaultsData.IMD_Faults.IsolationState < ISOLATION_MIN_NO_FAULT_VAL){
        return TRUE;
    }
    if (BMSFaultsData.IMD_Faults.IsolationState > ISOLATION_MAX_NO_FAULT_VAL){
        return TRUE;
    }

    return FALSE;
}


bool AnyFaults(){
    return AnyBMSFaults() || AnySlaveFaults() || AnyIMDFaults();
}
//----------------------------------------------------------------------------------------------------
BMSFaultsData_t* GetBMSFaultsData(){
    return &BMSFaultsData;
}
//----------------------------------------------------------------------------------------------------

uint16_t GetAllSlaveFaults(){
    return BMSFaultsData.Slave_Faults;
}
IMDData_t GetIMDFaults(){
    return BMSFaultsData.IMD_Faults;
}
//void GetIMDFaults_slip(IMDStateEnum *const IMDState, uint8_t *const IsolationState){
//    *IMDState       = BMSFaultsData.IMD_Faults.IMDState;
//    *IsolationState = BMSFaultsData.IMD_Faults.IsolationState;
//
//}
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
//void ClearIMDFault(){
//    BMSFaultsData.IMD_Faults.IMDState       = IMD_NO_FAULT_VAL;
//}
void ClearAllFaults(){
//    ClearIMDFault();
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

//    uint16_t DCC[NUMBER_OF_CELLS];
//    memset(DCC,0,NUMBER_OF_CELLS * sizeof(uint16_t));
//    SetConfig_DCC(DCC);
//    Write_CFGR();

    SetAllFansDuty(99);
    ShutDownCharger_Fault();
    SendFault_Cans(&BMSFaultsData);

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
  void SetIMDFaults(const IMDData_t * const IMDdata){
      BMSFaultsData.IMD_Faults = *IMDdata ;

      Fault_Handler();
  }
  //----------------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------------

void init_BMS_Faults(){
    Gio_State_t IMD_F = gioGetBitHelper(GIO_IMD_FAULT_BIT);
    if ((IMD_F & 1) == 0){
        IMD_FAULT_GIO_Notification();
    }
    ClearAllFaults();
}
