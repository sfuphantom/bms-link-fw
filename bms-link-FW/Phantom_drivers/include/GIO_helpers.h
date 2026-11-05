/*
 * GIO_helpers.h
 *
 *  Created on: Jul 24, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_GIO_HELPERS_H_
#define PHANTOM_DRIVERS_INCLUDE_GIO_HELPERS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "gio.h"
#include "reg_gio.h"
#include "reg_spi.h"        // spiPORTn
#include "reg_het.h"        // hetPORTn
#include "PhantomTimers.h"

// Short critical section. Override both (before this header) for an RTOS,
// e.g. with taskENTER_CRITICAL / taskEXIT_CRITICAL. "state" is a uint32_t the caller declares.
#ifndef CRITICAL_ENTER
  #if defined(__TI_COMPILER_VERSION__)
    #define CRITICAL_ENTER(state)   do { (state) = _get_CPSR(); _disable_IRQ(); } while (0)
    #define CRITICAL_EXIT(state)    do { if (((state) & 0x80U) == 0U) { _enable_IRQ(); } } while (0)
  #else
    #define CRITICAL_ENTER(state)   ((void)((state) = 0U))
    #define CRITICAL_EXIT(state)    ((void)(state))
  #endif
#endif

//--------------------------------------------------------------------------
// Types
//--------------------------------------------------------------------------
typedef enum { GIO_LOW = 0, GIO_HIGH = 1 } GioLevel_t;
typedef enum { GIO_INPUT = 0, GIO_OUTPUT = 1 } GioDir_t;

// Every port that has GIO style registers (gioPORT_t). Add a port here and in gioHw_port().
typedef enum {
    GIO_PORT_A     = 0,
    GIO_PORT_B     = 1,
    GIO_PORT_SPI1  = 2,
    GIO_PORT_SPI2  = 3,
    GIO_PORT_SPI3  = 4,
    GIO_PORT_SPI4  = 5,
    GIO_PORT_SPI5  = 6,
    GIO_PORT_HET1  = 7,
    GIO_PORT_HET2  = 8,
} GioPort_t;

#define GIO_ID(port, bit)       (((uint32_t)(port) * 32U) + (uint32_t)(bit))
#define GIO_ID_PORT(pin)        ((GioPort_t)((uint32_t)(pin) / 32U))
#define GIO_ID_BIT(pin)         ((uint32_t)(pin) % 32U)
#define GIO_ID_MASK(pin)        (1UL << GIO_ID_BIT(pin))

//--------------------------------------------------------------------------
// Enum 1: a unique ID for every pin, made with GIO_ID(port, bit).
// Comment out the pins that do not exist on your package.
//--------------------------------------------------------------------------
typedef enum {
    // A
    PIN_A0           = GIO_ID(GIO_PORT_A, 0),
    PIN_A1           = GIO_ID(GIO_PORT_A, 1),
    PIN_A2           = GIO_ID(GIO_PORT_A, 2),
    PIN_A3           = GIO_ID(GIO_PORT_A, 3),
    PIN_A4           = GIO_ID(GIO_PORT_A, 4),
    PIN_A5           = GIO_ID(GIO_PORT_A, 5),
    PIN_A6           = GIO_ID(GIO_PORT_A, 6),
    PIN_A7           = GIO_ID(GIO_PORT_A, 7),
    // B
    PIN_B0           = GIO_ID(GIO_PORT_B, 0),
    PIN_B1           = GIO_ID(GIO_PORT_B, 1),
    PIN_B2           = GIO_ID(GIO_PORT_B, 2),
    PIN_B3           = GIO_ID(GIO_PORT_B, 3),
    PIN_B4           = GIO_ID(GIO_PORT_B, 4),
    PIN_B5           = GIO_ID(GIO_PORT_B, 5),
    PIN_B6           = GIO_ID(GIO_PORT_B, 6),
    PIN_B7           = GIO_ID(GIO_PORT_B, 7),
    // SPI1
    PIN_SPI1_CS0     = GIO_ID(GIO_PORT_SPI1, 0),
    PIN_SPI1_CS1     = GIO_ID(GIO_PORT_SPI1, 1),
    PIN_SPI1_CS2     = GIO_ID(GIO_PORT_SPI1, 2),
    PIN_SPI1_CS3     = GIO_ID(GIO_PORT_SPI1, 3),
    PIN_SPI1_CS4     = GIO_ID(GIO_PORT_SPI1, 4),
    PIN_SPI1_CS5     = GIO_ID(GIO_PORT_SPI1, 5),
    PIN_SPI1_CS6     = GIO_ID(GIO_PORT_SPI1, 6),
    PIN_SPI1_CS7     = GIO_ID(GIO_PORT_SPI1, 7),
    PIN_SPI1_ENA     = GIO_ID(GIO_PORT_SPI1, 8),
    PIN_SPI1_CLK     = GIO_ID(GIO_PORT_SPI1, 9),
    PIN_SPI1_SIMO    = GIO_ID(GIO_PORT_SPI1, 10),
    PIN_SPI1_SOMI    = GIO_ID(GIO_PORT_SPI1, 11),
    // SPI2
    PIN_SPI2_CS0     = GIO_ID(GIO_PORT_SPI2, 0),
    PIN_SPI2_CS1     = GIO_ID(GIO_PORT_SPI2, 1),
    PIN_SPI2_CS2     = GIO_ID(GIO_PORT_SPI2, 2),
    PIN_SPI2_CS3     = GIO_ID(GIO_PORT_SPI2, 3),
    PIN_SPI2_CS4     = GIO_ID(GIO_PORT_SPI2, 4),
    PIN_SPI2_CS5     = GIO_ID(GIO_PORT_SPI2, 5),
    PIN_SPI2_CS6     = GIO_ID(GIO_PORT_SPI2, 6),
    PIN_SPI2_CS7     = GIO_ID(GIO_PORT_SPI2, 7),
    PIN_SPI2_ENA     = GIO_ID(GIO_PORT_SPI2, 8),
    PIN_SPI2_CLK     = GIO_ID(GIO_PORT_SPI2, 9),
    PIN_SPI2_SIMO    = GIO_ID(GIO_PORT_SPI2, 10),
    PIN_SPI2_SOMI    = GIO_ID(GIO_PORT_SPI2, 11),
    // SPI3
    PIN_SPI3_CS0     = GIO_ID(GIO_PORT_SPI3, 0),
    PIN_SPI3_CS1     = GIO_ID(GIO_PORT_SPI3, 1),
    PIN_SPI3_CS2     = GIO_ID(GIO_PORT_SPI3, 2),
    PIN_SPI3_CS3     = GIO_ID(GIO_PORT_SPI3, 3),
    PIN_SPI3_CS4     = GIO_ID(GIO_PORT_SPI3, 4),
    PIN_SPI3_CS5     = GIO_ID(GIO_PORT_SPI3, 5),
    PIN_SPI3_CS6     = GIO_ID(GIO_PORT_SPI3, 6),
    PIN_SPI3_CS7     = GIO_ID(GIO_PORT_SPI3, 7),
    PIN_SPI3_ENA     = GIO_ID(GIO_PORT_SPI3, 8),
    PIN_SPI3_CLK     = GIO_ID(GIO_PORT_SPI3, 9),
    PIN_SPI3_SIMO    = GIO_ID(GIO_PORT_SPI3, 10),
    PIN_SPI3_SOMI    = GIO_ID(GIO_PORT_SPI3, 11),
    // SPI4
    PIN_SPI4_CS0     = GIO_ID(GIO_PORT_SPI4, 0),
    PIN_SPI4_CS1     = GIO_ID(GIO_PORT_SPI4, 1),
    PIN_SPI4_CS2     = GIO_ID(GIO_PORT_SPI4, 2),
    PIN_SPI4_CS3     = GIO_ID(GIO_PORT_SPI4, 3),
    PIN_SPI4_CS4     = GIO_ID(GIO_PORT_SPI4, 4),
    PIN_SPI4_CS5     = GIO_ID(GIO_PORT_SPI4, 5),
    PIN_SPI4_CS6     = GIO_ID(GIO_PORT_SPI4, 6),
    PIN_SPI4_CS7     = GIO_ID(GIO_PORT_SPI4, 7),
    PIN_SPI4_ENA     = GIO_ID(GIO_PORT_SPI4, 8),
    PIN_SPI4_CLK     = GIO_ID(GIO_PORT_SPI4, 9),
    PIN_SPI4_SIMO    = GIO_ID(GIO_PORT_SPI4, 10),
    PIN_SPI4_SOMI    = GIO_ID(GIO_PORT_SPI4, 11),
    // SPI5
    PIN_SPI5_CS0     = GIO_ID(GIO_PORT_SPI5, 0),
    PIN_SPI5_CS1     = GIO_ID(GIO_PORT_SPI5, 1),
    PIN_SPI5_CS2     = GIO_ID(GIO_PORT_SPI5, 2),
    PIN_SPI5_CS3     = GIO_ID(GIO_PORT_SPI5, 3),
    PIN_SPI5_CS4     = GIO_ID(GIO_PORT_SPI5, 4),
    PIN_SPI5_CS5     = GIO_ID(GIO_PORT_SPI5, 5),
    PIN_SPI5_CS6     = GIO_ID(GIO_PORT_SPI5, 6),
    PIN_SPI5_CS7     = GIO_ID(GIO_PORT_SPI5, 7),
    PIN_SPI5_ENA     = GIO_ID(GIO_PORT_SPI5, 8),
    PIN_SPI5_CLK     = GIO_ID(GIO_PORT_SPI5, 9),
    PIN_SPI5_SIMO    = GIO_ID(GIO_PORT_SPI5, 10),
    PIN_SPI5_SOMI    = GIO_ID(GIO_PORT_SPI5, 11),
    // HET1
    PIN_HET1_0       = GIO_ID(GIO_PORT_HET1, 0),
    PIN_HET1_1       = GIO_ID(GIO_PORT_HET1, 1),
    PIN_HET1_2       = GIO_ID(GIO_PORT_HET1, 2),
    PIN_HET1_3       = GIO_ID(GIO_PORT_HET1, 3),
    PIN_HET1_4       = GIO_ID(GIO_PORT_HET1, 4),
    PIN_HET1_5       = GIO_ID(GIO_PORT_HET1, 5),
    PIN_HET1_6       = GIO_ID(GIO_PORT_HET1, 6),
    PIN_HET1_7       = GIO_ID(GIO_PORT_HET1, 7),
    PIN_HET1_8       = GIO_ID(GIO_PORT_HET1, 8),
    PIN_HET1_9       = GIO_ID(GIO_PORT_HET1, 9),
    PIN_HET1_10      = GIO_ID(GIO_PORT_HET1, 10),
    PIN_HET1_11      = GIO_ID(GIO_PORT_HET1, 11),
    PIN_HET1_12      = GIO_ID(GIO_PORT_HET1, 12),
    PIN_HET1_13      = GIO_ID(GIO_PORT_HET1, 13),
    PIN_HET1_14      = GIO_ID(GIO_PORT_HET1, 14),
    PIN_HET1_15      = GIO_ID(GIO_PORT_HET1, 15),
    PIN_HET1_16      = GIO_ID(GIO_PORT_HET1, 16),
    PIN_HET1_17      = GIO_ID(GIO_PORT_HET1, 17),
    PIN_HET1_18      = GIO_ID(GIO_PORT_HET1, 18),
    PIN_HET1_19      = GIO_ID(GIO_PORT_HET1, 19),
    PIN_HET1_20      = GIO_ID(GIO_PORT_HET1, 20),
    PIN_HET1_21      = GIO_ID(GIO_PORT_HET1, 21),
    PIN_HET1_22      = GIO_ID(GIO_PORT_HET1, 22),
    PIN_HET1_23      = GIO_ID(GIO_PORT_HET1, 23),
    PIN_HET1_24      = GIO_ID(GIO_PORT_HET1, 24),
    PIN_HET1_25      = GIO_ID(GIO_PORT_HET1, 25),
    PIN_HET1_26      = GIO_ID(GIO_PORT_HET1, 26),
    PIN_HET1_27      = GIO_ID(GIO_PORT_HET1, 27),
    PIN_HET1_28      = GIO_ID(GIO_PORT_HET1, 28),
    PIN_HET1_29      = GIO_ID(GIO_PORT_HET1, 29),
    PIN_HET1_30      = GIO_ID(GIO_PORT_HET1, 30),
    PIN_HET1_31      = GIO_ID(GIO_PORT_HET1, 31),
    // HET2
    PIN_HET2_0       = GIO_ID(GIO_PORT_HET2, 0),
    PIN_HET2_1       = GIO_ID(GIO_PORT_HET2, 1),
    PIN_HET2_2       = GIO_ID(GIO_PORT_HET2, 2),
    PIN_HET2_3       = GIO_ID(GIO_PORT_HET2, 3),
    PIN_HET2_4       = GIO_ID(GIO_PORT_HET2, 4),
    PIN_HET2_5       = GIO_ID(GIO_PORT_HET2, 5),
    PIN_HET2_6       = GIO_ID(GIO_PORT_HET2, 6),
    PIN_HET2_7       = GIO_ID(GIO_PORT_HET2, 7),
    PIN_HET2_8       = GIO_ID(GIO_PORT_HET2, 8),
    PIN_HET2_9       = GIO_ID(GIO_PORT_HET2, 9),
    PIN_HET2_10      = GIO_ID(GIO_PORT_HET2, 10),
    PIN_HET2_11      = GIO_ID(GIO_PORT_HET2, 11),
    PIN_HET2_12      = GIO_ID(GIO_PORT_HET2, 12),
    PIN_HET2_13      = GIO_ID(GIO_PORT_HET2, 13),
    PIN_HET2_14      = GIO_ID(GIO_PORT_HET2, 14),
    PIN_HET2_15      = GIO_ID(GIO_PORT_HET2, 15),
    PIN_HET2_16      = GIO_ID(GIO_PORT_HET2, 16),
    PIN_HET2_17      = GIO_ID(GIO_PORT_HET2, 17),
    PIN_HET2_18      = GIO_ID(GIO_PORT_HET2, 18),
    PIN_HET2_19      = GIO_ID(GIO_PORT_HET2, 19),
    PIN_HET2_20      = GIO_ID(GIO_PORT_HET2, 20),
    PIN_HET2_21      = GIO_ID(GIO_PORT_HET2, 21),
    PIN_HET2_22      = GIO_ID(GIO_PORT_HET2, 22),
    PIN_HET2_23      = GIO_ID(GIO_PORT_HET2, 23),
    PIN_HET2_24      = GIO_ID(GIO_PORT_HET2, 24),
    PIN_HET2_25      = GIO_ID(GIO_PORT_HET2, 25),
    PIN_HET2_26      = GIO_ID(GIO_PORT_HET2, 26),
    PIN_HET2_27      = GIO_ID(GIO_PORT_HET2, 27),
    PIN_HET2_28      = GIO_ID(GIO_PORT_HET2, 28),
    PIN_HET2_29      = GIO_ID(GIO_PORT_HET2, 29),
    PIN_HET2_30      = GIO_ID(GIO_PORT_HET2, 30),
    PIN_HET2_31      = GIO_ID(GIO_PORT_HET2, 31),

    GIO_PIN_NONE = 0xFFFF,
} GioPin_t;

//--------------------------------------------------------------------------
// Enum 2: names used by the application, each one is a pin ID from enum 1
//--------------------------------------------------------------------------
typedef enum {
    GIO_DEBUG_PIN           = PIN_A2,
//    GIO_START_CHARGING_PIN  = PIN_A5,
    GIO_IMD_FAULT_PIN       = PIN_A6,
    GIO_BMS_FAULT_PIN       = PIN_A7,
    GIO_SLAVE_CS_PIN        = PIN_SPI3_CS0,
    GIO_HV_CS_PIN           = PIN_SPI3_CS5,
    GIO_NAME_PIN_NONE       = GIO_PIN_NONE,
} GioName_t;

//--------------------------------------------------------------------------
// Hardware wrapper. The only functions that call gio.h / touch the port registers.
// They work on any port that is a gioPORT_t: GIO A/B, the SPI pins and the HET pins.
// For another processor, rewrite these (and the port list above).
//--------------------------------------------------------------------------
gioPORT_t *gioHw_port(const GioPort_t port);                                  // NULL if unknown
GioLevel_t gioHw_read(gioPORT_t * const port, const uint32_t bit);
void       gioHw_write(gioPORT_t * const port, const uint32_t bit, const GioLevel_t level);
void       gioHw_toggle(gioPORT_t * const port, const uint32_t bit);
void       gioHw_setDirection(gioPORT_t * const port, const uint32_t bit, const GioDir_t dir);
void       gioHw_writeMask(gioPORT_t * const port, const uint32_t mask, const GioLevel_t level);
void       gioHw_intEnable(gioPORT_t * const port, const uint32_t bit);       // GIO A/B only
void       gioHw_intDisable(gioPORT_t * const port, const uint32_t bit);

//--------------------------------------------------------------------------
// Pins. A bad pin is ignored (reads as GIO_LOW).
//--------------------------------------------------------------------------
GioLevel_t gio_read(const GioName_t pin);
void       gio_write(const GioName_t pin, const GioLevel_t level);
GioLevel_t gio_toggle(const GioName_t pin);                  // returns the new level
void       gio_setDirection(const GioName_t pin, const GioDir_t dir);

// All the pins in mask change in the same write (one port only). Build a mask with GIO_ID_MASK.
void gio_writeMask(const GioPort_t port, const uint32_t mask, const GioLevel_t level);
// Pins can be on different ports, written one after the other
void gio_writeMany(const GioName_t * const pins, const size_t count, const GioLevel_t level);

//--------------------------------------------------------------------------
// Output helpers
//--------------------------------------------------------------------------
bool gio_writeIf(const GioName_t pin, const GioLevel_t level, const bool condition);   // returns condition

// level for width, then back to the opposite level; gap is only between pulses
void gio_pulseN(const GioName_t pin, const uint32_t count, const GioLevel_t level,
                const ticks_t width, const ticks_t gap);
#define gio_pulse(pin, level, width)    gio_pulseN((pin), 1U, (level), (width), 0U)

void gio_toggleN(const GioName_t pin, const uint32_t count, const ticks_t halfPeriod);

// Non-blocking. Start the Periodic_t with periodic_init. Returns true when it toggled.
bool gio_toggleEvery(Periodic_t * const p, const GioName_t pin);

void gio_pwm(const GioName_t pin, const ticks_t period, const float duty, ticks_t * const periodStart);

// Non-blocking pulse: start it, then call update often. update returns true while the pulse is on.
typedef struct {
    GioName_t   pin;
    GioLevel_t level;
    Timeout_t  timer;
} GioTimedPulse_t;

void gio_timedPulse_start(GioTimedPulse_t * const p, const GioName_t pin, const GioLevel_t level, const ticks_t width);
bool gio_timedPulse_update(GioTimedPulse_t * const p);

//--------------------------------------------------------------------------
// Input helpers
//--------------------------------------------------------------------------
// Non-blocking: call it often. Returns how long the pin has been at level (0 if it is not).
// Start the TrueTimer_t as {0}.
ticks_t gio_timeAtLevel(TrueTimer_t * const t, const GioName_t pin, const GioLevel_t level);

// Did the pin change since the last gio_watch call
typedef struct {
    GioName_t   pin;
    GioLevel_t last;
} GioWatch_t;

void   gio_watchInit(GioWatch_t * const w, const GioName_t pin);
Edge_t gio_watch(GioWatch_t * const w);                     // EDGE_NONE / EDGE_RISING / EDGE_FALLING

void   gio_debounceInit(Debouncer_t * const d, const GioName_t pin);
Edge_t gio_debounce(Debouncer_t * const d, const GioName_t pin, const ticks_t hold);

// Blocking: true if the pin stayed at level for stable before timeout ran out (timeout 0 = forever)
bool gio_waitStable(const GioName_t pin, const GioLevel_t level, const ticks_t stable, const ticks_t timeout);

//--------------------------------------------------------------------------
// Send bytes with software timing.
// clock == PIN_NONE : serial with start and stop bit (8N1), data idles high
// otherwise         : data changes while clock is low, clock idles low
//--------------------------------------------------------------------------
#define GIO_BITRATE_TO_TICKS(bitsPerSecond)  (((ticks_t)TICKS_PER_US * 1000000U) / (bitsPerSecond))

typedef struct {
    GioName_t data;
    GioName_t clock;
    ticks_t  bitTicks;
    bool     msbFirst;
} GioSerial_t;

void gio_serialInit(GioSerial_t * const s, const GioName_t data, const GioName_t clock,
                    const uint32_t bitsPerSecond, const bool msbFirst);
void gio_sendByte(const GioSerial_t * const s, const uint8_t value);
void gio_sendBytes(const GioSerial_t * const s, const uint8_t * const bytes, const size_t count);

//--------------------------------------------------------------------------
// Interrupts (GIO ports A and B only). Edge and priority are still set in HALCoGen.
// The handler runs in the interrupt, keep it short. Without a handler the
// interrupts are only counted.
//--------------------------------------------------------------------------
typedef void (*GioCallback_t)(const GioName_t pin);

bool     gio_attach(const GioName_t pin, const GioCallback_t handler);   // handler may be NULL
void     gio_detach(const GioName_t pin);
uint32_t gio_takeInterrupts(const GioName_t pin);                        // how many since the last call

// Old per-pin hooks. They are weak and still called when no handler is attached.
void Debug_GIO_Notification(void);
void IMD_FAULT_GIO_Notification(void);

#endif /* PHANTOM_DRIVERS_INCLUDE_GIO_HELPERS_H_ */
