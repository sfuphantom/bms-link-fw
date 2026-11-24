/*
 * SPI_stubs.c
 *
 * Hardware-specific implementation of SPI operations.
 */

#include "SPI_stubs.h"

#if defined(BOARD_TMS570_HW)

#include "spi.h"
#include "reg_spi.h"
#include "PhantomTimers.h"
#include "PhantomAlgorithms.h"

#ifndef REG_FOR_SPI
  #define REG_FOR_SPI           spiREG3
#endif

#define SPI_HW_CSNR             0x01U               /* CS0 is a GIO pin, so hardware CS is not asserted */
#define SPI_ERR_FLAGS           0x5FU               /* DLENERR, TIMEOUT, PARITYERR, DESYNC, BITERR, RXOVR */
#define SPI_FLG_RXINT           0x100U
#define SPI_GCR1_SPIEN          (1UL << 24U)
#define SPI_BYTE_TIMEOUT_US     1000U
#define SPI_BYTE_TIMEOUT_TICKS  US_TO_TICKS(SPI_BYTE_TIMEOUT_US)

#ifndef SPI_USE_HAL_EXCHANGE
  #define SPI_USE_HAL_EXCHANGE  1
#endif

static volatile bool s_hwAborted = false;

void spiHw_init(void)
{
    spiInit();
}

uint32_t spiHw_errors(void)
{
    return REG_FOR_SPI->FLG & SPI_ERR_FLAGS;
}

void spiHw_clearErrors(void)
{
    s_hwAborted = false;
    REG_FOR_SPI->FLG = SPI_ERR_FLAGS;       /* write 1 to clear */
}

void spiHw_reset(void)
{
    s_hwAborted = true;
    REG_FOR_SPI->GCR1 &= ~SPI_GCR1_SPIEN;
    REG_FOR_SPI->GCR1 |=  SPI_GCR1_SPIEN;
}

SpiStatus_t spiHw_exchange(uint8_t fmt, uint16_t tx, uint16_t *rx)
{
    if (s_hwAborted) {
        return SPI_ERR_ABORTED;
    }

#if SPI_USE_HAL_EXCHANGE
    spiDAT1_t config;
    uint16    txWord = tx;
    uint16    rxWord = 0U;

    config.CS_HOLD = 0U;
    config.WDEL    = 0U;
    config.DFSEL   = (SPIDATAFMT_t)fmt;
    config.CSNR    = SPI_HW_CSNR;

    (void)spiTransmitAndReceiveData(REG_FOR_SPI, &config, 1U, &txWord, &rxWord);
    if (rx != NULL) {
        *rx = rxWord;
    }
#else
    REG_FOR_SPI->DAT1 = ((uint32_t)fmt << 24U) | ((uint32_t)SPI_HW_CSNR << 16U) | (uint32_t)tx;

    const ticks_t start = timer_tic_tick();
    while ((REG_FOR_SPI->FLG & SPI_FLG_RXINT) == 0U) {
        if (s_hwAborted)                                   { return SPI_ERR_ABORTED; }
        if (hasTimeElapsed(start, SPI_BYTE_TIMEOUT_TICKS)) { return SPI_ERR_TIMEOUT; }
    }

    if (rx != NULL) {
        *rx = (uint16_t)REG_FOR_SPI->BUF;       /* reading BUF clears the RX flag */
    } else {
        (void)REG_FOR_SPI->BUF;
    }
#endif

    return (spiHw_errors() != 0U) ? SPI_ERR_BUS : SPI_OK;
}

#elif defined(BOARD_STM32_HW)
#error "BOARD_STM32_HW is not implemented yet"
#elif defined(BOARD_SIL)
#error "BOARD_SIL is not implemented yet"
#else
#error "No valid BOARD selected in Hardware_Select.h"
#endif
