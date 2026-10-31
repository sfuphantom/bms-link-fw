/*
Author: Tanjosh Sidhu
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "spi_helpers.h"
#include "GIO_helpers.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Hardware.h"
#include "ltc6811_commands.h"
#include "PhantomHelpers.h"
#include "PhantomTimers.h"

#define SLAVE_CS_PIN ((GioPin_t)GIO_SLAVE_CS_PIN)

static uint16_t pec15Table[256];
static bool     pecReady = false;
static bool     adcFree  = true;

static uint32_t errorCount[SLAVE_STATUS_COUNT];
static uint8_t  groupScratch[NUMBER_OF_REG_BYTES_PER_CMD];
static uint8_t  multiScratch[MAX_CMDS_PER_READ * NUMBER_OF_REG_BYTES_PER_CMD];

//---------------------------------------------------------------------------------------------------------
// Status
//---------------------------------------------------------------------------------------------------------
static const char * const statusText[SLAVE_STATUS_COUNT] = {
    "ok", "spi error", "spi busy", "spi aborted", "timeout", "pec error",
    "no data", "read back differs", "asleep", "bit field layout", "bad argument",
};

const char *SlaveStatus_text(const SlaveStatus_t status)
{
    return (status < SLAVE_STATUS_COUNT) ? statusText[status] : "unknown";
}

uint32_t Slave_ErrorCount(const SlaveStatus_t status)
{
    return (status < SLAVE_STATUS_COUNT) ? errorCount[status] : 0U;
}

void Slave_ClearErrorCounts(void)
{
    memset(errorCount, 0, sizeof(errorCount));
}

SlaveStatus_t Slave_NoteError(const SlaveStatus_t status)
{
    if (status != SLAVE_OK) { errorCount[status]++; }
    return status;
}

static SlaveStatus_t fromSpi(const SpiStatus_t status)
{
    switch (status) {
        case SPI_OK:          return SLAVE_OK;
        case SPI_ERR_TIMEOUT: return SLAVE_ERR_TIMEOUT;
        case SPI_ERR_ABORTED: return SLAVE_ERR_ABORTED;
        case SPI_ERR_BUSY:    return SLAVE_ERR_BUSY;
        default:              return SLAVE_ERR_SPI;
    }
}

//---------------------------------------------------------------------------------------------------------
// PEC
//---------------------------------------------------------------------------------------------------------
void pec15_init(void)
{
    uint16_t i;
    for (i = 0; i < 256U; i++) {
        uint16_t remainder = (uint16_t)(i << 7);
        uint8_t  bit;

        for (bit = 0; bit < 8U; bit++) {
            if ((remainder & 0x4000U) != 0U) { remainder = (uint16_t)((remainder << 1) ^ PEC_CHARACTERISTIC_POLYNOMIAL); }
            else                             { remainder = (uint16_t)(remainder << 1); }
        }
        pec15Table[i] = remainder;
    }
    pecReady = true;
}

uint16_t pec15_calc(const uint8_t * const data, const size_t len)
{
    uint16_t remainder = PEC_INIT_VALUE;
    size_t   i;

    if (data == NULL) { return SPI_DUMMY_U16; }
    if (!pecReady)    { pec15_init(); }

    for (i = 0; i < len; i++) {
        const uint8_t address = (uint8_t)(((remainder >> 7) ^ data[i]) & 0xFFU);
        remainder = (uint16_t)((remainder << 8) ^ pec15Table[address]);
    }
    return (uint16_t)(remainder << 1);      // the CRC15 has a 0 in the LSB
}

uint16_t pec15_cmd(const uint16_t cmd)
{
    uint8_t bytes[2];

    write_u16_BE(bytes, cmd);
    return pec15_calc(bytes, sizeof(bytes));
}

//---------------------------------------------------------------------------------------------------------
// One message = CS low ... CS high. The pieces below are only used inside a message.
//---------------------------------------------------------------------------------------------------------
static SlaveStatus_t beginMsg(void)
{
    return fromSpi(spi_begin(SLAVE_SPI_OWNER, SLAVE_CS_PIN, SLAVE_LOCK_WAIT_TICKS));
}

static void endMsg(void)
{
    spi_end(SLAVE_SPI_OWNER, SLAVE_CS_PIN);
}

static SlaveStatus_t sendCmd(const uint16_t cmd)
{
    SpiStatus_t status = spi_send_u16(cmd, SPI_BIG_ENDIAN);

    if (status == SPI_OK) { status = spi_send_u16(pec15_cmd(cmd), SPI_BIG_ENDIAN); }
    return fromSpi(status);
}

static SlaveStatus_t writeGroup(const uint8_t * const data)
{
    SpiStatus_t status = spi_send_arr8(data, BYTES_PER_REG_GROUP);

    if (status == SPI_OK) { status = spi_send_u16(pec15_calc(data, BYTES_PER_REG_GROUP), SPI_BIG_ENDIAN); }
    return fromSpi(status);
}

static SlaveStatus_t readGroup(uint8_t * const data)
{
    uint16_t rxPec = 0U;
    SpiStatus_t status = spi_receive_arr8(data, BYTES_PER_REG_GROUP);

    if (status == SPI_OK) { status = spi_receive_u16(&rxPec, SPI_BIG_ENDIAN); }
    if (status != SPI_OK) { return fromSpi(status); }

    return (pec15_calc(data, BYTES_PER_REG_GROUP) == rxPec) ? SLAVE_OK : SLAVE_ERR_PEC;
}

// After a command, with CS still low: one clock per chip, then clock until a byte has bit 0 set
static SlaveStatus_t pollDone(void)
{
    SpiWait_t w;
    SpiStatus_t status = spi_receive_arr8(NULL, NUMBER_OF_GARBAGE_BYTES);

    if (status != SPI_OK) { return fromSpi(status); }

    ZERO_STRUCT(w);
    while (!spi_poll_anyBit(&w, 0x01U, MAX_POLLS_TIMEOUT)) {
        if (w.done) { return fromSpi(w.status); }
    }
    return SLAVE_OK;
}

//---------------------------------------------------------------------------------------------------------
// Wake up
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t wakeup_idle(void)
{
    const uint8_t dummy = SPI_DUMMY_BYTE;
    uint8_t i;

    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL; i++) {
        const SlaveStatus_t status = fromSpi(spi_transact(SLAVE_SPI_OWNER, SLAVE_CS_PIN,
                                                          SLAVE_LOCK_WAIT_TICKS, &dummy, NULL, 1U));
        if (status != SLAVE_OK) { return Slave_NoteError(status); }
    }
    return SLAVE_OK;
}

SlaveStatus_t wakeup_sleep(void)
{
    SlaveStatus_t status = beginMsg();
    uint8_t i;

    if (status != SLAVE_OK) { return Slave_NoteError(status); }

    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL; i++) {
        if (i > 0U) { spi_csLow(SLAVE_CS_PIN); }
        delay_ms_us(0, 2 * tWAKE_us);
        spi_csHigh(SLAVE_CS_PIN);
        delay_ms_us(0, 10);
    }

    endMsg();
    return SLAVE_OK;
}

//---------------------------------------------------------------------------------------------------------
// Commands
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t SendCmd(const uint16_t cmd)
{
    SlaveStatus_t status = beginMsg();
    if (status != SLAVE_OK) { return Slave_NoteError(status); }

    status = sendCmd(cmd);

    endMsg();
    return Slave_NoteError(status);
}

SlaveStatus_t SendCmdAndPoll(const uint16_t cmd)
{
    SlaveStatus_t status = beginMsg();
    if (status != SLAVE_OK) { return Slave_NoteError(status); }

    status = sendCmd(cmd);
    if (status == SLAVE_OK) { status = pollDone(); }

    endMsg();
    if (status == SLAVE_OK) { adcFree = true; }
    return Slave_NoteError(status);
}

SlaveStatus_t StartADC(const uint16_t cmd)
{
    const SlaveStatus_t status = SendCmd(cmd);

    if (status == SLAVE_OK) { adcFree = false; }
    return status;
}

SlaveStatus_t PollADC(bool * const free)
{
    uint8_t rx = 0U;
    SlaveStatus_t status = wakeup_idle();

    if (status == SLAVE_OK) { status = beginMsg(); }
    if (status != SLAVE_OK) { return Slave_NoteError(status); }

    status = sendCmd(LTC6811_PLADC);
    if (status == SLAVE_OK) { status = fromSpi(spi_receive_arr8(NULL, NUMBER_OF_GARBAGE_BYTES)); }
    if (status == SLAVE_OK) { status = fromSpi(spi_receive_u8(&rx)); }

    endMsg();
    if (status != SLAVE_OK) { return Slave_NoteError(status); }

    adcFree = ((rx & 0x01U) != 0U);
    *free   = adcFree;
    return SLAVE_OK;
}

bool isADCFree(void)
{
    bool free = false;

    if (adcFree) { return true; }
    return (PollADC(&free) == SLAVE_OK) && free;
}

bool isADCBusy(void)
{
    return !isADCFree();
}

SlaveStatus_t waitADCFree(const ticks_t period, const ticks_t timeout)
{
    Timeout_t t;
    bool free = false;

    if (adcFree) { return SLAVE_OK; }
    timeout_start(&t, timeout);

    for (;;) {
        const SlaveStatus_t status = PollADC(&free);

        if (status != SLAVE_OK) { return status; }
        if (free)               { return SLAVE_OK; }
        if (timeout_expired(&t)) { return Slave_NoteError(SLAVE_ERR_TIMEOUT); }

        delay_ticks(period);                // the SPI is free for others meanwhile
    }
}

SlaveStatus_t ConvertADC(const uint16_t cmd)
{
    const ticks_t period  = US_TO_TICKS(ADC_CHECK_PERIOD_US);
    const ticks_t timeout = US_TO_TICKS(ADC_TIMEOUT_US);
    SlaveStatus_t status  = waitADCFree(period, timeout);       // never start over a running conversion

    if (status != SLAVE_OK) { return status; }

#if ADC_WAIT_FREES_SPI
    status = StartADC(cmd);
    if (status == SLAVE_OK) { status = waitADCFree(period, timeout); }
    return status;
#else
    return SendCmdAndPoll(cmd);
#endif
}

//---------------------------------------------------------------------------------------------------------
// Register groups
//---------------------------------------------------------------------------------------------------------

// Reads the first `boards` chips. Stops at the first bad PEC.
static SlaveStatus_t readOnce(const uint16_t cmd, uint8_t * const data, const uint8_t boards)
{
    uint8_t i;
    SlaveStatus_t status = beginMsg();
    if (status != SLAVE_OK) { return Slave_NoteError(status); }

    status = sendCmd(cmd);
    for (i = 0; i < boards && status == SLAVE_OK; i++) {
        status = readGroup(&data[i * BYTES_PER_REG_GROUP]);
    }

    endMsg();
    return Slave_NoteError(status);
}

// Only a bad PEC is worth another try
static SlaveStatus_t readRetry(const uint16_t cmd, uint8_t * const data, const uint8_t boards)
{
    SlaveStatus_t status = SLAVE_ERR_PEC;
    uint8_t attempt;

    for (attempt = 0; attempt < NUMBER_OF_FAILS_ALLOWED; attempt++) {
        status = readOnce(cmd, data, boards);
        if (status != SLAVE_ERR_PEC) { return status; }
    }
    return status;
}

SlaveStatus_t ReadRegGroup(const uint16_t cmd, uint8_t * const data)
{
    return readRetry(cmd, data, NUMBER_OF_SLAVE_BOARDS_TOTAL);
}

// The chips before this one still have to be clocked out, the ones after it do not
SlaveStatus_t ReadRegGroup_Slave(const uint16_t cmd, const uint8_t board, uint8_t * const data)
{
    uint8_t upTo[NUMBER_OF_REG_BYTES_PER_CMD];
    const SlaveStatus_t status = (board < NUMBER_OF_SLAVE_BOARDS_TOTAL) ? readRetry(cmd, upTo, board + 1U)
                                                                        : SLAVE_ERR_ARG;
    if (status == SLAVE_OK) { memcpy(data, &upTo[board * BYTES_PER_REG_GROUP], BYTES_PER_REG_GROUP); }
    return status;
}

static SlaveStatus_t writeOnce(const uint16_t cmd, const uint8_t * const data)
{
    uint8_t i;
    SlaveStatus_t status = beginMsg();
    if (status != SLAVE_OK) { return Slave_NoteError(status); }

    status = sendCmd(cmd);
    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL && status == SLAVE_OK; i++) {
        const uint8_t board = (uint8_t)(NUMBER_OF_SLAVE_BOARDS_TOTAL - 1U - i);     // the far end goes first
        status = writeGroup(&data[board * BYTES_PER_REG_GROUP]);
    }

    endMsg();
    return Slave_NoteError(status);
}

SlaveStatus_t WriteRegGroup(const uint16_t cmd, const uint8_t * const data)
{
    return writeOnce(cmd, data);
}

SlaveStatus_t WriteThenReadRegGroup(const uint16_t W_cmd, const uint16_t R_cmd, const uint8_t * const W_data)
{
    uint8_t R_data[NUMBER_OF_REG_BYTES_PER_CMD];
    SlaveStatus_t status = SLAVE_ERR_MISMATCH;
    uint8_t attempt;

    for (attempt = 0; attempt < NUMBER_OF_FAILS_ALLOWED; attempt++) {
        status = WriteRegGroup(W_cmd, W_data);
        if (status != SLAVE_OK) { return status; }

        status = ReadRegGroup(R_cmd, R_data);
        if (status == SLAVE_OK) {
            if (arr_u8_equal(W_data, R_data, NUMBER_OF_REG_BYTES_PER_CMD)) { return SLAVE_OK; }
            status = Slave_NoteError(SLAVE_ERR_MISMATCH);
        }
        if (status != SLAVE_ERR_PEC && status != SLAVE_ERR_MISMATCH) { return status; }
    }
    return status;
}

//---------------------------------------------------------------------------------------------------------
// Several commands. In `data`, the groups of one chip are next to each other.
//---------------------------------------------------------------------------------------------------------
static size_t slotOffset(const uint8_t numCmds, const uint8_t cmdIdx, const uint8_t board)
{
    return ((size_t)board * numCmds + cmdIdx) * BYTES_PER_REG_GROUP;
}

// one group of every chip: data -> sub
static void gatherGroups(uint8_t * const sub, const uint8_t * const data, const uint8_t numCmds, const uint8_t cmdIdx)
{
    uint8_t board;
    for (board = 0; board < NUMBER_OF_SLAVE_BOARDS_TOTAL; board++) {
        memcpy(&sub[board * BYTES_PER_REG_GROUP], &data[slotOffset(numCmds, cmdIdx, board)], BYTES_PER_REG_GROUP);
    }
}

// one group of every chip: sub -> data
static void scatterGroups(uint8_t * const data, const uint8_t * const sub, const uint8_t numCmds, const uint8_t cmdIdx)
{
    uint8_t board;
    for (board = 0; board < NUMBER_OF_SLAVE_BOARDS_TOTAL; board++) {
        memcpy(&data[slotOffset(numCmds, cmdIdx, board)], &sub[board * BYTES_PER_REG_GROUP], BYTES_PER_REG_GROUP);
    }
}

SlaveStatus_t ReadMultiRegGroups(const uint16_t * const cmds, const uint8_t numCmds, uint8_t * const data)
{
    uint8_t i;
    for (i = 0; i < numCmds; i++) {
        SlaveStatus_t status = wakeup_idle();

        if (status == SLAVE_OK) { status = ReadRegGroup(cmds[i], groupScratch); }
        if (status != SLAVE_OK) { return status; }

        scatterGroups(data, groupScratch, numCmds, i);
    }
    return SLAVE_OK;
}

SlaveStatus_t ReadMultiRegGroups_u16(const uint16_t * const cmds, const uint8_t numCmds, uint16_t * const data)
{
    SlaveStatus_t status;

    if (numCmds > MAX_CMDS_PER_READ) { return SLAVE_ERR_ARG; }

    status = ReadMultiRegGroups(cmds, numCmds, multiScratch);
    if (status == SLAVE_OK) { bytes_to_u16_LE(multiScratch, data, (size_t)numCmds * NUMBER_OF_REG_WORDS_PER_CMD); }
    return status;
}

SlaveStatus_t WriteMultiRegGroups(const uint16_t * const cmds, const uint8_t numCmds, const uint8_t * const data)
{
    uint8_t i;
    for (i = 0; i < numCmds; i++) {
        SlaveStatus_t status;

        gatherGroups(groupScratch, data, numCmds, i);

        status = wakeup_idle();
        if (status == SLAVE_OK) { status = WriteRegGroup(cmds[i], groupScratch); }
        if (status != SLAVE_OK) { return status; }
    }
    return SLAVE_OK;
}

SlaveStatus_t WriteThenReadMultiRegGroups(const uint16_t * const W_cmds, const uint16_t * const R_cmds,
                                          const uint8_t numCmds, const uint8_t * const data)
{
    uint8_t i;
    for (i = 0; i < numCmds; i++) {
        SlaveStatus_t status;

        gatherGroups(groupScratch, data, numCmds, i);

        status = wakeup_idle();
        if (status == SLAVE_OK) { status = WriteThenReadRegGroup(W_cmds[i], R_cmds[i], groupScratch); }
        if (status != SLAVE_OK) { return status; }
    }
    return SLAVE_OK;
}

//---------------------------------------------------------------------------------------------------------
SlaveStatus_t SendClearThenCheckCMD(const uint16_t cmd_clear, const uint16_t cmd_check)
{
    uint8_t reg[NUMBER_OF_REG_BYTES_PER_CMD];
    SlaveStatus_t status = SLAVE_ERR_MISMATCH;
    uint8_t attempt;

    for (attempt = 0; attempt < NUMBER_OF_FAILS_ALLOWED; attempt++) {
        status = SendCmd(cmd_clear);
        if (status != SLAVE_OK) { return status; }

        status = ReadRegGroup(cmd_check, reg);
        if (status == SLAVE_OK) {
            if (arr_u8_all(reg, NUMBER_OF_REG_BYTES_PER_CMD, isEqual, SPI_DUMMY_BYTE)) { return SLAVE_OK; }
            status = Slave_NoteError(SLAVE_ERR_MISMATCH);
        }
        if (status != SLAVE_ERR_PEC && status != SLAVE_ERR_MISMATCH) { return status; }
    }
    return status;
}
