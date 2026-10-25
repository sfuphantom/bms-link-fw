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
// Clock: the RTI free running counter is only 32 bit (wraps every 2^32 ticks, ~107 s at 40 MHz).
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
    return MAKE_TICK(min, sec, ms, us, tick);
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
    delay_ticks(US_TO_TICKS((ticks_t)ms * 1000U + us));
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
// Waiting for a condition
//----------------------------------------------------------------------------------------
// Starts the wait on the first call, and runs Condition if a check is due.
static bool waitConditionMet(WaitUntil_t * const w, BoolFunc Condition, const ticks_t checkPeriod){
    const ticks_t now = getNow_tick();

    if (!w->started){
        w->started = true;
        w->start   = now;
        w->checks  = 0;
    }

    if (w->checks > 0 && (now - w->lastCheck) < checkPeriod){
        return false;
    }

    w->lastCheck = now;
    w->checks++;
    return Condition();
}

static WaitResult_t waitFinish(WaitUntil_t * const w, const WaitResult_t result){
    if (result != WAIT_CONTINUE){
        w->started = false;     // next call starts a new wait
    }
    return result;
}

WaitResult_t waitUntil_nonblocking_count(WaitUntil_t * const w, BoolFunc Condition,
                                         const uint32_t maxChecks, const ticks_t checkPeriod){
    if (waitConditionMet(w, Condition, checkPeriod)){
        return waitFinish(w, WAIT_DONE);
    }
    const bool timedOut = (maxChecks > 0) && (w->checks >= maxChecks);
    return waitFinish(w, timedOut ? WAIT_TIMEOUT : WAIT_CONTINUE);
}

WaitResult_t waitUntil_nonblocking_time(WaitUntil_t * const w, BoolFunc Condition,
                                        const ticks_t maxWait, const ticks_t checkPeriod){
    if (waitConditionMet(w, Condition, checkPeriod)){
        return waitFinish(w, WAIT_DONE);
    }
    const bool timedOut = (maxWait > 0) && hasTimeElapsed(w->start, maxWait);
    return waitFinish(w, timedOut ? WAIT_TIMEOUT : WAIT_CONTINUE);
}

bool waitUntil_blocking_count(BoolFunc Condition, const uint32_t maxChecks, const ticks_t checkPeriod){
    WaitUntil_t  w = {0};
    WaitResult_t result;

    do {
        result = waitUntil_nonblocking_count(&w, Condition, maxChecks, checkPeriod);
    } while (result == WAIT_CONTINUE);

    return result == WAIT_DONE;
}

bool waitUntil_blocking_time(BoolFunc Condition, const ticks_t maxWait, const ticks_t checkPeriod){
    WaitUntil_t  w = {0};
    WaitResult_t result;

    do {
        result = waitUntil_nonblocking_time(&w, Condition, maxWait, checkPeriod);
    } while (result == WAIT_CONTINUE);

    return result == WAIT_DONE;
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

//----------------------------------------------------------------------------------------
// Edge detection
//----------------------------------------------------------------------------------------
bool detectEdge(const bool input, bool * const last, const EdgeMode_t mode){
    const bool rising  = input && !*last;
    const bool falling = !input && *last;
    *last = input;

    switch (mode){
        case EDGE_DETECT_RISING:  return rising;
        case EDGE_DETECT_FALLING: return falling;
        default:                  return rising || falling;
    }
}

//----------------------------------------------------------------------------------------
// How long has an input been true
//----------------------------------------------------------------------------------------
bool trueTimer_update(TrueTimer_t * const t, const bool input, ticks_t * const trueFor){
    const ticks_t now = getNow_tick();

    if (detectEdge(input, &t->last, EDGE_DETECT_RISING)){
        t->since = now;
    }

    *trueFor = input ? (now - t->since) : 0;
    return input;
}

bool trueTimer_updateFunc(TrueTimer_t * const t, BoolFunc Input, ticks_t * const trueFor){
    return trueTimer_update(t, Input(), trueFor);
}

//----------------------------------------------------------------------------------------
// Software PWM
//----------------------------------------------------------------------------------------
bool pwmSignalGen(const ticks_t period, const float duty, ticks_t * const periodStart){
    if (period == 0){
        return false;
    }

    const ticks_t now = getNow_tick();
    ticks_t elapsed = now - *periodStart;

    if (elapsed >= period){
        *periodStart += elapsed - (elapsed % period);   // move to the start of the current period
        elapsed = elapsed % period;
    }

    const ticks_t onTicks = (ticks_t)(clampValue(duty, 0.0, 1.0) * (double)period);
    return elapsed < onTicks;
}
