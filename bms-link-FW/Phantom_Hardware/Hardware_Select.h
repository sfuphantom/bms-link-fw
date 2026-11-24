/*
 * Hardware_Select.h
 *
 * Master product and board configuration.
 * Select PRODUCT and BOARD here, or pass via compiler -D flags.
 */

#ifndef PHANTOM_HARDWARE_HARDWARE_SELECT_H_
#define PHANTOM_HARDWARE_HARDWARE_SELECT_H_

/* ------------------------------------------------------------------------- */
/* 1. Products                                                               */
/* ------------------------------------------------------------------------- */
#define PRODUCT_BMS         1
#define PRODUCT_VCU         2
#define PRODUCT_OTHER       3

/* ------------------------------------------------------------------------- */
/* 2. Boards                                                                 */
/* ------------------------------------------------------------------------- */
#define BOARD_TMS570_HW     1
#define BOARD_STM32_HW      2
#define BOARD_SIL           3

/* ------------------------------------------------------------------------- */
/* Active Selection                                                          */
/* ------------------------------------------------------------------------- */
#ifndef PRODUCT
#define PRODUCT             PRODUCT_BMS
#endif

#ifndef BOARD
#define BOARD               BOARD_TMS570_HW
#endif

/* ------------------------------------------------------------------------- */
/* Validation                                                                */
/* ------------------------------------------------------------------------- */
#if !defined(PRODUCT) || ((PRODUCT != PRODUCT_BMS) && (PRODUCT != PRODUCT_VCU) && (PRODUCT != PRODUCT_OTHER))
#error "Valid PRODUCT must be set (PRODUCT_BMS, PRODUCT_VCU, PRODUCT_OTHER)"
#endif

#if !defined(BOARD) || ((BOARD != BOARD_TMS570_HW) && (BOARD != BOARD_STM32_HW) && (BOARD != BOARD_SIL))
#error "Valid BOARD must be set (BOARD_TMS570_HW, BOARD_STM32_HW, BOARD_SIL)"
#endif

#endif /* PHANTOM_HARDWARE_HARDWARE_SELECT_H_ */
