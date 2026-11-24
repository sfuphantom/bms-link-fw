/*
 * GIO_helpers.c
 *
 * High-level GIO helper routines (timing, debouncing, pulses, interrupts).
 * Hardware independent: calls GIO_stubs for register access.
 */

#include "GIO_helpers.h"
#include "GIO_stubs.h"
#include "PhantomAlgorithms.h"
#include "PhantomTimers.h"

/* ------------------------------------------------------------------------- */
/* Core Read / Write                                                         */
/* ------------------------------------------------------------------------- */
GioLevel_t gio_read(const GioReadName_t pin)
{
    return gioHw_read(pin);
}

GioLevel_t gio_readOutput(const GioWriteName_t pin)
{
    return gioHw_readOutput(pin);
}

void gio_write(const GioWriteName_t pin, const GioLevel_t level)
{
    gioHw_write(pin, level);
}

GioLevel_t gio_toggle(const GioWriteName_t pin)
{
    return gioHw_toggle(pin);
}

/* ------------------------------------------------------------------------- */
/* Output Helpers                                                            */
/* ------------------------------------------------------------------------- */
static GioLevel_t opposite(const GioLevel_t level)
{
    return (level == GIO_HIGH) ? GIO_LOW : GIO_HIGH;
}

void gio_writeMany(const GioWriteName_t * const pins, const size_t count, const GioLevel_t level)
{
    size_t i;
    for (i = 0; i < count; i++) {
        gio_write(pins[i], level);
    }
}

bool gio_writeIf(const GioWriteName_t pin, const GioLevel_t level, const bool condition)
{
    if (condition) { gio_write(pin, level); }
    return condition;
}

void gio_pulseN(const GioWriteName_t pin, const uint32_t count, const GioLevel_t level,
                const ticks_t width, const ticks_t gap)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        gio_write(pin, level);
        delay_ticks(width);
        gio_write(pin, opposite(level));

        if (i + 1U < count) { delay_ticks(gap); }
    }
}

void gio_toggleN(const GioWriteName_t pin, const uint32_t count, const ticks_t halfPeriod)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        (void)gio_toggle(pin);

        if (i + 1U < count) { delay_ticks(halfPeriod); }
    }
}

bool gio_toggleEvery(Periodic_t * const p, const GioWriteName_t pin)
{
    if (!periodic_due(p)) { return false; }

    (void)gio_toggle(pin);
    return true;
}

void gio_pwm(const GioWriteName_t pin, const ticks_t period, const float duty, ticks_t * const periodStart)
{
    const bool high = pwmSignalGen(period, duty, periodStart);
    gio_write(pin, high ? GIO_HIGH : GIO_LOW);
}

void gio_timedPulse_start(GioTimedPulse_t * const p, const GioWriteName_t pin, const GioLevel_t level, const ticks_t width)
{
    p->pin   = pin;
    p->level = level;

    gio_write(pin, level);
    timeout_start(&p->timer, width);
}

bool gio_timedPulse_update(GioTimedPulse_t * const p)
{
    if (!p->timer.active)            { return false; }
    if (!timeout_expired(&p->timer)) { return true; }

    gio_write(p->pin, opposite(p->level));
    timeout_cancel(&p->timer);
    return false;
}

/* ------------------------------------------------------------------------- */
/* Input Helpers                                                             */
/* ------------------------------------------------------------------------- */
ticks_t gio_timeAtLevel(TrueTimer_t * const t, const GioReadName_t pin, const GioLevel_t level)
{
    ticks_t duration = 0;

    (void)trueTimer_update(t, gio_read(pin) == level, &duration);
    return duration;
}

void gio_watchInit(GioWatch_t * const w, const GioReadName_t pin)
{
    w->pin  = pin;
    w->last = gio_read(pin);
}

Edge_t gio_watch(GioWatch_t * const w)
{
    const GioLevel_t before = w->last;
    const GioLevel_t now    = gio_read(w->pin);

    w->last = now;

    if (now == before)   { return EDGE_NONE; }
    if (now == GIO_HIGH) { return EDGE_RISING; }
    return EDGE_FALLING;
}

void gio_debounceInit(Debouncer_t * const d, const GioReadName_t pin)
{
    debounce_init(d, gio_read(pin) == GIO_HIGH);
}

Edge_t gio_debounce(Debouncer_t * const d, const GioReadName_t pin, const ticks_t hold)
{
    return debounce_time(d, gio_read(pin) == GIO_HIGH, hold);
}

