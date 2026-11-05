/*
 * spi_drivers.h
 *
 *  Created on: Jul 2, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_SPI_DRIVERS_H_
#define PHANTOM_DRIVERS_INCLUDE_SPI_DRIVERS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "spi.h"
#include "GIO_helpers.h"
#include "PhantomTimers.h"

//---------------------------------------------------------------------------------------------------------
// Settings
//---------------------------------------------------------------------------------------------------------
#ifndef REG_FOR_SPI
  #define REG_FOR_SPI           spiREG3
#endif

#define SPI_BYTE_FMT            SPI_FMT_0           // 8 bit format
#define SPI_WORD_FMT            SPI_FMT_1           // 16 bit format
#define SPI_HW_CSNR             0x01U               // CS0 is a GIO pin here, so the hardware CS is not used
#define SPI_ERR_FLAGS           0x5FU               // DLENERR, TIMEOUT, PARITYERR, DESYNC, BITERR, RXOVR
#define SPI_FLG_RXINT           0x100U
#define SPI_GCR1_SPIEN          (1UL << 24U)
#define SPI_BYTE_TIMEOUT_US     1000U               // give up on one byte after this

// 1 = spiHw_exchange uses spiTransmitAndReceiveData from spi.h. That function has no timeout
// and spi_forceEnd cannot free it, so it stays 0 unless you accept that.
#ifndef SPI_USE_HAL_EXCHANGE
  #define SPI_USE_HAL_EXCHANGE  0
#endif

// Lets other tasks run while waiting for the bus. Define it as taskYIELD() or similar.
#ifndef SPI_YIELD
  #define SPI_YIELD()           ((void)0)
#endif

#define SPI_DUMMY_BYTE          0xFFU
#define SPI_DUMMY_U16           0xFFFFU
#define SPI_DUMMY_U32           0xFFFFFFFFU

//---------------------------------------------------------------------------------------------------------
// Types
//---------------------------------------------------------------------------------------------------------
typedef enum {
    SPI_OK = 0,
    SPI_ERR_TIMEOUT,        // no answer in time
    SPI_ERR_BUS,            // an error flag in SPIFLG
    SPI_ERR_ABORTED,        // spi_forceEnd was called
    SPI_ERR_BUSY,           // someone else has the bus
    SPI_ERR_ARG,
} SpiStatus_t;

typedef enum { SPI_BIG_ENDIAN, SPI_LITTLE_ENDIAN } SpiEndian_t;     // which byte of a value goes out first

typedef uint32_t SpiOwner_t;                // any nonzero number that is different for each task
#define SPI_NO_OWNER    0U
#define SPI_NO_WAIT     1U                  // lock timeout: one try (0 would mean forever)

//---------------------------------------------------------------------------------------------------------
// Hardware wrapper. The only functions that call spi.h or touch the SPI registers.
// For another processor or SPI module, rewrite these four.
//---------------------------------------------------------------------------------------------------------
SpiStatus_t spiHw_exchange(const SPIDATAFMT_t fmt, const uint16_t tx, uint16_t * const rx);   // one word
uint32_t    spiHw_errors(void);             // error flags that are set, 0 = none
void        spiHw_clearErrors(void);
void        spiHw_reset(void);              // drops whatever is being shifted

//---------------------------------------------------------------------------------------------------------
// Bus ownership (for an RTOS). Everyone who shares the bus has to use it, the exchange functions
// do not check it. Pass SPI_NO_OWNER to spi_begin / spi_end / spi_transact to skip the lock.
//---------------------------------------------------------------------------------------------------------
bool spi_isFree(void);
bool spi_isBusy(void);
bool spi_tryLock(const SpiOwner_t owner);
bool spi_lock(const SpiOwner_t owner, const ticks_t timeout);   // timeout 0 = forever
void spi_unlock(const SpiOwner_t owner);                        // does nothing if owner does not have the bus

// Safe to call from an interrupt. Stops the message in progress (every exchange returns SPI_ERR_ABORTED),
// raises the chip select, resets the SPI shift logic and frees the bus. Stays stopped until spi_resume().
void spi_forceEnd(void);
void spi_resume(void);
bool spi_isAborted(void);

//---------------------------------------------------------------------------------------------------------
// Chip select (any GIO pin, see GIO_helpers.h)
//---------------------------------------------------------------------------------------------------------
void spi_csLow(const GioName_t cs);
void spi_csHigh(const GioName_t cs);
bool spi_csIsLow(const GioName_t cs);

// lock + clear errors + CS low        /      CS high + unlock
SpiStatus_t spi_begin(const SpiOwner_t owner, const GioName_t cs, const ticks_t lockTimeout);
void        spi_end(const SpiOwner_t owner, const GioName_t cs);

// begin + exchange bytes + end, in one call. tx or rx may be NULL.
SpiStatus_t spi_transact(const SpiOwner_t owner, const GioName_t cs, const ticks_t lockTimeout,
                         const uint8_t * const tx, uint8_t * const rx, const size_t len);

//---------------------------------------------------------------------------------------------------------
// exchange = send and receive at the same time.
// send     = exchange, what comes back is dropped.
// receive  = exchange, dummy bytes go out (SPI_DUMMY_*). Passing NULL as rx drops the data (just clocks).
// order only matters for 16 and 32 bit values: the byte that goes out first.
// All of them return as soon as something fails; the received data is then incomplete.
//---------------------------------------------------------------------------------------------------------
SpiStatus_t spi_exchange_u8(const uint8_t tx, uint8_t * const rx);
SpiStatus_t spi_exchange_u16(const uint16_t tx, uint16_t * const rx, const SpiEndian_t order);
SpiStatus_t spi_exchange_u32(const uint32_t tx, uint32_t * const rx, const SpiEndian_t order);
SpiStatus_t spi_exchange_u16_hw(const uint16_t tx, uint16_t * const rx);    // one 16 bit hardware word, MSB first, faster

SpiStatus_t spi_exchange_arr8(const uint8_t * const tx, uint8_t * const rx, const size_t count);
SpiStatus_t spi_exchange_arr16(const uint16_t * const tx, uint16_t * const rx, const size_t count, const SpiEndian_t order);
SpiStatus_t spi_exchange_arr32(const uint32_t * const tx, uint32_t * const rx, const size_t count, const SpiEndian_t order);

SpiStatus_t spi_send_u8(const uint8_t tx);
SpiStatus_t spi_send_u16(const uint16_t tx, const SpiEndian_t order);
SpiStatus_t spi_send_u32(const uint32_t tx, const SpiEndian_t order);
SpiStatus_t spi_send_arr8(const uint8_t * const tx, const size_t count);
SpiStatus_t spi_send_arr16(const uint16_t * const tx, const size_t count, const SpiEndian_t order);
SpiStatus_t spi_send_arr32(const uint32_t * const tx, const size_t count, const SpiEndian_t order);

SpiStatus_t spi_receive_u8(uint8_t * const rx);
SpiStatus_t spi_receive_u16(uint16_t * const rx, const SpiEndian_t order);
SpiStatus_t spi_receive_u32(uint32_t * const rx, const SpiEndian_t order);
SpiStatus_t spi_receive_arr8(uint8_t * const rx, const size_t count);
SpiStatus_t spi_receive_arr16(uint16_t * const rx, const size_t count, const SpiEndian_t order);
SpiStatus_t spi_receive_arr32(uint32_t * const rx, const size_t count, const SpiEndian_t order);

//---------------------------------------------------------------------------------------------------------
// Keep clocking dummy bytes until what comes back matches.
//   value    : byte == value
//   allBits  : every bit of mask is 1  ((byte & mask) == mask)
//   anyBit   : at least one bit of mask is 1  ((byte & mask) != 0)
// maxBytes 0 = no limit (spi_forceEnd still stops it).
//
// spi_wait_*: blocking. Returns how many bytes were clocked, 0 if it never matched (timeout, error or abort).
// spi_poll_*: not blocking. Clocks ONE byte per call and returns true only when it matched.
//             Start the SpiWait_t as {0}. done with status != SPI_OK means it failed, the next call starts over.
//---------------------------------------------------------------------------------------------------------
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
