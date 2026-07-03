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
#include "Phantom_Can.h"

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
typedef struct {
    uint16 max_voltage_dV;
    uint16 max_current_dA;
    bool charge_enable;
} ChargerCmd_t;

typedef struct {
    uint16 output_voltage_dV;
    uint16 output_current_dA;
    uint8 status_flags;
} ChargerStatus_t;
//----------------------------------------------------------------------------------------
typedef struct  {
  ChargerState_t ChargerState;

  ChargerStatus_t ChargerStatusData;
  ChargerCmd_t   ChargerTargetData;

//  uint16_t TargetChargerCurrent;
//  uint16_t TargetChargerVoltage;
//  bool enable;
//
//  uint16_t OutputChargerVoltage;
//  uint16_t OutputChargerCurrent;
//  uint8_t status;

  float EstimateTimeDone;
} ChargerData_t;

//----------------------------------------------------------------------------------------------------

bool isCharging();
bool StartCharging();
bool DoneCharging();
bool StopCharging();
bool ResetCharging();

ChargerState_t CheckChargerState(const bool start_charging);
//----------------------------------------------------------------------------------------------------
bool sendCmd2ChargerWraper();
bool GetChargerStatusWraper();
//----------------------------------------------------------------------------------------------------
// TODO: use cans to talk with chager
uint16_t GetChargerTargetVoltage();
uint16_t GetChargerTargetCurrent();
uint16_t GetChargerOutputVoltage();
uint16_t GetChargerOutputCurrent();
bool isChargerEnabled();
uint8_t GetChargerStatus();
//----------------------------------------------------------------------------------------------------
bool changeChargeState(const uint16_t Volt, const uint16_t current, const bool enable);
bool SetChargerVoltage(const uint16_t Volt);
bool SetChargerCurrent(const uint16_t current);
bool SetChargerLimits(const uint16_t Volt, const uint16_t current);

bool TurnChargerOn();
bool TurnChargerOff();
bool ShutDownCharger_Fault();
//----------------------------------------------------------------------------------------------------
bool CalcNewCurrentSetting(const float avg_SOC, const float max_SOC);
//----------------------------------------------------------------------------------------------------
ChargerData_t* GetChargerDataPrt();
void initCharger();
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
ChargerData_t ChargerData;
static ChargerStatus_t* ChargerDateRead = &ChargerData.ChargerStatusData;
static ChargerCmd_t*   ChargerDateWrite = &ChargerData.ChargerTargetData;

#endif /* PHANTOM_DRIVERS_INCLUDE_CHARGER_H_ */
