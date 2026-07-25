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
//    hetInit();
//    gioSetDirection(hetPORT1, 1U<<GIO_START_CHARGING_BIT);

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
//    gioSetBitHelper(GIO_BMS_FAULT_BIT, GIO_HIGH);
//    init_BMS_Faults();
//
//
//    const hetSIGNAL_t signal = {0, 40};
//    pwmSetSignal(FAN_HET_RAM, 0, signal);
//    pwmSetSignal(FAN_HET_RAM, 3, signal);
//    pwmStart(FAN_HET_RAM, 0);
//    pwmStart(FAN_HET_RAM, 3);



//    hetREG1->PSL    &= ~(1 << GIO_BMS_FAULT_BIT);  // pull-down (0), pick based on your circuit



//    gioSetDirection(hetPORT1, 1U<<GIO_START_CHARGING_BIT);
//    gioSetDirection(hetPORT1, 1U<<GIO_START_CHARGING_BIT);
//    gioSetDirection(hetPORT1, 1U<<GIO_START_CHARGING_BIT);

//    enableAllInterrupts();
////
//    ecapInit();
//    ecapStartCounter(ecapREG6);
//    ecapEnableCapture(ecapREG6);
//    ecapREG6->ECCTL1 |=1<<8U;
//    ecapREG6->ECCTL1 |=3<<14U;


    volatile float VoltCells[NUMBER_OF_CELLS];
//    float VoltGPIO[NUMBER_OF_GPIOS];
//
    volatile float AvgCellVolt_f, AvgCellSoC, MinCellVolt_f, tempChip1, tempChip2, MaxCellVolt_f;
//    hetSIGNAL_t signal;
//    int i=0;

//    StartAllFans();
//    SetAllFansDuty(50);

//    char T[] = "12345678";
//    char R[8];
//    Gio_State_t T1, G1 , G2, G3;

//    #define S_ARRAY_SIZE 20
//    uint32_t Start_Array = 0;
//    uint32_t Start_Array_Mask = (1U<<S_ARRAY_SIZE)-1;
//    bool FansOn = FALSE;
//    bool ChangeFanStatue = FALSE;
//    Gio_State_t GIO_Start_Level = GIO_HIGH;
//    Gio_State_t DEBUG_FAULT_LEVEL = GIO_HIGH;


//    hetSIGNAL_t signal = {100, 40};
//    SetAllFansSignal(signal);

//    StartAllFans();
//    int x=0;
//    int jump = 10;
//    while(1){
//        for(x=0; x<100; x+=jump){
//            SetAllFansDuty(x);
//        }
//    }
    SetChargingStatus(CH);

