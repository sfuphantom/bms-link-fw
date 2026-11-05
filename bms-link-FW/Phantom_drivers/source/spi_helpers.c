/*
 * spi_drivers.c
 *
 *  Created on: Jul 2, 2026
 *      Author: tanjo
 */

#include "spi.h"
#include "SPI_helpers.h"
#include "PhantomAlgorithms.h"

#define SPI_BYTE_TIMEOUT_TICKS      US_TO_TICKS(SPI_BYTE_TIMEOUT_US)

typedef struct {
    volatile SpiOwner_t owner;
    volatile bool       abort;
    volatile GioName_t   activeCS;
} SpiBus_t;

static SpiBus_t bus = { SPI_NO_OWNER, false, GIO_NAME_PIN_NONE };

//---------------------------------------------------------------------------------------------------------
// Hardware wrapper: the only code that calls spi.h or touches the SPI registers
//---------------------------------------------------------------------------------------------------------
uint32_t spiHw_errors(void)
{
    return REG_FOR_SPI->FLG & SPI_ERR_FLAGS;
}

void spiHw_clearErrors(void)
{
    REG_FOR_SPI->FLG = SPI_ERR_FLAGS;       // write 1 to clear
}

// SPIEN low clears the shift registers, flags and buffers
void spiHw_reset(void)
{
    REG_FOR_SPI->GCR1 &= ~SPI_GCR1_SPIEN;
    REG_FOR_SPI->GCR1 |=  SPI_GCR1_SPIEN;
}

SpiStatus_t spiHw_exchange(const SPIDATAFMT_t fmt, const uint16_t tx, uint16_t * const rx)
{
    if (bus.abort) { return SPI_ERR_ABORTED; }

#if SPI_USE_HAL_EXCHANGE
    spiDAT1_t config;
    uint16    txWord = tx;
    uint16    rxWord = 0U;

    config.CS_HOLD = 0U;
    config.WDEL    = 0U;
    config.DFSEL   = fmt;
    config.CSNR    = SPI_HW_CSNR;

    (void)spiTransmitAndReceiveData(REG_FOR_SPI, &config, 1U, &txWord, &rxWord);
    *rx = rxWord;
#else
    REG_FOR_SPI->DAT1 = ((uint32_t)fmt << 24U) | ((uint32_t)SPI_HW_CSNR << 16U) | (uint32_t)tx;

    const ticks_t start = timer_tic_tick();
    while ((REG_FOR_SPI->FLG & SPI_FLG_RXINT) == 0U) {
        if (bus.abort)                                     { return SPI_ERR_ABORTED; }
        if (hasTimeElapsed(start, SPI_BYTE_TIMEOUT_TICKS)) { return SPI_ERR_TIMEOUT; }
    }

    *rx = (uint16_t)REG_FOR_SPI->BUF;       // reading BUF clears the RX flag
#endif

    return (spiHw_errors() != 0U) ? SPI_ERR_BUS : SPI_OK;
}

//---------------------------------------------------------------------------------------------------------
// Bus ownership
//---------------------------------------------------------------------------------------------------------
bool spi_isFree(void) { return bus.owner == SPI_NO_OWNER; }
bool spi_isBusy(void) { return bus.owner != SPI_NO_OWNER; }

bool spi_tryLock(const SpiOwner_t owner)
{
    uint32_t irq;
    bool     got = false;

    if (owner == SPI_NO_OWNER) { return false; }

    CRITICAL_ENTER(irq);
    if (bus.owner == SPI_NO_OWNER && !bus.abort) {
        bus.owner = owner;
        got = true;
    }
    CRITICAL_EXIT(irq);

    return got;
}

bool spi_lock(const SpiOwner_t owner, const ticks_t timeout)
{
    Timeout_t t;
    timeout_start(&t, timeout);

    while (!spi_tryLock(owner)) {
        if (bus.abort)                              { return false; }
        if (timeout != 0U && timeout_expired(&t))   { return false; }
        SPI_YIELD();
    }
    return true;
}

void spi_unlock(const SpiOwner_t owner)
{
    uint32_t irq;

    CRITICAL_ENTER(irq);
    if (bus.owner == owner) { bus.owner = SPI_NO_OWNER; }
    CRITICAL_EXIT(irq);
}

void spi_forceEnd(void)
{
    const GioName_t cs = bus.activeCS;

    bus.abort = true;                       // first, so an exchange in progress stops

    if (cs != GIO_PIN_NONE) {
        gio_write(cs, GIO_HIGH);
        bus.activeCS = GIO_NAME_PIN_NONE;
    }
    spiHw_reset();
    bus.owner = SPI_NO_OWNER;
}

void spi_resume(void)
{
    spiHw_clearErrors();
    bus.abort = false;
}

bool spi_isAborted(void) { return bus.abort; }

//---------------------------------------------------------------------------------------------------------
// Chip select
//---------------------------------------------------------------------------------------------------------
void spi_csLow(const GioName_t cs)
{
    bus.activeCS = cs;
    gio_write(cs, GIO_LOW);
}

