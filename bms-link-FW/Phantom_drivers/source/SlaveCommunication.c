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

CS_Level Current_CS_Level = HIGH;

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
uint16_t swap_word_bytes(uint16_t input){
    uint16_t FirstByte = (input >> 8) & 0xFF;
    uint16_t LastByte  = (input << 8);
    uint16_t output = FirstByte | LastByte;
    return output;
}
 void swap_word_bytes_arr(uint16_t *input, uint16_t *output, uint16_t len){
    uint16_t i;
    for (i = 0; i < len; i++)
        output[i] = swap_word_bytes(input[i]);
 }

//----------------------------------------------------------------------------------------
void delay_ms_us(uint32_t ms, uint32_t us){
    volatile uint32_t i, j;
    // Approximate loops per ms, tune by measurement.
    const uint32_t loops_per_us = 10;
    const uint32_t loops_per_ms = loops_per_us * 1000;

    for (i = 0; i < ms; i++) {
        for (j = 0; j < loops_per_ms; j++);
    }
    for (i = 0; i < us; i++) {
        for (j = 0; j < loops_per_us; j++);
    }
}


float ADC2Volt(uint16_t ADC_Word){
    uint16_t ADC_Mask = ADC_Word & ADC_RESOLUTION_BIT_MASK;
    float Volt = ADC_Mask * ADC_RESOLUTION_VOLTS;
    return Volt;
}

void ADC2Volt_arr(uint16_t* ADC_Words, float* Volts, uint16_t len){
    int i;
    for(i=0; i<len; i++)
        Volts[i] = ADC2Volt(ADC_Words[i]);
}



//----------------------------------------------------------------------------------------


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
     uint8_t address;


     uint16_t remainder = PEC_INIT_VALUE;//PEC seed, 16

     int i, j;
     uint8_t jShift;
     for (i = 0; i < len; i++){
         for(j=1; j>=0; j--){
             jShift = 8*j;
             uint8_t byte = (uint8_t) (data[i] >> jShift);
             address = ((remainder >> 7) ^ byte) & 0xff;//calculate PEC table address
             remainder = (remainder << 8 ) ^ pec15Table[address];
         }
     }
     return remainder<<1;//The CRC15 has a 0 in the LSB so the final value must be multiplied by 2
 }
