/*
Author: Tanjosh Sidhu
*/

#ifndef SLAVECOMMUNICATION_DRIVERS_H
#define SLAVECOMMUNICATION_DRIVERS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "SlaveCommunation_Hardware.h"
#include "PhantomAlgorithms.h"
#include "PhantomTimers.h"
#include "SPI_helpers.h"

/////////////////////////////////////////////////////////////////
#define SLAVE_NUMBER_OF_FAILS_ALLOWED         2
/////////////////////////////////////////////////////////////////
#define NUMBER_OF_CELLS_SLAVES             (CELLS_PER_SLAVE_BOARD     * NUMBER_OF_SLAVE_BOARDS_TOTAL)
#define NUMBER_OF_AUCILIARY_SLAVES         (AUCILIARY_PER_SLAVE_BOARD * NUMBER_OF_SLAVE_BOARDS_TOTAL)
#define NUMBER_OF_GPIOS_SLAVES             (GPIOS_PER_SLAVE_BOARD     * NUMBER_OF_SLAVE_BOARDS_TOTAL)
#define NUMBER_OF_REF_2ND_SLAVES           (REF_2ND_PER_SLAVE_BOARD   * NUMBER_OF_SLAVE_BOARDS_TOTAL)

#define NUMBER_OF_CONFIG_BYTES_SLAVES      (NUMBER_OF_REG_BYTES_PER_CMD * NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD)
#define NUMBER_OF_STAT_BYTES_SLAVES        (NUMBER_OF_REG_BYTES_PER_CMD * NUMBER_OF_STAT_REG_GROUPS_PER_BOARD)

#define MAX_CMDS_PER_READ                  4        // cell voltages A to D
/////////////////////////////////////////////////////////////////
// Who owns the SPI bus while talking to the slaves. With an RTOS define it as the task id.
#ifndef SLAVE_SPI_OWNER
  #define SLAVE_SPI_OWNER               1U
#endif
#define SLAVE_LOCK_WAIT_TICKS           US_TO_TICKS(20000U)

// After an ADC command: FALSE = keep clocking with CS low until the chips are done,
//                       TRUE  = free the SPI, wait ADC_CHECK_PERIOD_US, check again
#ifndef ADC_WAIT_FREES_SPI
  #define ADC_WAIT_FREES_SPI            FALSE
#endif
#define ADC_CHECK_PERIOD_US             500U
#define ADC_TIMEOUT_US                  250000U

#define MAX_POLLS_TIMEOUT               2000
#define NUMBER_OF_GARBAGE_BYTES         (bitsToBytes(NUMBER_OF_SLAVE_BOARDS_TOTAL))    // one clock per chip in the chain
//------------------------------------------------------------------------
#define PEC_INIT_VALUE                  0x0010
#define PEC_CHARACTERISTIC_POLYNOMIAL   0x4599

typedef enum {
    SLAVE_OK = 0,
    SLAVE_ERR_SPI,          // SPI error flag or bad argument
    SLAVE_ERR_BUSY,         // someone else has the SPI
    SLAVE_ERR_ABORTED,      // spi_forceEnd was called
    SLAVE_ERR_TIMEOUT,      // chips never finished
    SLAVE_ERR_PEC,          // data came back with a wrong PEC
    SLAVE_ERR_NO_DATA,      // registers still hold the "not converted" value
    SLAVE_ERR_MISMATCH,     // read back is not what was written
    SLAVE_ERR_ASLEEP,       // chips lost their configuration or did not answer
    SLAVE_ERR_LAYOUT,       // bit fields do not match the register layout
    SLAVE_ERR_ARG,
    SLAVE_STATUS_COUNT,
} SlaveStatus_t;

const char *SlaveStatus_text(const SlaveStatus_t status);
SlaveStatus_t Slave_NoteError(const SlaveStatus_t status);    // counts it, returns it
uint32_t    Slave_ErrorCount(const SlaveStatus_t status);
void        Slave_ClearErrorCounts(void);
//---------------------------------------------------------------------------------------------------------
void     pec15_init(void);
uint16_t pec15_calc(const uint8_t * const data, const size_t len);
uint16_t pec15_cmd(const uint16_t cmd);
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t wakeup_idle(void);        // chips awake, link idle
SlaveStatus_t wakeup_sleep(void);       // chips asleep
//---------------------------------------------------------------------------------------------------------
// Commands
SlaveStatus_t SendCmd(const uint16_t cmd);
SlaveStatus_t SendCmdAndPoll(const uint16_t cmd);   // keeps CS low and clocks until the ADC is done

// ADC busy / free. The flag is only a shortcut, PollADC asks the chips.
SlaveStatus_t StartADC(const uint16_t cmd);         // command only, the SPI is free right away
SlaveStatus_t PollADC(bool * const free);
bool          isADCFree(void);
bool          isADCBusy(void);
SlaveStatus_t waitADCFree(const ticks_t period, const ticks_t timeout);     // check, free the SPI, wait, check again
SlaveStatus_t ConvertADC(const uint16_t cmd);       // start + wait, the way ADC_WAIT_FREES_SPI says
//---------------------------------------------------------------------------------------------------------
// Register groups. Data is bytes in wire order, 6 per chip, chip 0 (the one next to the host) first.
SlaveStatus_t WriteRegGroup(const uint16_t cmd, const uint8_t * const data);
SlaveStatus_t ReadRegGroup(const uint16_t cmd, uint8_t * const data);
SlaveStatus_t ReadRegGroup_Slave(const uint16_t cmd, const uint8_t board, uint8_t * const data);   // one chip, 6 bytes
SlaveStatus_t WriteThenReadRegGroup(const uint16_t W_cmd, const uint16_t R_cmd, const uint8_t * const W_data);

// Several commands. The result has all the groups of chip 0, then all of chip 1, ...
SlaveStatus_t ReadMultiRegGroups(const uint16_t * const cmds, const uint8_t numCmds, uint8_t * const data);
SlaveStatus_t WriteMultiRegGroups(const uint16_t * const cmds, const uint8_t numCmds, const uint8_t * const data);
SlaveStatus_t WriteThenReadMultiRegGroups(const uint16_t * const W_cmds, const uint16_t * const R_cmds,
                                          const uint8_t numCmds, const uint8_t * const data);

// ADC values are 16 bit little endian on the wire; these give real numbers
SlaveStatus_t ReadMultiRegGroups_u16(const uint16_t * const cmds, const uint8_t numCmds, uint16_t * const data);

SlaveStatus_t SendClearThenCheckCMD(const uint16_t cmd_clear, const uint16_t cmd_check);

#endif /*SLAVECOMMUNICATION_DRIVERS_H*/
