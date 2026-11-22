#include "sys_common.h"
#include "system.h"

#include "BMS_Routines.h"
#include "BMS_Tasks.h"

#include "PhantomAlgorithms.h"
#include "PhantomTimers.h"

#include "BMS_Data.h"
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

    volatile ticks_t tic, toc;
    volatile int i;
    volatile TimingStats_t MeasureCellVoltageStats, CheckSlaveFlagsStats, SetBalanceRateStats;
    Periodic_t debug_tick = { ticks_make(0,1,0,0,0), 0};
    volatile float avgCellVolt;

    while(1){
        restart_BMS_system();

        while(!Fault_Any()){
            Do_BMS_Tasks();
            if(periodic_due(&debug_tick)){
                uint16_t DCC[NUMBER_OF_SLAVE_BOARDS_TOTAL];
                ReadConfig_DCC(DCC);
                avgCellVolt = Cell_GetAvgVolt();
//                MeasureCellVoltageStats = BMSTask[0].stats;
//                SetBalanceRateStats = BMSTask[1].stats;
//                CheckSlaveFlagsStats = BMSTask[2].stats;
                avgCellVolt+=0;
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
