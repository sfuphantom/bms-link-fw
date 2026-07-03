/*
 * SlaveCommunation_Hardware.h
 *
 *  Created on: Jun 16, 2026
 *      Author: tanjo
 */

#ifndef PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_HARDWARE_H_
#define PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_HARDWARE_H_
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
//#include "PhantomHelpers.h"
/////////////////////////////////////////////////////////////////
#define NUMBER_OF_SLAVE_BOARDS          1

#define BYTES_PER_REG_GROUP             6
#define WORDS_PER_REG_GROUP             (BYTES_PER_REG_GROUP / 2)

#define NUMBER_OF_REG_BYTES_PER_CMD     (BYTES_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_REG_WORDS_PER_CMD     (WORDS_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS)
/////////////////////////////////////////////////////////////////
#define tWAKE_us  400
#define tCYCLE_us 3325
#define tSLEEP_ms 2200
#define tREFUP_us 4400
#define tIDEL_us  4300
#define fADC_kHz  3300
//////////////////////////////////////////////////////////////////
////SPI
//#define SPI_WAIT_BYTE_FINISH_COUNT 0xFF
//
//#define REG_FOR_SPI     spiREG3
#define CS_PIN_ID       0x00U
//#define CS_PIN_MASK     (1U << CS_PIN_ID)//0xFEU//0b11111110 << Active low
//#define CS_HOLD_MASK    0x10000000U
//#define SPI_WDEL        FALSE
//
//#define SPI_CONFIG0_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_0 << 24U) | (uint32)CS_PIN_MASK<<16)
//#define SPI_CONFIG1_WORD (((uint32)SPI_WDEL<<26U) | ((uint32)SPI_FMT_1 << 24U) | (uint32)CS_PIN_MASK<<16)
/////////////////////////////////////////////////////////////////


#define CELLS_PER_SLAVE_BOARD          12
#define AUCILIARY_PER_SLAVE_BOARD       6
#define CELL_IN_PARALLEL                5

#define GPIOS_PER_SLAVE_BOARD           (AUCILIARY_PER_SLAVE_BOARD-1)
#define REF_2ND_PER_SLAVE_BOARD         1
#define NUMBER_OF_CLEAR_CMDS            3
/////////////////////////////////////////////////////////////////
#define NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD           1
#define NUMBER_OF_STAT_REG_GROUPS_PER_BOARD             2

#define NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD     (CELLS_PER_SLAVE_BOARD/WORDS_PER_REG_GROUP)
#define NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD     (AUCILIARY_PER_SLAVE_BOARD/WORDS_PER_REG_GROUP)
/////////////////////////////////////////////////////////////////
#define NUMBER_OF_CELLS             (CELLS_PER_SLAVE_BOARD     * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_AUCILIARY         (AUCILIARY_PER_SLAVE_BOARD * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_GPIOS             (GPIOS_PER_SLAVE_BOARD     * NUMBER_OF_SLAVE_BOARDS)
#define NUMBER_OF_REF_2ND           (REF_2ND_PER_SLAVE_BOARD   * NUMBER_OF_SLAVE_BOARDS)

//#define CELL_IN_SERIES               NUMBER_OF_CELLS
#define NUMBER_OF_CONFIG_WORDS      (NUMBER_OF_REG_WORDS_PER_CMD * NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD)
#define NUMBER_OF_STAT_WORDS        (NUMBER_OF_REG_WORDS_PER_CMD * NUMBER_OF_STAT_REG_GROUPS_PER_BOARD)
////////////////////////////////////////////////////////////////////
#define ADC_MEASURE_MODE 2 //0=Fast, 1=Normal, 2=Filtered
#define ADC_MEASURE_DISCHARGE_PERMITED TRUE
//////////////////////////////////////////////////////////////////
// ADC specs
#define ADC2MICRO_VOLTS                 100
#define ADC2VOLTS                       ((float)(ADC2MICRO_VOLTS) * 1e-6f)
#define ADC_OFFSET_VOLTS                (ADC2VOLTS * 0.00f)

#define ADC_RESOLUTION_BIT              14
#define ADC_MRCRO_VOLT_NOISE            250

#define ADC_MAX_VOLT                    0.0f
#define ADC_MIN_VOLT                    5.0f

#define ITMP_MILLI_VOLTS_2_CELCIUS      7.5f
#define ITMP_KELVIN_2_CELCIUS           273
//////////////////////////////////////////////////////////////////


#endif /* PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_HARDWARE_H_ */
