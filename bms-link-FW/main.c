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
#include "BMS_Routines.h"
#include "BMS_Tasks.h"


#include "PhantomHelpers.h"
#include "PhantomTimers.h"


#include "BatteryData.h"
#include "Fault_handler.h"


#include "SlaveCommunation_Functions.h"


#include "rti.h"
#include "het.h"
#include "reg_het.h"
#include "ecap.h"
#include "reg_ecap.h" //for ecapREG2 in capGetSignal call
#include "etpwm.h"
#include "can.h"
#include "reg_can.h"
#include "Fans.h"
#include "sci.h"

const bool CH = TRUE;
int in=0;

void Send_CV_and_SlaveTemp();

void main(void)
{
    init_BMS_system();

//    hetREG1->DIR &= ~(1 << GIO_START_CHARGING_BIT);
//    hetREG1->DIR &= ~(1 << GIO_DEGUBING_BIT1);
//    hetREG1->DIR |=  (1 << GIO_BMS_FAULT_BIT);
//
//    hetREG1->PULDIS &= ~(1 << GIO_START_CHARGING_BIT);  // enable pull
//    hetREG1->PULDIS &= ~(1 << GIO_DEGUBING_BIT1);  // enable pull
//    hetREG1->PULDIS &= ~(1 << GIO_BMS_FAULT_BIT);  // enable pull
//
//    hetREG1->PSL    &= ~(1 << GIO_START_CHARGING_BIT);  // pull-down (0), pick based on your circuit
//    hetREG1->PSL    &= ~(1 << GIO_DEGUBING_BIT1);  // pull-down (0), pick based on your circuit
//    hetREG1->PSL    |=  (1 << GIO_BMS_FAULT_BIT);  // pull-up (1) or
//


//    hetREG1->PSL    &= ~(1 << GIO_BMS_FAULT_BIT);  // pull-down (0), pick based on your circuit

//    gioSetDirection(hetPORT1, 1U<<GIO_START_CHARGING_BIT);

    volatile float VoltCells[NUMBER_OF_CELLS];
    volatile float AvgCellVolt_f, AvgCellSoC, MinCellVolt_f, tempChip1, tempChip2, MaxCellVolt_f;
    volatile float tempChip[NUMBER_OF_SLAVE_BOARDS];

    SetChargingStatus(CH);

//    const sciBASE_t * UARTReg = sciREG;
    volatile uint32_t tic, toc;

    volatile int i;
    volatile uint8_t data[8];
    uint32_t in=0 ;
    uint32_t now;
    while(1){
        now=getNow_tick();
//        if(in == 123){
//            now=getNow_tick();
//            in=0;
//        }
//        else{
//            in++;
//        }

//        MonitorFullBatteryDataRoutine();

//        CellVoltageControlRoutine();
//        SlaveFlagsRoutine();

//        MonitorCellTempRoutine();

        Do_BMS_Tasks();
        AvgCellVolt_f = GetAvgCellVolt_float();
        MinCellVolt_f = GetMinCellVolt_float();
        MaxCellVolt_f = GetMaxCellVolt_float();

        int Slave;
        for(Slave = 0; Slave<NUMBER_OF_SLAVE_BOARDS; Slave++){
            tempChip[Slave] = Slave_ADC2Celcius(StatusRegData[Slave].ITMP);
        }
//        AvgCellSoC = GetAvgCellSOC();

        if(AnyFaults()){
//            init_BMS_system();
            ClearAllFaults();
            SetChargingStatus(CH);
            SetAllPWM_Regs(0xF);
        }

        if(rtiTimerExpired(4, 50, 0)){

            int a=1+1;
//            Send_CV_and_SlaveTemp();
////            uint32_t time_us = timeFunction_VoidVoid_us(Send_CV_and_SlaveTemp);
        }

//        if(rtiTimerExpired(5, 200, 0)){
//
//            in=0;
//        }
    }
}




//void SCI_PrintArray_16(uint16_t *data, uint16_t count){
//    uint8_t buf[2];  // 2 data bytes + null terminator for SCI_Print's strlen()
//    uint16_t i;
//
//    for (i = 0; i < count; i++)
//    {
//        buf[0] = (uint8_t)((data[i] >> 8) & 0xFF);  // high byte first (big-endian)
//        buf[1] = (uint8_t)(data[i] & 0xFF);         // low byte
//
//
//        sciSendByte(scilinREG, buf[0]);
//        sciSendByte(scilinREG, buf[1]);
//    }
//}
//
//void Send_CV_and_SlaveTemp(){
//    sciSendByte(scilinREG, 0);
//
//    SCI_PrintArray_16(GetCellVoltReadPrt(), NUMBER_OF_CELLS);
//
//    const ConfigReg* ConfigReg_prt = &ConfigRegWriteData[0];
//    const StatusReg* StatusReg_prt = GetStatusRegData();
//
//    float tempChip;
//    uint16_t tempChip_16, DCC;
//
//    int i;
//
//    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++)
//    {
//        tempChip = Slave_ADC2Celcius(StatusReg_prt -> ITMP);
//        tempChip_16 = (uint16_t)(tempChip*1000);
//
//        SCI_PrintArray_16(&tempChip_16, 1);
//
//        StatusReg_prt++;
//    }
//    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++)
//    {
//        DCC = ConfigReg_prt->DCC;
//
//        SCI_PrintArray_16(&DCC, 1);
//
//        ConfigReg_prt++;
//    }
//
//}
