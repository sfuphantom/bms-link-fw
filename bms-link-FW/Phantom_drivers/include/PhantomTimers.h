/*
 * PhantomTimers.h
 *
 *  Created on: Jul 13, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_PHANTOMTIMERS_H_
#define PHANTOM_DRIVERS_INCLUDE_PHANTOMTIMERS_H_

#define RTI_CLOCK_MEG_HZ    10U  // 10 MHz
#define RTI_US_2_TICKS      (RTI_CLOCK_MEG_HZ)
#define USE_RTI_DELAY TRUE

// -----------------------------------------
#define MIN_SEC_MS_US_TICK_2_TICK(min,sec,ms,us,tick)     ((uint64_t)((60e6 * min + sec*1e6 + ms*1e3 + us) * RTI_US_2_TICKS + tick))
// -----------------------------------------
//typedef struct {
//    void (*Func)(void);
//    uint32_t Period;
//    uint32_t LastDone;
//}doPeriod_t;

union nowTimerTicks_t {
    uint64_t FullTicks;
    uint32_t OFandTickCounter[2];
};
// -----------------------------------------

void delay_ms_us(const uint32_t ms, const uint32_t us);
//bool rtiTimerExpired(const uint32_t id, const uint32_t ms, const uint32_t us);
uint64_t getNow_tick();
uint64_t timer_tic_tick();
uint64_t timer_toc_us(const uint64_t tic);


uint64_t timeFunction_VoidVoid_us(void (*FuncPrt)(void));


uint8_t debounceBool(const bool input, uint32_t * const perv, const uint8_t changeNum);

bool hasTimeElapsed_rti(const uint64_t start_ticks, uint64_t const wait_ticks);
bool hasPeriodExpired_rti(const uint64_t period_tick, uint64_t * const last_tick);
// -----------------------------------------
void initPhantomTimers();
#endif /* PHANTOM_DRIVERS_INCLUDE_PHANTOMTIMERS_H_ */
