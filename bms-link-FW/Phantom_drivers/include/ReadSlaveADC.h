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


#define NumberOfSlaves = 1;
uint8_t Ref_ON = 0;

#define ADC_Voltage_cmd [(LTC6811_ADCV >> 8) & 0x00ff, LTC6811_ADCV & 0x00ff]



void uint16_t2ByteArray(uint16_t in, uint8_t* out);
uint16_t ByteArray2uint16_t(uint8_t* in);

bool setupACD_init();

void wakeup_idle(uint8_t total_ic);
void wakeup_sleep(uint8_t total_ic);

void MasterCommandAllSlaves(uint16_t cmd, uint16_t* data, uint8_t NumOfSlaves, spiDAT1_t dataconfig);
//
//bool setupACD_init();
//bool WakeupYourSlaves_ADC()
//bool ProcessData(uint16_t* Raw, uint16_t* OutPut, uint8_t data_len);
//bool MasterCommandAllYourSlaves(uint16_t cmd, uint16_t* data, uint8_t NumOfSlaves, spiDAT1_t dataconfig);
//
//int SaveData(uint16_t* Disnation, uint16_t* data);
//
//
//
//bool Sleep2Standby();
//
//uint8_t* uint16_t2ByteArray(uint16_t TheBytes);
