/*
 * Fault_handler.h
 *
 *  Created on: Jun 23, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_Fault_HANDLER_H_
#define PHANTOM_DRIVERS_INCLUDE_Fault_HANDLER_H_

#include "gio.h"
#include "reg_gio.h"

//--------------------------------------------------------------------------
#define GIO_PORT_A gioPORTA
#define GIO_PORT_B gioPORTB

#define GIO_MAX_PERIOD_US 3
//--------------------------------------------------------------------------
#define SLAVE_NO_FAULT_VAL      0
#define ISOLATION_NO_FAULT_VAL  0
#define IMD_NO_FAULT_VAL        FALSE
#define BMS_NO_FALUT_VAL        0
//--------------------------------------------------------------------------
enum Gio_Bits { GIO_DEGUBING_BIT1        = 2,

                GIO_IMD_FAULT_BIT        = 6,
                GIO_BMS_FAULT_BIT        = 7,

                GIO_START_CHARGING_BIT   = 8,
};
//#define (sizeof(Gio_Bits)/sizeof(Gio_Bits[1]))
typedef enum{GIO_LOW, GIO_HIGH, GIO_FALLING_EGDE, GIO_RISING_EGDE} Gio_State_t;
//--------------------------------------------------------------------------
typedef struct{
        uint8_t  IMD_Faults;
        uint8_t  IsolationFaults;
        uint16_t Slave_Faults;
        uint8_t  BMS_Faults;
        uint8_t Charger_Faults;
}BMSFaultsData_t;
//--------------------------------------------------------------------------


Gio_State_t gioGetBitHelper(const uint8_t bit);
Gio_State_t gioSetBitHelper(const uint8_t bit, const Gio_State_t NewState);
Gio_State_t gioToggleBitHelper(const uint8_t bit);
//--------------------------------------------------------------------------
typedef enum {BAD_OV_flags, BAD_UV_flags, BAD_THSD, BAD_MUXFAIL, BAD_ITMP, BAD_VA, BAD_VD, BAD_REF2ND, BAD_SLAVE_CONNECTION_FLAG}Slave_Faults;
typedef enum {BAD_CURRENT_FLAG}BMS_Faults;
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

 void SetBMSFault(const uint8_t Val, const uint8_t bitSize, const BMS_Faults Fault);
//----------------------------------------------------------------------------------------------------
 void SetIMDFaults(const uint8_t IMDState, const uint8_t IsolationState);
 void ClearIMDFaults();
 bool AnyIMDFaults();
//--------------------------------------------------------------------------
bool clearIMDFaults();
bool clearSlaveFaults();
bool clearOtherBMSFaults();
bool clear_AllFault();
void init_BMS_Faults();
//--------------------------------------------------------------------------
bool AnyFaults();
//--------------------------------------------------------------------------

BMSFaultsData_t BMSFaultsData;


#endif /* PHANTOM_DRIVERS_INCLUDE_Fault_HANDLER_H_ */
