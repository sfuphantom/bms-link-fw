/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "sys_common.h"
#include "system.h"

/* USER CODE BEGIN (1) */
#include "Fault_handler.h"
/* USER CODE END */

/* USER CODE BEGIN (2) */
/* USER CODE END */

void wait_us(uint32 time_us){
    int i;
    for(;time_us>0;time_us--){
        for(i=0;i<10;i++){

        }
    }
}


void GIO_main(void)
{
/* USER CODE BEGIN (3) */
    gioInit();
//    gioInit();

    while(1)        /* continious desplay        */
    {
//        int i;
//        for(i=0;i<32;i++)
//            gioToggleBit(gioPORTA, i);
        Gio_State_t T0 = gioToggleBitHelper(GIO_IMD_FAULT_BIT);
        Gio_State_t T1 = gioToggleBitHelper(GIO_BMS_FAULT_BIT);
//        Gio_State_t T2 = gioToggleBitHelper(GIO_START_CHARGING_BIT);
//        Gio_State_t T3 = gioToggleBitHelper(GIO_DEGUBING_BIT1);

//        wait_us(10);

        Gio_State_t Get0 = gioGetBitHelper(GIO_IMD_FAULT_BIT);
        Gio_State_t Get1 = gioGetBitHelper(GIO_BMS_FAULT_BIT);
        Gio_State_t Get2 = gioGetBitHelper(GIO_START_CHARGING_BIT);
        Gio_State_t Get3 = gioGetBitHelper(GIO_DEGUBING_BIT1);
        if (Get2 != GIO_LOW || Get3 != GIO_LOW ){
           int a=1+1;
           a++;
        }
//
//        gioSetBitHelper(GIO_IMD_FAULT_BIT, GIO_LOW);
//        gioSetBitHelper(GIO_BMS_FAULT_BIT, GIO_LOW);
//        gioSetBitHelper(GIO_START_CHARGING_BIT, GIO_LOW);
//        gioSetBitHelper(GIO_DEGUBING_BIT1, GIO_LOW);



    };

/* USER CODE END */
}



