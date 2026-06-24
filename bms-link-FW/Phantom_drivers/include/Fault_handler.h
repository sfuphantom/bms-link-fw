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
//--------------------------------------------------------------------------
enum Gio_Bits { GIO_IMD_FAULT_BIT        = 2,
                GIO_BMS_FAULT_BIT        = 5,
                GIO_START_CHARGING_BIT   = 6,
                GIO_DEGUBING_BIT1        = 7,
};
//#define (sizeof(Gio_Bits)/sizeof(Gio_Bits[1]))
typedef enum{GIO_LOW, GIO_HIGH, GIO_RISING_EGDE, GIO_FALLING_EGDE} Gio_State_t;

//--------------------------------------------------------------------------
struct {
        uint8_t  IMD_Faults;
        uint16_t Slave_Faults;
        uint8_t  BMS_Faults;
        uint8_t Charger_Faults;
}Faults_t;
//--------------------------------------------------------------------------


Gio_State_t gioGetBitHelper(const uint8_t bit);
Gio_State_t gioSetBitHelper(const uint8_t bit, const Gio_State_t NewState);
Gio_State_t gioToggleBitHelper(const uint8_t bit);
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
bool clearIMDFaults();
bool clearSlaveFaults();
bool clearOtherBMSFaults();
bool clear_AllFault();
void init_Fault();


#endif /* PHANTOM_DRIVERS_INCLUDE_Fault_HANDLER_H_ */
