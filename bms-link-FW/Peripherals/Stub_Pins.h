/*
 * Stub_Pins.h
 *
 * Private physical pin definitions, ports, and capability flags for stubs.
 * Helpers and application code must NEVER include this file (per R4).
 */

#ifndef PERIPHERALS_STUB_PINS_H_
#define PERIPHERALS_STUB_PINS_H_

#include <stdint.h>
#include <stdbool.h>
#include "Hardware_Select.h"
#include "PhantomPinAlloc.h"

#if BOARD == BOARD_TMS570_HW

/* ------------------------------------------------------------------------- */
/* Ports with GIO-style register control                                     */
/* ------------------------------------------------------------------------- */
typedef enum {
    GIO_PORT_A     = 0,
    GIO_PORT_B     = 1,
    GIO_PORT_SPI1  = 2,
    GIO_PORT_SPI2  = 3,
    GIO_PORT_SPI3  = 4,
    GIO_PORT_SPI4  = 5,
    GIO_PORT_SPI5  = 6,
    GIO_PORT_HET1  = 7,
    GIO_PORT_HET2  = 8,
    GIO_PORT_COUNT = 9
} GioPort_t;

#define GIO_ID(port, bit)       (((uint32_t)(port) * 32U) + (uint32_t)(bit))
#define GIO_ID_PORT(pin)        ((GioPort_t)((uint32_t)(pin) / 32U))
#define GIO_ID_BIT(pin)         ((uint32_t)(pin) % 32U)
#define GIO_ID_MASK(pin)        (1UL << GIO_ID_BIT(pin))

/* Pins per port on TMS570: A and B have 8, SPI ports have 12, HET has 32 */
#define GIO_PORT_PINS(port)     (((port) <= GIO_PORT_B) ? 8U : (((port) <= GIO_PORT_SPI5) ? 12U : 32U))