void spi_csHigh(const GioName_t cs)
{
    gio_write(cs, GIO_HIGH);
    if (bus.activeCS == cs) { bus.activeCS = GIO_NAME_PIN_NONE; }
}

bool spi_csIsLow(const GioName_t cs)
{
    return gio_read(cs) == GIO_LOW;
}

SpiStatus_t spi_begin(const SpiOwner_t owner, const GioName_t cs, const ticks_t lockTimeout)
{
    if (owner != SPI_NO_OWNER && !spi_lock(owner, lockTimeout)) {
        return bus.abort ? SPI_ERR_ABORTED : SPI_ERR_BUSY;
    }
    if (bus.abort) { return SPI_ERR_ABORTED; }

    spiHw_clearErrors();
    spi_csLow(cs);
    return SPI_OK;
}

void spi_end(const SpiOwner_t owner, const GioName_t cs)
{
    // After spi_forceEnd the bus (and the CS) may belong to someone else, leave them alone
    if (owner != SPI_NO_OWNER && bus.owner != owner) { return; }

    spi_csHigh(cs);
    spi_unlock(owner);
}

SpiStatus_t spi_transact(const SpiOwner_t owner, const GioName_t cs, const ticks_t lockTimeout,
                         const uint8_t * const tx, uint8_t * const rx, const size_t len)
{
    SpiStatus_t status = spi_begin(owner, cs, lockTimeout);
    if (status != SPI_OK) { return status; }

    status = spi_exchange_arr8(tx, rx, len);

    spi_end(owner, cs);
    return status;
}

//---------------------------------------------------------------------------------------------------------
// Exchange
//---------------------------------------------------------------------------------------------------------
SpiStatus_t spi_exchange_u8(const uint8_t tx, uint8_t * const rx)
{
    uint16_t rxWord = 0U;
    const SpiStatus_t status = spiHw_exchange(SPI_BYTE_FMT, tx, &rxWord);

    if (rx != NULL) { *rx = (uint8_t)rxWord; }
    return status;
}

SpiStatus_t spi_exchange_u16_hw(const uint16_t tx, uint16_t * const rx)
{
    uint16_t rxWord = 0U;
    const SpiStatus_t status = spiHw_exchange(SPI_WORD_FMT, tx, &rxWord);

    if (rx != NULL) { *rx = rxWord; }
    return status;
}

// 2 or 4 byte value, one byte at a time. Byte k of a value is (value >> 8k), on any processor.
static SpiStatus_t exchangeValue(const uint32_t tx, uint32_t * const rx, const uint8_t bytes, const SpiEndian_t order)
{
    uint32_t rxValue = 0U;
    uint8_t  i;

    for (i = 0; i < bytes; i++) {
        const uint8_t k = (order == SPI_BIG_ENDIAN) ? (uint8_t)(bytes - 1U - i) : i;
        uint8_t       rxByte = 0U;

        const SpiStatus_t status = spi_exchange_u8((uint8_t)(tx >> (8U * k)), &rxByte);
        if (status != SPI_OK) { return status; }

        rxValue |= (uint32_t)rxByte << (8U * k);
    }

    if (rx != NULL) { *rx = rxValue; }
    return SPI_OK;
}

SpiStatus_t spi_exchange_u16(const uint16_t tx, uint16_t * const rx, const SpiEndian_t order)
{
    uint32_t rxValue = 0U;
    const SpiStatus_t status = exchangeValue(tx, &rxValue, 2U, order);

    if (status == SPI_OK && rx != NULL) { *rx = (uint16_t)rxValue; }
    return status;
}

SpiStatus_t spi_exchange_u32(const uint32_t tx, uint32_t * const rx, const SpiEndian_t order)
{
    return exchangeValue(tx, rx, 4U, order);
}

SpiStatus_t spi_exchange_arr8(const uint8_t * const tx, uint8_t * const rx, const size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) {
        uint8_t rxByte = 0U;
        const SpiStatus_t status = spi_exchange_u8((tx != NULL) ? tx[i] : SPI_DUMMY_BYTE, &rxByte);

        if (status != SPI_OK) { return status; }
        if (rx != NULL)       { rx[i] = rxByte; }
    }
    return SPI_OK;
}

SpiStatus_t spi_exchange_arr16(const uint16_t * const tx, uint16_t * const rx, const size_t count, const SpiEndian_t order)
{
    size_t i;
    for (i = 0; i < count; i++) {
        uint16_t rxWord = 0U;
        const SpiStatus_t status = spi_exchange_u16((tx != NULL) ? tx[i] : SPI_DUMMY_U16, &rxWord, order);

        if (status != SPI_OK) { return status; }
        if (rx != NULL)       { rx[i] = rxWord; }
    }
    return SPI_OK;
}

