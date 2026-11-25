/*
 * PhantomPinAlloc.h
 *
 * Logical hardware names for helpers and application.
 * Contains NO physical pin or port types (per requirement R4).
 */

#ifndef PERIPHERALS_PHANTOMPINALLOC_H_
#define PERIPHERALS_PHANTOMPINALLOC_H_

#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------------- */
/* Common Logical Pin Types                                                  */
/* ------------------------------------------------------------------------- */
typedef enum { GIO_LOW = 0, GIO_HIGH = 1 } GioLevel_t;
typedef enum { GIO_INPUT = 0, GIO_OUTPUT = 1 } GioDir_t;

/* ------------------------------------------------------------------------- */
/* 1. GIO Logical Names (Split Read / Write per R5)                         */
/* ------------------------------------------------------------------------- */
typedef enum {
    GIO_READ_IMD_FAULT,
    GIO_READ_COUNT
} GioReadName_t;

#define GIO_READ_NONE   ((GioReadName_t)0xFFFF)

typedef enum {
    GIO_WRITE_DEBUG,
    GIO_WRITE_BMS_FAULT,
    GIO_WRITE_IMD_FAULT,
    GIO_WRITE_SLAVE_CS,
    GIO_WRITE_HV_CS,
    GIO_WRITE_COUNT
} GioWriteName_t;

#define GIO_WRITE_NONE  ((GioWriteName_t)0xFFFF)

/* ------------------------------------------------------------------------- */
/* 2. PWM Logical Names (Split Read / Write per R5)                         */
/* ------------------------------------------------------------------------- */
typedef enum {
    PWM_READ_IMD_IN,
    PWM_READ_COUNT
} PwmReadName_t;

typedef enum {
    PWM_WRITE_FAN_OUT,
    PWM_WRITE_COUNT
} PwmWriteName_t;

/* ------------------------------------------------------------------------- */
/* 3. CAN Logical Boxes (Split Rx / Tx per R5)                              */
/* ------------------------------------------------------------------------- */
typedef enum {
    CAN_RX_VCU2BMS_DATA,
    CAN_RX_CHARGER2BMS_DATA,
    CAN_RX_COUNT
} CanRxBox_t;

#define CAN_RX_NONE     ((CanRxBox_t)CAN_RX_COUNT)

typedef enum {
    CAN_TX_BMS2ALL_FAULT,
    CAN_TX_BMS2VCU_DATA,
    CAN_TX_BMS2CHARGER_DATA,
    CAN_TX_COUNT
} CanTxBox_t;

#define CAN_TX_NONE     ((CanTxBox_t)CAN_TX_COUNT)

/* ------------------------------------------------------------------------- */
/* 4. Analog Logical Names (ADC Read / DAC Write per R5)                    */
/* ------------------------------------------------------------------------- */
typedef enum {
    ADC_NAME_COUNT = 0
} AdcName_t;

typedef enum {
    DAC_NAME_COUNT = 0
} DacName_t;

/* ------------------------------------------------------------------------- */
/* Startup Hardware Misuse Check (R9)                                       */
/* ------------------------------------------------------------------------- */
bool board_selfCheck(void);

#endif /* PERIPHERALS_PHANTOMPINALLOC_H_ */
