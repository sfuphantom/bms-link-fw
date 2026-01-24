/*
Author: Tanjosh Sidhu
*/

#include "ReadSlaveADC.h"
//#include "iso_spi_driver.h"
#include <stdint.h>
#include <stdbool.h>


uint16_t ByteArray2uint16_t(uint8_t* in){
    const uint8_t ArraySize = 2;
    uint16_t out = 0;
    int i=0;
    for (i=0; i<ArraySize; i++)
        out |= ((uint16_t)in[i]) << (i<<3);
    return out;
}

void wakeup_sleep(uint8_t total_ic){
    #define DummySize 2

    const uint16_t dummyCmd = LTC6811_RDCFGA;
    uint8_t AllSlaveOut[DummySize];
    int i;
    for(i=0; i<total_ic; i++)
        read_reg(dummyCmd, &AllSlaveOut[0], DummySize);
}

bool setupACD_init(uint8_t total_ic){
    #define NumOfBytes2Config 6
    uint8_t CFGA[NumOfBytes2Config] = {0};
//    CFGR[0] = 0b00000100;
//    CFGR[0] = 1<<2;

    wakeup_sleep(total_ic);

    return write_reg(LTC6811_WRCFGA, CFGA, NumOfBytes2Config);
}

bool SetRefOn(bool NewState){
    const uint8_t dataLen = 1;
    const uint8_t RefOnBit = 2;

    uint8_t Data2Write = NewState? (1<<RefOnBit): 0;

    return write_reg(LTC6811_WRCFGA, &Data2Write, dataLen);
}

void readAllADC(uint16_t* dataOut, uint8_t total_ic){
    MasterCommandAllSlaves(LTC6811_RDCVA, dataOut, total_ic);
}

void buySlaves(uint8_t NumOfSlaves){
    setupACD_init(NumberOfSlaves);
}
void WipTheSlavesAwake(uint8_t NumOfSlaves){
    wakeup_sleep(NumOfSlaves);
}
void MasterCommandAllSlaves(uint16_t cmd, uint16_t* dataOut, uint8_t NumOfSlaves){
    #define UnitSize        2

    uint8_t AllSlaveOut[UnitSize];

    SetRefOn(1);

    int i=0;
    for (i=0; i<NumOfSlaves; i++){
        read_reg(cmd, &AllSlaveOut[0], UnitSize);

        dataOut[i] = ByteArray2uint16_t(&AllSlaveOut[0]);
    }
    SetRefOn(0);
}




