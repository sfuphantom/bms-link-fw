/*
 * GIO_helpers.h
 *
 * High-level GIO helper routines (timing, debouncing, pulses, interrupts).
 * Hardware independent: uses logical pin names only (per requirement R4).
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_GIO_HELPERS_H_
#define PHANTOM_DRIVERS_INCLUDE_GIO_HELPERS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "Cpu_stubs.h"
#include "PhantomPinAlloc.h"
#include "PhantomTimers.h"

/* ------------------------------------------------------------------------- */
/* Core Read / Write Helpers                                                 */
/* ------------------------------------------------------------------------- */
GioLevel_t gio_read(const GioReadName_t pin);
GioLevel_t gio_readOutput(const GioWriteName_t pin);
void       gio_write(const GioWriteName_t pin, const GioLevel_t level);
GioLevel_t gio_toggle(const GioWriteName_t pin);

void       gio_writeMany(const GioWriteName_t * const pins, const size_t count, const GioLevel_t level);
bool       gio_writeIf(const GioWriteName_t pin, const GioLevel_t level, const bool condition);

/* ------------------------------------------------------------------------- */
/* Output Timing Helpers                                                     */
/* ------------------------------------------------------------------------- */
void gio_pulseN(const GioWriteName_t pin, const uint32_t count, const GioLevel_t level,
                const ticks_t width, const ticks_t gap);
#define gio_pulse(pin, level, width)    gio_pulseN((pin), 1U, (level), (width), 0U)

void gio_toggleN(const GioWriteName_t pin, const uint32_t count, const ticks_t halfPeriod);
bool gio_toggleEvery(Periodic_t * const p, const GioWriteName_t pin);

void gio_pwm(const GioWriteName_t pin, const ticks_t period, const float duty, ticks_t * const periodStart);

typedef struct {
    GioWriteName_t pin;
    GioLevel_t     level;
    Timeout_t      timer;
} GioTimedPulse_t;

void gio_timedPulse_start(GioTimedPulse_t * const p, const GioWriteName_t pin, const GioLevel_t level, const ticks_t width);
bool gio_timedPulse_update(GioTimedPulse_t * const p);

/* ------------------------------------------------------------------------- */
/* Input Timing Helpers                                                      */
/* ------------------------------------------------------------------------- */
ticks_t gio_timeAtLevel(TrueTimer_t * const t, const GioReadName_t pin, const GioLevel_t level);

typedef struct {
    GioReadName_t pin;
    GioLevel_t    last;
} GioWatch_t;

void   gio_watchInit(GioWatch_t * const w, const GioReadName_t pin);
Edge_t gio_watch(GioWatch_t * const w);

void   gio_debounceInit(Debouncer_t * const d, const GioReadName_t pin);
Edge_t gio_debounce(Debouncer_t * const d, const GioReadName_t pin, const ticks_t hold);

bool   gio_waitStable(const GioReadName_t pin, const GioLevel_t level, const ticks_t stable, const ticks_t timeout);

/* ------------------------------------------------------------------------- */
/* Software Serial Helpers                                                   */
/* ------------------------------------------------------------------------- */
#define GIO_BITRATE_TO_TICKS(bitsPerSecond)  (((ticks_t)TICKS_PER_US * 1000000U) / (bitsPerSecond))

typedef struct {
    GioWriteName_t data;
    GioWriteName_t clock;
    ticks_t        bitTicks;
    bool           msbFirst;
} GioSerial_t;

void gio_serialInit(GioSerial_t * const s, const GioWriteName_t data, const GioWriteName_t clock,
                    const uint32_t bitsPerSecond, const bool msbFirst);
void gio_sendByte(const GioSerial_t * const s, const uint8_t value);
void gio_sendBytes(const GioSerial_t * const s, const uint8_t * const bytes, const size_t count);

/* ------------------------------------------------------------------------- */
/* Interrupt Registration Helpers                                            */
/* ------------------------------------------------------------------------- */
typedef void (*GioCallback_t)(const GioReadName_t pin);

bool     gio_attach(const GioReadName_t pin, const GioCallback_t handler);
void     gio_detach(const GioReadName_t pin);
uint32_t gio_takeInterrupts(const GioReadName_t pin);

/* Weak notifications */
void Debug_GIO_Notification(void);
void IMD_FAULT_GIO_Notification(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_GIO_HELPERS_H_ */
