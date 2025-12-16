/*
Author: Mohnish Devarapalli
*/

#ifndef ISO_SPI_DRIVER_H
#define ISO_SPI_DRIVER_H

#include <stdbool.h>
#include "ltc6811_commands.h"

bool write_reg();

bool read_reg();

int32_t generate_pec();

#endif // ISO_SPI_DRIVER_H