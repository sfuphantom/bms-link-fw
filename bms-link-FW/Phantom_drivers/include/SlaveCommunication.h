/*
Author: Tanjosh Sidhu
*/

//#include "iso_spi_driver.h"
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"

/////////////////////////////////////////////////////////////////

#define BYTES_REG_GROUP 6
#define WORD_REG_GROUP (BYTES_REG_GROUP / 2)

/////////////////////////////////////////////////////////////////
#define NUMBER_OF_SLAVE_BOARDS      1
#define CELLS_PER_SLAVE_BOARD       12
#define GPIOS_PER_SLAVE_BOARD       5

#define NUMBER_OF_CELLS             (CELLS_PER_SLAVE_BOARD * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_GPIOS             (GPIOS_PER_SLAVE_BOARD * NUMBER_OF_SLAVE_BOARDS)

#define NUMBER_OF_CELL_CMD_GROUPS       4
#define NUMBER_OF_GPIO_CMD_GROUPS       2

#define NUMBER_OF_REG_WORDS_PER_CMD (WORD_REG_GROUP * NUMBER_OF_SLAVE_BOARDS)


/////////////////////////////////////////////////
//SPI

#define REG_FOR_SPI     spiREG3
#define CS_HOLD_MASK    0x10000000U
#define CS_PIN_ID       0x0U
#define CS_PIN_MASK     0xFEU//0b11111110 << Active low
#define SPI_WDEL        FALSE

typedef enum  {LOW, HIGH} CS_Level;


#define SPI_CONFIG0_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_0 << 24U) | (uint32)CS_PIN_MASK<<16)
//#define SPI_CONFIG1_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_1 << 24U) | (uint32)CS_PIN_MASK<<16)


///////////////////////////////////////////////////
//
#define REFON_BIT 2
#define REFON_MASK 0x04



#define NUMBER_OF_SHIFT_BYTES ((NUMBER_OF_SLAVE_BOARDS+1)/2)

//////////////////////////////////////////////////////////////////
// Poll ADC
#define PADC_GARBAGE_CLOCK_CYCLES_BYTES (((NUMBER_OF_SLAVE_BOARDS-1)>>3)+1)


//////////////////////////////////////////////////////////////////
// ADC specs
#define MICRO_2_UNIT_FLOAT 1000000F

#define ADC_RESOLUTION_MICRO_VOLTS      100
#define ADC_RESOLUTION_VOLTS            (ADC_RESOLUTION_MICRO_VOLTS / 1000000.0f)
#define ADC_RESOLUTION_BIT_MASK         0xFFFC
#define ADC_MRCRO_VOLT_NOISE            250
#define ADC_MAX_VOLT    0.0F
#define ADC_MIN_VOLT    5.0F


//////////////////////////////////////////////////////////////////
#define tWAKE_us  400
#define tCYCLE_us 3325
//#define tREFUP_us 400
#define tSLEEP_ms 2200
#define tREFUP_us 4400
#define tIDEL_us  4300
#define fADC_kHz  3300
//////////////////////////////////////////////////////////////////
#define SPI_DUMMY_DATA_BYTE 0xFFU
#define SPI_DUMMY_DATA_WORD 0xFFFFU
#define SPI_DUMMY_DATA_DWORD 0xFFFF FFFFU
#define SPI_DUMMY_DATA_QWORD 0xFFFF FFFF FFFF FFFFU
//#define SPI_DUMMY_DATA 0xFFFF
#define SPI_DUMMY_CMD  0xFFFF

#define MINUS1_32 0xFFFF FFFF FFFF FFFF
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

//uint16_t SPI_SR2Link_HalfWord(uint16_t Tx);
//uint8_t SPI_SR2Link_Byte(uint8_t Tx);


//uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize);
//uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize);
//uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize);
//uint32 SendCmd2Slave(uint16_t cmd);

void wait_SDO();

uint32_t send_spi_2_link(uint16_t cmd, uint16_t *tx_data, uint16_t *rx_data, uint16_t data_words);
uint32_t send_spi_2_link_chain(uint16_t cmd, uint16_t *tx_data, uint16_t *rx_data, uint16_t words_per_ic);

void wakeup_sleep();
void initLink();
void CommandAllFullSlave2Read(uint16_t* cmds, uint16_t* dataOut);
void ReadAllSlaves_Volt(uint16_t* dataOut);
///////////////////////////////////////////////////////////////


uint32 ReadReg(uint16_t cmd, uint16_t* data);
uint32 WriteReg(uint16_t cmd, uint16_t* data);