SpiStatus_t spi_exchange_arr32(const uint32_t * const tx, uint32_t * const rx, const size_t count, const SpiEndian_t order)
{
    size_t i;
    for (i = 0; i < count; i++) {
        uint32_t rxWord = 0U;
        const SpiStatus_t status = spi_exchange_u32((tx != NULL) ? tx[i] : SPI_DUMMY_U32, &rxWord, order);

        if (status != SPI_OK) { return status; }
        if (rx != NULL)       { rx[i] = rxWord; }
    }
    return SPI_OK;
}

//---------------------------------------------------------------------------------------------------------
// Send and receive
//---------------------------------------------------------------------------------------------------------
SpiStatus_t spi_send_u8(const uint8_t tx)                                          { return spi_exchange_u8(tx, NULL); }
SpiStatus_t spi_send_u16(const uint16_t tx, const SpiEndian_t order)               { return spi_exchange_u16(tx, NULL, order); }
SpiStatus_t spi_send_u32(const uint32_t tx, const SpiEndian_t order)               { return spi_exchange_u32(tx, NULL, order); }
SpiStatus_t spi_send_arr8(const uint8_t * const tx, const size_t count)            { return spi_exchange_arr8(tx, NULL, count); }
SpiStatus_t spi_send_arr16(const uint16_t * const tx, const size_t count, const SpiEndian_t order)  { return spi_exchange_arr16(tx, NULL, count, order); }
SpiStatus_t spi_send_arr32(const uint32_t * const tx, const size_t count, const SpiEndian_t order)  { return spi_exchange_arr32(tx, NULL, count, order); }

SpiStatus_t spi_receive_u8(uint8_t * const rx)                                     { return spi_exchange_u8(SPI_DUMMY_BYTE, rx); }
SpiStatus_t spi_receive_u16(uint16_t * const rx, const SpiEndian_t order)          { return spi_exchange_u16(SPI_DUMMY_U16, rx, order); }
SpiStatus_t spi_receive_u32(uint32_t * const rx, const SpiEndian_t order)          { return spi_exchange_u32(SPI_DUMMY_U32, rx, order); }
SpiStatus_t spi_receive_arr8(uint8_t * const rx, const size_t count)               { return spi_exchange_arr8(NULL, rx, count); }
SpiStatus_t spi_receive_arr16(uint16_t * const rx, const size_t count, const SpiEndian_t order)     { return spi_exchange_arr16(NULL, rx, count, order); }
SpiStatus_t spi_receive_arr32(uint32_t * const rx, const size_t count, const SpiEndian_t order)     { return spi_exchange_arr32(NULL, rx, count, order); }

//---------------------------------------------------------------------------------------------------------
// Clock until it matches
//---------------------------------------------------------------------------------------------------------
typedef enum {
    MATCH_VALUE,
    MATCH_ALL_BITS,
    MATCH_ANY_BIT,
} Match_t;

static bool isMatch(const Match_t how, const uint8_t rx, const uint8_t arg)
{
    switch (how) {
        case MATCH_VALUE:    return rx == arg;
        case MATCH_ALL_BITS: return (rx & arg) == arg;
        case MATCH_ANY_BIT:  return (rx & arg) != 0U;
        default:             return false;
    }
}

static bool pollStep(SpiWait_t * const w, const Match_t how, const uint8_t arg, const uint32_t maxBytes)
{
    uint8_t rx = 0U;

    if (w->done) { ZERO_STRUCT(*w); }       // the last call finished, start a new wait

    w->status = spi_receive_u8(&rx);
    if (w->status != SPI_OK) {
        w->done = true;
        return false;
    }

    w->clocked++;
    if (isMatch(how, rx, arg)) {
        w->done = true;
        return true;
    }

    if (maxBytes != 0U && w->clocked >= maxBytes) {
        w->status = SPI_ERR_TIMEOUT;
        w->done   = true;
    }
    return false;
}

static uint32_t waitBlocking(const Match_t how, const uint8_t arg, const uint32_t maxBytes)
{
    SpiWait_t w;
    ZERO_STRUCT(w);

    while (!pollStep(&w, how, arg, maxBytes)) {
        if (w.done) { return 0U; }
    }
    return w.clocked;
}

uint32_t spi_wait_value(const uint8_t value, const uint32_t maxBytes)   { return waitBlocking(MATCH_VALUE, value, maxBytes); }
uint32_t spi_wait_allBits(const uint8_t mask, const uint32_t maxBytes)  { return waitBlocking(MATCH_ALL_BITS, mask, maxBytes); }
uint32_t spi_wait_anyBit(const uint8_t mask, const uint32_t maxBytes)   { return waitBlocking(MATCH_ANY_BIT, mask, maxBytes); }

bool spi_poll_value(SpiWait_t * const w, const uint8_t value, const uint32_t maxBytes)   { return pollStep(w, MATCH_VALUE, value, maxBytes); }
bool spi_poll_allBits(SpiWait_t * const w, const uint8_t mask, const uint32_t maxBytes)  { return pollStep(w, MATCH_ALL_BITS, mask, maxBytes); }
bool spi_poll_anyBit(SpiWait_t * const w, const uint8_t mask, const uint32_t maxBytes)   { return pollStep(w, MATCH_ANY_BIT, mask, maxBytes); }
