/*
Author: Tanjosh Sidhu
*/

//#include "iso_spi_driver.h"
#include "spi.h"
#include <stdint.h>
#include <stdbool.h>
#include "ltc6811_commands.h"
#include "SlaveCommunication.h"
//#include "rti.h"


//spiDAT1_t *SPI_LinkConfigData;
spiBASE_t *Slave_SPI_REG = REG_FOR_SPI;
struct SlaveBatteryCellData_struct SlaveBatteryCellData;
uint16_t ShiftBytes[NUMBER_OF_SLAVE_BOARDS-1] = {0};

//----------------------------------------------------------------------------------------
void delay_ms_us(uint32_t ms, uint32_t us)
{
    volatile uint32_t i, j;
    // Approximate loops per ms, tune by measurement.
    const uint32_t loops_per_ms = 100000; // example for 200 MHz
    const uint32_t loops_per_us = 100;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < loops_per_ms; j++);
    }
    for (i = 0; i < us; i++) {
        for (j = 0; j < loops_per_us; j++);
    }
}

uint16_t pec15Table[256];
#define PEC_INIT_VALUE 0x0010
#define PEC_CHARACTERISTIC_POLYNOMIAL 0x4599

 void init_PEC15_Table(){
     uint16_t remainder;
     int i, bit;
     for (i = 0; i < 256; i++){
         remainder = i << 7;
         for (bit = 8; bit > 0; --bit){
             if (remainder & 0x4000){
                 remainder = ((remainder << 1));
                 remainder = (remainder ^ PEC_CHARACTERISTIC_POLYNOMIAL);
             }
             else
             {
                 remainder = ((remainder << 1));
             }
         }
         pec15Table[i] = remainder & 0xFFFF;
     }
 }
 uint16_t pec15_calc(uint8_t len, uint16_t *data){
         uint16_t remainder;
         uint8_t address;

         remainder = PEC_INIT_VALUE;//PEC seed, 16

         int i, j, Jrevese;
         for (i = 0; i < len; i++){
             for(j=0; j<2;j++){
                 Jrevese = 1-j;
                 uint16_t word = data[i];
                 uint8_t byte = (uint8_t) ((word >> (8*Jrevese)) & 0xFFU);
                 address = ((remainder >> 7) ^ byte) & 0xff;//calculate PEC table address
                 remainder = (remainder << 8 ) ^ pec15Table[address];
             }
         }
     return remainder<<1;//The CRC15 has a 0 in the LSB so the final value must be multiplied by 2
 }



uint16_t SPI_SR2Link_HalfWord(uint16_t Tx, bool CS){
    uint32 CS_HOLD = CS? 0U:CS_HOLD_MASK;
    Slave_SPI_REG->DAT1 =   CS_HOLD | SPI_CONFIG0_WORD | (uint32)Tx;
    while((Slave_SPI_REG->FLG & 0x00000100U) != 0x00000100U){} /* Wait */
    uint16_t Rx = Slave_SPI_REG->BUF;
    return Rx;
}
uint8_t SPI_SR2Link_Byte(uint16_t Tx, bool CS_LOW_END){
    uint32 CS_HOLD = CS_LOW_END? CS_HOLD_MASK:0U;
    Slave_SPI_REG->DAT1 =   CS_HOLD | SPI_CONFIG0_WORD | (uint32)Tx;
    while((Slave_SPI_REG->FLG & 0x00000100U) != 0x00000100U){} /* Wait */
    uint8_t Rx = Slave_SPI_REG->BUF;
    return Rx;
}
uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize, bool CS_Low_End){
    uint16 Tx_Data, Rx_Data;

    uint16 tx_Pec = pec15_calc(MsgSize, Tx);

   while(MsgSize-- && !(Slave_SPI_REG->FLG & 0xFF)){

        Tx_Data =   (Tx == NULL)  ? SPI_DUMMY_DATA : *Tx++;

        Rx_Data = SPI_SR2Link_HalfWord(Tx_Data, TRUE);
        if(Rx != NULL)
            *Rx++ = Rx_Data;
    }
    uint16_t Rx_pec = SPI_SR2Link_HalfWord(tx_Pec, CS_Low_End);

    return (Slave_SPI_REG->FLG & 0xFF);
}
uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize,  bool CS_Config){

    uint32 Out1 = SPI_SendAndRecevie_Links(NULL, Rx, MsgSize, CS_Config);
    return 0;
}
uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize,  bool CS_Config){
    return SPI_SendAndRecevie_Links(Tx, NULL, MsgSize, CS_Config);
}
uint32 SendCmd2Slave(uint16_t cmd, bool CS_Config){
    return SPI_Send2Links(&cmd, 1, CS_Config);
}
void wait_SDO_poll(){
    SendCmd2Slave(LTC6811_PLADC, TRUE);

    int i;
    uint16_t status;

    SPI_SR2Link_HalfWord(SPI_DUMMY_DATA, TRUE);
    for (i = 0, status = 0; i < NUMBER_OF_SLAVE_BOARDS; i++, status = 0)
        while (!(status & 0x0001))
            status = SPI_SR2Link_HalfWord(SPI_DUMMY_DATA, TRUE);

    status = SPI_SR2Link_HalfWord(SPI_DUMMY_DATA, FALSE);
}

