/*
 * GIO_helpers.c
 *
 *  Created on: Jul 24, 2026
 *      Author: tanjo
 */

#include "gio.h"
#include "GIO_helpers.h"
#include "PhantomAlgorithms.h"
#include "PhantomTimers.h"

//--------------------------------------------------------------------------
// Hardware wrapper: the only code that calls gio.h or touches the port registers
//--------------------------------------------------------------------------
gioPORT_t *gioHw_port(const GioPort_t port)
{
    switch (port) {
        case GIO_PORT_A:    return gioPORTA;
        case GIO_PORT_B:    return gioPORTB;
        case GIO_PORT_SPI1: return spiPORT1;
        case GIO_PORT_SPI2: return spiPORT2;
        case GIO_PORT_SPI3: return spiPORT3;
        case GIO_PORT_SPI4: return spiPORT4;
        case GIO_PORT_SPI5: return spiPORT5;
        case GIO_PORT_HET1: return hetPORT1;
        case GIO_PORT_HET2: return hetPORT2;
        default:            return NULL;
    }
}

GioLevel_t gioHw_read(gioPORT_t * const port, const uint32_t bit)
{
    return (gioGetBit(port, bit) != 0U) ? GIO_HIGH : GIO_LOW;
}

void gioHw_write(gioPORT_t * const port, const uint32_t bit, const GioLevel_t level)
{
    gioSetBit(port, bit, (uint32)level);
}

void gioHw_toggle(gioPORT_t * const port, const uint32_t bit)
{
    gioToggleBit(port, bit);
}

void gioHw_setDirection(gioPORT_t * const port, const uint32_t bit, const GioDir_t dir)
{
    const uint32_t mask = 1UL << bit;
    const uint32_t now  = port->DIR;

    gioSetDirection(port, (dir == GIO_OUTPUT) ? (now | mask) : (now & ~mask));
}

void gioHw_writeMask(gioPORT_t * const port, const uint32_t mask, const GioLevel_t level)
{
    if (level == GIO_HIGH) { port->DSET = mask; }
    else                   { port->DCLR = mask; }
}

void gioHw_intEnable(gioPORT_t * const port, const uint32_t bit)  { gioEnableNotification(port, bit); }
void gioHw_intDisable(gioPORT_t * const port, const uint32_t bit) { gioDisableNotification(port, bit); }

//--------------------------------------------------------------------------
// Pin ID -> port and bit. false if the pin does not exist.
//--------------------------------------------------------------------------
static bool decode(const GioName_t pin, gioPORT_t ** const port, uint32_t * const bit)
{
    const GioPort_t id       = GIO_ID_PORT(pin);
    const uint32_t  bitLimit = (id <= GIO_PORT_B) ? 8U : 32U;     // GIO A and B have 8 pins

    *port = gioHw_port(id);
    *bit  = GIO_ID_BIT(pin);

    return (*port != NULL) && (*bit < bitLimit);
}

GioLevel_t gio_read(const GioName_t pin)
{
    gioPORT_t *port;
    uint32_t   bit;

    if (!decode(pin, &port, &bit)) { return GIO_LOW; }
    return gioHw_read(port, bit);
}

void gio_write(const GioName_t pin, const GioLevel_t level)
{
    gioPORT_t *port;
    uint32_t   bit;

    if (decode(pin, &port, &bit)) { gioHw_write(port, bit, level); }
}

GioLevel_t gio_toggle(const GioName_t pin)
{
    gioPORT_t *port;
    uint32_t   bit;

    if (!decode(pin, &port, &bit)) { return GIO_LOW; }

    gioHw_toggle(port, bit);
    return gioHw_read(port, bit);
}

void gio_setDirection(const GioName_t pin, const GioDir_t dir)
{
    gioPORT_t *port;
    uint32_t   bit;

    if (decode(pin, &port, &bit)) { gioHw_setDirection(port, bit, dir); }
}

void gio_writeMask(const GioPort_t id, const uint32_t mask, const GioLevel_t level)
{
    gioPORT_t * const port = gioHw_port(id);

    if (port != NULL) { gioHw_writeMask(port, mask, level); }
}

//--------------------------------------------------------------------------
// Output helpers
//--------------------------------------------------------------------------
static GioLevel_t opposite(const GioLevel_t level)
{
    return (level == GIO_HIGH) ? GIO_LOW : GIO_HIGH;
}

void gio_writeMany(const GioName_t * const pins, const size_t count, const GioLevel_t level)
{
    size_t i;
    for (i = 0; i < count; i++) {
        gio_write(pins[i], level);
    }
}

bool gio_writeIf(const GioName_t pin, const GioLevel_t level, const bool condition)
{
    if (condition) { gio_write(pin, level); }
    return condition;
}

void gio_pulseN(const GioName_t pin, const uint32_t count, const GioLevel_t level,
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

void gio_toggleN(const GioName_t pin, const uint32_t count, const ticks_t halfPeriod)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        (void)gio_toggle(pin);

        if (i + 1U < count) { delay_ticks(halfPeriod); }
    }
}

bool gio_toggleEvery(Periodic_t * const p, const GioName_t pin)
{
    if (!periodic_due(p)) { return false; }

    (void)gio_toggle(pin);
    return true;
}

void gio_pwm(const GioName_t pin, const ticks_t period, const float duty, ticks_t * const periodStart)
{
    const bool high = pwmSignalGen(period, duty, periodStart);
    gio_write(pin, high ? GIO_HIGH : GIO_LOW);
}

