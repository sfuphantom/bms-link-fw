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
//static union nowTimerTicks_t now_ticks;
static uint32_t rti_OverFlowCounter = 0;
static volatile const uint32_t * const nowTickReg = &rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
void initPhantomTimers(){
    rti_OverFlowCounter = 0;
}

void rtiOverflow0_ISR(void){
#warning test if this function is called
    rti_OverFlowCounter++;
}
void rtiNotification(uint32 notification){
    if(notification == rtiNOTIFICATION_COUNTER0){
        if(rtiREG1->INTFLAG & 0x20000U){
            rtiREG1->INTFLAG &= ~0x20000U;
            rtiOverflow0_ISR();
        }
    }
}


//----------------------------------------------------------------------------------------
 void delay_ms_us(const uint32_t ms, const uint32_t us){
    #if USE_RTI_DELAY
        const uint64 total_us = (uint64)ms * 1000UL + us;
        const uint64 total_ticks = total_us * RTI_US_2_TICKS;

        /* Start counter block 0 if it is not already running */
        if ((rtiREG1->GCTRL & (1U << rtiCOUNTER_BLOCK0)) == 0U)
        {
            rtiStartCounter(rtiCOUNTER_BLOCK0);
        }

        const uint32 start = rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
        /* Wait until the required number of ticks has elapsed */
//        rtiBASE_t * rtiRegDelay = rtiREG1;
//        const uint32_t * timer = &rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
        while ((rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx - start) < (uint32)total_ticks)
        {
            /* Busy wait */
        }
//        rtiStopCounter(rtiCOUNTER_BLOCK0);
    #else
        volatile uint32_t i, j;
        // Approximate loops per ms, tune by measurement.
        const uint32_t loops_per_us = 11;
        const uint32_t loops_per_ms = loops_per_us * 1000;

        for (i = 0; i < ms; i++) {
            for (j = 0; j < loops_per_ms; j++);
        }
        for (i = 0; i < us; i++) {
            for (j = 0; j < loops_per_us; j++);
        }
    #endif
}
///* Static storage for the start ticks of each software timer */
//static uint32  rti_timer_last[RTI_MAX_TIMERS];
//static boolean rti_timer_valid[RTI_MAX_TIMERS];
//
//void rtiTimerStart(const uint8_t id)
//{
//    if (id >= RTI_MAX_TIMERS) return;
//
//    /* Ensure counter block 0 is running */
//    if ((rtiREG1->GCTRL & (1U << rtiCOUNTER_BLOCK0)) == 0U) {
//        rtiStartCounter(rtiCOUNTER_BLOCK0);
//    }
//
//    rti_timer_last[id] = rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
//    rti_timer_valid[id] = true;
//}
//
//bool rtiTimerExpired(const uint32_t id, const uint32_t ms, const uint32_t us)
//{
//    if (id >= RTI_MAX_TIMERS) return false;
//
//    /* First call: initialise the timer and return false */
//    if (!rti_timer_valid[id]) {
//        rtiTimerStart(id);
//        return true;
//    }
//
//    /* Make sure counter block 0 is running */
//    if ((rtiREG1->GCTRL & (1U << rtiCOUNTER_BLOCK0)) == 0U) {
//        rtiStartCounter(rtiCOUNTER_BLOCK0);
//    }
//
//    uint32 now      = rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
//    uint32 last     = rti_timer_last[id];
//    uint32 elapsed  = now - last;   /* wraps safely with unsigned arithmetic */
//
//    /* Convert ms+us to RTI ticks */
//    uint64 required_ticks = ((uint64)ms * 1000UL + us) * RTI_US_2_TICKS;
//
//    if (elapsed >= required_ticks) {
//        /* Reload the start time to the current moment (periodic behaviour) */
//        rti_timer_last[id] = now;
//        return true;
//    }
//    return false;
//}
#warning race condition? I think the do while fixed it double check
uint64_t getNow_tick(){
    union nowTimerTicks_t now_tick;
    do{
        now_tick.OFandTickCounter[0] = rti_OverFlowCounter;
    //    now_tick.OFandTickCounter[1] = rtiREG1->CNT[rtiCOUNTER_BLOCK0].FRCx;
        now_tick.OFandTickCounter[1] = *nowTickReg;}
    while(rti_OverFlowCounter != now_tick.OFandTickCounter[0]);
    return now_tick.FullTicks;
}
//uint32_t getNow_us(){
//    return getNow_tick()/RTI_US_2_TICKS;
//}
//uint32_t getNow_ms(){
//    return getNow_tick()/RTI_MS_2_TICKS;
//}
uint64_t timer_tic_tick(){
    return getNow_tick();
}
uint64_t timer_toc_us(const uint64_t tic){
    const uint64_t now = getNow_tick();
    uint64_t diff = (now-tic)/RTI_US_2_TICKS;
    return diff;
}
//------------------------------------------------------------------------------------
bool hasTimeElapsed_rti(const uint64_t start_ticks, uint64_t const wait_ticks){
    const uint64_t now_tick = getNow_tick();
    const bool hasElapsed  = (now_tick - start_ticks > wait_ticks);
    return hasElapsed;
}
bool hasPeriodExpired_rti(const uint64_t period_tick, uint64_t * const last_tick){

    const uint64_t now_tick = getNow_tick();
    const bool hasElapsed  = (now_tick - *last_tick > period_tick);

    if(hasElapsed){
        *last_tick = now_tick;
    }

    return hasElapsed;
}
//------------------------------------------------------------------------------------
uint64_t timeFunction_VoidVoid_us(void (*FuncPrt)(void)){
        const uint64_t tic = timer_tic_tick();

        FuncPrt();

        const uint64_t toc_us = timer_toc_us(tic);
        return toc_us;
}
//------------------------------------------------------------------------------------
uint8_t debounceBool(const bool input, uint32_t * const perv, const uint8_t changeNum){
    const uint32_t changeMask = (1U<<changeNum) -1;

    uint32_t pervVal = *perv;

    pervVal <<=1;
    pervVal |= (uint32_t) input;
    pervVal &= changeMask;

    const bool ValChanged = *perv != pervVal;

    *perv = pervVal;


    if(pervVal == changeMask && ValChanged){
        return 0b10;
    }
    else if(pervVal == 0U && ValChanged){
        return 0b01;
    }
    else{
        return 0b00;
    }
}

inline uint64_t Min_Sec_MS_US_Tick_2_Ticks(const uint16_t min,const uint16_t sec, const uint16_t ms, const uint16_t us, const uint16_t tick){
    const uint64_t min2us = 60e6 * (uint64_t)min;
    const uint64_t sec2us = 1e6  * (uint64_t)sec;
    const uint64_t ms2us  = 1e3  * (uint64_t)ms;
    const uint64_t total_us = min2us + sec2us + ms2us + (uint64_t)us;

    const uint64_t total_ticks = total_us * RTI_US_2_TICKS + tick;

    return total_ticks;
}

//bool debounce_Tflipflop(const bool input, bool currentState, uint32_t * const perv, const uint8_t changeNum){
//
//    uint8_t comand = debounceBool(input, currentState, perv, changeNum);
//}



