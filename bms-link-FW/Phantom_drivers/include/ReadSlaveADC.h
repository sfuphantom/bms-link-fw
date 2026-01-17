/*
Author: Tanjosh Sidhu
*/

#include "iso_spi_driver.h"
#include <stdint.h>
#include <stdbool.h>

//spiDAT1_t dataconfig_t;
//spiDAT1_t dataconfig_r;
//
//dataconfig1_t.CS_HOLD = FALSE;
//dataconfig1_t.WDEL    = TRUE;
//dataconfig1_t.DFSEL   = SPI_FMT_0;
//dataconfig1_t.CSNR    = 0xFE;


#define NumberOfSlaves  1


void uint16_t2ByteArray(uint16_t in, uint8_t* out);
uint16_t ByteArray2uint16_t(uint8_t* in);

bool setupACD_init(uint8_t total_ic);
void wakeup_sleep(uint8_t total_ic);
bool SetRefOn(bool NewState);
void readAllADC(uint16_t cmd, uint16_t* data, uint8_t total_ic);


void WipTheSlavesAwake(uint8_t NumOfSlaves);
void buySlaves(uint8_t NumOfSlaves);
void MasterCommandAllSlaves(uint16_t cmd, uint16_t* data, uint8_t NumOfSlaves);
