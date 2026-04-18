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

//----------------------------------------------------------------------------------------
// Convert uint16_t array (words) to uint8_t array (bytes)
// Output array must be at least 2 * len in size
void words_to_bytes(uint16_t *words, uint8_t *bytes, uint16_t len)
{
    uint16_t i;

    for (i = 0; i < len; i++)
    {
        bytes[2 * i]     = (uint8_t)(words[i] & 0xFF);        // Low byte
        bytes[2 * i + 1] = (uint8_t)((words[i] >> 8) & 0xFF); // High byte
    }
}

// Convert uint8_t array (bytes) to uint16_t array (words)
// Input array must be 2 * len in size
void bytes_to_words(uint8_t *bytes, uint16_t *words, uint16_t len){
    uint16_t i;
    for (i = 0; i < len; i++){
        words[i] = (uint16_t)bytes[2*i] |
                   ((uint16_t)bytes[2*i+1] << 8);
    }
}
 void swap_word_bytes_arr(uint16_t *input, uint16_t *output, uint16_t len){
    uint16_t i;
    for (i = 0; i < len; i++)
        output[i] = (uint16_t)((input[i] >> 8) | (input[i] << 8));
 }
 uint16_t swap_word_bytes(uint16_t input){
     uint16_t output = (input >> 8) | (input << 8);
     return output;
 }
//----------------------------------------------------------------------------------------
void delay_ms_us(uint32_t ms, uint32_t us){
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
    int i, bit;
    uint16_t remainder;
    for (i = 0; i < 256; i++){
        remainder = i << 7;
        for (bit = 8; bit > 0; --bit){
            if (remainder & 0x4000)
                remainder = (remainder << 1) ^ PEC_CHARACTERISTIC_POLYNOMIAL;
            else
                remainder = (remainder << 1);

        }
        pec15Table[i] = remainder & 0xFFFF;
    }
}
 uint16_t pec15_calc(uint8_t len, uint16_t *data){
     if (data == NULL)
         return SPI_DUMMY_DATA_WORD;
     uint16_t remainder;
     uint8_t address;

     remainder = PEC_INIT_VALUE;//PEC seed, 16

     int i, j;
     uint8_t jShift;
     for (i = 0; i < len; i++){
         for(j=1; j>=0; j--){
             jShift = 8*j;
             uint16_t word = data[i];
             uint8_t byte = (uint8_t) (word >> jShift);
             address = ((remainder >> 7) ^ byte) & 0xff;//calculate PEC table address
             remainder = (remainder << 8 ) ^ pec15Table[address];
         }
     }
     return remainder<<1;//The CRC15 has a 0 in the LSB so the final value must be multiplied by 2
 }
//---------------------------------------------------------------------------------------------------------
 uint8_t SPI_SR2Link_Byte(uint8_t Tx, bool CS_LOW_END){
     uint32 CS_HOLD = CS_LOW_END? CS_HOLD_MASK:0U;
     Slave_SPI_REG->DAT1 =   CS_HOLD | SPI_CONFIG1_WORD | (uint32)Tx;
     while((Slave_SPI_REG->FLG & 0x00000100U) != 0x00000100U){} /* Wait */
     uint8_t Rx = Slave_SPI_REG->BUF;
     return Rx;
 }
 uint16_t SPI_SR2Link_Word(uint16_t Tx, bool CS_LOW_END){
     uint8_t Tx0, Tx1, Rx0, Rx1;

     Tx0 = Tx>>8;
     Tx1 = Tx;

     Rx0 = SPI_SR2Link_Byte(Tx0, TRUE);
     Rx1 = SPI_SR2Link_Byte(Tx1, CS_LOW_END);

     uint16_t Rx = (uint16_t)Rx0<<8 | (uint16_t)Rx1;

     return Rx;
 }
 uint32_t SPI_SR2Link_Int(uint32_t Tx, bool CS_LOW_END){
     uint16_t Tx0, Tx1, Rx0, Rx1;

     Tx0 = (uint16_t)Tx>>16;
     Tx1 = (uint16_t)Tx;

     Rx0 = SPI_SR2Link_Word(Tx0, TRUE);
     Rx1 = SPI_SR2Link_Word(Tx1, CS_LOW_END);

     uint32_t Rx = (uint32_t)Rx0<<8 | (uint32_t)Rx1;

     return Rx;
 }
 uint64_t SPI_SR2Link_Long(uint64_t Tx, bool CS_LOW_END){
     uint32_t Tx0, Tx1, Rx0, Rx1;

     Tx0 = (uint32_t)Tx>>16;
     Tx1 = (uint32_t)Tx;

     Rx0 = SPI_SR2Link_Int(Tx0, TRUE);
     Rx1 = SPI_SR2Link_Int(Tx1, CS_LOW_END);

     uint64_t Rx = (uint64_t)Rx0<<8 | (uint64_t)Rx1;

     return Rx;
 }
 //---------------------------------------------------------------------------------------------------------
 void wakeup_isoSPI(){
     bool current_CS_Hold = !(Slave_SPI_REG->DAT1 & CS_HOLD_MASK);

//     if (!current_CS_Hold)
//         return;
     int i;
     for(i = 0; i<NUMBER_OF_SLAVE_BOARDS; i++)
         SPI_SR2Link_Byte(SPI_DUMMY_DATA_BYTE, FALSE);
 }


 uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize, bool CS_Low_End){
//     wakeup_isoSPI();
     uint16 Tx_Data, Rx_Data;

     uint16 tx_Pec = pec15_calc(MsgSize, Tx);
//     MsgSize = MsgSize > 1 ? MsgSize+4:MsgSize;
    while(MsgSize-- && !(Slave_SPI_REG->FLG & 0xFF)){

         Tx_Data =   (Tx == NULL)  ? SPI_DUMMY_DATA_WORD : *Tx++;

         Rx_Data = SPI_SR2Link_Word(Tx_Data, TRUE);
         if(Rx != NULL)
             *Rx++ = Rx_Data;
     }
     uint16_t Rx_pec = SPI_SR2Link_Word(tx_Pec, CS_Low_End);

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
//     uint16_t cmd_swap = swap_word_bytes(cmd);
     return SPI_Send2Links(&cmd, 1, CS_Config);
 }

