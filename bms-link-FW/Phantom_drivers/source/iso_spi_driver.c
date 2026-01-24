#include <stdint.h>
#include <stdbool.h>

#include "iso_spi_driver.h"
#include "spi.h"  // SPI driver functions
#include "sys_common.h"

#define PEC_INIT_VALUE 0x0010

// 0100 0101 1001 1001 for the polynomicla x^15 + x^14 + x^10 + x^8 + x^7 + x^4 + x^3 + 1 
#define PEC_CHARACTERISTIC_POLYNOMIAL 0x4599

#define CMD_BYTE_LENGTH 2
#define PEC_BYTE_LENGTH 2
#define SPI_FRAME

// SPI configuration for ISO SPI communication
// might need to change this based on hardware schematic idk 
#define ISO_SPI_MODULE spiREG1

// SPI data format configuration
static spiDAT1_t iso_spi_config = {
    .CS_HOLD = FALSE,  // release CS after transaction
    .WDEL    = TRUE,   // word delay enabled
    .DFSEL   = SPI_FMT_0,  // use format 0
    .CSNR    = 0xFE    // cs0 (0xFE = CS0)
};

void wakeup_spi(void)
{
    // TODO: Implement wake-up sequence for LTC6820
    // For now, this is a placeholder - implement based on LTC6820 datasheet
}

uint16_t calculate_pec(uint8_t *data, uint8_t len){
    uint16_t pec = PEC_INIT_VALUE;
    int i;
    for (i = 0; i < len; i++)
    {
        uint8_t byte = data[i];

        int bit;
        for (bit = 7; bit >= 0; bit--)
        {
            uint8_t din = (byte >> bit) & 0x01;
            uint8_t feedback = din ^ ((pec >> 14) & 0x01);

            pec <<= 1;
            if (feedback)
            {
                pec ^= PEC_CHARACTERISTIC_POLYNOMIAL;
            }
        }
    }

    return (pec & 0x7FFF);
}



/*
For read 
and write commands, a single command is sent, and then 
the stacked devices effectively turn into a cascaded shift 
register, in which data is shifted through each device to 
the next higher (on a write) or the next lower (on a read) 
device in the stack. See the Serial Interface section.

pg 55
*/