/* ------------------------------------------------------------------------- */
/* Physical Pin Definitions                                                  */
/* ------------------------------------------------------------------------- */
typedef enum {
    /* GIO Port A */
    PIN_A0           = GIO_ID(GIO_PORT_A, 0),
    PIN_A1           = GIO_ID(GIO_PORT_A, 1),
    PIN_A2           = GIO_ID(GIO_PORT_A, 2),
    PIN_A3           = GIO_ID(GIO_PORT_A, 3),
    PIN_A4           = GIO_ID(GIO_PORT_A, 4),
    PIN_A5           = GIO_ID(GIO_PORT_A, 5),
    PIN_A6           = GIO_ID(GIO_PORT_A, 6),
    PIN_A7           = GIO_ID(GIO_PORT_A, 7),

    /* GIO Port B */
    PIN_B0           = GIO_ID(GIO_PORT_B, 0),
    PIN_B1           = GIO_ID(GIO_PORT_B, 1),
    PIN_B2           = GIO_ID(GIO_PORT_B, 2),
    PIN_B3           = GIO_ID(GIO_PORT_B, 3),
    PIN_B4           = GIO_ID(GIO_PORT_B, 4),
    PIN_B5           = GIO_ID(GIO_PORT_B, 5),
    PIN_B6           = GIO_ID(GIO_PORT_B, 6),
    PIN_B7           = GIO_ID(GIO_PORT_B, 7),

    /* SPI1 */
    PIN_SPI1_CS0     = GIO_ID(GIO_PORT_SPI1, 0),
    PIN_SPI1_CS1     = GIO_ID(GIO_PORT_SPI1, 1),
    PIN_SPI1_CS2     = GIO_ID(GIO_PORT_SPI1, 2),
    PIN_SPI1_CS3     = GIO_ID(GIO_PORT_SPI1, 3),
    PIN_SPI1_CS4     = GIO_ID(GIO_PORT_SPI1, 4),
    PIN_SPI1_CS5     = GIO_ID(GIO_PORT_SPI1, 5),
    PIN_SPI1_CS6     = GIO_ID(GIO_PORT_SPI1, 6),
    PIN_SPI1_CS7     = GIO_ID(GIO_PORT_SPI1, 7),
    PIN_SPI1_ENA     = GIO_ID(GIO_PORT_SPI1, 8),
    PIN_SPI1_CLK     = GIO_ID(GIO_PORT_SPI1, 9),
    PIN_SPI1_SIMO    = GIO_ID(GIO_PORT_SPI1, 10),
    PIN_SPI1_SOMI    = GIO_ID(GIO_PORT_SPI1, 11),

    /* SPI2 */
    PIN_SPI2_CS0     = GIO_ID(GIO_PORT_SPI2, 0),
    PIN_SPI2_CS1     = GIO_ID(GIO_PORT_SPI2, 1),
    PIN_SPI2_CS2     = GIO_ID(GIO_PORT_SPI2, 2),
    PIN_SPI2_CS3     = GIO_ID(GIO_PORT_SPI2, 3),
    PIN_SPI2_CS4     = GIO_ID(GIO_PORT_SPI2, 4),
    PIN_SPI2_CS5     = GIO_ID(GIO_PORT_SPI2, 5),
    PIN_SPI2_CS6     = GIO_ID(GIO_PORT_SPI2, 6),
    PIN_SPI2_CS7     = GIO_ID(GIO_PORT_SPI2, 7),
    PIN_SPI2_ENA     = GIO_ID(GIO_PORT_SPI2, 8),
    PIN_SPI2_CLK     = GIO_ID(GIO_PORT_SPI2, 9),
    PIN_SPI2_SIMO    = GIO_ID(GIO_PORT_SPI2, 10),
    PIN_SPI2_SOMI    = GIO_ID(GIO_PORT_SPI2, 11),

    /* SPI3 */
    PIN_SPI3_CS0     = GIO_ID(GIO_PORT_SPI3, 0),
    PIN_SPI3_CS1     = GIO_ID(GIO_PORT_SPI3, 1),
    PIN_SPI3_CS2     = GIO_ID(GIO_PORT_SPI3, 2),
    PIN_SPI3_CS3     = GIO_ID(GIO_PORT_SPI3, 3),
    PIN_SPI3_CS4     = GIO_ID(GIO_PORT_SPI3, 4),
    PIN_SPI3_CS5     = GIO_ID(GIO_PORT_SPI3, 5),
    PIN_SPI3_CS6     = GIO_ID(GIO_PORT_SPI3, 6),
    PIN_SPI3_CS7     = GIO_ID(GIO_PORT_SPI3, 7),
    PIN_SPI3_ENA     = GIO_ID(GIO_PORT_SPI3, 8),
    PIN_SPI3_CLK     = GIO_ID(GIO_PORT_SPI3, 9),
    PIN_SPI3_SIMO    = GIO_ID(GIO_PORT_SPI3, 10),
    PIN_SPI3_SOMI    = GIO_ID(GIO_PORT_SPI3, 11),

    /* SPI4 */
    PIN_SPI4_CS0     = GIO_ID(GIO_PORT_SPI4, 0),
    PIN_SPI4_CS1     = GIO_ID(GIO_PORT_SPI4, 1),
    PIN_SPI4_CS2     = GIO_ID(GIO_PORT_SPI4, 2),
    PIN_SPI4_CS3     = GIO_ID(GIO_PORT_SPI4, 3),
    PIN_SPI4_CS4     = GIO_ID(GIO_PORT_SPI4, 4),
    PIN_SPI4_CS5     = GIO_ID(GIO_PORT_SPI4, 5),
    PIN_SPI4_CS6     = GIO_ID(GIO_PORT_SPI4, 6),
    PIN_SPI4_CS7     = GIO_ID(GIO_PORT_SPI4, 7),
    PIN_SPI4_ENA     = GIO_ID(GIO_PORT_SPI4, 8),
    PIN_SPI4_CLK     = GIO_ID(GIO_PORT_SPI4, 9),
    PIN_SPI4_SIMO    = GIO_ID(GIO_PORT_SPI4, 10),
    PIN_SPI4_SOMI    = GIO_ID(GIO_PORT_SPI4, 11),

    /* SPI5 */
    PIN_SPI5_CS0     = GIO_ID(GIO_PORT_SPI5, 0),
    PIN_SPI5_CS1     = GIO_ID(GIO_PORT_SPI5, 1),
    PIN_SPI5_CS2     = GIO_ID(GIO_PORT_SPI5, 2),
    PIN_SPI5_CS3     = GIO_ID(GIO_PORT_SPI5, 3),
    PIN_SPI5_CS4     = GIO_ID(GIO_PORT_SPI5, 4),
    PIN_SPI5_CS5     = GIO_ID(GIO_PORT_SPI5, 5),
    PIN_SPI5_CS6     = GIO_ID(GIO_PORT_SPI5, 6),
    PIN_SPI5_CS7     = GIO_ID(GIO_PORT_SPI5, 7),
    PIN_SPI5_ENA     = GIO_ID(GIO_PORT_SPI5, 8),
    PIN_SPI5_CLK     = GIO_ID(GIO_PORT_SPI5, 9),
    PIN_SPI5_SIMO    = GIO_ID(GIO_PORT_SPI5, 10),
    PIN_SPI5_SOMI    = GIO_ID(GIO_PORT_SPI5, 11),

    /* HET1 */
    PIN_HET1_0       = GIO_ID(GIO_PORT_HET1, 0),
    PIN_HET1_1       = GIO_ID(GIO_PORT_HET1, 1),
    PIN_HET1_2       = GIO_ID(GIO_PORT_HET1, 2),
    PIN_HET1_3       = GIO_ID(GIO_PORT_HET1, 3),
    PIN_HET1_4       = GIO_ID(GIO_PORT_HET1, 4),
    PIN_HET1_5       = GIO_ID(GIO_PORT_HET1, 5),
    PIN_HET1_6       = GIO_ID(GIO_PORT_HET1, 6),
    PIN_HET1_7       = GIO_ID(GIO_PORT_HET1, 7),
    PIN_HET1_8       = GIO_ID(GIO_PORT_HET1, 8),
    PIN_HET1_9       = GIO_ID(GIO_PORT_HET1, 9),
    PIN_HET1_10      = GIO_ID(GIO_PORT_HET1, 10),
    PIN_HET1_11      = GIO_ID(GIO_PORT_HET1, 11),
    PIN_HET1_12      = GIO_ID(GIO_PORT_HET1, 12),
    PIN_HET1_13      = GIO_ID(GIO_PORT_HET1, 13),
    PIN_HET1_14      = GIO_ID(GIO_PORT_HET1, 14),
    PIN_HET1_15      = GIO_ID(GIO_PORT_HET1, 15),
    PIN_HET1_16      = GIO_ID(GIO_PORT_HET1, 16),
    PIN_HET1_17      = GIO_ID(GIO_PORT_HET1, 17),
    PIN_HET1_18      = GIO_ID(GIO_PORT_HET1, 18),
    PIN_HET1_19      = GIO_ID(GIO_PORT_HET1, 19),
    PIN_HET1_20      = GIO_ID(GIO_PORT_HET1, 20),
    PIN_HET1_21      = GIO_ID(GIO_PORT_HET1, 21),
    PIN_HET1_22      = GIO_ID(GIO_PORT_HET1, 22),
    PIN_HET1_23      = GIO_ID(GIO_PORT_HET1, 23),
    PIN_HET1_24      = GIO_ID(GIO_PORT_HET1, 24),
    PIN_HET1_25      = GIO_ID(GIO_PORT_HET1, 25),
    PIN_HET1_26      = GIO_ID(GIO_PORT_HET1, 26),
    PIN_HET1_27      = GIO_ID(GIO_PORT_HET1, 27),
    PIN_HET1_28      = GIO_ID(GIO_PORT_HET1, 28),
    PIN_HET1_29      = GIO_ID(GIO_PORT_HET1, 29),
    PIN_HET1_30      = GIO_ID(GIO_PORT_HET1, 30),
    PIN_HET1_31      = GIO_ID(GIO_PORT_HET1, 31),

    /* HET2 */
    PIN_HET2_0       = GIO_ID(GIO_PORT_HET2, 0),
    PIN_HET2_1       = GIO_ID(GIO_PORT_HET2, 1),
    PIN_HET2_2       = GIO_ID(GIO_PORT_HET2, 2),
    PIN_HET2_3       = GIO_ID(GIO_PORT_HET2, 3),
    PIN_HET2_4       = GIO_ID(GIO_PORT_HET2, 4),
    PIN_HET2_5       = GIO_ID(GIO_PORT_HET2, 5),
    PIN_HET2_6       = GIO_ID(GIO_PORT_HET2, 6),
    PIN_HET2_7       = GIO_ID(GIO_PORT_HET2, 7),
    PIN_HET2_8       = GIO_ID(GIO_PORT_HET2, 8),
    PIN_HET2_9       = GIO_ID(GIO_PORT_HET2, 9),
    PIN_HET2_10      = GIO_ID(GIO_PORT_HET2, 10),
    PIN_HET2_11      = GIO_ID(GIO_PORT_HET2, 11),
    PIN_HET2_12      = GIO_ID(GIO_PORT_HET2, 12),
    PIN_HET2_13      = GIO_ID(GIO_PORT_HET2, 13),
    PIN_HET2_14      = GIO_ID(GIO_PORT_HET2, 14),
    PIN_HET2_15      = GIO_ID(GIO_PORT_HET2, 15),
    PIN_HET2_16      = GIO_ID(GIO_PORT_HET2, 16),
    PIN_HET2_17      = GIO_ID(GIO_PORT_HET2, 17),
    PIN_HET2_18      = GIO_ID(GIO_PORT_HET2, 18),
    PIN_HET2_19      = GIO_ID(GIO_PORT_HET2, 19),
    PIN_HET2_20      = GIO_ID(GIO_PORT_HET2, 20),
    PIN_HET2_21      = GIO_ID(GIO_PORT_HET2, 21),
    PIN_HET2_22      = GIO_ID(GIO_PORT_HET2, 22),
    PIN_HET2_23      = GIO_ID(GIO_PORT_HET2, 23),
    PIN_HET2_24      = GIO_ID(GIO_PORT_HET2, 24),
    PIN_HET2_25      = GIO_ID(GIO_PORT_HET2, 25),
    PIN_HET2_26      = GIO_ID(GIO_PORT_HET2, 26),
    PIN_HET2_27      = GIO_ID(GIO_PORT_HET2, 27),
    PIN_HET2_28      = GIO_ID(GIO_PORT_HET2, 28),
    PIN_HET2_29      = GIO_ID(GIO_PORT_HET2, 29),
    PIN_HET2_30      = GIO_ID(GIO_PORT_HET2, 30),
    PIN_HET2_31      = GIO_ID(GIO_PORT_HET2, 31),

    GIO_PIN_NONE     = 0xFFFF
} GioPin_t;