// void Send_Shift_Bytes(){
//     int i;
//     for (i=0; i<NUMBER_OF_SHIFT_BYTES; i++){
//         uint8_t SHIFT_BYTE_Rx = SPI_SR2Link_Byte(SPI_DUMMY_DATA, TRUE);
//     }
//     uint8_t SHIFT_BYTE_Rx = SPI_SR2Link_Byte(SPI_DUMMY_DATA, FALSE);
// }

 uint32 ReadWriteReg(uint16_t cmd, uint16_t* data_Tx, uint16_t* data_Rx){
     bool CS_HOLD = TRUE;
     SendCmd2Slave(cmd, CS_HOLD);

     int i, index;

     for(i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
         index = WORD_REG_GROUP * i;
         CS_HOLD = i < (NUMBER_OF_SLAVE_BOARDS -1);

        SPI_SendAndRecevie_Links(&data_Tx[index], &data_Rx[index], WORD_REG_GROUP, CS_HOLD);
     }
     return 0;
 }
// uint32 WriteReg(uint16_t cmd, uint16_t *data) {
//     wakeup_sleep();
//     uint16_t cmdPEC = pec15_calc(1, &cmd);
//     SPI_SR2Link_Word(cmd, TRUE);
//     SPI_SR2Link_Word(cmdPEC, TRUE);
//
//     uint16_t i;
//     for (i = 0; i < WORD_REG_GROUP; i++) {
//         SPI_SR2Link_Word(data[i], TRUE);
//     }
//     uint16_t dataPEC = pec15_calc(WORD_REG_GROUP, data);
//     SPI_SR2Link_Word(dataPEC, FALSE);
//     return 0;
// }
//
// uint32 ReadReg(uint16_t cmd, uint16_t *data) {
//     wakeup_sleep();
//     uint16_t cmdPEC = pec15_calc(1, &cmd);
//     SPI_SR2Link_Word(cmd, TRUE);
//     SPI_SR2Link_Word(cmdPEC, TRUE);
//     uint16_t i;
//     for (i = 0; i < WORD_REG_GROUP; i++) {
//         data[i] = SPI_SR2Link_Word(SPI_DUMMY_DATA_WORD, TRUE);
//     }
//     uint16_t rxPEC = SPI_SR2Link_Word(SPI_DUMMY_DATA_WORD, FALSE);
//     // (Optional) verify rxPEC against calculated PEC over data[]
//     return 0;
// }

  uint32 WriteReg(uint16_t cmd, uint16_t* data){
//      wakeup_idle();
      SendCmd2Slave(cmd, TRUE);
      SPI_Send2Links(data, WORD_REG_GROUP, FALSE);

      return 0;
  }
 uint32 ReadReg(uint16_t cmd, uint16_t* data){
//     wakeup_idle();
     SendCmd2Slave(cmd, TRUE);

//     uint16_t Rx_pec[NUMBER_OF_SLAVE_BOARDS];
     SPI_Recevie_Links(data, WORD_REG_GROUP, FALSE);

     return 0;
 }

 //------------------------------------------------
 void wakeup_idle(){ //Number of ICs in the system
     int i;
     for (i=0; i<NUMBER_OF_SLAVE_BOARDS; i++)
         SPI_SR2Link_Byte(SPI_DUMMY_DATA_BYTE, FALSE);
 }
 void wakeup_sleep(){
     int i;
     for(i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
         SPI_SR2Link_Word(SPI_DUMMY_CMD, TRUE);
         delay_ms_us(0, tWAKE_us);
         SPI_SR2Link_Word(SPI_DUMMY_CMD, FALSE);
         delay_ms_us(0, 10);
     }
 }

 void initLink(){
     init_PEC15_Table();
     //------------------------------------------------
     uint8_t CFGA_Bytes[BYTES_REG_GROUP] = {0};
     uint8_t CFGB_Bytes[BYTES_REG_GROUP] = {0};
     CFGB_Bytes[1] = 0b00011111;
     CFGA_Bytes[0] |= 1<<2;

     uint16_t CFGA_Words[WORD_REG_GROUP];
     uint16_t CFGB_Words[WORD_REG_GROUP];

     bytes_to_words(CFGA_Bytes, CFGA_Words, WORD_REG_GROUP);
     bytes_to_words(CFGB_Bytes, CFGB_Words, WORD_REG_GROUP);

     wakeup_sleep();

     WriteReg(LTC6811_WRCFGA, CFGA_Words);
     WriteReg(LTC6811_WRCFGB, CFGB_Words);
 }
