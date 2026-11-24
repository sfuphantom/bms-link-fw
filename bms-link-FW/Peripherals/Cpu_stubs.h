/*
 * Cpu_stubs.h
 *
 * CPU-level operations: critical sections, compiler intrinsics, and hardware init.
 */

#ifndef PERIPHERALS_CPU_STUBS_H_
#define PERIPHERALS_CPU_STUBS_H_

#include <stdint.h>
#include <stdbool.h>
#include "Hardware_Select.h"

#if BOARD == BOARD_TMS570_HW

#if defined(__TI_COMPILER_VERSION__)
  #define CRITICAL_ENTER(state)   do { (state) = _get_CPSR(); _disable_IRQ(); } while (0)
  #define CRITICAL_EXIT(state)    do { if (((state) & 0x80U) == 0U) { _enable_IRQ(); } } while (0)
#else
  #define CRITICAL_ENTER(state)   ((void)((state) = 0U))
  #define CRITICAL_EXIT(state)    ((void)(state))
#endif

#elif BOARD == BOARD_STM32_HW
  #error "STM32 CPU stubs not implemented"
#elif BOARD == BOARD_SIL
  #error "SIL CPU stubs not implemented"
#else
  #error "Unknown BOARD"
#endif

/* System and interrupt controller initialization */
void cpuHw_init(void);

#endif /* PERIPHERALS_CPU_STUBS_H_ */
