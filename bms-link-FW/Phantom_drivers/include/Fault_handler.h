/*
 * Fault_handler.h
 *
 *  Created on: Jun 23, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_Fault_HANDLER_H_
#define PHANTOM_DRIVERS_INCLUDE_Fault_HANDLER_H_

//#include "gio.h"
//#include "reg_gio.h"

//--------------------------------------------------------------------------
//#define GIO_PORT_A gioPORTA
//#define GIO_PORT_B gioPORTB

//#define GIO_MAX_PERIOD_US 3
//--------------------------------------------------------------------------
//enum Gio_Bits {
//                GIO_DEGUBING_BIT1        = 2,
//
//                GIO_IMD_FAULT_BIT        = 6,
//                GIO_BMS_FAULT_BIT        = 7,
//
//                GIO_START_CHARGING_BIT   = 5,
//
////                GIO_DEGUBING_BIT1        = 25,
////
////                GIO_IMD_FAULT_BIT        = 2,
////                GIO_BMS_FAULT_BIT        = 5,
////
////                GIO_START_CHARGING_BIT   = 7,
//};
//#define (sizeof(Gio_Bits)/sizeof(Gio_Bits[1]))
//typedef enum{GIO_LOW, GIO_HIGH, GIO_FALLING_EGDE, GIO_RISING_EGDE} Gio_State_t;
//--------------------------------------------------------------------------

// ENUMS FOR IMD MESSAGE MAPPING
// IMD State Enum
// message names are taken from IMD Datasheet
typedef enum{
    Normal_Condition,
    Short_Circuit,
    Undervoltage_Condition,
    Speed_Start_Measurement_Good, // related to start-up
    Speed_Start_Measurement_Bad, // related to start-up
    Device_Error,
    Connection_Fault_Earth,
    Bad_Info,
    Undefined_Fault
}IMDStateEnum;

//// Isolation State Enum
//typedef enum{
//    Normal, // When Duty Cycle 5-10%
//    Normal_75, // Usually when Duty Cycle 10-30%
//    Normal_50, // Usually when Duty Cycle 30-60%
//    Normal_25, // Usually when Duty Cycle 60-90%
//    Isolation_Failure, //When Duty Cycle is 90-95%
//    Unknown // Duty Cycle is outside of range (5-95%)
//}IsolationStateEnum;
// IMDData Struct which contains the status of the IMD and Isolation
typedef struct{
    IMDStateEnum IMDState;
//    IsolationStateEnum IsolationState;
    uint8_t IsolationState;
}IMDData_t;


//--------------------------------------------------------------------------
#define SLAVE_NO_FAULT_VAL      0
//#define ISOLATION_NO_FAULT_VAL  Normal
#define ISOLATION_MIN_NO_FAULT_VAL  75
#define ISOLATION_MAX_NO_FAULT_VAL  100

#define IMD_NO_FAULT_VAL        Normal_Condition
#define BMS_NO_FALUT_VAL        0
#define CHARGER_NO_FALUT_VAL    0
//--------------------------------------------------------------------------
typedef struct{
        IMDData_t IMD_Faults;
        uint16_t Slave_Faults;
        uint8_t  BMS_Faults;
//        uint8_t  Charger_Faults;
}BMSFaultsData_t;
//--------------------------------------------------------------------------
//Gio_State_t gioGetBitHelper(const uint8_t bit);
//Gio_State_t gioSetBitHelper(const uint8_t bit, const Gio_State_t NewState);
//Gio_State_t gioToggleBitHelper(const uint8_t bit);
//--------------------------------------------------------------------------
typedef enum {BAD_TEMP_flags, BAD_OV_flags, BAD_UV_flags, BAD_THSD, BAD_MUXFAIL, BAD_ITMP, BAD_VA, BAD_VD, BAD_REF2ND}Slave_Faults;
typedef enum {DEBUG_FLAG, BAD_HV_VOLT_FLAG, CHARGER_COMMS_FAULT, BAD_SLAVE_CONNECTION_FLAG}BMS_Faults;
//----------------------------------------------------------------------------------------------------
 void SetAllSlaveFaults(const uint16_t NewSlaveFaults);
 void AddSlaveFaults(const uint16_t NewSlaveFaults);
 void ClearAllSlaveFaults();
 uint16_t GetAllSlaveFaults();
 bool AnySlaveFaults();
 void SetSlaveFault(const uint8_t Val, const uint8_t bitSize, const Slave_Faults Fault);
 void SetSlaveFault_bool(const bool Val, const Slave_Faults Fault);
 void SetSlaveFault_bool_HIGH(const Slave_Faults Fault);
 bool GetSlaveFault_bool(const Slave_Faults Fault);

 //----------------------------------------------------------------------------------------------------
 void SetBMSFault(const uint8_t Val, const uint8_t bitSize, const BMS_Faults Fault);
 void AddBMSFaults(const uint16_t NewBMSFaults);
  void SetBMSFault_bool(const bool Val, const BMS_Faults Fault);
  void SetBMSFault_bool_HIGH(const BMS_Faults Fault);
 //----------------------------------------------------------------------------------------------------
 void SetIMDFaults(const IMDData_t * const IMDdata);
 void ClearIMDFaults();
 bool AnyIMDFaults();
//--------------------------------------------------------------------------
//bool clearIMDFaults();
bool clearSlaveFaults();
bool clearOtherBMSFaults();
//bool clear_AllFault();
void init_BMS_Faults();
void ClearAllFaults();
//--------------------------------------------------------------------------
bool AnyFaults();
//--------------------------------------------------------------------------
void Fault_Handler();
//--------------------------------------------------------------------------
BMSFaultsData_t* GetBMSFaultsData();
//--------------------------------------------------------------------------
BMSFaultsData_t BMSFaultsData;


#endif /* PHANTOM_DRIVERS_INCLUDE_Fault_HANDLER_H_ */
