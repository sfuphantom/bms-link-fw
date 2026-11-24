/*
 * spi_helpers.h
 *
 * High-level SPI transaction and bus management helpers.
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_SPI_HELPERS_H_
#define PHANTOM_DRIVERS_INCLUDE_SPI_HELPERS_H_
#ifndef PHANTOM_DRIVERS_INCLUDE_SPI_DRIVERS_H_
#define PHANTOM_DRIVERS_INCLUDE_SPI_DRIVERS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "SPI_stubs.h"
#include "GIO_helpers.h"
#include "PhantomTimers.h"

/* Lets other tasks run while waiting for the bus. */
#ifndef SPI_YIELD
  #define SPI_YIELD()           ((void)0)
#endif

/* Bus ownership types (pass SPI_NO_OWNER to skip locking) */
typedef uint32_t SpiOwner_t;
#define SPI_NO_OWNER    0U
#define SPI_NO_WAIT     1U                  /* lock timeout: one try */

/* Bus ownership functions */
bool spi_isFree(void);
bool spi_isBusy(void);
bool spi_tryLock(const SpiOwner_t owner);
bool spi_lock(const SpiOwner_t owner, const ticks_t timeout);   /* timeout 0 = forever */
void spi_unlock(const SpiOwner_t owner);

/* Abort and resume control */
void spi_forceEnd(void);
void spi_resume(void);
bool spi_isAborted(void);

/* Chip select management */
void spi_csLow(const GioWriteName_t cs);
void spi_csHigh(const GioWriteName_t cs);
bool spi_csIsLow(const GioWriteName_t cs);

/* Transaction scoping */
SpiStatus_t spi_begin(const SpiOwner_t owner, const GioWriteName_t cs, const ticks_t lockTimeout);
void        spi_end(const SpiOwner_t owner, const GioWriteName_t cs);

/* Complete transaction: begin + exchange bytes + end */
SpiStatus_t spi_transact(const SpiOwner_t owner, const GioWriteName_t cs, const ticks_t lockTimeout,
                         const uint8_t * const tx, uint8_t * const rx, const size_t len);

/* High-level exchange functions */
SpiStatus_t spi_exchange_u8(const uint8_t tx, uint8_t * const rx);
SpiStatus_t spi_exchange_u16(const uint16_t tx, uint16_t * const rx, const SpiEndian_t order);
SpiStatus_t spi_exchange_u32(const uint32_t tx, uint32_t * const rx, const SpiEndian_t order);
SpiStatus_t spi_exchange_u16_hw(const uint16_t tx, uint16_t * const rx);

SpiStatus_t spi_exchange_arr8(const uint8_t * const tx, uint8_t * const rx, const size_t count);
SpiStatus_t spi_exchange_arr16(const uint16_t * const tx, uint16_t * const rx, const size_t count, const SpiEndian_t order);
SpiStatus_t spi_exchange_arr32(const uint32_t * const tx, uint32_t * const rx, const size_t count, const SpiEndian_t order);

/* High-level send functions */
SpiStatus_t spi_send_u8(const uint8_t tx);
SpiStatus_t spi_send_u16(const uint16_t tx, const SpiEndian_t order);
SpiStatus_t spi_send_u32(const uint32_t tx, const SpiEndian_t order);
SpiStatus_t spi_send_arr8(const uint8_t * const tx, const size_t count);
SpiStatus_t spi_send_arr16(const uint16_t * const tx, const size_t count, const SpiEndian_t order);
SpiStatus_t spi_send_arr32(const uint32_t * const tx, const size_t count, const SpiEndian_t order);

/* High-level receive functions */
SpiStatus_t spi_receive_u8(uint8_t * const rx);
SpiStatus_t spi_receive_u16(uint16_t * const rx, const SpiEndian_t order);
SpiStatus_t spi_receive_u32(uint32_t * const rx, const SpiEndian_t order);
SpiStatus_t spi_receive_arr8(uint8_t * const rx, const size_t count);
SpiStatus_t spi_receive_arr16(uint16_t * const rx, const size_t count, const SpiEndian_t order);
SpiStatus_t spi_receive_arr32(uint32_t * const rx, const size_t count, const SpiEndian_t order);

/* Polling / waiting on incoming byte matching */
typedef struct {
    uint32_t    clocked;
    SpiStatus_t status;
    bool        done;
} SpiWait_t;

uint32_t spi_wait_value(const uint8_t value, const uint32_t maxBytes);
uint32_t spi_wait_allBits(const uint8_t mask, const uint32_t maxBytes);
uint32_t spi_wait_anyBit(const uint8_t mask, const uint32_t maxBytes);

bool spi_poll_value(SpiWait_t * const w, const uint8_t value, const uint32_t maxBytes);
bool spi_poll_allBits(SpiWait_t * const w, const uint8_t mask, const uint32_t maxBytes);
bool spi_poll_anyBit(SpiWait_t * const w, const uint8_t mask, const uint32_t maxBytes);

#endif /* PHANTOM_DRIVERS_INCLUDE_SPI_DRIVERS_H_ */
#endif /* PHANTOM_DRIVERS_INCLUDE_SPI_HELPERS_H_ */