//---------------------------------------------------------------------------------------------------------
 void setCS(CS_Level level){
     if(level == LOW)
         Slave_SPI_REG->PC3 &= ~(1U << CS_PIN_ID);
     else if(level == HIGH)
         Slave_SPI_REG->PC3 |=  (1U << CS_PIN_ID);
     Current_CS_Level = level;
 }
 CS_Level GetCS(){
     return Current_CS_Level;
 }

 uint8_t SPI_SR2Link_BYTE(uint8_t Tx){
     Slave_SPI_REG->DAT1 =   SPI_CONFIG0_WORD | (uint32)(Tx);
     while((Slave_SPI_REG->FLG & 0x00000100U) != 0x00000100U){} /* Wait */
     uint8_t Rx = Slave_SPI_REG->BUF;
//      Rx = Slave_SPI_REG->EMU;
     return Rx;
 }
 uint16_t SPI_SR2Link_WORD(uint16_t Tx){
     const uint8_t HalfBits = 8;
     uint8_t Tx0, Tx1, Rx0, Rx1;

     Tx0 = Tx>>HalfBits;
     Tx1 = Tx;

     Rx0 = SPI_SR2Link_BYTE(Tx0);
     Rx1 = SPI_SR2Link_BYTE(Tx1);

     uint16_t Rx = (uint16_t)Rx0<<HalfBits | (uint16_t)Rx1;

     return Rx;
 }
 uint32_t SPI_SR2Link_DWORD(uint32_t Tx){
     const uint8_t HalfBits = 16;
     Tx=Tx;
     uint16_t Tx0, Tx1, Rx0, Rx1;

     Tx0 = Tx>>HalfBits;
     Tx1 = Tx;

     Rx0 = SPI_SR2Link_WORD(Tx0);
     Rx1 = SPI_SR2Link_WORD(Tx1);

     uint32_t Rx = (uint32_t)Rx0<<HalfBits | (uint32_t)Rx1;

     return Rx;
 }
 uint64_t SPI_SR2Link_QWORD(uint64_t Tx){
     const uint8_t HalfBits = 32;
     uint32_t Tx0, Tx1, Rx0, Rx1;

     Tx0 = Tx>>HalfBits;
     Tx1 = Tx;

     Rx0 = SPI_SR2Link_DWORD(Tx0);
     Rx1 = SPI_SR2Link_DWORD(Tx1);

     uint64_t Rx = (uint64_t)Rx0<<HalfBits | (uint64_t)Rx1;

     return Rx;
 }

 void SPI_Clock_BYTES(uint8_t Bytes2Clock){
     int i;
     for(i=0;i<Bytes2Clock; i++)
         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
 }
 //---------------------------------------------------------------------------------------------------------

 uint32 SPI_SendAndRecevie_Links(uint16_t* Tx, uint16_t* Rx, uint32 MsgSize){
     uint16_t Tx_Data = SPI_DUMMY_DATA_WORD;
     uint16_t Rx_Data;

     uint16 tx_Pec = pec15_calc(MsgSize, Tx);
    while(MsgSize-- && !(Slave_SPI_REG->FLG & 0xFF)){
        if(Tx)
            Tx_Data = *Tx++;

         Rx_Data = SPI_SR2Link_WORD(Tx_Data);

         if(Rx)
             *Rx++ = Rx_Data;
     }
     uint16_t Rx_pec = SPI_SR2Link_WORD(tx_Pec);
//     return (Slave_SPI_REG->FLG & 0xFF);
     return Rx_pec;
 }
 uint32 SPI_Recevie_Links(uint16_t* Rx, uint32 MsgSize){
     return SPI_SendAndRecevie_Links(NULL, Rx, MsgSize);
 }
 uint32 SPI_Send2Links(uint16_t* Tx, uint32 MsgSize){
     return SPI_SendAndRecevie_Links(Tx, NULL, MsgSize);
 }
 uint32 SendCmd2Slave(uint16_t cmd){
     uint16_t cmd_pec = pec15_calc(1, &cmd);
     uint32_t FullCmdTx = (uint32_t)cmd<<16 | (uint32_t)cmd_pec;

//     SPI_SR2Link_WORD(cmd);
     uint32_t FullCmdRx = SPI_SR2Link_DWORD(FullCmdTx);
     return FullCmdRx;
//     return 0;
 }
 uint32 SendCMD2Slave_alone(uint16_t cmd){
     setCS(LOW);
     uint32 RX = SendCmd2Slave(cmd);
     setCS(HIGH);
     return RX;
 }

 uint16_t ReadWriteReg(uint16_t cmd, uint16_t* data_Tx, uint16_t* data_Rx){
     uint16_t Pec_Equal = 0x0000;
//     wakeup_idle();
     setCS(LOW);
     uint32_t cmdRx = SendCmd2Slave(cmd);

//     swap_word_bytes_arr(data_Tx, data_Tx, WORD_REG_GROUP);
//     delay_ms_us(0,10);

     int i;
     uint16_t Rx_Pec_Mesg, Rx_Pec_Calc;
     for(i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
        Rx_Pec_Mesg = SPI_SendAndRecevie_Links(data_Tx, data_Rx, WORD_REG_GROUP);
        Rx_Pec_Calc = pec15_calc(WORD_REG_GROUP,  data_Rx);

        if(data_Rx)
            data_Rx += WORD_REG_GROUP;

        if(Rx_Pec_Mesg == Rx_Pec_Calc)
            Pec_Equal |= 1<<i;
     }

     setCS(HIGH);

//
     return Pec_Equal;
 }
 uint32 WriteReg(uint16_t cmd, uint16_t *data){
//     swap_word_bytes_arr(data, data, WORD_REG_GROUP);
     return ReadWriteReg(cmd, data, NULL);
 }
 uint32 ReadReg(uint16_t cmd, uint16_t *data){
     uint16_t data_Pec = ReadWriteReg(cmd, NULL, data);
     swap_word_bytes_arr(data, data, WORD_REG_GROUP);
     return data_Pec;
  }

 //------------------------------------------------
 void wakeup_idle(){ //Number of ICs in the system
     int i;
     for (i=0; i<NUMBER_OF_SLAVE_BOARDS; i++)
         setCS(LOW);
         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
         setCS(HIGH);
 }

 void wakeup_sleep(void) {
     setCS(LOW);



     // Now send a dummy byte (this will also toggle CS automatically)
     int i;
     for (i = 0; i < NUMBER_OF_SLAVE_BOARDS; i++) {
         setCS(LOW);
         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
         delay_ms_us(0, tWAKE_us);
         setCS(HIGH);
         delay_ms_us(0, 10);
     }
 }
