/** @file example_SPI_Master_Slave.c
*   @brief Application main file
*   @date 25.July.2013
*   @version 03.06.00
*
*   This file contains an example of SPI1 and SPI3 Master / Slave configurations.
*
*   PIN Connections must be as Below
*     ---------------         ---------------
*     SPI1 ( Master )          SPI3 ( SLave)
*     ---------------         ---------------
*     SIM0             --->    SIMO
*     S0MI             <---    SOMI
*     CLK              --->    CLK
*     CS0              --->    CS0
*
*  ------------------
*  GUI configurations
*  ------------------
*  1) Driver TAB
*       - Select SPI3
*       - Select SPI1
*  2) VIm Channel 0-31
*       - Enable SPI3 Level 0 and Level 1 channels.
*  3) SPI3 TAB
*       - SPI3 Global SubTAB
*       	- Uncheck Master Mode
*       	- Uncheck Internal Clock
*       - SPI3 Port SubTAB
*       	- Uncheck DIR for CS 0
*  3) SPI1 TAB
*       - Have it default
*  4) Generate Code.
*
*/

/* (c) Texas Instruments 2009-2013, All rights reserved. */

/* 
* Copyright (C) 2009-2015 Texas Instruments Incorporated - www.ti.com
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  INCLUDING NEGLIGENCE OR OTHERWISE ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "sys_common.h"
#include "system.h"

/* USER CODE BEGIN (1) */
#include "spi.h"
#include "iso_spi_driver.h"
/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
uint16 TX_Data_Master[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };
uint16 TX_Data_Slave[16]  = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 };
uint16 RX_Data_Master[16] = { 0 };
uint16 RX_Data_Slave[16]  = { 0 };

/**
 * This function tests writing configuration data to the LTC6811 BMS chip.
 * Config Register Group A is 6 bytes and contains various configuration settings.
 */
bool test_write_reg(void)
{
    // Config Register Group A data (6 bytes)
    // // This is example data - adjust based on your configuration needs
    // // Byte 0-1: Cell discharge enable, voltage reference, etc.
    // // Byte 2-3: GPIO configuration, ADC mode, etc.
    // // Byte 4-5: Various configuration bits
    // uint8_t config_data[6] = {
    //     0x00, 0x00,  // Cell discharge enable flags, voltage reference
    //     0x00, 0x00,  // GPIO configuration, ADC mode
    //     0x00, 0x00   // Additional configuration bits
    // };
    
    // // Write to Config Register Group A
    // bool result = write_reg(LTC6811_WRCFGA, config_data, 6);
    
// 6 bytes of configuration data
    uint8_t config_bytes[6];

    config_bytes[0] = 0x00; // Keep GPIOs off/high
    config_bytes[1] = 0x00; // VUV
    config_bytes[2] = 0x00; // VUV/VOV
    config_bytes[3] = 0x00; // VOV
    config_bytes[4] = 0x01; // DCC1 = 1 (Cell 1 Discharge)
    config_bytes[5] = 0x10; // DCTO and DCC 9-12

    // 1. Send WRCFG Command (0x00 0x01)
    // 2. Send Command PEC
    // 3. Send the 6 config_bytes
    // 4. Send the Data PEC
	while (1){
	 bool result = write_reg(LTC6811_WRCFGA,  config_bytes, 6);
	}
	return true;
}

/**
 * This function tests reading configuration data from the LTC6811 BMS chip.
 * Config Register Group A is 6 bytes.
 */
// bool test_read_reg(void)
// {
//     // Buffer to store read data (6 bytes for Config Register A)
//     uint8_t read_data[6] = {0};
    
//     // Read from Config Register Group A
//     bool result = read_reg(LTC6811_RDCFGA, read_data, 6);
    
//     return result;
// }
/* USER CODE END */

void main(void)
{
/* USER CODE BEGIN (3) */

	spiDAT1_t dataconfig1_t;

	dataconfig1_t.CS_HOLD = FALSE;
	dataconfig1_t.WDEL    = TRUE;
	dataconfig1_t.DFSEL   = SPI_FMT_0;
	dataconfig1_t.CSNR    = 0xFE;


	/* Enable CPU Interrupt through CPSR */
	_enable_IRQ();

	/* Initialize SPI Module Based on GUI configuration
	 * SPI1 - Master ( SIMO, SOMI, CLK, CS0 )
	 * SPI3 - Slave  ( SIMO, SOMI, CLK, CS0 )
	 * */
	spiInit();
	
	// Test GPIO1 pulse function in a loop to verify SPI operation on oscilloscope
	// This toggles GPIO1 by writing to CFGR0 register
	while(1){
		// Set GPIO1 high (pull-down OFF) - GPIO1 bit = 1 in CFGR0
		test_gpio1_pulse_spi(true);
		
		// Delay for 0.5 seconds (half period)
		// Machine cycle time: 5.56 nanoseconds
		// Delay needed: 0.5 seconds = 500,000,000 nanoseconds
		// Iterations needed: 500,000,000 / 5.56 ≈ 89,928,057
		// Using 90,000,000 iterations for approximately 0.5 seconds
		volatile uint32_t delay;
		for (delay = 0; delay < 90000000; delay++);
		
		// Set GPIO1 low (pull-down ON) - GPIO1 bit = 0 in CFGR0
		test_gpio1_pulse_spi(false);
		
		// Delay for 0.5 seconds (half period)
		for (delay = 0; delay < 90000000; delay++);
	}
/* USER CODE END */
}

/* USER CODE BEGIN (4) */
/* USER CODE END */
