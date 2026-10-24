/*
 * Fans.c
 *
 *  Created on: Jun 30, 2026
 *      Author: tanjo
 */

#include <string.h>
#include "het.h"
#include "Fans.h"

/* Compile-time check, works with #defines, enums and consts alike */
#define FANS_STATIC_ASSERT(name, cond) typedef char name[(cond) ? 1 : -1]
FANS_STATIC_ASSERT(fans_total_is_multiple_of_module,
                   (NUMBER_OF_FANS_TOTAL % NUMBER_OF_FANS_PER_MODULE) == 0);
FANS_STATIC_ASSERT(fans_start_duty_in_range,
                   FAN_START_DUTY >= FAN_MIN_DUTY && FAN_START_DUTY <= FAN_MAX_DUTY);

/* WARNING: assumes fan index == HET PWM channel number. If the wiring differs,
 * change this one macro (or point it at a lookup table). */
#define FAN_PWM_CHANNEL(fanID) (fanID)

/* Last duty written, in percent. WARNING: not protected against ISR/task
 * concurrency; wrap writers in a critical section if both contexts use this. */
static uint8_t FanDuty[NUMBER_OF_FANS_TOTAL];

static uint8_t clampDuty(const uint8_t duty)
{
    if (duty > FAN_MAX_DUTY) return FAN_MAX_DUTY;
    /* WARNING: clamping to the minimum means a fan can never be fully stopped.
     * Remove this line if 0% must be allowed. */
    if (duty < FAN_MIN_DUTY) return FAN_MIN_DUTY;
    return duty;
}

///////////////////////////////////////////////////////////
bool Fans_SetDuty(const uint8_t fanID, const uint8_t duty)
{
    if (fanID >= NUMBER_OF_FANS_TOTAL) return false;

    const uint8_t safeDuty = clampDuty(duty);
    pwmSetDuty(FAN_HET_RAM, FAN_PWM_CHANNEL(fanID), safeDuty);
    FanDuty[fanID] = safeDuty;
    return true;
}

bool Fans_GetDuty(const uint8_t fanID, uint8_t *const duty)
{
    if (fanID >= NUMBER_OF_FANS_TOTAL || duty == NULL) return false;

    *duty = FanDuty[fanID];
    return true;
}

///////////////////////////////////////////////////////////
bool Fans_SetModuleDuty(const uint8_t moduleID, const uint8_t duty)
{
    if (moduleID >= FAN_MODULE_COUNT) return false;

    const uint8_t firstFan = moduleID * NUMBER_OF_FANS_PER_MODULE;
    uint8_t i;
    for (i = 0; i < NUMBER_OF_FANS_PER_MODULE; i++) {
        Fans_SetDuty(firstFan + i, duty);
    }
    return true;
}

bool Fans_GetModuleDuty(const uint8_t moduleID, uint8_t out[NUMBER_OF_FANS_PER_MODULE])
{
    if (moduleID >= FAN_MODULE_COUNT || out == NULL) return false;

    memcpy(out, &FanDuty[moduleID * NUMBER_OF_FANS_PER_MODULE], NUMBER_OF_FANS_PER_MODULE);
    return true;
}

///////////////////////////////////////////////////////////
void Fans_SetAllDuty(const uint8_t duty)
{
    uint8_t i;
    for ( i = 0; i < NUMBER_OF_FANS_TOTAL; i++) {
        Fans_SetDuty(i, duty);
    }
}

void Fans_GetAllDuty(uint8_t out[NUMBER_OF_FANS_TOTAL])
{
    if (out == NULL) return;
    memcpy(out, FanDuty, sizeof(FanDuty));
}

///////////////////////////////////////////////////////////
void Fans_Init(void)
{
    const hetSIGNAL_t signal = {FAN_START_DUTY, FAN_PWM_PERIOD_US};
    uint8_t i;
    for (i = 0; i < NUMBER_OF_FANS_TOTAL; i++) {
        pwmSetSignal(FAN_HET_RAM, FAN_PWM_CHANNEL(i), signal);
        pwmStart(FAN_HET_RAM, FAN_PWM_CHANNEL(i));
        FanDuty[i] = FAN_START_DUTY;
    }
}

void Fans_SetFault(void)
{
    Fans_SetAllDuty(FAN_MAX_DUTY);
}

///////////////////////////////////////////////////////////
/* WARNING: assumes current scales linearly with duty. Real fans usually don't
 * (and have startup surge), so treat this as a rough estimate only. */
float Fans_EstimateCurrent_A(void)
{
    uint32_t dutySum = 0;
    uint8_t i;
    for (i = 0; i < NUMBER_OF_FANS_TOTAL; i++) {
        dutySum += FanDuty[i];
    }
    return FAN_CURRENT_AT_100_A * (float)dutySum / 100.0f;
}
