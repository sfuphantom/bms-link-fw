/*
 * GIO_stubs.c
 *
 * TMS570 GIO hardware implementation. The ONLY GIO file including HALCoGen headers.
 */

#include "Hardware_Select.h"
#include "GIO_stubs.h"
#include "Stub_Pins.h"

#if BOARD == BOARD_TMS570_HW

#include "gio.h"
#include "reg_gio.h"
#include "reg_spi.h"
#include "reg_het.h"

/* Forward declare old hooks for weak fallback */
void Debug_GIO_Notification(void);
void IMD_FAULT_GIO_Notification(void);

/* ------------------------------------------------------------------------- */
/* Hardware port register lookup                                             */
/* ------------------------------------------------------------------------- */
static gioPORT_t *gioHw_port(const GioPort_t port)
{
    switch (port) {
        case GIO_PORT_A:    return gioPORTA;
        case GIO_PORT_B:    return gioPORTB;
        case GIO_PORT_SPI1: return spiPORT1;
        case GIO_PORT_SPI2: return spiPORT2;
        case GIO_PORT_SPI3: return spiPORT3;
        case GIO_PORT_SPI4: return spiPORT4;
        case GIO_PORT_SPI5: return spiPORT5;
        case GIO_PORT_HET1: return hetPORT1;
        case GIO_PORT_HET2: return hetPORT2;
        default:            return NULL;
    }
}

static bool decodePin(const GioPin_t pin, gioPORT_t ** const port, uint32_t * const bit)
{
    const GioPort_t id = GIO_ID_PORT(pin);
    if ((uint32_t)id >= (uint32_t)GIO_PORT_COUNT) {
        return false;
    }

    *port = gioHw_port(id);
    *bit  = GIO_ID_BIT(pin);

    return (*port != NULL) && (*bit < GIO_PORT_PINS(id));
}

void gioHw_init(void)
{
    gioInit();
}

GioLevel_t gioHw_read(const GioReadName_t pin)
{
    gioPORT_t *port;
    uint32_t   bit;
    const GioPin_t hwPin = alloc_gioReadPin(pin);

    if (!decodePin(hwPin, &port, &bit)) { return GIO_LOW; }
    return (gioGetBit(port, bit) != 0U) ? GIO_HIGH : GIO_LOW;
}

void gioHw_write(const GioWriteName_t pin, const GioLevel_t level)
{
    gioPORT_t *port;
    uint32_t   bit;
    const GioPin_t hwPin = alloc_gioWritePin(pin);

    if (decodePin(hwPin, &port, &bit)) {
        gioSetBit(port, bit, (uint32)level);
    }
}

GioLevel_t gioHw_toggle(const GioWriteName_t pin)
{
    gioPORT_t *port;
    uint32_t   bit;
    const GioPin_t hwPin = alloc_gioWritePin(pin);

    if (!decodePin(hwPin, &port, &bit)) { return GIO_LOW; }
    gioToggleBit(port, bit);
    return (gioGetBit(port, bit) != 0U) ? GIO_HIGH : GIO_LOW;
}

GioLevel_t gioHw_readOutput(const GioWriteName_t pin)
{
    gioPORT_t *port;
    uint32_t   bit;
    const GioPin_t hwPin = alloc_gioWritePin(pin);

    if (!decodePin(hwPin, &port, &bit)) { return GIO_LOW; }
    return (gioGetBit(port, bit) != 0U) ? GIO_HIGH : GIO_LOW;
}

bool gioHw_intEnable(const GioReadName_t pin)
{
    gioPORT_t *port;
    uint32_t   bit;
    const GioPin_t hwPin = alloc_gioReadPin(pin);

    if (!decodePin(hwPin, &port, &bit)) { return false; }
    if (port != gioPORTA && port != gioPORTB) { return false; }
    gioEnableNotification(port, bit);
    return true;
}

void gioHw_intDisable(const GioReadName_t pin)
{
    gioPORT_t *port;
    uint32_t   bit;
    const GioPin_t hwPin = alloc_gioReadPin(pin);

    if (decodePin(hwPin, &port, &bit)) {
        if (port == gioPORTA || port == gioPORTB) {
            gioDisableNotification(port, bit);
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Interrupt dispatcher                                                      */
/* ------------------------------------------------------------------------- */
void gioNotification(gioPORT_t *port, uint32 bit)
{
    if (port != gioPORTA && port != gioPORTB) { return; }

    const GioPort_t id = (port == gioPORTA) ? GIO_PORT_A : GIO_PORT_B;
    const GioPin_t pin = (GioPin_t)GIO_ID(id, bit);

    const GioReadName_t readName = alloc_gioReadName(pin);
    if (readName != GIO_READ_NONE) {
        gio_handleInterrupt(readName);
    } else if (pin == PIN_A2) {
        /* Debug notification fallback */
        Debug_GIO_Notification();
    }
}

#elif BOARD == BOARD_STM32_HW
  #error "STM32 GIO stubs not implemented"
#elif BOARD == BOARD_SIL
  #error "SIL GIO stubs not implemented"
#else
  #error "Unknown BOARD"
#endif
