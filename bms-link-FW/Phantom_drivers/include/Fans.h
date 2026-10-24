/*
 * Fans.h
 *
 *  Created on: Jun 30, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_FANS_H_
#define PHANTOM_DRIVERS_INCLUDE_FANS_H_

#include <stdint.h>
#include <stdbool.h>
#include "het.h"
#include "BatteryModule.h"
#include "FullBattery_Hardware.h"

/* ---- Hardware selection: which HET block drives the fans (1 or 2) ---- */
#define FANS_HET_INSTANCE 1

#if FANS_HET_INSTANCE == 1
    #define FAN_HET_REG  hetREG1
    #define FAN_HET_RAM  hetRAM1
    #define FAN_HET_PORT hetPORT1
#elif FANS_HET_INSTANCE == 2
    #define FAN_HET_REG  hetREG2
    #define FAN_HET_RAM  hetRAM2
    #define FAN_HET_PORT hetPORT2
#else
    #error "FANS_HET_INSTANCE must be 1 or 2"
#endif

/* ---- Tuning ---- */
#define FAN_MAX_DUTY      (95u)     /* percent */
#define FAN_MIN_DUTY      (5u)      /* percent */
#define FAN_START_DUTY    (95u)     /* percent, applied at init */
#define FAN_PWM_PERIOD_US (40.0)    /* 25 kHz, the standard 4-wire fan PWM frequency */

/* WARNING: placeholder value. Set this to the measured current of ONE fan at 100% duty. */
#define FAN_CURRENT_AT_100_A (1.0f)

/* NUMBER_OF_FANS_TOTAL and NUMBER_OF_FANS_PER_MODULE come from BatteryModule.h / FullBattery_Hardware.h */
#define FAN_MODULE_COUNT (NUMBER_OF_FANS_TOTAL / NUMBER_OF_FANS_PER_MODULE)

/* All setters return false (and do nothing) if an ID is out of range. */
bool Fans_SetDuty(uint8_t fanID, uint8_t duty);
bool Fans_GetDuty(uint8_t fanID, uint8_t *duty);

bool Fans_SetModuleDuty(uint8_t moduleID, uint8_t duty);
bool Fans_GetModuleDuty(uint8_t moduleID, uint8_t out[NUMBER_OF_FANS_PER_MODULE]);

void Fans_SetAllDuty(uint8_t duty);
void Fans_GetAllDuty(uint8_t out[NUMBER_OF_FANS_TOTAL]);

void Fans_Init(void);
void Fans_SetFault(void);   /* everything to max */

float Fans_EstimateCurrent_A(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_FANS_H_ */