uint32_t send_spi_2_link(uint16_t cmd, uint16_t *tx_data, uint16_t *rx_data, uint16_t data_words){
    SendCmd2Slave(cmd, TRUE);

//    if (data_words == 0 || (rx_data == NULL && tx_data == NULL))
//        returnVal = 0;

    uint16_t ShiftBytes_RX[NUMBER_OF_SLAVE_BOARDS];

    uint32_t returnVal = SPI_SendAndRecevie_Links(tx_data, rx_data, data_words, TRUE);


    uint32_t returnVal1 = SPI_SendAndRecevie_Links(ShiftBytes, ShiftBytes_RX, NUMBER_OF_SLAVE_BOARDS-1, FALSE);
    return returnVal;
}
uint32 send_spi_2_link_chain(uint16_t cmd, uint16_t *tx_data, uint16_t *rx_data, uint16_t words_per_ic){

    if (words_per_ic == 0)
        return 0;

    uint16_t total_words = words_per_ic * NUMBER_OF_SLAVE_BOARDS;

    uint32_t returnVal = send_spi_2_link(cmd, tx_data, rx_data, total_words);

    return returnVal;
}

void WriteAllSlaves_Config(uint8_t Config2Read, uint8_t *datain){
    uint16_t cmds[2] = {LTC6811_WRCFGA, LTC6811_WRCFGB};
    uint16_t tx[CFGA_BYTES];
    int i,j;
    uint16_t HalfWord, DI;
    for(i=0;i<CFGA_HALF_WORDS;i++){
        DI=datain[i];
        for(j=0;j<2;j++){
            HalfWord = DI >> (8*j);
            tx[i*2+j] = (uint8_t)(HalfWord & 0x00FFU);
        }
    }
    send_spi_2_link_chain(cmds[Config2Read], tx, NULL, CFGA_HALF_WORDS);

}


// --------------------------------------------------------------------------------------------------
void WakeupSlave_isoSPI(){
    SendCmd2Slave(SPI_DUMMY_CMD, FALSE);
}
void wakeup_sleep(){
    int i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
        SendCmd2Slave(SPI_DUMMY_CMD, TRUE);
        delay_ms_us(0, tWAKE_us);
        SendCmd2Slave(SPI_DUMMY_CMD, FALSE);
    }
//    delay_us(300);
}

void initLink(){
    init_PEC15_Table();
    //------------------------------------------------
        uint8_t CFGA[CFGA_BYTES] = {0};
        uint8_t CFGB[CFGA_BYTES] = {0};
    //    CFGR[0] = 0b00000100;
    //    CFGR[0] |= 1<<2;

        wakeup_sleep();

        uint16_t tx16[CFGA_HALF_WORDS];

        int i;
        for (i = 0; i < CFGA_HALF_WORDS; i++)
            tx16[i] = ((uint16_t)CFGA[i*2] << 8) | CFGA[i*2 + 1];

        SendCmd2Slave(LTC6811_WRCFGA, TRUE);
        SPI_Send2Links(&tx16[0], CFGA_HALF_WORDS, FALSE);
}


// --------------------------------------------------------------------------------------------------
void CommandAllFullSlave2Read(uint16_t* cmds, uint16_t* dataOut){
    uint16_t dataOutSub[(NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD)*NUMBER_OF_SLAVE_BOARDS];
    int i, j, k;
    int DO_idx, DOS_idx;
    for(i=0; i<NUMBER_OF_CELL_CMD_GROUPS; i++){
        uint16_t current_cmd = cmds[i];

        send_spi_2_link_chain(current_cmd,
                              NULL,   // optional (can be NULL)
                              &dataOutSub[0],   // optional (can be NULL)
                              NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD);

        for(j=0; j<NUMBER_OF_SLAVE_BOARDS; j++){
            for(k=0; k<NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD; k++){
                DO_idx = NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD*i + CELLS_PER_SLAVE_BOARD*j + k;
                DOS_idx = NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD*j+k;
                dataOut[DO_idx]=dataOutSub[DOS_idx];
            }
        }
//        delay_us(300);
    }
}

void ReadAllSlaves_Volt(uint16_t* dataOut){
    uint16_t Volt_cmds[NUMBER_OF_CELL_CMD_GROUPS] = {LTC6811_RDCVA, LTC6811_RDCVB, LTC6811_RDCVC, LTC6811_RDCVD};

//    SetRefOn(1);
    WakeupSlave_isoSPI();
    SendCmd2Slave(LTC6811_ADCV, FALSE);
    delay_ms_us(5, 0);
    WakeupSlave_isoSPI();
    CommandAllFullSlave2Read(&Volt_cmds[0], dataOut);
//    SetRefOn(0);
}

void ReadAllSlaves_Config(uint8_t Config2Read, uint8_t *dataOut){
    uint16_t cmds[2] = {LTC6811_RDCFGA, LTC6811_RDCFGB};
    uint16_t rx[CFGA_HALF_WORDS];
    send_spi_2_link_chain(cmds[Config2Read], NULL, rx, CFGA_HALF_WORDS);
    int i,j;
    uint16_t HalfWord, Rx;
    for(i=0;i<CFGA_HALF_WORDS;i++){
        Rx=rx[i];
        for(j=0;j<2;j++){
            HalfWord = Rx >> (8*j);
            *dataOut = (uint8_t)(HalfWord & 0x00FFU);
            dataOut++;
        }
    }
}



void ReadAllSlaves_GPIO(uint8_t Config2Read, uint8_t *dataOut){

}

void CmdAllSlaves_Measure(){

}

//void Read1Slave_Temp(uint16_t* dataOut){
//    const uint16_t  Temp_cmds[NUMBER_OF_CELL_CMD_GROUPS] = {LTC6811_RDCVA, LTC6811_RDCVB, LTC6811_RDCVC, LTC6811_RDCVD};
//
//    CommandFullSlave2Read(&Volt_cmds[0], dataOut)
//}
