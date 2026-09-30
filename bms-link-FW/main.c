#include "sys_common.h"
#include "system.h"

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

bool clearFault = true;

void main(void)
{
    init_BMS_system();
//    restart_BMS_system();

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

//    hetREG1->PSL    &= ~(1 << GIO_BMS_FAULT_BIT);  // pull-down (0), pick based on your circuit

//    gioSetDirection(hetPORT1, 1U<<GIO_START_CHARGING_BIT);

    volatile float VoltCells[NUMBER_OF_CELLS_SERIES];
    volatile float AvgCellVolt_f, AvgCellSoC, MinCellVolt_f, tempChip1, tempChip2, MaxCellVolt_f;
    volatile float tempChip[NUMBER_OF_SLAVE_BOARDS_TOTAL];

//    SetChargingStatus(CH);

//    const sciBASE_t * UARTReg = sciREG;
    volatile uint32_t tic, toc;

    volatile int i;

    volatile const uint64_t debug_period_tick = MIN_SEC_MS_US_TICK_2_TICK(0,1,0,0,0);
    uint64_t last_debug_tick = 0;

    while(1){
//        restart_BMS_system();
        restart_BMS_system();
//        SetChargingStatus(CH);

        while(! AnyFaults()){
            Do_BMS_Tasks();
            if(hasPeriodExpired_rti(debug_period_tick, &last_debug_tick)){
                int a=0;
                a++;
            }
        }


        clearFault = false;
#warning This is not complete >>
        clearFault = true; // get rid of this, VCU should set to true;
        do{
            //Do_BMS_FaultStateTask();
        }
        while(!clearFault);

    }

}