// Data array should already be in little endian form 
// Such that data[0] is the lsb byte
bool write_reg(uint16_t cmd, uint8_t* data, uint8_t data_len){

//   wakeup_spi();
// deprecated clock_spi() function idk how it works lol
  // Prepare command bytes (LSB first)
  const uint8_t cmd0 = cmd & 0xFF;
  const uint8_t cmd1 = (cmd >> 8) & 0xFF; 
  
  const uint8_t cmd_array[CMD_BYTE_LENGTH] = {cmd0, cmd1};

  // calculate command pec
  const uint16_t cmd_pec = calculate_pec(cmd_array, CMD_BYTE_LENGTH);

  // calculate data pec
  const uint16_t data_pec = calculate_pec(data, data_len);


  const uint8_t cmd_pec_lsb = (cmd_pec & 0xFF);
  const uint8_t cmd_pec_msb = ((cmd_pec >> 8)  & 0xFF);

  const uint8_t data_pec_lsb = (data_pec & 0xFF);
  const uint8_t data_pec_msb = (data_pec >> 8) & 0xFF;
  // + 1 so account for odd cases
  const uint8_t spi_frame_byte_size = CMD_BYTE_LENGTH + PEC_BYTE_LENGTH + data_len + PEC_BYTE_LENGTH;
  const uint8_t spi_frame_size = (spi_frame_byte_size) / 2;
  uint16_t spi_frame [spi_frame_size]; 
  bool frame_is_odd = (spi_frame_byte_size % 2) ? true : false;
  // prep data + data pec buffer (data_len + 2 bytes)
  const uint8_t data_buf_len = data_len + PEC_BYTE_LENGTH;
  uint8_t data_buf[data_buf_len];
  // copy user data
  uint8_t i;
  for (i = 0; i < data_len; i++) {
    data_buf[i] = data[i]; // last byte if odd number
  }
  // append data pec (lsb first)
  data_buf[data_len] = data_pec_lsb;
  data_buf[data_len + 1] = data_pec_msb;


    spi_frame[0] = (cmd0 << 8) | cmd1;
    spi_frame[1] = (cmd_pec_lsb << 8) | cmd_pec_msb;
    // pack bytes into words
    for (i = 0; i < data_buf_len; i += 2) {
        if (i + 1 < data_buf_len) 
        {
            spi_frame[2 + (i / 2)] = (data_buf[i] << 8) | data_buf[i + 1]; // last byte if odd number
        } 
        // else 
        // {
        //     spi_frame[2 + (i / 2)] = data_buf[i];  // last byte if odd number
        // }
    }
    iso_spi_config.CS_HOLD =  frame_is_odd ? TRUE : FALSE; // Set this to true so we can send 2 seperate frames

    spiTransmitData(ISO_SPI_MODULE, &iso_spi_config, spi_frame_size, spi_frame);
    
    iso_spi_config.CS_HOLD = FALSE;
    if (frame_is_odd)
    {
        uint16_t last_byte = ((uint16_t)data_buf[data_buf_len - 1]) << 8;
        spiTransmitData(ISO_SPI_MODULE, &iso_spi_config, 1, &last_byte);
    }


//   // prep command + command pec buffer (4 bytes: cmd0, cmd1, pec0, pec1)
//   uint8_t cmd_buf[4];
//   cmd_buf[0] = cmd0;
//   cmd_buf[1] = cmd1;
//   cmd_buf[2] = cmd_pec & 0xFF;      // pec lsb first
//   cmd_buf[3] = (cmd_pec >> 8) & 0xFF;

//   // prep data + data pec buffer (data_len + 2 bytes)
//   uint8_t data_buf[data_len + 2];
//   // copy user data
//   for (uint8_t i = 0; i < data_len; i++) {
//     data_buf[i] = data[i]; // last byte if odd number
//   }
//   // append data pec (lsb first)
//   data_buf[data_len] = data_pec & 0xFF;
//   data_buf[data_len + 1] = (data_pec >> 8) & 0xFF;

//   // send command + command PEC (4 bytes = 2 words)
//   uint16_t cmd_words[2];
//   cmd_words[0] = cmd_buf[0] | (cmd_buf[1] << 8);  // little-endian 0+
//   cmd_words[1] = cmd_buf[2] | (cmd_buf[3] << 8);
//   spiTransmitData(ISO_SPI_MODULE, &iso_spi_config, 2, cmd_words);

//   // send data + data pec ((data_len + 2) bytes = (data_len + 2 + 1)/2 word)
//   uint16_t data_word_count = (data_len + 2 + 1) / 2;  // round up
//   uint16_t data_words[data_word_count];
//   // pack bytes into words
//   for (uint8_t i = 0; i < data_len + 2; i += 2) {
//     if (i + 1 < data_len + 2) {
//       data_words[i / 2] = data_buf[i] | (data_buf[i + 1] << 8); // last byte if odd number
//     } else {
//       data_words[i / 2] = data_buf[i];  // last byte if odd number
//     }
//   }
//   spiTransmitData(ISO_SPI_MODULE, &iso_spi_config, data_word_count, data_words);

  return true;
}

// alot of implementation details taken from https://www.youtube.com/watch?v=ZHYFnXI9K70 brp is pretty good
/*
 * cursor summary for read_reg (to see if my logic makes sense):
 * read_reg - Read data from LTC6811 via LTC6820 ISO SPI converter
 * 
 * For read operations with LTC6811:
 * 1. Send command (2 bytes) + command PEC (2 bytes) - total 4 bytes
 * 2. Receive data (data_len bytes) + data PEC (2 bytes) - total (data_len + 2) bytes
 * 3. Validate received data PEC
 * 
 * Protocol:
 * - Command is sent LSB first (cmd0, cmd1)
 * - Command PEC is sent LSB first (cmd_pec0, cmd_pec1)
 * - Data is received as-is
 * - Data PEC is received LSB first (data_pec0, data_pec1)

 */
