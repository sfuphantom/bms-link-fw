/*
 * Cpu_stubs.c
 *
 * Board-specific CPU implementation for TMS570.
 */

#include "Hardware_Select.h"
#include "Cpu_stubs.h"

#if BOARD == BOARD_TMS570_HW

#include "sys_common.h"
#include "system.h"
#include "sys_vim.h"
#include "sys_core.h"

void cpuHw_init(void)
{
    systemInit();
    vimInit();
    _enable_IRQ();
    _enable_interrupt_();
}

#elif BOARD == BOARD_STM32_HW
  #error "STM32 CPU stubs not implemented"
#elif BOARD == BOARD_SIL
  #error "SIL CPU stubs not implemented"
#else
  #error "Unknown BOARD"
#endif
