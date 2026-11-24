/*
 * GIO_stubs.h
 *
 * Hardware-level GIO wrappers and interrupt handling for TMS570.
 * Only Peripherals/ and stubs may include vendor headers.
 */

#ifndef PERIPHERALS_GIO_STUBS_H_
#define PERIPHERALS_GIO_STUBS_H_

#include <stdint.h>
#include <stdbool.h>
#include "Hardware_Select.h"
#include "PhantomPinAlloc.h"

#if BOARD == BOARD_TMS570_HW

/* ------------------------------------------------------------------------- */
/* Hardware Wrapper Functions                                                */
/* ------------------------------------------------------------------------- */
void       gioHw_init(void);
GioLevel_t gioHw_read(const GioReadName_t pin);
void       gioHw_write(const GioWriteName_t pin, const GioLevel_t level);
GioLevel_t gioHw_toggle(const GioWriteName_t pin);
GioLevel_t gioHw_readOutput(const GioWriteName_t pin);
bool       gioHw_intEnable(const GioReadName_t pin);
void       gioHw_intDisable(const GioReadName_t pin);

/* Upcall from HALCoGen interrupt into helper layer */
void       gio_handleInterrupt(const GioReadName_t pin);

#elif BOARD == BOARD_STM32_HW
  #error "STM32 GIO stubs not implemented"
#elif BOARD == BOARD_SIL
  #error "SIL GIO stubs not implemented"
#else
  #error "Unknown BOARD"
#endif

#endif /* PERIPHERALS_GIO_STUBS_H_ */