bool read_reg(uint16_t cmd, uint8_t* data, uint8_t data_len){

//   wakeup_spi();
// Prepare command bytes (LSB first)
const uint8_t cmd0 = cmd & 0xFF;
const uint8_t cmd1 = (cmd >> 8) & 0xFF;

const uint8_t cmd_array[CMD_BYTE_LENGTH] = {cmd0, cmd1};

// Calculate command PEC
const uint16_t cmd_pec = calculate_pec(cmd_array, CMD_BYTE_LENGTH);

const uint8_t cmd_pec_lsb = cmd_pec & 0xFF;
const uint8_t cmd_pec_msb = (cmd_pec >> 8) & 0xFF;

// Pack command + PEC into 16-bit words
uint16_t cmd_words[2];
cmd_words[0] = (cmd0 << 8) | cmd1;
cmd_words[1] = (cmd_pec_lsb << 8) | cmd_pec_msb;

// Total bytes expected from LTC6811: data + data PEC
const uint8_t rx_byte_len = data_len + PEC_BYTE_LENGTH;
const bool rx_is_odd = (rx_byte_len % 2) != 0;

// Number of full 16-bit words to receive
const uint16_t rx_word_count = rx_byte_len / 2;

// Buffers
uint16_t rx_words[rx_word_count];
uint16_t dummy_tx[rx_word_count];
uint8_t rx_buf[rx_byte_len];

// Clear dummy TX
uint16_t i;
for (i = 0; i < rx_word_count; i++) {
    dummy_tx[i] = 0x0000;
}

// ---- SPI TRANSACTION START ----

// Hold the CSB low until end of read
iso_spi_config.CS_HOLD = true;
// Send command + command PEC

uint16_t cmd_rx[2];
spiTransmitAndReceiveData(
    ISO_SPI_MODULE,
    &iso_spi_config,
    2,
    cmd_words,
    cmd_rx
);

// Receive full words, hold CS if an odd byte remains
// iso_spi_config.CS_HOLD = rx_is_odd ? TRUE : FALSE;

if (rx_word_count > 0) {
    spiTransmitAndReceiveData(
        ISO_SPI_MODULE,
        &iso_spi_config,
        rx_word_count,
        dummy_tx,
        rx_words
    );
}

// Receive final odd byte if needed
uint8_t last_byte = 0;
if (rx_is_odd) {
    uint16_t last_word = 0x0000;

    iso_spi_config.CS_HOLD = FALSE;

    spiTransmitAndReceiveData(
        ISO_SPI_MODULE,
        &iso_spi_config,
        1,
        &last_word,
        &last_word
    );

    // Only the first byte is valid
    last_byte = (uint8_t)((last_word >> 8) & 0xFF);
}

// ---- SPI TRANSACTION END ----

// Unpack received full words into byte buffer
for (i = 0; i < rx_word_count; i++) {
    rx_buf[(i * 2)]     = (rx_words[i] >> 8) & 0xFF;
    rx_buf[(i * 2) + 1] = rx_words[i] & 0xFF;
}

// Append last byte if odd
if (rx_is_odd) {
    rx_buf[rx_byte_len - 1] = last_byte;
}

// Copy received data to user buffer (exclude PEC)
for (i = 0; i < data_len; i++) {
    data[i] = rx_buf[i];
}

// Extract received PEC
const uint16_t received_pec =
    rx_buf[data_len] | (rx_buf[data_len + 1] << 8);

// Validate PEC
const uint16_t calculated_data_pec = calculate_pec(data, data_len);
if (calculated_data_pec != received_pec) {
    return false;
}

  return true;
//   // Prepare command bytes (LSB first, same as write_reg)
//   const uint8_t cmd0 = cmd & 0xFF;
//   const uint8_t cmd1 = (cmd >> 8) & 0xFF; 
  
//   const uint8_t cmd_array[CMD_BYTE_LENGTH] = {cmd0, cmd1};

//   // calculate command pec (same as write_reg)
//   const uint16_t cmd_pec = calculate_pec(cmd_array, CMD_BYTE_LENGTH);

//   // prep command + command pec buffer (4 bytes: cmd0, cmd1, pec0, pec1) (same as write_reg)
//   uint8_t cmd_buf[4];
//   cmd_buf[0] = cmd0;
//   cmd_buf[1] = cmd1;
//   cmd_buf[2] = cmd_pec & 0xFF;      // pec lsb first
//   cmd_buf[3] = (cmd_pec >> 8) & 0xFF;

//   // pack command into 16-bit words for SPI (same as write_reg)
//   uint16_t cmd_words[2];
//   cmd_words[0] = cmd_buf[0] | (cmd_buf[1] << 8);
//   cmd_words[1] = cmd_buf[2] | (cmd_buf[3] << 8);

//   // receive buffer for data + pec (data_len + 2 bytes = (data_len + 2 + 1)/2 word)
//   uint16_t rx_word_count = (data_len + 2 + 1) / 2;  // Round up
//   uint16_t rx_words[rx_word_count];
//   uint16_t dummy_tx[rx_word_count];  // Dummy TX data (SPI is full-duplex)
//   for (uint16_t i = 0; i < rx_word_count; i++) {
//     dummy_tx[i] = 0x0000;  // Send zeros while receiving
//   }

//   // Send command + command PEC, receive data + data PEC
//   // First send command (2 words), while receiving initial data
//   uint16_t cmd_rx[2];
//   spiTransmitAndReceiveData(ISO_SPI_MODULE, &iso_spi_config, 2, cmd_words, cmd_rx);

//   // Continue receiving data + PEC by sending dummy data
//   spiTransmitAndReceiveData(ISO_SPI_MODULE, &iso_spi_config, rx_word_count, dummy_tx, rx_words);

//   // Unpack received words into bytes
//   uint8_t rx_buf[data_len + 2];
//   for (uint8_t i = 0; i < data_len + 2; i += 2) {
//     if (i + 1 < data_len + 2) {
//       rx_buf[i] = rx_words[i / 2] & 0xFF;      // LSB
//       rx_buf[i + 1] = (rx_words[i / 2] >> 8) & 0xFF;  // MSB
//     } else {
//       rx_buf[i] = rx_words[i / 2] & 0xFF;  // last byte if odd
//     }
//   }

//   // move received data to user data buffer (excluding pec)
//   for (uint8_t i = 0; i < data_len; i++) {
//     data[i] = rx_buf[i];
//   }

//   // take received pec (last 2 bytes)
//   uint8_t received_pec0 = rx_buf[data_len];
//   uint8_t received_pec1 = rx_buf[data_len + 1];
//   uint16_t received_pec = received_pec0 | (received_pec1 << 8);

//   // calculate pec for received data to validate (same as write_reg)
//   const uint16_t calculated_data_pec = calculate_pec(data, data_len);
  
//   // validate pec (same as write_reg)
//   if (calculated_data_pec != received_pec) {
//     return false; // pec mismatch - data corruption detected any other flags needed here?
//   }

//   return true;
}

