/*
 * Charger.h
 *
 *  Created on: Jun 17, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_CHARGER_H_
#define PHANTOM_DRIVERS_INCLUDE_CHARGER_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_CHARGER_CURRENT_AMPS    400
#define MIN_CHARGER_CURRENT_AMPS    1
#define OFF_CHARGER_CURRENT_AMPS    0

#define CHARGER_CURRENT_AMPS_RANGE

#define MAX_CHARGER_VOLTAGE
#define MIN_CHARGER_VOLTAGE
//----------------------------------------------------------------------------------------------------
#define MIN_CONTAMINATION_DELAY_MS
#define MAX_CONTAMINATION_DELAY_MS
#define MAX_PROPAGATION_DELAY_MS

#define MAX_OVERSHOOT_CURRENT_PERCENTAGE    01.0f
#define MAX_OVERSHOOT_CURRENT
#define MIN_RISE_TIME_CURRENT_MS
#define MAX_RISE_TIME_CURRENT_MS
#define MAX_STEADY_STATE_TIME_CURRENT_MS
#define MAX_STEADY_STATE_TIME_CURRENT_MS
#define MAX_STEADY_STATE_TIME_CURRENT_MS
//----------------------------------------------------------------------------------------------------
typedef enum {NOT_CHARGING, Start_CHARGING, CHARGING, DONE_CHARGING, CHARGER_FALUT} ChargerState_t;
//----------------------------------------------------------------------------------------------------
struct ChargerData_t {
  ChargerState_t ChargerState;

  uint16_t ChargerCurrent;
  uint16_t ChargerVoltage;

  uint16_t ChargerFaluts;
};

//----------------------------------------------------------------------------------------------------
bool isCharging();
void StartCharging();
void DoneCharging();

ChargerState_t CheckChargerState();
//----------------------------------------------------------------------------------------------------
uint16_t GetChargerFaluts();
void SetChargerFaluts(const uint16_t Faluts);
//----------------------------------------------------------------------------------------------------
void SetChargerVoltage(const uint16_t Volt);
void SetChargerCurrent(const uint16_t current);
uint16_t GetChargerVoltage();
uint16_t GetChargerCurrent();

void TurnChargerOn();
void TurnChargerOff();

uint16_t CalcNewCurrentSetting(const float SOC);
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
struct ChargerData_t ChargerData;


#endif /* PHANTOM_DRIVERS_INCLUDE_CHARGER_H_ */
