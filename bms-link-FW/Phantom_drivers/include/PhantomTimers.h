/*
 * PhantomTimers.h
 *
 *  Created on: Jul 13, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_PHANTOMTIMERS_H_
#define PHANTOM_DRIVERS_INCLUDE_PHANTOMTIMERS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------
// Time units. One tick = 1 / RTI_CLOCK_MEG_HZ microseconds.
// -----------------------------------------
#define RTI_CLOCK_MEG_HZ    40U     // 40 MHz
#define TICKS_PER_US        (RTI_CLOCK_MEG_HZ)
#define US_PER_MS            1E3
#define US_PER_SEC           1E6
#define US_PER_MIN           6E7

typedef uint64_t ticks_t;           // 64 bit: the 32 bit RTI counter + an overflow count
typedef void (*VoidFunc)(void);
typedef bool (*BoolFunc)(void);

// Work for constants and variables. The argument is cast first, so there is no 32 bit overflow.
#define US_TO_TICKS(us)     ((ticks_t)(us) * TICKS_PER_US)
#define MS_TO_TICKS(ms)     US_TO_TICKS((ticks_t)(ms)  * 1000000U)
#define SEC_TO_TICKS(sec)   US_TO_TICKS((ticks_t)(sec) * US_PER_SEC)

#define TICKS_TO_US(ticks)  ((ticks_t)(ticks)   / TICKS_PER_US)
#define TICKS_TO_MS(ticks)  (TICKS_TO_US(ticks) / US_PER_MS)

ticks_t ticks_make(const uint32_t min, const uint32_t sec, const  uint32_t ms, const uint32_t us, const uint32_t tick);
#define MAKE_TICK(MIN, SEC, MS, US, TICK)    US_TO_TICKS( (ticks_t)MIN * US_PER_MIN \
                                                        + (ticks_t)SEC * US_PER_SEC \
                                                        + (ticks_t)MS  * US_PER_MS \
                                                        + US) + TICK
// -----------------------------------------
// Clock
// -----------------------------------------
void    initPhantomTimers(void);    // call once, after rtiInit()
ticks_t getNow_tick(void);

ticks_t timer_tic_tick(void);                   // same as getNow_tick, reads better in pairs
ticks_t timer_toc_tick(const ticks_t tic);
ticks_t timer_toc_us(const ticks_t tic);

// -----------------------------------------
// Blocking waits (they do not need the overflow interrupt)
// -----------------------------------------
void delay_ticks(ticks_t ticks);
void delay_ms_us(const uint32_t ms, const uint32_t us);

// -----------------------------------------
// Non-blocking waits
// -----------------------------------------
bool hasTimeElapsed(const ticks_t start, const ticks_t wait);

// One shot. timeout_expired stays true after expiring until you start it again.
typedef struct {
    ticks_t start;
    ticks_t wait;
    bool    active;
} Timeout_t;

void    timeout_start(Timeout_t * const t, const ticks_t wait);
void    timeout_cancel(Timeout_t * const t);
bool    timeout_expired(const Timeout_t * const t);     // false if not started
ticks_t timeout_remaining(const Timeout_t * const t);   // 0 if expired or not started

// Repeating, without drift. If you fall behind it skips the missed periods instead of bursting.
typedef struct {
    ticks_t period;
    ticks_t last;
} Periodic_t;

void periodic_init(Periodic_t * const p, const ticks_t period);
bool periodic_due(Periodic_t * const p);                // true once per period

// Calls Callable until the time is up. Returns how many times it was called.
uint32_t doUntilTimeLimit(VoidFunc Callable, const ticks_t timeLimit_tick);

// -----------------------------------------
// Waiting for a condition
// maxChecks / maxWait of 0 = wait forever.
// checkPeriod = time between two calls of Condition (0 = every call). The first check is immediate.
// -----------------------------------------
typedef enum {
    WAIT_CONTINUE = 0,  // not true yet, limit not reached
    WAIT_DONE,          // Condition returned true
    WAIT_TIMEOUT        // limit reached first
} WaitResult_t;

// Start it as {0}. It resets itself after WAIT_DONE / WAIT_TIMEOUT, so it can be reused.
typedef struct {
    ticks_t  start;
    ticks_t  lastCheck;
    uint32_t checks;
    bool     started;
} WaitUntil_t;

WaitResult_t waitUntil_nonblocking_count(WaitUntil_t * const w, BoolFunc Condition,
                                         const uint32_t maxChecks, const ticks_t checkPeriod);
WaitResult_t waitUntil_nonblocking_time(WaitUntil_t * const w, BoolFunc Condition,
                                        const ticks_t maxWait, const ticks_t checkPeriod);

// true = Condition became true before the limit
bool waitUntil_blocking_count(BoolFunc Condition, const uint32_t maxChecks, const ticks_t checkPeriod);
bool waitUntil_blocking_time(BoolFunc Condition, const ticks_t maxWait, const ticks_t checkPeriod);

// Turns an expression into a bool f(void) so it can be passed to the functions above:
//     MAKE_BOOL_FUNC(isButtonDown, gioGetBit(gioPORTA, 3) == 0)     // no ; needed
// It is static, so an unused one gets the compiler's unused-function warning, and writing
// "x = 5" instead of "x == 5" gets the compiler's assignment-in-condition warning (-Wall).
#define MAKE_BOOL_FUNC(name, ...)                   \
    static bool name(void)                          \
    {                                               \
        if (__VA_ARGS__) { return true; }           \
        return false;                               \
    }

// -----------------------------------------
// Timing functions ("decorators")
// -----------------------------------------
typedef struct {
    uint32_t count;
    ticks_t  last;
    ticks_t  min;
    ticks_t  max;
    ticks_t  total;
} TimingStats_t;

void    timingStats_reset(TimingStats_t * const s);
void    timingStats_add(TimingStats_t * const s, const ticks_t duration);
ticks_t timingStats_avg(const TimingStats_t * const s);

uint64_t timeFunction_VoidVoid_us(VoidFunc FuncPtr);
ticks_t  timeFunction_stats(VoidFunc FuncPtr, TimingStats_t * const stats);   // returns this run, in ticks

// Times any statement, whatever its arguments or return value:
//     TIME_STATEMENT(&stats, value = readAdc(3));
// Don't use return/break/goto inside it, the time would not be recorded.
#define TIME_STATEMENT(stats, ...)                                  \
    do {                                                            \
        const ticks_t tic_ = timer_tic_tick();                      \
        __VA_ARGS__;                                                \
        timingStats_add((stats), timer_toc_tick(tic_));             \
    } while (0)

// -----------------------------------------
// Debouncing a bool (switch, digital input, ...)
// The output only changes after the input has disagreed with it for long enough.
// Any sample that agrees with the output restarts the count.
// -----------------------------------------
typedef enum {
    EDGE_NONE    = 0,
    EDGE_FALLING = 1,
    EDGE_RISING  = 2
} Edge_t;

typedef struct {
    bool     state;     // debounced value, read it directly
    uint32_t count;     // samples in a row that disagreed with state
    ticks_t  since;     // when the disagreeing started
} Debouncer_t;

void   debounce_init(Debouncer_t * const d, const bool initial);

// Input must be the new value for `samples` calls in a row (call it at a steady rate).
Edge_t debounce_samples(Debouncer_t * const d, const bool input, const uint32_t samples);

// Input must be the new value for `hold` ticks. Call it as often as you like.
Edge_t debounce_time(Debouncer_t * const d, const bool input, const ticks_t hold);

// Blocking: true if the input stayed at `level` for stable_tick before timeout_tick ran out.
bool waitForInput(bool (*readInput)(void), const bool level,
                  const ticks_t stable_tick, const ticks_t timeout_tick);

// -----------------------------------------
// Edge detection. `last` is kept by the caller, start it at the input's current value.
// -----------------------------------------
typedef enum {
    EDGE_DETECT_RISING,
    EDGE_DETECT_FALLING,
    EDGE_DETECT_BOTH
} EdgeMode_t;

bool detectEdge(const bool input, bool * const last, const EdgeMode_t mode);

// -----------------------------------------
// How long has an input been true
// -----------------------------------------
typedef struct {
    bool    last;
    ticks_t since;      // when the input last went true
} TrueTimer_t;          // start it as {0}

// Returns input. trueFor = time it has been true, 0 if it is false.
bool trueTimer_update(TrueTimer_t * const t, const bool input, ticks_t * const trueFor);
bool trueTimer_updateFunc(TrueTimer_t * const t, BoolFunc Input, ticks_t * const trueFor);

// -----------------------------------------
// PWM from software. Call it often, it returns the output level right now.
// duty is 0.0 to 1.0. periodStart is kept by the caller: start it at 0 or getNow_tick().
// A late call skips the missed periods and keeps the phase. period 0 gives false.
// -----------------------------------------
bool pwmSignalGen(const ticks_t period, const float duty, ticks_t * const periodStart);

#ifdef __cplusplus
}
#endif

#endif /* PHANTOM_DRIVERS_INCLUDE_PHANTOMTIMERS_H_ */