/**
 * Test function to generate PWM-like pulses on S pins for SPI verification
 * 
 * This function configures the S-Control register to generate pulses on S1 pin
 * and starts the pulse sequence. The pulses can be observed on an oscilloscope
 * to verify SPI communication is working correctly.
 * 
 * S-Control Register Format (6 bytes):
 * - SCTRL0: SCTL2[3:0] | SCTL1[3:0]  (controls S2 and S1)
 * - SCTRL1: SCTL4[3:0] | SCTL3[3:0]  (controls S4 and S3)
 * - SCTRL2: SCTL6[3:0] | SCTL5[3:0]  (controls S6 and S5)
 * - SCTRL3: SCTL8[3:0] | SCTL7[3:0]  (controls S8 and S7)
 * - SCTRL4: SCTL10[3:0] | SCTL9[3:0] (controls S10 and S9)
 * - SCTRL5: SCTL12[3:0] | SCTL11[3:0] (controls S12 and S11)
 * 
 * S pin control values:
 * - 0x0: Drive low
 * - 0x1-0x7: Generate 1-7 pulses (pulse rate: 6.44kHz, 155µs period, 77.6µs pulse width)
 * - 0x8: Drive high
 * - 0x9-0xF: Reserved
 * 
 * @return true if commands were sent successfully, false otherwise
 */
bool test_gpio_pwm_spi(void)
{
    // Configure S-Control register to generate pulses on S1 pin
    // SCTRL0 byte: SCTL2[3:0] = 0x0 (S2 off) | SCTL1[3:0] = 0x3 (S1 generates 3 pulses)
    // All other S pins set to 0x0 (off)
    uint8_t sctrl_data[6] = {
        0x03,  // SCTRL0: S2=0x0, S1=0x3 (3 pulses)
        0x00,  // SCTRL1: S4=0x0, S3=0x0
        0x00,  // SCTRL2: S6=0x0, S5=0x0
        0x00,  // SCTRL3: S8=0x0, S7=0x0
        0x00,  // SCTRL4: S10=0x0, S9=0x0
        0x00   // SCTRL5: S12=0x0, S11=0x0
    };
    
    // Write S-Control register
    bool write_success = write_reg(LTC6811_WRSCTRL, sctrl_data, 6);
    if (!write_success) {
        return false;
    }
    
    // Start S-Control pulse sequence
    // STSCTRL command starts the pulsing after command PEC is received
    // The command itself doesn't require data, but we need to send command + PEC
    uint16_t stsctrl_cmd = LTC6811_STSCTRL;
    const uint8_t cmd0 = stsctrl_cmd & 0xFF;
    const uint8_t cmd1 = (stsctrl_cmd >> 8) & 0xFF;
    const uint8_t cmd_array[CMD_BYTE_LENGTH] = {cmd0, cmd1};
    const uint16_t cmd_pec = calculate_pec(cmd_array, CMD_BYTE_LENGTH);
    
    const uint8_t cmd_pec_lsb = (cmd_pec & 0xFF);
    const uint8_t cmd_pec_msb = ((cmd_pec >> 8) & 0xFF);
    
    // Pack command + PEC into 16-bit words
    uint16_t cmd_words[2];
    cmd_words[0] = (cmd0 << 8) | cmd1;
    cmd_words[1] = (cmd_pec_lsb << 8) | cmd_pec_msb;
    
    // Send STSCTRL command
    iso_spi_config.CS_HOLD = FALSE;
    spiTransmitData(ISO_SPI_MODULE, &iso_spi_config, 2, cmd_words);
    
    return true;
}
