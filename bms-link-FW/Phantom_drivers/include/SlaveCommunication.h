/*
Author: Tanjosh Sidhu
*/

//#include "iso_spi_driver.h"
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"

/////////////////////////////////////////////////////////////////

#define NUMBER_OF_SLAVE_BOARDS      1
#define CELLS_PER_SLAVE_BOARD       12
#define NUMBER_OF_CELLS             (CELLS_PER_SLAVE_BOARD * NUMBER_OF_SLAVE_BOARDS)

#define NUMBER_OF_CELL_CMD_GROUPS       4
#define NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD     (CELLS_PER_SLAVE_BOARD / NUMBER_OF_CELL_CMD_GROUPS)

#define REG_FOR_SPI spiREG1
#define CS_HIGH 0U
#define CS_LOW  0x04000000U
#define CS_HOLD_MASK 0x10000000U

#define REFON_BIT 2
#define CFGA_BYTES 6
#define CFGA_HALF_WORDS (CFGA_BYTES / 2)

//////////////////////////////////////////////////////////////////
#define tWAKE_us  400
#define tCYCLE_us 3325
#define tREFUP_us 400
#define tSLEEP_ms 2200
#define tREFUP_us 4400
#define fADC_kHz  3300
//////////////////////////////////////////////////////////////////
#define SPI_DUMMY_DATA 0x0000
#define SPI_DUMMY_CMD  0x0000


#define CS_LL    0b1010
#define CS_HH    0b1111
#define CS_LH    0b1011
#define CS_HL    0b1110
#define CS_XH    0b0011
#define CS_XL    0b0010
#define CS_LX    0b1000
#define CS_HX    0b1100
#define CS_XX    0b0000

#define MINUS1 0xFFFF FFFF FFFF FFFF
//////////////////////////////////////////////////////////////////
struct SlaveBatteryCellData_struct {
  uint16_t Temp[NUMBER_OF_CELLS];
  uint16_t Volt[NUMBER_OF_CELLS];
  // uint16_t Cells_to_discharge[NUMBER_OF_SLAVE_BOARDS];
};
//////////////////////////////////////////////////////////////////
void delay_ms_us(uint32_t ms, uint32_t us);

uint16_t pec15_calc(uint8_t len, uint16_t *data);
uint32 GetChipSelect();
uint16_t SPI_SR2Link_HalfWord(uint16_t Tx, uint32 regConfig);

uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize, bool CS_Config);
uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize,  bool CS_Config);
uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize,  bool CS_Config);
uint32 SendCmd2Slave(uint16_t cmd, bool CS_Config);

void wait_SDO();

uint32_t send_spi_2_link(uint16_t cmd, uint16_t *tx_data, uint16_t *rx_data, uint16_t data_words);
uint32_t send_spi_2_link_chain(uint16_t cmd, uint16_t *tx_data, uint16_t *rx_data, uint16_t words_per_ic);

void wakeup_sleep();
void initLink();
void CommandAllFullSlave2Read(uint16_t* cmds, uint16_t* dataOut);
void ReadAllSlaves_Volt(uint16_t* dataOut);
///////////////////////////////////////////////////////////////