void gio_timedPulse_start(GioTimedPulse_t * const p, const GioName_t pin, const GioLevel_t level, const ticks_t width)
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

//--------------------------------------------------------------------------
// Input helpers
//--------------------------------------------------------------------------
ticks_t gio_timeAtLevel(TrueTimer_t * const t, const GioName_t pin, const GioLevel_t level)
{
    ticks_t duration = 0;

    (void)trueTimer_update(t, gio_read(pin) == level, &duration);
    return duration;
}

void gio_watchInit(GioWatch_t * const w, const GioName_t pin)
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

void gio_debounceInit(Debouncer_t * const d, const GioName_t pin)
{
    debounce_init(d, gio_read(pin) == GIO_HIGH);
}

Edge_t gio_debounce(Debouncer_t * const d, const GioName_t pin, const ticks_t hold)
{
    return debounce_time(d, gio_read(pin) == GIO_HIGH, hold);
}

bool gio_waitStable(const GioName_t pin, const GioLevel_t level, const ticks_t stable, const ticks_t timeout)
{
    const bool want = (level == GIO_HIGH);
    Debouncer_t d;
    Timeout_t   t;

    debounce_init(&d, !want);       // start on the other level, so it has to see the change
    timeout_start(&t, timeout);

    while (d.state != want) {
        (void)debounce_time(&d, gio_read(pin) == GIO_HIGH, stable);

        if (timeout != 0U && timeout_expired(&t)) { return (d.state == want); }
    }
    return true;
}

//--------------------------------------------------------------------------
// Send bytes
//--------------------------------------------------------------------------
static void waitUntilOffset(const ticks_t start, const ticks_t offset)
{
    while (!hasTimeElapsed(start, offset)) { }
}

void gio_serialInit(GioSerial_t * const s, const GioName_t data, const GioName_t clock,
                    const uint32_t bitsPerSecond, const bool msbFirst)
{
    s->data     = data;
    s->clock    = clock;
    s->bitTicks = (bitsPerSecond == 0U) ? 0U : GIO_BITRATE_TO_TICKS(bitsPerSecond);
    s->msbFirst = msbFirst;

    gio_setDirection(data, GIO_OUTPUT);
    if (clock == GIO_PIN_NONE) {
        gio_write(data, GIO_HIGH);
    } else {
        gio_setDirection(clock, GIO_OUTPUT);
        gio_write(clock, GIO_LOW);
    }
}

void gio_sendByte(const GioSerial_t * const s, const uint8_t value)
{
    const bool    framed  = (s->clock == GIO_PIN_NONE);
    const ticks_t start   = timer_tic_tick();
    ticks_t       slotEnd = 0;      // every bit is timed from start, so the error does not add up
    uint8_t       i;

    if (framed) {                   // start bit
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

    if (framed) {                   // stop bit
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

//--------------------------------------------------------------------------
// Interrupts. One slot per interrupt pin: A0-A7 are 0-7, B0-B7 are 8-15
//--------------------------------------------------------------------------
#define GIO_INT_SLOTS 16U

static volatile uint32_t intCount[GIO_INT_SLOTS];
static GioCallback_t     intHandler[GIO_INT_SLOTS];

static int intSlot(const GioName_t pin)
{
    const GioPort_t port = GIO_ID_PORT(pin);
    const uint32_t  bit  = GIO_ID_BIT(pin);

    if (bit >= 8U)          { return -1; }
    if (port == GIO_PORT_A) { return (int)bit; }
    if (port == GIO_PORT_B) { return (int)(8U + bit); }
    return -1;
}

bool gio_attach(const GioName_t pin, const GioCallback_t handler)
{
    const int slot = intSlot(pin);
    if (slot < 0) { return false; }

    intHandler[slot] = handler;
    gioHw_intEnable(gioHw_port(GIO_ID_PORT(pin)), GIO_ID_BIT(pin));
    return true;
}

void gio_detach(const GioName_t pin)
{
    const int slot = intSlot(pin);
    if (slot < 0) { return; }

    gioHw_intDisable(gioHw_port(GIO_ID_PORT(pin)), GIO_ID_BIT(pin));
    intHandler[slot] = NULL;
}

uint32_t gio_takeInterrupts(const GioName_t pin)
{
    const int slot = intSlot(pin);
    uint32_t irq;
    uint32_t count;

    if (slot < 0) { return 0U; }

    CRITICAL_ENTER(irq);
    count = intCount[slot];
    intCount[slot] = 0U;
    CRITICAL_EXIT(irq);

    return count;
}

#pragma WEAK(Debug_GIO_Notification)
void Debug_GIO_Notification(void) { }

#pragma WEAK(IMD_FAULT_GIO_Notification)
void IMD_FAULT_GIO_Notification(void) { }

static void oldHooks(const GioName_t pin)
{
    switch (pin) {
        case GIO_DEBUG_PIN:     Debug_GIO_Notification();     break;
        case GIO_IMD_FAULT_PIN: IMD_FAULT_GIO_Notification(); break;
        default: break;
    }
}

// Called by the HALCoGen interrupt code
void gioNotification(gioPORT_t *port, uint32 bit)
{
    if (port != gioPORTA && port != gioPORTB) { return; }

    const GioPort_t id   = (port == gioPORTA) ? GIO_PORT_A : GIO_PORT_B;
    const GioName_t  pin  = (GioName_t)GIO_ID(id, bit);
    const int       slot = intSlot(pin);
    if (slot < 0) { return; }

    intCount[slot]++;

    if (intHandler[slot] != NULL) { intHandler[slot](pin); }
    else                          { oldHooks(pin); }
}