bool gio_waitStable(const GioReadName_t pin, const GioLevel_t level, const ticks_t stable, const ticks_t timeout)
{
    const bool want = (level == GIO_HIGH);
    Debouncer_t d;
    Timeout_t   t;

    debounce_init(&d, !want);
    timeout_start(&t, timeout);

    while (d.state != want) {
        (void)debounce_time(&d, gio_read(pin) == GIO_HIGH, stable);

        if (timeout != 0U && timeout_expired(&t)) { return (d.state == want); }
    }
    return true;
}

/* ------------------------------------------------------------------------- */
/* Software Serial                                                           */
/* ------------------------------------------------------------------------- */
static void waitUntilOffset(const ticks_t start, const ticks_t offset)
{
    while (!hasTimeElapsed(start, offset)) { }
}

void gio_serialInit(GioSerial_t * const s, const GioWriteName_t data, const GioWriteName_t clock,
                    const uint32_t bitsPerSecond, const bool msbFirst)
{
    s->data     = data;
    s->clock    = clock;
    s->bitTicks = (bitsPerSecond == 0U) ? 0U : GIO_BITRATE_TO_TICKS(bitsPerSecond);
    s->msbFirst = msbFirst;

    if (clock == GIO_WRITE_NONE) {
        gio_write(data, GIO_HIGH);
    } else {
        gio_write(clock, GIO_LOW);
    }
}

void gio_sendByte(const GioSerial_t * const s, const uint8_t value)
{
    const bool    framed  = (s->clock == GIO_WRITE_NONE);
    const ticks_t start   = timer_tic_tick();
    ticks_t       slotEnd = 0;
    uint8_t       i;

    if (framed) {
        gio_write(s->data, GIO_LOW);
        slotEnd += s->bitTicks;
        waitUntilOffset(start, slotEnd);
    }

    for (i = 0; i < 8U; i++) {
        const uint8_t bitIdx = s->msbFirst ? (uint8_t)(7U - i) : i;

        gio_write(s->data, (((value >> bitIdx) & 1U) != 0U) ? GIO_HIGH : GIO_LOW);

        if (!framed) {
            waitUntilOffset(start, slotEnd + (s->bitTicks / 2U));
            gio_write(s->clock, GIO_HIGH);
        }

        slotEnd += s->bitTicks;
        waitUntilOffset(start, slotEnd);

        if (!framed) { gio_write(s->clock, GIO_LOW); }
    }

    if (framed) {
        gio_write(s->data, GIO_HIGH);
        slotEnd += s->bitTicks;
        waitUntilOffset(start, slotEnd);
    }
}

void gio_sendBytes(const GioSerial_t * const s, const uint8_t * const bytes, const size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) {
        gio_sendByte(s, bytes[i]);
    }
}

/* ------------------------------------------------------------------------- */
/* Interrupts                                                                */
/* ------------------------------------------------------------------------- */
#define GIO_INT_SLOTS (size_t)GIO_READ_COUNT

static volatile uint32_t intCount[GIO_INT_SLOTS];
static GioCallback_t     intHandler[GIO_INT_SLOTS];

bool gio_attach(const GioReadName_t pin, const GioCallback_t handler)
{
    if ((size_t)pin >= GIO_INT_SLOTS) { return false; }

    intHandler[pin] = handler;
    return gioHw_intEnable(pin);
}

void gio_detach(const GioReadName_t pin)
{
    if ((size_t)pin >= GIO_INT_SLOTS) { return; }

    gioHw_intDisable(pin);
    intHandler[pin] = NULL;
}

uint32_t gio_takeInterrupts(const GioReadName_t pin)
{
    uint32_t irq;
    uint32_t count;

    if ((size_t)pin >= GIO_INT_SLOTS) { return 0U; }

    CRITICAL_ENTER(irq);
    count = intCount[pin];
    intCount[pin] = 0U;
    CRITICAL_EXIT(irq);

    return count;
}

#pragma WEAK(Debug_GIO_Notification)
void Debug_GIO_Notification(void) { }

#pragma WEAK(IMD_FAULT_GIO_Notification)
void IMD_FAULT_GIO_Notification(void) { }

static void oldHooks(const GioReadName_t pin)
{
    switch (pin) {
        case GIO_READ_IMD_FAULT: IMD_FAULT_GIO_Notification(); break;
        default: break;
    }
}

void gio_handleInterrupt(const GioReadName_t pin)
{
    if ((size_t)pin >= GIO_INT_SLOTS) { return; }

    intCount[pin]++;

    if (intHandler[pin] != NULL) {
        intHandler[pin](pin);
    } else {
        oldHooks(pin);
    }
}
