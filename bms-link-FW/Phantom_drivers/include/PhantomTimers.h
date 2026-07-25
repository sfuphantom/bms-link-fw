/*
 * PhantomTimers.h
 *
 *  Created on: Jul 13, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_PHANTOMTIMERS_H_
#define PHANTOM_DRIVERS_INCLUDE_PHANTOMTIMERS_H_

#define RTI_CLOCK_MEG_HZ    10U  // 10 MHz
#define RTI_CLOCK_HZ        (RTI_CLOCK_MEG_HZ*1000000UL)  // 10 MHz
#define RTI_US_2_TICKS      (RTI_CLOCK_MEG_HZ)
#define RTI_MS_2_TICKS      (RTI_US_2_TICKS * 1000UL)
#define RTI_SEC_2_TICKS     (RTI_MS_2_TICKS * 1000UL)


#define RTI_MAX_TIMERS 8

#define USE_RTI_DELAY TRUE

// -----------------------------------------

void delay_ms_us(const uint32_t ms, const uint32_t us);
bool rtiTimerExpired(const uint32_t id, const uint32_t ms, const uint32_t us);
uint32 getNow_tick();
uint32_t getNow_us();
uint32_t getNow_ms();
uint32_t timer_tic_tick();
uint32_t timer_toc_us(const uint32_t tic);
uint8_t debounceBool(const bool input, uint32_t * const perv, const uint8_t changeNum);

#endif /* PHANTOM_DRIVERS_INCLUDE_PHANTOMTIMERS_H_ */