//    const sciBASE_t * UARTReg = sciREG;
    volatile uint32_t tic, toc;

    volatile int i;
    volatile uint8_t data[8];
    while(1){


//        for(i=1;i<64;i++){
//            uint32_t out = canIsRxMessageArrived(canREG1, i);
//            if(out){
//                i=i;
//                canGetData(canREG1, i, data);
//
//                continue;
//            }
//
//        }

//        DEBUG_FAULT_LEVEL = (Gio_State_t)((hetREG1->DIN >> GIO_DEGUBING_BIT1) & 1);//gioGetBitHelper(GIO_START_CHARGING_BIT);
//        if(DEBUG_FAULT_LEVEL == GIO_LOW){
//            SetBMSFault_bool_HIGH(DEBUG_FLAG);
//
//        }
//
////        if(i&1){
////            gioSetBitHelper(GIO_BMS_FAULT_BIT, GIO_HIGH);
////        }
////        else {
////            gioSetBitHelper(GIO_BMS_FAULT_BIT, GIO_LOW);
////        }
////        i++;
//
//        if(!rtiTimerExpired(0, 50, 0)){
//            continue;
//        }
//        GIO_Start_Level = (Gio_State_t)((hetREG1->DIN >> GIO_START_CHARGING_BIT) & 1);//gioGetBitHelper(GIO_START_CHARGING_BIT);
//
//        Start_Array <<= 1;
//        Start_Array |= (uint32_t)(GIO_Start_Level == GIO_HIGH);
//        Start_Array &= Start_Array_Mask;
//
//        if(ChangeFanStatue && Start_Array == Start_Array_Mask && !AnyFaults()){
////            StartAllFans();
//
//            ChangeFanStatue = FALSE;
//
//            FansOn = !FansOn;
//            if(FansOn){
//                pwmSetDuty(FAN_HET_RAM, 0, 95);
//                pwmSetDuty(FAN_HET_RAM, 3, 95);
//            }
//            else {
//                pwmSetDuty(FAN_HET_RAM, 0, 5);
//                pwmSetDuty(FAN_HET_RAM, 3, 5);
//            }
//
////            SetAllFansDuty(50);
//        }
//        else if(!ChangeFanStatue && Start_Array == 0 && !AnyFaults()){
////            StopAllFans();
//
//            ChangeFanStatue = TRUE;
//
////            SetAllFansDuty(0);
//        }
//
//
//        if(AnyFaults()){
//            pwmSetDuty(FAN_HET_RAM, 0, 5);
//            pwmSetDuty(FAN_HET_RAM, 3, 5);
////            init_BMS_Faults();
//        }


        HV_DataRoutine();

//        volatile uint32_t tic = timer_tic_tick();
//        CellVoltageControlRoutine();
//        volatile uint32_t toc_V = timer_toc_us(tic);

//        tic = timer_tic_tick();
//        MonitorCellTempRoutine();
//        uint32_t toc_T = timer_toc_us(tic);

//        tic = timer_tic_tick();
//        SlaveFlagsRoutine();
//        volatile uint32_t toc_F = timer_toc_us(tic);
////
////
//        CellVoltageControlTask();
//////#if USE_ANILOG_GPIO
//////        MonitorCellTempTask();
//////#endif
//        SlaveFlagsCheckTasks();
////
////

//        Do_BMS_Tasks();
//        AvgCellVolt_f = GetAvgCellVolt_float();
//        MinCellVolt_f = GetMinCellVolt_float();
//        MaxCellVolt_f = GetMaxCellVolt_float();
////
//        tempChip1 = Slave_ADC2Celcius(GetStatusRegData()->ITMP);

//        tempChip2 = Slave_ADC2Celcius((GetStatusRegData()+1)->ITMP);

//        AvgCellSoC = GetAvgCellSOC();


        if(AnyFaults()){
//            init_BMS_system();
            ClearAllFaults();
            SetChargingStatus(CH);
            SetAllPWM_Regs(0xF);
        }
//
//        if(rtiTimerExpired(4, 50, 0)){
////            tic = timer_tic_tick();
//
//            Send_CV_and_SlaveTemp();
////            toc = timer_toc_us(tic);
//        }

//        if(rtiTimerExpired(5, 500, 0)){
//
//            in=0;
//        }


//        if(i<1){
//            i=0;
//            initLink();
//            initBatteryData();
//        }
//        else
//            i++;




    }
}




void SCI_PrintArray_16(uint16_t *data, uint16_t count){
    uint8_t buf[2];  // 2 data bytes + null terminator for SCI_Print's strlen()
    uint16_t i;

    for (i = 0; i < count; i++)
    {
        buf[0] = (uint8_t)((data[i] >> 8) & 0xFF);  // high byte first (big-endian)
        buf[1] = (uint8_t)(data[i] & 0xFF);         // low byte


        sciSendByte(scilinREG, buf[0]);
        sciSendByte(scilinREG, buf[1]);
    }
}

void Send_CV_and_SlaveTemp(){
    sciSendByte(scilinREG, 0);

    SCI_PrintArray_16(GetCellVoltReadPrt(), NUMBER_OF_CELLS);

    const struct ConfigReg* ConfigReg_prt = &ConfigRegWriteData[0];
    const struct StatusReg* StatusReg_prt = GetStatusRegData();

    float tempChip;
    uint16_t tempChip_16, DCC;

    int i;

    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++)
    {
        tempChip = Slave_ADC2Celcius(StatusReg_prt -> ITMP);
        tempChip_16 = (uint16_t)(tempChip*1000);

        SCI_PrintArray_16(&tempChip_16, 1);

        StatusReg_prt++;
    }
    for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++)
    {
        DCC = ConfigReg_prt->DCC;

        SCI_PrintArray_16(&DCC, 1);

        ConfigReg_prt++;
    }

}
