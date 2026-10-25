/*
 * Fault_handler.h
 *
 * Fault_Handler() does NOT check for faults, only call it after a fault was recorded.
 * Use the Fault_Raise*() functions, they record the fault, call Fault_Handler() and then
 * run the routine that belongs to that specific fault.
 */
#ifndef PHANTOM_DRIVERS_INCLUDE_FAULT_HANDLER_H_
#define PHANTOM_DRIVERS_INCLUDE_FAULT_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "SlaveCommunation_Hardware.h"
#include "IMD_hardware.h"

//--------------------------------------------------------------------------
// Each value is a bit number in the matching fault word. Add new faults before the COUNT.

typedef enum {
    DEBUG_FLAG, BAD_HV_VOLT_FLAG, CHARGER_COMMS_FAULT, BAD_SLAVE_CONNECTION_FLAG,
    CELL_OVERVOLT_FLAG, CELL_UNDERVOLT_FLAG, OVERTEMP_FLAG, UNDERTEMP_FLAG,
    IMD_FAULT_FLAG, CONTACTOR_FAULT_FLAG, EXTERNAL_FAULT_FLAG, IMD_FAULT,
    NUM_BMS_FAULTS
} BMS_Faults;

//--------------------------------------------------------------------------
// Everything about the faults, 8 bytes max (checked at compile time in the .c)
// FirstFault: FIRST_FAULT_NONE, a BMS_Faults value, or FIRST_FAULT_SLAVE_BIT | a Slave_Faults value
// TripCount:  how many faults were newly set since the last clear (stops at 255)
#define FIRST_FAULT_NONE        0xFFu
#define FIRST_FAULT_SLAVE_BIT   0x80u

typedef struct {
    uint16_t Slave_Faults;
    uint16_t BMS_Faults;
    uint8_t  IMDState;          // IMDStateEnum when the IMD fault was raised
//    uint8_t  IsolationState;    // isolation percent when the IMD fault was raised
    uint8_t  FirstFault;
    uint8_t  TripCount;
} BMSFaultsData_t;

//--------------------------------------------------------------------------
void Fault_Init(void);          // call after CAN and the charger are initialized
void Fault_Handler(void);       // fault line low, charger off, send over CAN. No checking!

void Fault_RaiseBMS(const BMS_Faults fault);
void Fault_RaiseSlave(const Slave_Faults fault);
void Fault_RaiseSlaveMask(const uint16_t mask);     // a mask of 0 does nothing

void set_IMD_fault_state(const float isolation, const IMDStateEnum state);
void Fault_RaiseIMD();

void Fault_ClearBMS(const BMS_Faults fault);
void Fault_ClearSlave(const uint16_t mask);
void Fault_ClearAll(void);      // fault line only goes high again when nothing is left set

bool Fault_Any(void);
bool Fault_AnyBMS(void);
bool Fault_AnySlave(void);
bool Fault_IsBMSSet(const BMS_Faults fault);
bool Fault_IsSlaveSet(const Slave_Faults fault);
uint16_t Fault_GetBMSFlags(void);
uint16_t Fault_GetSlaveFlags(void);
BMSFaultsData_t Fault_Get(void);        // copy, safe to read while interrupts raise faults

bool Fault_SendCAN(void);
bool Fault_ReceiveCAN(BMSFaultsData_t* const data);

// GIO interrupt callbacks
void Debug_GIO_Notification(void);
void IMD_FAULT_GIO_Notification(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_FAULT_HANDLER_H_ */