// void wakeup_sleep(){
//     int i;
//     for(i=0; i<NUMBER_OF_SLAVE_BOARDS; i++){
//         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
//         delay_ms_us(0, tWAKE_us);
//         SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);
//         delay_ms_us(0, 10);
//     }
// }

 void initLink(){
     init_PEC15_Table();
     //------------------------------------------------
     uint8_t CFGA_Bytes[BYTES_REG_GROUP] = {0};
     uint8_t CFGB_Bytes[BYTES_REG_GROUP] = {0};
     CFGB_Bytes[1] = 0b00011111;
     CFGA_Bytes[0] |= 1<<2;
     CFGA_Bytes[0] |= REFON_MASK;

     uint16_t CFGA_Words[WORD_REG_GROUP];
     uint16_t CFGB_Words[WORD_REG_GROUP];

     bytes_to_words(CFGA_Bytes, CFGA_Words, WORD_REG_GROUP);
     bytes_to_words(CFGB_Bytes, CFGB_Words, WORD_REG_GROUP);

     wakeup_sleep();

     WriteReg(LTC6811_WRCFGA, CFGA_Words);
     WriteReg(LTC6811_WRCFGB, CFGB_Words);
 }

 // -----------------------------------------------------------


 bool checkSDO(){
     uint16_t status;

     setCS(LOW);
     SendCmd2Slave(LTC6811_PLADC);

     status = SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);

     setCS(HIGH);

     return status;
 }

 bool waitSDO(){
     uint16_t status;

     setCS(LOW);
     SendCmd2Slave(LTC6811_PLADC);
     int i;
     for (i = 0, status = 0; i < NUMBER_OF_SLAVE_BOARDS; i++, status = 0)
         while (!(status & 0x0001))
             status = SPI_SR2Link_BYTE(SPI_DUMMY_DATA_BYTE);;

     setCS(HIGH);
     return status;
 }


 bool GetVoltageReadings(uint16_t *data){
     if(!checkSDO())
         return FALSE;

     const uint16_t cmds[NUMBER_OF_CELL_CMD_GROUPS] = {LTC6811_RDCVA, LTC6811_RDCVB, LTC6811_RDCVC, LTC6811_RDCVD};

     uint16_t VoltSub[NUMBER_OF_REG_WORDS_PER_CMD];

     int i, j;
     for(i=0;i<NUMBER_OF_CELL_CMD_GROUPS;i++){
         ReadReg(cmds[i], VoltSub);

         for(j = 0;j < NUMBER_OF_SLAVE_BOARDS;j++){
             uint8_t idx0 = WORD_REG_GROUP * i + CELLS_PER_SLAVE_BOARD * j;
             uint8_t idx1 = WORD_REG_GROUP * j;

             memcpy(&data[idx0], &VoltSub[idx1], WORD_REG_GROUP);
         }
     }
     return TRUE;
 }

 bool GetGPIOReadings(uint16_t *data){
     if(!checkSDO())
         return FALSE;
     uint16_t cmds[NUMBER_OF_GPIO_CMD_GROUPS] = {LTC6811_RDAUXA,LTC6811_RDAUXB};

     uint16_t GPIO_A[NUMBER_OF_REG_WORDS_PER_CMD];
     uint16_t GPIO_B[NUMBER_OF_REG_WORDS_PER_CMD];

     ReadReg(cmds[0],GPIO_A);
     ReadReg(cmds[1],GPIO_B);

     int i;
     for (i=0;i<NUMBER_OF_SLAVE_BOARDS;i++){
         uint8_t idx0 = i * GPIOS_PER_SLAVE_BOARD;
         uint8_t idx1 = i * WORD_REG_GROUP;
         memcpy(&data[idx0], &GPIO_A[idx1], WORD_REG_GROUP);
         memcpy(&data[idx0], &GPIO_B[idx1], GPIOS_PER_SLAVE_BOARD - WORD_REG_GROUP);
     }

     return TRUE;
 }

