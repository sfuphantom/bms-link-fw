/*
Author: Mohnish Devarapalli
*/

#ifndef ISO_SPI_DRIVER_H
#define ISO_SPI_DRIVER_H

#include "ltc6811_commands.h"
#include "stdint.h"

// Function declarations
// void wakeup_spi(void);  // Wake up LTC6820 ISO SPI converter
bool write_reg(uint16_t cmd, uint8_t* data, uint8_t data_len);
// bool read_reg(uint16_t cmd, uint8_t* data, uint8_t data_len);
uint16_t calculate_pec(uint8_t *data, uint8_t len);
#endif // ISO_SPI_DRIVER_H
