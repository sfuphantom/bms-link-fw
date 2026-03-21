/*
 * cell_balancing.h
 *
 *  Created on: Mar 14, 2026
 *      Author: joeyw
 */

#ifndef BMS_MASTER_INCLUDE_CELL_BALANCING_H_
#define BMS_MASTER_INCLUDE_CELL_BALANCING_H_

#include <stdint.h>
#include <stdbool.h>

//Random values atm, check
#define NUM_CELLS 16 //double check
#define BAL_THRESHOLD 10
#define BAL_MIN 3000
#define BAL_MAX 4200

#define MOSFET_ON 1
#define MOSFET_OFF 0

typedef enum {
    BAL_OK = 0,
    BAL_ERR_PARAM = -1,
    BAL_ERR_FAULT = -2,

} BalanceStatus_t;


BalanceStatus_t BalanceCycle(uint16_t *voltages, uint8_t num_cells, bool *switches);

void MOSFET_EN(bool EN, int cell);
void check_cells();

#endif /* BMS_MASTER_INCLUDE_CELL_BALANCING_H_ */
