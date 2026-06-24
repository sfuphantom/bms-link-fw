/*
 * BMS_gio.h
 *
 *  Created on: Jun 22, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_BMS_GIO_H_
#define PHANTOM_DRIVERS_INCLUDE_BMS_GIO_H_

#include "gio.h"

enum Gio_Bits {IMD_FALUT_BIT        = 2,
               BMS_FALUT_BIT        = 5,
               START_CHARGING_BIT   = 6,
               FREE_BIT1            = 7,
};
typedef enum{GIO_LOW, GIO_HIGH, GIO_RISING_EGDE, GIO_FALLING_EGDE} Gio_State_t;

void GIO_inturupt_Notification(gioPORT_t *port, uint32 bit);

void init_BMS_GIO();
void reset_BMS_GIO();

Gio_State_t read_START_CHARGING_GIO();

void IMD_FALUT_BIT_GIO(Gio_State_t NewState);
void BMS_FALUT_BIT_GIO(Gio_State_t NewState);


#endif /* PHANTOM_DRIVERS_INCLUDE_BMS_GIO_H_ */
