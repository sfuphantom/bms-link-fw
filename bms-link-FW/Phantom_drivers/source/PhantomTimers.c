/*
 * PhantomTimers.c
 *
 *  Created on: Jul 13, 2026
 *      Author: tanjo
 */

#include <stdint.h>
#include <stdbool.h>
#include "rti.h"
#include "PhantomHelpers.h"
#include "PhantomTimers.h"

//----------------------------------------------------------------------------------------
// Clock: the RTI free running counter is only 32 bit (wraps every ~429 s at 10 MHz).
// An interrupt counts the wraps, so the full time is (wraps << 32) | counter.
//----------------------------------------------------------------------------------------
static volatile uint32_t rti_overflowCount = 0;
static volatile const uint32_t * const nowTickReg = &rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;

static void startCounterIfStopped(void){
    if ((rtiREG1->GCTRL & (1U << rtiCOUNTER_BLOCK0)) == 0U){
        rtiStartCounter(rtiCOUNTER_BLOCK0);
    }
}

void initPhantomTimers(void){
    rti_overflowCount = 0;
    startCounterIfStopped();
    rtiEnableNotification(rtiNOTIFICATION_COUNTER0);
}

// The HAL calls this from the RTI interrupt.
void rtiNotification(uint32 notification){
    if (notification == rtiNOTIFICATION_COUNTER0){
        rtiREG1->INTFLAG = rtiNOTIFICATION_COUNTER0;    // write 1 to clear (no read-modify-write)
        rti_overflowCount++;
    }
}

ticks_t getNow_tick(void){
    uint32_t overflows;
    uint32_t counter;

    // if the interrupt fired between the two reads, read again
    do {
        overflows = rti_overflowCount;
        counter   = *nowTickReg;
    } while (overflows != rti_overflowCount);

    return ((ticks_t)overflows << 32) | counter;    // no union, so byte order doesn't matter
}

ticks_t timer_tic_tick(void){
    return getNow_tick();
}

ticks_t timer_toc_tick(const ticks_t tic){
    return getNow_tick() - tic;
}

ticks_t timer_toc_us(const ticks_t tic){
    return TICKS_TO_US(timer_toc_tick(tic));
}


ticks_t ticks_make(const uint32_t min, const uint32_t sec, const uint32_t ms,
                   const uint32_t us, const uint32_t tick){
    const ticks_t total_us = (ticks_t)min * US_PER_MIN
                           + (ticks_t)sec * US_PER_SEC
                           + (ticks_t)ms  * US_PER_MS
                           + us;
    return US_TO_TICKS(total_us) + tick;
}

//----------------------------------------------------------------------------------------
// Blocking waits. They read the raw 32 bit counter, so they also work inside an interrupt.
//----------------------------------------------------------------------------------------
void delay_ticks(ticks_t ticks){
    startCounterIfStopped();

    while (ticks > 0){
        // 32 bit subtraction wraps correctly, but only for waits under 2^32 ticks
        const uint32_t chunk = (uint32_t)MIN_VAL(ticks, (ticks_t)INT32_MAX);
        const uint32_t start = *nowTickReg;

        while ((uint32_t)(*nowTickReg - start) < chunk){
            /* busy wait */
        }
        ticks -= chunk;
    }
}

void delay_ms_us(const uint32_t ms, const uint32_t us){
    delay_ticks(ticks_make(0,0,ms,us,0));
}

//----------------------------------------------------------------------------------------
// Non-blocking waits
//----------------------------------------------------------------------------------------
bool hasTimeElapsed(const ticks_t start, const ticks_t wait){
    return (getNow_tick() - start) >= wait;
}

void timeout_start(Timeout_t * const t, const ticks_t wait){
    t->start  = getNow_tick();
    t->wait   = wait;
    t->active = true;
}

void timeout_cancel(Timeout_t * const t){
    t->active = false;
}

bool timeout_expired(const Timeout_t * const t){
    return t->active && hasTimeElapsed(t->start, t->wait);
}

