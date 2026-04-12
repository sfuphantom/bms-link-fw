/*
Author: Tanjosh Sidhu
*/

#include "iso_spi_driver.h"
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"
#include "SlaveCommunication.h"


#define NUMBER_OF_SLAVE_BOARDS 1
#define IC_PER_BOARD 12
#define NumberOfSlavesIC    (IC_PER_BOARD * NUMBER_OF_SLAVE_BOARDS)

#define MSG_SIZE_BYTES      2
#define MSG_SIZE_BITS       MSG_SIZE_BYTES<<3


void uint16_t2ByteArray(uint16_t in, uint8_t* out);
uint16_t ByteArray2uint16_t(uint8_t* in);

void setupACD_init();
void wakeup_sleep();
void SetRefOn(bool NewState);
void readAllADC(uint16_t* data);


void WipTheSlavesAwake();
void buySlaves();
void MasterCommandAllSlaves(uint16_t cmd, uint16_t* data);


//#define SPI_dataconfig_t dataconfig_t
#define REG_FOR_SPI spiREG1
