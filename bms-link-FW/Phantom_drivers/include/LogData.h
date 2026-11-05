/*
 * LogData.h
 *
 *  Data the MCU collected. One MSG_DATA frame with every enabled section.
 *  Every frame carries the "now" tick, the Python side uses it as the time of the row.
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_LOGDATA_H_
#define PHANTOM_DRIVERS_INCLUDE_LOGDATA_H_

#include <stdint.h>
#include <stdbool.h>

#include "LogHelpers.h"

#define SERIAL_SECTION_MAX  1024          // biggest data section, in bytes

// float -> integer scales used by the Compress_ functions. Keep them the same in uart_receiver.py
#define SCALE_CELL_VOLT     10000.0f
#define SCALE_TEMP          100.0f      // signed
#define SCALE_RES           10000.0f
#define SCALE_SOC           100.0f
#define SCALE_PACK_VOLT     100.0f
#define SCALE_CURRENT       10.0f       // signed

// One block of data inside MSG_DATA. Same numbers in uart_receiver.py (SecId).
typedef enum {
    SEC_CELL_VOLT = 1,
    SEC_CELL_TEMP,
    SEC_CELL_RES,
    SEC_CELL_SOC,
    SEC_PACK,
    SEC_IMD,
    SEC_CHARGER,
    SEC_SLAVE_STATE,
    SEC_FANS,
    SEC_PWM,
    SEC_BMS_STATE
} SectionId_t;

typedef struct {
    uint32_t framesDropped;     // another send was in progress (ISR), the frame was thrown away
    uint32_t sizeMismatch;      // a Compress_ function wrote another size than the len in its table
    uint8_t  badSection;        // the last section that did
} SerialStats_t;

// Sends one MSG_DATA with every enabled section, in the order of the sections[] table in the .c
void SendDataRoutine_Serial(void);
void Serial_EnableSection(const SectionId_t id, const bool enable);

const SerialStats_t * Serial_GetStats(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_LOGDATA_H_ */
