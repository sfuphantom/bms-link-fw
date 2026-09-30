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

#include "BatteryCell_Hardware.h"
#include "FullBattery_Hardware.h"

/////////////////////////////////////////////////////////////////
#define CELLS_PER_SLAVE_BOARD          12
#define AUCILIARY_PER_SLAVE_BOARD       6
#define CELL_IN_PARALLEL                5

#define GPIOS_PER_SLAVE_BOARD           (AUCILIARY_PER_SLAVE_BOARD-1)
/////////////////////////////////////////////////////////////////
//#define NUMBER_OF_SLAVE_BOARDS_TOTAL          ((NUMBER_OF_CELLS_SERIES + CELLS_PER_SLAVE_BOARD - 1)/(CELLS_PER_SLAVE_BOARD))

#define BYTES_PER_REG_GROUP             6
#define WORDS_PER_REG_GROUP             (BYTES_PER_REG_GROUP / 2)

#define NUMBER_OF_REG_BYTES_PER_CMD     (BYTES_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS_TOTAL)
#define NUMBER_OF_REG_WORDS_PER_CMD     (WORDS_PER_REG_GROUP * NUMBER_OF_SLAVE_BOARDS_TOTAL)
/////////////////////////////////////////////////////////////////
#define tWAKE_us  400
#define tCYCLE_us 3325
#define tSLEEP_ms 2200
#define tREFUP_us 4400
#define tIDEL_us  4300
#define fADC_kHz  3300
/////////////////////////////////////////////////////////////////
#define REF_2ND_PER_SLAVE_BOARD         1
#define NUMBER_OF_CLEAR_CMDS            3
/////////////////////////////////////////////////////////////////
#define NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD           1
#define NUMBER_OF_STAT_REG_GROUPS_PER_BOARD             2

#define NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD     (CELLS_PER_SLAVE_BOARD/WORDS_PER_REG_GROUP)
#define NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD     (AUCILIARY_PER_SLAVE_BOARD/WORDS_PER_REG_GROUP)
////////////////////////////////////////////////////////////////////
#define ADC_MEASURE_MODE 2 //0=Fast, 1=Normal, 2=Filtered
#define ADC_MEASURE_DISCHARGE_PERMITED FALSE
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
#define SLAVE_ADC2VOLTS(ADC_VAL)            (ADC_VAL * ADC2VOLTS)
#define SLAVE_ADC2TEMP(ADC_VAL)             ((SLAVE_ADC2VOLTS(ADC_VAL) / ITMP_MILLI_VOLTS_2_CELCIUS * 1000) - ITMP_KELVIN_2_CELCIUS)

#define VOLTS2SLAVE_ADC(V)                  ((uint16_t)(V/ADC2VOLTS))
#define Temp2SLAVE_ADC(T)                   (VOLTS2SLAVE_ADC((T+ITMP_KELVIN_2_CELCIUS) * ITMP_MILLI_VOLTS_2_CELCIUS / 1000))
//////////////////////////////////////////////////////////////////
#define RES_ON_OHMS                     33

#define DCC_MAX_CURRENT_mA              100
#define DCC_MAX_CURRENT_A               (1000*DCC_MAX_CURRENT_mA)



#endif /* PHANTOM_DRIVERS_INCLUDE_SLAVECOMMUNATION_HARDWARE_H_ */
