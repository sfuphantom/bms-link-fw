/*
 * GIO_helpers.h
 *
 *  Created on: Jul 24, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_GIO_HELPERS_H_
#define PHANTOM_DRIVERS_INCLUDE_GIO_HELPERS_H_
#include "gio.h"
#include "reg_gio.h"

#define GIO_PORT_A gioPORTA
#define GIO_PORT_B gioPORTB

#define GIO_MAX_PERIOD_US 3
//--------------------------------------------------------------------------
enum Gio_Bits {
                GIO_DEGUBING_BIT1        = 2,

                GIO_IMD_FAULT_BIT        = 6,
                GIO_BMS_FAULT_BIT        = 7,

                GIO_START_CHARGING_BIT   = 5,

//                GIO_DEGUBING_BIT1        = 25,
//
//                GIO_IMD_FAULT_BIT        = 2,
//                GIO_BMS_FAULT_BIT        = 5,
//
//                GIO_START_CHARGING_BIT   = 7,
};
//--------------------------------------------------------------------------
typedef enum{GIO_LOW, GIO_HIGH, GIO_FALLING_EGDE, GIO_RISING_EGDE} Gio_State_t;
//--------------------------------------------------------------------------
Gio_State_t gioGetBitHelper(const uint8_t bit);
Gio_State_t gioSetBitHelper(const uint8_t bit, const Gio_State_t NewState);
Gio_State_t gioToggleBitHelper(const uint8_t bit);

void Debug_GIO_Notification();
void IMD_FAULT_GIO_Notification();

#endif /* PHANTOM_DRIVERS_INCLUDE_GIO_HELPERS_H_ */