ticks_t timeout_remaining(const Timeout_t * const t){
    if (!t->active){
        return 0;
    }
    const ticks_t elapsed = getNow_tick() - t->start;
    return (elapsed >= t->wait) ? 0 : (t->wait - elapsed);
}

void periodic_init(Periodic_t * const p, const ticks_t period){
    p->period = period;
    p->last   = getNow_tick();
}

bool periodic_due(Periodic_t * const p){
    const ticks_t now = getNow_tick();

    if (now - p->last < p->period){
        return false;
    }

    p->last += p->period;                   // keeps the rate exact
    if (now - p->last >= p->period){        // way behind: skip ahead instead of firing in a burst
        p->last = now;
    }
    return true;
}

uint32_t doUntilTimeLimit(VoidFunc Callable, const ticks_t timeLimit_tick){
    const ticks_t start_tick = getNow_tick();
    uint32_t calls = 0;

    while (!hasTimeElapsed(start_tick, timeLimit_tick)){
        Callable();
        calls++;
    }
    return calls;
}

//----------------------------------------------------------------------------------------
// Timing functions
//----------------------------------------------------------------------------------------
void timingStats_reset(TimingStats_t * const s){
    ZERO_STRUCT(*s);
    s->min = UINT64_MAX;    // so the first run always becomes the min
}

void timingStats_add(TimingStats_t * const s, const ticks_t duration){
    s->count++;
    s->last   = duration;
    s->total += duration;
    s->min    = MIN_VAL(s->min, duration);
    s->max    = MAX_VAL(s->max, duration);
}

ticks_t timingStats_avg(const TimingStats_t * const s){
    return (s->count == 0) ? 0 : (s->total / s->count);
}

uint64_t timeFunction_VoidVoid_us(VoidFunc FuncPtr){
    const ticks_t tic = timer_tic_tick();
    FuncPtr();
    return timer_toc_us(tic);
}

ticks_t timeFunction_stats(VoidFunc FuncPtr, TimingStats_t * const stats){
    const ticks_t tic = timer_tic_tick();
    FuncPtr();
    const ticks_t duration = timer_toc_tick(tic);

    timingStats_add(stats, duration);
    return duration;
}

//----------------------------------------------------------------------------------------
// Debouncing
//----------------------------------------------------------------------------------------
void debounce_init(Debouncer_t * const d, const bool initial){
    d->state = initial;
    d->count = 0;
    d->since = 0;
}

static Edge_t debounce_commit(Debouncer_t * const d, const bool input){
    d->state = input;
    d->count = 0;
    return input ? EDGE_RISING : EDGE_FALLING;
}

Edge_t debounce_samples(Debouncer_t * const d, const bool input, const uint32_t samples){
    if (input == d->state){
        d->count = 0;
        return EDGE_NONE;
    }

    d->count++;
    return (d->count >= samples) ? debounce_commit(d, input) : EDGE_NONE;
}

Edge_t debounce_time(Debouncer_t * const d, const bool input, const ticks_t hold){
    if (input == d->state){
        d->count = 0;
        return EDGE_NONE;
    }

    if (d->count == 0){
        d->since = getNow_tick();   // first sample that disagrees
    }
    if (d->count < UINT32_MAX){
        d->count++;
    }

    return hasTimeElapsed(d->since, hold) ? debounce_commit(d, input) : EDGE_NONE;
}

bool waitForInput(bool (*readInput)(void), const bool level,
                  const ticks_t stable_tick, const ticks_t timeout_tick){
    Debouncer_t debouncer;
    Timeout_t   timeout;

    debounce_init(&debouncer, !level);      // starts at the other level, so we wait for `level`
    timeout_start(&timeout, timeout_tick);

    while (!timeout_expired(&timeout)){
        debounce_time(&debouncer, readInput(), stable_tick);
        if (debouncer.state == level){
            return true;
        }
    }
    return false;
}
