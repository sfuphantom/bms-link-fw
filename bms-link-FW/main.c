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
*           - Uncheck Master Mode
*           - Uncheck Internal Clock
*       - SPI3 Port SubTAB
*           - Uncheck DIR for CS 0
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
#include "SlaveCommunication.h"
#include "ltc6811_commands.h"

//#define SPI_Test


/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

#ifdef SPI_Test
/* USER CODE BEGIN (2) */
uint16 TX_Data_Master[16] = { 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };
uint16 TX_Data_Slave[16]  = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 };
uint16 RX_Data_Master[16] = { 0 };
uint16 RX_Data_Slave[16]  = { 0 };
/* USER CODE END */

void main(void)
{
/* USER CODE BEGIN (3) */

    spiDAT1_t dataconfig1_t;

    dataconfig1_t.CS_HOLD = TRUE;
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
    while(1){
        /* Initiate SPI3 Transmit and Receive through Interrupt Mode */

        spiSendAndGetData(spiREG3, &dataconfig1_t, 16, TX_Data_Slave, RX_Data_Slave);

        /* Initiate SPI1 Transmit and Receive through Polling Mode*/
        setCS(LOW);
        spiTransmitAndReceiveData(spiREG1, &dataconfig1_t, 16, TX_Data_Master, RX_Data_Master);
        setCS(HIGH);
    }
    while(1);
/* USER CODE END */
}

#else


uint16_t VoltData[NUMBER_OF_CELLS];
float Volt[NUMBER_OF_CELLS];
uint16_t GPIO_Data[GPIOS_PER_SLAVE_BOARD];
uint16_t S_Control[12];

uint16_t REGA[WORD_REG_GROUP];
uint16_t REGB[WORD_REG_GROUP];
uint16_t Read_Reg_A[WORD_REG_GROUP];
uint16_t Read_Reg_B[WORD_REG_GROUP];
uint16_t pec_test[7]={0,1,2,2,2,2,2};

void main(void)
{
/* USER CODE BEGIN (3) */
    _enable_IRQ();
    spiInit();
    initLink();

    uint16_t pec1 = pec15_calc(1, &pec_test[0]);
    uint16_t pec2 = pec15_calc(1, &pec_test[1]);
    uint16_t pec3 = pec15_calc(1, &pec_test[3]);
    uint16_t pec4 = pec15_calc(5, &pec_test[2]);

    int i=0;

    REGA[0] = 0x8000;
    REGA[1] = 0xFFFF;
    REGA[2] = 0x0001;
    REGB[0] = 0xFFFF;
    REGB[1] = 0xFFFF;
    REGB[2] = 0xFFFF;
    WriteReg(LTC6811_WRCFGA, REGB);

//    swap_word_bytes_arr(REGA, REGA, 3);

    bool ReadV, ReadG;

//    Write_CFGR_General( 1, 0, 0x1F, 0x0AAA, 0x1, 0x000,0x000);

    while(1){
//        SPI_SR2Link_WORD(0x1371);
//        SPI_SR2Link_WORD(0xF0F0);
//        SPI_SR2Link_WORD(0xAAAA);
//        SPI_SR2Link_QWORD(0x0000FFFF0000FFFF, FALSE);
//        SPI_SR2Link_QWORD(0xAAAAAAAAAAAAAAAA, TRUE);

//        delay_ms_us(0, 2);
//        delay_ms_us(0, 2);

//        SPI_SR2Link_BYTE(0xFF);
//
//        setCS(HIGH);
//        delay_ms_us(0, 1);
//        wakeup_sleep();



//        bytes_to_words(CFGA_Bytes, CFGA_Words, WORD_REG_GROUP);
//        bytes_to_words(CFGB_Bytes, CFGB_Words, WORD_REG_GROUP);
//        uint32 Pec = ReadReg(LTC6811_RDCFGA, Read_Reg_A);
//        ReadReg(LTC6811_RDCFGB, Read_Reg_B);

        delay_ms_us(4,0);
        ReadV = GetVoltageReadings(VoltData);
        ReadG = GetGPIOReadings(GPIO_Data);
        ADC2Volt_arr(VoltData, Volt, NUMBER_OF_CELLS);


        if(ReadV && ReadG){
            MeasureALL(3,0);
            i=0;
        }
        i++;


//        uint16_t PecTest = ReadReg(LTC6811_RDCFGA, Read_Reg_A);
////        WriteReg(LTC6811_WRCFGA, REGB);
////        Write_CFGR(0xFFF);
////        Write_CFGR_General( 1, 0, 0x1F, 0x0AAA, 0x, 0x000,0xFFF);
//        if (i == 0x0F){
//            i=1;
//        }
//        else
//            i++;
//        ReadReg(LTC6811_RDCFGB, Read_Reg_B);
//
//        WriteReg(LTC6811_WRCFGA, REGA);
//        WriteReg(LTC6811_WRCFGB, REGB);


//        REGA[0] = 0xAAAA;
//        REGA[1] = 0xAAAA;
//        REGA[2] = 0xAAAA;
//        REGB[0] = 0xAAAA;
//        REGB[1] = 0xAAAA;
//        REGB[2] = 0xAAAA;
//        WriteReg(LTC6811_WRCFGA, REGA);
//        WriteReg(LTC6811_WRCFGB, REGB);
//        ReadAllSlaves_Volt(VoltData);
    }
/* USER CODE END */
}
/* USER CODE BEGIN (4) */
/* USER CODE END */

#endif
