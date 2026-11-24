/*
 * PhantomPinAlloc.c
 *
 * Board and product pin mappings with compile-time assertions and runtime checks.
 */

#include <stddef.h>
#include "Hardware_Select.h"
#include "PhantomPinAlloc.h"
#include "Stub_Pins.h"

#if BOARD == BOARD_TMS570_HW

/* ------------------------------------------------------------------------- */
/* Pin Capability and Package Info Table (TMS570)                            */
/* ------------------------------------------------------------------------- */
typedef struct {
    GioPin_t  pin;
    PinInfo_t info;
} BoardPinEntry_t;

static const BoardPinEntry_t pinTable[] = {
    { PIN_A2,        { 0 /* TODO */, F_GIO | F_BROKEN_OUT } },
    { PIN_A6,        { 0 /* TODO */, F_GIO | F_IRQ | F_BROKEN_OUT } },
    { PIN_A7,        { 0 /* TODO */, F_GIO | F_MOSFET } },
    { PIN_SPI3_CS0,  { 0 /* TODO */, F_GIO } },
    { PIN_SPI3_CS5,  { 0 /* TODO */, F_GIO } },
    { PIN_HET1_0,    { 0 /* TODO */, F_PWM_WRITE } },
};

const PinInfo_t * pin_getInfo(const GioPin_t pin)
{
    size_t i;
    for (i = 0; i < (sizeof(pinTable) / sizeof(pinTable[0])); i++) {
        if (pinTable[i].pin == pin) {
            return &pinTable[i].info;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------------- */
/* Product Pin Allocation Tables                                             */
/* ------------------------------------------------------------------------- */
#if PRODUCT == PRODUCT_BMS

static const GioPin_t gioReadRows[GIO_READ_COUNT] = {
    [GIO_READ_IMD_FAULT]  = PIN_A6,
};
_Static_assert((sizeof(gioReadRows) / sizeof(gioReadRows[0])) == GIO_READ_COUNT, "gioReadRows count mismatch");

static const GioPin_t gioWriteRows[GIO_WRITE_COUNT] = {
    [GIO_WRITE_DEBUG]     = PIN_A2,
    [GIO_WRITE_BMS_FAULT] = PIN_A7,
    [GIO_WRITE_IMD_FAULT] = PIN_A6,
    [GIO_WRITE_SLAVE_CS]  = PIN_SPI3_CS0,
    [GIO_WRITE_HV_CS]     = PIN_SPI3_CS5,
};
_Static_assert((sizeof(gioWriteRows) / sizeof(gioWriteRows[0])) == GIO_WRITE_COUNT, "gioWriteRows count mismatch");

GioPin_t alloc_gioReadPin(const GioReadName_t name)
{
    if ((size_t)name >= (size_t)GIO_READ_COUNT) {
        return GIO_PIN_NONE;
    }
    return gioReadRows[name];
}

GioPin_t alloc_gioWritePin(const GioWriteName_t name)
{
    if ((size_t)name >= (size_t)GIO_WRITE_COUNT) {
        return GIO_PIN_NONE;
    }
    return gioWriteRows[name];
}

GioReadName_t alloc_gioReadName(const GioPin_t pin)
{
    size_t i;
    for (i = 0; i < (size_t)GIO_READ_COUNT; i++) {
        if (gioReadRows[i] == pin) {
            return (GioReadName_t)i;
        }
    }
    return GIO_READ_NONE;
}

#elif PRODUCT == PRODUCT_VCU
  #error "VCU pin allocation not implemented"
#elif PRODUCT == PRODUCT_OTHER
  #error "OTHER pin allocation not implemented"
#else
  #error "Unknown PRODUCT"
#endif

/* ------------------------------------------------------------------------- */
/* Misuse Protection Check (R9)                                              */
/* ------------------------------------------------------------------------- */
bool board_selfCheck(void)
{
    bool ok = true;
    size_t i;

    /* Verify GIO read pins have F_GIO and are not reserved/power */
    for (i = 0; i < (size_t)GIO_READ_COUNT; i++) {
        const GioPin_t pin = alloc_gioReadPin((GioReadName_t)i);
        const PinInfo_t * const info = pin_getInfo(pin);
        if (info == NULL) {
            ok = false;
        } else {
            if ((info->flags & F_GIO) == 0U) { ok = false; }
            if ((info->flags & (F_RESERVED | F_GND | F_POWER)) != 0U) { ok = false; }
        }
    }

    /* Verify GIO write pins have F_GIO and are not reserved/power */
    for (i = 0; i < (size_t)GIO_WRITE_COUNT; i++) {
        const GioPin_t pin = alloc_gioWritePin((GioWriteName_t)i);
        const PinInfo_t * const info = pin_getInfo(pin);
        if (info == NULL) {
            ok = false;
        } else {
            if ((info->flags & F_GIO) == 0U) { ok = false; }
            if ((info->flags & (F_RESERVED | F_GND | F_POWER)) != 0U) { ok = false; }
        }
    }

    return ok;
}

#elif BOARD == BOARD_STM32_HW
  #error "STM32 PhantomPinAlloc not implemented"
#elif BOARD == BOARD_SIL
  #error "SIL PhantomPinAlloc not implemented"
#else
  #error "Unknown BOARD"
#endif