/* ------------------------------------------------------------------------- */
/* Pin Capability Flags (32-bit bitfield per R7)                             */
/* ------------------------------------------------------------------------- */
#define F_GIO        (1UL << 0)    /* SPI CS pins are GIO */
#define F_PWM_READ   (1UL << 1)
#define F_PWM_WRITE  (1UL << 2)
#define F_SPI_CLK    (1UL << 3)
#define F_SPI_MISO   (1UL << 4)
#define F_SPI_MOSI   (1UL << 5)
#define F_ADC        (1UL << 6)
#define F_DAC        (1UL << 7)
#define F_CAN        (1UL << 8)
#define F_LED        (1UL << 9)
#define F_MOSFET     (1UL << 10)
#define F_BROKEN_OUT (1UL << 11)
#define F_3V3        (1UL << 12)
#define F_5V         (1UL << 13)
#define F_12V        (1UL << 14)
#define F_POWER      (1UL << 15)
#define F_GND        (1UL << 16)
#define F_I2C_SDA    (1UL << 17)
#define F_I2C_SCL    (1UL << 18)
#define F_UART_TX    (1UL << 19)
#define F_UART_RX    (1UL << 20)
#define F_IRQ        (1UL << 21)
#define F_5V_TOL     (1UL << 22)
#define F_RESERVED   (1UL << 23)

typedef struct {
    uint16_t chipPin;   /* Package pin number (or placeholder /* TODO * / per R8) */
    uint32_t flags;     /* Capability bitmask (F_*) */
} PinInfo_t;

/* Physical lookup functions for stubs */
GioPin_t       alloc_gioReadPin(const GioReadName_t name);
GioPin_t       alloc_gioWritePin(const GioWriteName_t name);
GioReadName_t  alloc_gioReadName(const GioPin_t pin);
uint32_t       alloc_canRxHwBox(const CanRxBox_t box);
uint32_t       alloc_canTxHwBox(const CanTxBox_t box);
CanRxBox_t     alloc_canRxBox(const uint32_t hwBox);
CanTxBox_t     alloc_canTxBox(const uint32_t hwBox);
const PinInfo_t * pin_getInfo(const GioPin_t pin);

#elif BOARD == BOARD_STM32_HW
  #error "STM32 Stub_Pins not implemented"
#elif BOARD == BOARD_SIL
  #error "SIL Stub_Pins not implemented"
#else
  #error "Unknown BOARD"
#endif

#endif /* PERIPHERALS_STUB_PINS_H_ */
