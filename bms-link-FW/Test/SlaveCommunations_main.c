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
//#include "spi.h"
//#include "SlaveCommunication_Drivers.h"
#include "BMS_Routines.h"
#include "PhantomHelpers.h"

#include "BatteryData.h"
#include "BMS_Tasks.h"

#include "rti.h"

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

void SPI_main(void)
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


void SlaveCommunations_main(void)
{
/* USER CODE BEGIN (3) */
    init_BMS_system();


    int i=0;

    float VoltCells[NUMBER_OF_CELLS];
    float VoltGPIO[NUMBER_OF_GPIOS];

    float AvgCellVolt_f, AvgCellSoC, MinCellVolt_f;
    uint16_t AvgCellVolt_16;

    SetChargingStatus(TRUE);
    while(1){

//        delay_ms_us(1,0);
//        ToggleCS();

//        if(rtiTimerExpired(0, 1, 0)){
//            ToggleCS();
//        }

        uint32_t tic = timer_tic_tick();
        CellVoltageControlRoutine();
        uint32_t toc_V = timer_toc_us(tic);

//        tic = timer_tic_tick();
//        MonitorCellTempRoutine();
//        uint32_t toc_T = timer_toc_us(tic);

        tic = timer_tic_tick();
        SlaveFlagsRoutine();
        uint32_t toc_F = timer_toc_us(tic);


//        CellVoltageControlTask();
//#if USE_ANILOG_GPIO
//        MonitorCellTempTask();
//#endif
//        SlaveFlagsCheckTasks();

//        TaskSuperLoop(SlaveComunationSubTask, 3, keepAwake);

        Slave_ADC2Volt_arr(GetCellVoltReadPrt(), VoltCells, NUMBER_OF_CELLS);
        AvgCellVolt_16 = GetAvgCellVolt();
        AvgCellVolt_f = GetAvgCellVolt_float();
        MinCellVolt_f = GetMinCellVolt_float();

        AvgCellSoC = GetAvgCellSOC();

        if(i<1){
            i=0;
            initLink();
            initBatteryData();
        }
        else
            i++;



    }

/* USER CODE END */
}
/* USER CODE BEGIN (4) */
/* USER CODE END */

#endif
