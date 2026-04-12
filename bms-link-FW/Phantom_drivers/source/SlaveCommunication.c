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


spiDAT1_t *SPI_LinkConfigData;
spiBASE_t *Slave_SPI_REG = REG_FOR_SPI;
struct SlaveBatteryCellData_struct SlaveBatteryCellData;

//----------------------------------------------------------------------------------------
 void delay_ms_us(uint32_t ms, uint32_t us)
{
//    volatile uint32_t i, j;
//    // Approximate loops per ms, tune by measurement.
//    const uint32_t loops_per_ms = 100000; // example for 200 MHz
//    const uint32_t loops_per_us = 100;
//    for (i = 0; i < ms; i++) {
//        for (j = 0; j < loops_per_ms; j++);
//    }
//    for (i = 0; i < us; i++) {
//        for (j = 0; j < loops_per_us; j++);
//    }
}

#define PEC_INIT_VALUE 0x0010
#define PEC_CHARACTERISTIC_POLYNOMIAL 0x4599

uint16_t pec15_calc(uint8_t len, uint16_t *data){
    if(len == 0 || data == NULL)
        return SPI_DUMMY_DATA;

    uint16_t pec = PEC_INIT_VALUE;
    int i, j;
    for (i = 0; i < len; i++){
        for(j = 0;j<2;j++){
            uint8_t j_shift = j<<3;
            uint16_t halfWord = data[i];
            uint8_t byte = (halfWord >> j_shift) & 0xFF;

            int bit;
            for (bit = 7; bit >= 0; bit--)
            {
                uint8_t din = (byte >> bit) & 0x01;
                uint8_t feedback = din ^ ((pec >> 14) & 0x01);

                pec <<= 1;
                if (feedback)
                    pec ^= PEC_CHARACTERISTIC_POLYNOMIAL;
            }
        }
    }
    return (pec & 0x7FFF)<<1; //0x7FFF
}

uint16_t SPI_SR2Link_HalfWord(uint16_t Tx, uint32 regConfig){
    Slave_SPI_REG->DAT1 =   regConfig | (uint32)Tx;
    while((Slave_SPI_REG->FLG & 0x00000100U) != 0x00000100U){} /* Wait */
    uint16_t Rx = Slave_SPI_REG->BUF;
    return Rx;
}
uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize, bool CS_Config){
    uint16 Tx_Data, Rx_Data;

    uint32 WDelay = (SPI_LinkConfigData->WDEL) ? 0x04000000U : 0U;
    SPIDATAFMT_t DataFormat = SPI_LinkConfigData->DFSEL;
    uint8 ChipSelect = SPI_LinkConfigData->CSNR;

    const uint32 CS_HOLD_Start = CS_HOLD_MASK;
    const uint32 CS_HOLD_End   = CS_Config ? 0U : CS_HOLD_MASK;

    const uint32 DAT1_Config_Start    = ((uint32)DataFormat << 24U) | ((uint32)ChipSelect << 16U) | WDelay | CS_HOLD_Start;
    const uint32 DAT1_Config_End      = ((uint32)DataFormat << 24U) | ((uint32)ChipSelect << 16U) | WDelay | CS_HOLD_End;

    uint16 tx_Pec = pec15_calc(MsgSize, Tx);
   while(MsgSize-- && !(Slave_SPI_REG->FLG & 0xFF)){
        Tx_Data =   (Tx == NULL)  ? SPI_DUMMY_DATA : *Tx++;
        Rx_Data = SPI_SR2Link_HalfWord(Tx_Data, DAT1_Config_Start);

        if(Rx != NULL)
            *Rx++ = Rx_Data;
    }
    uint16_t Rx_pec = SPI_SR2Link_HalfWord(tx_Pec, DAT1_Config_End);

    return (Slave_SPI_REG->FLG & 0xFF);
}
uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize,  bool CS_Config){
    return SPI_SendAndRecevie_Links(NULL, Rx, MsgSize, CS_Config);
}
uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize,  bool CS_Config){
    return SPI_SendAndRecevie_Links(Tx, NULL, MsgSize, CS_Config);
}
uint32 SendCmd2Slave(uint16_t cmd, bool CS_Config){
    return SPI_Send2Links(&cmd, 1, CS_Config);
}
void wait_SDO(){
    SendCmd2Slave(LTC6811_PLADC, TRUE);

    int i;
    uint16_t status;

    for (i = 0, status = 0; i < NUMBER_OF_SLAVE_BOARDS; i++, status = 0)
        while (!(status & 0x0001))
            status = SPI_Send2Links(SPI_DUMMY_DATA, 0, TRUE);

    status = SPI_Send2Links(SPI_DUMMY_DATA, 0, FALSE);
}

