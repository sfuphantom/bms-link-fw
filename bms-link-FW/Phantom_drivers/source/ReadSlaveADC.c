/*
Author: Tanjosh Sidhu
*/

#include "ReadSlaveADC.h"
//#include "iso_spi_driver.h"
#include <stdint.h>
#include <stdbool.h>

void uint16_t2ByteArray(uint16_t in, uint8_t* out){
    const uint8_t ArraySize =2;
    int i=0;
    for (i=0; i<ArraySize; i++){
        int I=i<<3;
        out[i] = (uint8_t)((in >> I))& 0xFF;
    }
}
uint16_t ByteArray2uint16_t(uint8_t* in){
    const uint8_t ArraySize = 2;
    uint16_t out = 0;
    int i=0;
    for (i=0; i<ArraySize; i++)
        out |= ((uint16_t)in[i]) << (i<<3);
    return out;
}

bool SetRefOn(bool State){

}



void MasterCommandAllSlaves(uint16_t cmd, uint16_t* dataOut, uint8_t NumOfSlaves, spiDAT1_t dataconfig){
    const uint8_t UnitSize_Shift = 1;
    const uint8_t UnitSize = 1<<UnitSize_Shift;
    uint8_t cmdArr[UnitSize];

    uint8_t AllSlaveOut[UnitSize];

    bytes2ByteArray(cmd, &cmdArr[0]);

    SetRefOn(1);

    int i=0;
    for (i=0; i<NumOfSlaves; i++){
        spiSendAndGetData(spiREG3, &dataconfig1_t, UnitSize, &cmdArr[0], &AllSlaveOut[0]);
        uint16_t2ByteArray(&AllSlaveOut[0], dataOut[i]);
    }
    SetRefOn(0);
}




