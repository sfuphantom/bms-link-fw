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
#include "can.h"
#include "charger_can.h"

/* Include ESM header file - types, definitions and function declarations for system driver */
#include "esm.h"

#define D_SIZE 9
#define LOOPBACK_TEST 0

uint8 tx_data[D_SIZE] = {'H','E','R','C','U','L','E','S','\0'};
uint8 rx_data[D_SIZE] = {0};
uint32 error = 0;

uint32 checkPackets(uint8 *src_packet, uint8 *dst_packet, uint32 psize);
void CAN_Loopback_test();
void send_CAN_packets(ChargerCmd_t cmd);
/* USER CODE END */


/* USER CODE BEGIN (2) */
void CAN_Loopback_test(){
    canEnableloopback(canREG1, Internal_Lbk);
    canUpdateID(canREG1, canMESSAGE_BOX2, (uint32)0x40000000U | (uint32)0x00000000U | (uint32)(0x1806E5F4U & 0x1FFFFFFFU));

    ChargerCmd_t test_cmd = {
       .max_voltage_dV = 4000U,
       .max_current_dA = 200U,
       .charge_enable  = 1U
    };

    ChargerStatus_t received = {0};
    uint32 result = 0U;
    uint32 voltage_ok = 0U;
    uint32 current_ok = 0U;
    uint32 test_pass = 0U;

    Charger_SendCmd(&test_cmd);

    while (!canIsRxMessageArrived(canREG1, canMESSAGE_BOX2));

    uint8_t test_buf[8] = {0};
    canGetData(canREG1, canMESSAGE_BOX2, test_buf);

    result = Charger_GetStatus(&received);

    voltage_ok = (received.output_voltage_dV == 4000U) ? 1U : 0U;
    current_ok = (received.output_current_dA == 200U)  ? 1U : 0U;
    test_pass = (voltage_ok && current_ok) ? 1U : 0U;
}

void send_CAN_packets(ChargerCmd_t cmd){
//    can_error = canGetLastError(canREG1);
//    can_level = canGetErrorLevel(canREG1);
    uint32 tx_result;
    ChargerStatus_t status = {0};

    if (Charger_GetStatus(&status) != 0U) { }
    tx_result = Charger_SendCmd(&cmd);

    // Receive CAN messages
    if (Charger_GetStatus(&status) != 0U)
    {
       if (status.status_flags == 0U){
           cmd.charge_enable = 1U;
       }
       else{
           cmd.charge_enable = 0U;
           if (status.status_flags == CHARGER_FLAG_HW_FAULT)
       }
    }

    Charger_SendCmd(&cmd);
}
/* USER CODE END */


void main(void)
{
/* USER CODE BEGIN (3) */

    canInit();

#if LOOPBACK_TEST
    void CAN_Loopback_test();
    while(1);

#else
       // CMD is a packet stuffed with arbitrary values
       ChargerCmd_t cmd = {0};
       cmd.max_voltage_dV = 4000U;
       cmd.max_current_dA = 200U;
       cmd.charge_enable = 0U;


       //testing
//       uint32 can_error = canGetLastError(canREG1);
//       uint32 can_level = canGetErrorLevel(canREG1);

       while(1)
       {
           send_CAN_packets(cmd);
       }

#endif
/* USER CODE END */
}

/* USER CODE BEGIN (4) */
uint32 checkPackets(uint8 *src_packet, uint8 *dst_packet, uint32 psize)
{
   uint32 err=0;
   uint32 cnt=psize;

   while(cnt--)
   {
     if((*src_packet++) != (*dst_packet++))
     {
        err++;           /*  data error  */
     }
   }
   return (err);
}
/* USER CODE END */