uint32 MeasureALL(uint8_t MD, //ADC Mode
                  uint8_t DCP //Discharge Permit
                  ){

    uint16 DCP_bits = (DCP&0x01)<<4;
    uint16_t MD_bits = (MD & 0x03)<<7;
    uint16_t cmd = LTC6811_ADCVAX | MD_bits | DCP_bits;
    setCS(LOW);
    uint32 RX = SendCmd2Slave(cmd);
    SPI_Clock_BYTES(PADC_GARBAGE_CLOCK_CYCLES_BYTES);
    setCS(HIGH);
    return RX;
}



//void DischargeCells (uint16_t DCC)


uint32 Write_CFGR_General( bool refon, // The REFON bit
                         bool adcopt, // The ADCOPT bit
                         uint8_t gpio, // The GPIO bits
                         uint16_t DCC, // The DCC bits
                         uint8_t dcto, // The Dcto bits
                         uint16_t VUV, // The UV value
                         uint16_t  VOV // The OV value
                         ){

    uint16_t Config_Words[WORD_REG_GROUP] = {0};

    Config_Words[0] = ((VUV & 0x0FFF)<<8) | (((uint16_t)gpio & 0x1F) <<3) | ((uint16_t)refon <<2) | (uint16_t)adcopt;
    Config_Words[1] = (VOV & 0x0FFF) | ((VUV & 0x0FFF)>>8);
    Config_Words[2] = (((uint16_t) dcto & 0xF)<<12) | (DCC & 0x0FFF);

    swap_word_bytes_arr(Config_Words, Config_Words, WORD_REG_GROUP);

    return WriteReg(LTC6811_WRCFGA, Config_Words);
}

 void ClearSlaveRegs(){
    #define NUMBER_OF_CLEAR_CMDS 3
     uint16_t All_Cear_CMDs[NUMBER_OF_CLEAR_CMDS] = {LTC6811_CLRCELL, LTC6811_CLRAUX, LTC6811_CLRSTAT};

     int i;
     for (i=0;i<NUMBER_OF_CLEAR_CMDS;i++)
         SendCMD2Slave_alone(All_Cear_CMDs[i]);
 }

 uint32 Write_CFGR(uint16_t DCC){     // The DCC bits
     const bool refon   = TRUE;     // The REFON bit
     const bool adcopt  = TRUE;     // The ADCOPT bit
     const uint8_t gpio = 0b11111;  // The GPIO bits
     const uint8_t dcto = 0b1111;   // The Dcto bits
     const uint16_t VUV = 0x000;    // The UV value
     const uint16_t VOV = 0xFFF;    // The OV value


     return Write_CFGR_General(refon, adcopt, gpio, DCC, dcto, VUV, VOV);
 }

