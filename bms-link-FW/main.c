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
//#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunication_Routines.h"
#include "PhantomHelpers.h"

#include "BatteryData.h"
#include "BMS_Tasks.h"

#include "rti.h"
#include "UART_Comms.h"

void main(void)
{
/* USER CODE BEGIN (3) */
    _enable_IRQ();
    init_BMS_system();
    sciInit();
    


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
        SCI_SendCellVoltages();
        SCI_SendFaultsAndWarnings();

        if(i<1){
            i=0;
            initLink();
            initBatteryData();
        }
        else
            i++;
    }
}