uint32_t send_spi_2_link(uint16_t cmd, uint16_t *tx_data, uint16_t *rx_data, uint16_t data_words){
    SendCmd2Slave(cmd, TRUE);
    uint32_t returnVal;

    if (data_words == 0 || (rx_data == NULL && tx_data == NULL))
        returnVal = 0;

    if      (rx_data == NULL)
                                returnVal = SPI_Send2Links(tx_data, data_words, FALSE);
    else if (tx_data == NULL)
                                returnVal = SPI_Recevie_Links(rx_data, data_words, FALSE);
    else
                                returnVal = SPI_SendAndRecevie_Links(tx_data, rx_data, data_words, FALSE);
    return returnVal;
}
uint32 send_spi_2_link_chain(uint16_t cmd, uint16_t *tx_data, uint16_t *rx_data, uint16_t words_per_ic){
    // --- Send command (same as before) ---
//    send_spi_2_link(SPI, cmd, NULL, NULL, 0);

    if (words_per_ic == 0) return 0;

//    SetRefOn(1);

    uint16_t total_words = words_per_ic * NUMBER_OF_SLAVE_BOARDS;

    uint32_t returnVal = send_spi_2_link(cmd, tx_data, rx_data, total_words);

//    SetRefOn(0);
    return returnVal;
}
// --------------------------------------------------------------------------------------------------
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
    SPI_LinkConfigData->CS_HOLD = FALSE;
    SPI_LinkConfigData->WDEL    = TRUE;
    SPI_LinkConfigData->DFSEL   = SPI_FMT_0;
    SPI_LinkConfigData->CSNR    = 0xFE;

    //------------------------------------------------
        uint8_t CFGA[CFGA_BYTES] = {0};
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
    for(i=0; i<NUMBER_OF_CELL_CMD_GROUPS; i++){
        uint16_t current_cmd = cmds[i];

        send_spi_2_link_chain(current_cmd,
                              NULL,   // optional (can be NULL)
                              &dataOutSub[0],   // optional (can be NULL)
                              NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD);



        for(j=0; j<NUMBER_OF_SLAVE_BOARDS; j++){
            for(k=0; k<NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD; k++){
                dataOut[NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD*i + CELLS_PER_SLAVE_BOARD*j + k]=dataOutSub[NUMBER_OF_CELLS_PER_CMD_PER_SLAVE_BOARD*j+k];
            }
        }
//        delay_us(300);
    }
}

void ReadAllSlaves_Volt(uint16_t* dataOut){
    uint16_t Volt_cmds[NUMBER_OF_CELL_CMD_GROUPS] = {LTC6811_RDCVA, LTC6811_RDCVB, LTC6811_RDCVC, LTC6811_RDCVD};

//    SetRefOn(1);
    SendCmd2Slave(LTC6811_ADCV, FALSE);
    delay_ms_us(5, 0);
    CommandAllFullSlave2Read(&Volt_cmds[0], dataOut);
//    SetRefOn(0);
}



//void Read1Slave_Temp(uint16_t* dataOut){
//    const uint16_t  Temp_cmds[NUMBER_OF_CELL_CMD_GROUPS] = {LTC6811_RDCVA, LTC6811_RDCVB, LTC6811_RDCVC, LTC6811_RDCVD};
//
//    CommandFullSlave2Read(&Volt_cmds[0], dataOut)
//}
