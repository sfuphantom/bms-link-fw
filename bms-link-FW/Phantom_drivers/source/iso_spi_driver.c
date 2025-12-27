#include <stdint.h>
#include <stdbool.h>

#include "iso_spi_driver.h"

#define PEC_INIT_VALUE 0x0010

// 0100 0101 1001 1001 for the polynomicla x^15 + x^14 + x^10 + x^8 + x^7 + x^4 + x^3 + 1 
#define PEC_CHARACTERISTIC_POLYNOMIAL 0x4599

#define CMD_LENGTH 2

uint16_t calculate_pec(uint8_t *data, uint8_t len){
    uint16_t pec = PEC_INIT_VALUE;
    int i;
    for (i = 0; i < len; i++)
    {
        uint8_t byte = data[i];

        int bit;
        for (bit = 7; bit >= 0; bit--)
        {
            uint8_t din = (byte >> bit) & 0x01;
            uint8_t feedback = din ^ ((pec >> 14) & 0x01);

            pec <<= 1;
            if (feedback)
            {
                pec ^= PEC_CHARACTERISTIC_POLYNOMIAL;
            }
        }
    }

    return (pec & 0x7FFF);
}


/*
For read 
and write commands, a single command is sent, and then 
the stacked devices effectively turn into a cascaded shift 
register, in which data is shifted through each device to 
the next higher (on a write) or the next lower (on a read) 
device in the stack. See the Serial Interface section.

pg 55
*/

bool write_reg(uint16_t cmd, uint8_t* data, uint8_t data_len){

  wakeup_spi();

  const uint8_t cmd0 = cmd & 0xFF;
  const uint8_t cmd1 = (cmd >> 8) & 0xFF; 
  
  const uint8_t cmd_array[CMD_LENGTH] = {cmd0, cmd1};

  const uint16_t cmd_pec = calculate_pec(cmd_array, CMD_LENGTH);

  const uint16_t data_pec = calculate_pec(data, data_len);


  clock_spi();

  return true;
}