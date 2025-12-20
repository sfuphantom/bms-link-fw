/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "sys_common.h"
#include "system.h"

/* USER CODE BEGIN (1) */
#include "sci.h"


#define  TSIZE1 10
uint8  TEXT1[TSIZE1]= {'H','E','R','C','U','L','E','S',' ',' '};
#define  TSIZE2 18
uint8  TEXT2[TSIZE2]= {'M','I','C','R','O','C','O','N','T','R','O','L','L','E','R','S',' ',' '};
#define  TSIZE3 19
uint8  TEXT3[TSIZE3]= {'T','E','X','A','S',' ','I','N','S','T','R','U','M','E','N','T','S','\n','\r'};
#define  DTSize 10
uint8  DataTest_Tx[DTSize]= {'Y','o','u','r',' ','M','o','m','!','\n'};
uint8  DataTest_Rx[DTSize];

void sciDisplayText(sciBASE_t *sci, uint8 *dataIn, uint8 *dataOut, uint32 length);
void sciDisplayText_Tx(sciBASE_t *sci, uint8 *text, uint32 length);
void sciDisplayText_Rx(sciBASE_t *sci, uint8 *text, uint32 length);
void wait(uint32 time);


#define UART scilinREG
/* USER CODE END */

/* USER CODE BEGIN (2) */
/* USER CODE END */


void main(void)
{
/* USER CODE BEGIN (3) */

    sciInit();      /* initialize sci/sci-lin    */
                    /* even parity , 2 stop bits */

    while(1)        /* continious desplay        */
    {
//      sciDisplayText_Tx(UART,&TEXT1[0],TSIZE1);   /* send text code 1 */
//      sciDisplayText_Tx(UART,&TEXT2[0],TSIZE2);   /* send text code 2 */
//      sciDisplayText_Tx(UART,&TEXT3[0],TSIZE3);   /* send text code 3 */
//        sciDisplayText_Tx(UART, &DataTest_Tx[0], DTSIZE);   /* send DataTest code */
//        sciDisplayText_Rx(UART, &DataTest_Rx[0], DTSIZE);
        sciDisplayText(UART, &DataTest_Tx[0], &DataTest_Rx[0], DTSize);

      wait(200);
    };

/* USER CODE END */
}

void wait(uint32 time){time--;}

void sciDisplayText_Tx(sciBASE_t *sci, uint8 *text,uint32 length)
{
    while(length--)
    {
        while ((UART->FLR & 0x4) == 4); /* wait until busy */
        sciSendByte(UART,*text++);      /* send out text   */
    };
}


void sciDisplayText_Rx(sciBASE_t *sci, uint8 *data,uint32 length){
        uint8 rx;
        while(length--){
            while ((UART->FLR & 0x4) == 4);                     /* wait until busy */
            rx = sciReceiveByte(sci);
            *data++ = rx;      /* Read Message text   */
        };
}

void sciDisplayText(sciBASE_t *sci, uint8 *dataIn, uint8 *dataOut, uint32 length){
    uint8 rx;
    while(length--)
    {
        while ((UART->FLR & 0x4) == 4);
        sciSendByte(UART,*dataIn++);

        rx = sciReceiveByte(sci);
        *dataOut++ = rx;
    };
}

