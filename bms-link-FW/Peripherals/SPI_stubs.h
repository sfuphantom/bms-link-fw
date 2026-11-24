/*
 * SPI_stubs.h
 *
 * Hardware-independent interface for SPI hardware operations.
 */

#ifndef PERIPHERALS_SPI_STUBS_H_
#define PERIPHERALS_SPI_STUBS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "Hardware_Select.h"

/* SPI Status */
typedef enum {
    SPI_OK = 0,
    SPI_ERR_TIMEOUT,        /* no answer in time */
    SPI_ERR_BUS,            /* an error flag in SPIFLG */
    SPI_ERR_ABORTED,        /* spi_forceEnd was called */
    SPI_ERR_BUSY,           /* someone else has the bus */
    SPI_ERR_ARG             /* invalid argument */
} SpiStatus_t;

/* Byte order for multi-byte transfers */
typedef enum {
    SPI_BIG_ENDIAN,
    SPI_LITTLE_ENDIAN
} SpiEndian_t;

/* Data format identifiers (8-bit vs 16-bit) */
#define SPI_BYTE_FMT            0U
#define SPI_WORD_FMT            1U

/* Dummy byte patterns used for clocking in data */
#define SPI_DUMMY_BYTE          0xFFU
#define SPI_DUMMY_U16           0xFFFFU
#define SPI_DUMMY_U32           0xFFFFFFFFU

/* Hardware initialization */
void        spiHw_init(void);

/* Single word exchange (8-bit or 16-bit format) */
SpiStatus_t spiHw_exchange(uint8_t fmt, uint16_t tx, uint16_t *rx);

/* Check and clear hardware bus errors */
uint32_t    spiHw_errors(void);
void        spiHw_clearErrors(void);

/* Hardware reset (clears shift registers, flags and buffers) */
void        spiHw_reset(void);

#endif /* PERIPHERALS_SPI_STUBS_H_ */
