/*
 * cell_balancing.c
 *
 *  Created on: Mar 14, 2026
 *      Author: joeyw
 */


#include "cell_balancing.h"
#include <string.h>


BalanceStatus_t BalanceCycle(uint16_t *voltages, uint8_t num_cells, bool *switches){
    //Base Check
    if(voltages == NULL || switches == NULL || num_cells == 0 || num_cells > NUM_CELLS)
        return BAL_ERR_PARAM

    memset(switches, 0, num_cells * sizeof(bool));  //Set all switches off

    //Look for lowest voltage cell
    uint16_t v_min = UINT16_MAX;
    for(int i = 0; i < num_cells; i++){
        uint16_t v = voltages[i];
        if(v < BAL_MIN || v > BAL_MAX)
            return BAL_ERR_FAULT;
        if(v < v_min)
            v_min = v;
    }

    for(int i = 0; i < num_cells; i++){
        if (voltages[i] > v_min + BALANCE_THRESHOLD_MV)
            switches[i] = true;
        else
            switches[i] = false;
    }

    return BAL_OK
}


void MOSFET_EN(bool EN, int cell){
    if(MOSFET_ON){
        //enable gpio of cell
    }
    else{
        //turn off gpio of cell
    }
}

void check_cells(){
    for(int i = 0; i < NUM_CELLS; i++){
        if(cell_voltage[i] > BAL_THRESHOLD){
            MOSFET_EN(MOSFET_ON, i);
        }
        else{
            MOSFET_EN(MOSFET_OFF, i);
        }
    }
}


