#include <stdint.h>
#include <stdbool.h>

#include "iso_spi_driver.h"
#include "spi.h"  // SPI driver functions
#include "sys_common.h"

#define PEC_INIT_VALUE 0x0010

// 0100 0101 1001 1001 for the polynomicla x^15 + x^14 + x^10 + x^8 + x^7 + x^4 + x^3 + 1 
#define PEC_CHARACTERISTIC_POLYNOMIAL 0x4599

#define CMD_LENGTH 2

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

bool write_reg(uint16_t cmd, uint8_t* data, uint8_t data_len){

  wakeup_spi();
// deprecated clock_spi() function idk how it works lol
  // Prepare command bytes (LSB first)
  const uint8_t cmd0 = cmd & 0xFF;
  const uint8_t cmd1 = (cmd >> 8) & 0xFF; 
  
  const uint8_t cmd_array[CMD_LENGTH] = {cmd0, cmd1};

  // calculate command pec
  const uint16_t cmd_pec = calculate_pec(cmd_array, CMD_LENGTH);

  // calculate data pec
  const uint16_t data_pec = calculate_pec(data, data_len);

  // prep command + command pec buffer (4 bytes: cmd0, cmd1, pec0, pec1)
  uint8_t cmd_buf[4];
  cmd_buf[0] = cmd0;
  cmd_buf[1] = cmd1;
  cmd_buf[2] = cmd_pec & 0xFF;      // pec lsb first
  cmd_buf[3] = (cmd_pec >> 8) & 0xFF;

  // prep data + data pec buffer (data_len + 2 bytes)
  uint8_t data_buf[data_len + 2];
  // copy user data
  for (uint8_t i = 0; i < data_len; i++) {
    data_buf[i] = data[i]; // last byte if odd number
  }
  // append data pec (lsb first)
  data_buf[data_len] = data_pec & 0xFF;
  data_buf[data_len + 1] = (data_pec >> 8) & 0xFF;

  // send command + command PEC (4 bytes = 2 words)
  uint16_t cmd_words[2];
  cmd_words[0] = cmd_buf[0] | (cmd_buf[1] << 8);  // little-endian 0+
  cmd_words[1] = cmd_buf[2] | (cmd_buf[3] << 8);
  spiTransmitData(ISO_SPI_MODULE, &iso_spi_config, 2, cmd_words);

  // send data + data pec ((data_len + 2) bytes = (data_len + 2 + 1)/2 word)
  uint16_t data_word_count = (data_len + 2 + 1) / 2;  // round up
  uint16_t data_words[data_word_count];
  // pack bytes into words
  for (uint8_t i = 0; i < data_len + 2; i += 2) {
    if (i + 1 < data_len + 2) {
      data_words[i / 2] = data_buf[i] | (data_buf[i + 1] << 8); // last byte if odd number
    } else {
      data_words[i / 2] = data_buf[i];  // last byte if odd number
    }
  }
  spiTransmitData(ISO_SPI_MODULE, &iso_spi_config, data_word_count, data_words);

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

  wakeup_spi();

  // Prepare command bytes (LSB first, same as write_reg)
  const uint8_t cmd0 = cmd & 0xFF;
  const uint8_t cmd1 = (cmd >> 8) & 0xFF; 
  
  const uint8_t cmd_array[CMD_LENGTH] = {cmd0, cmd1};

  // calculate command pec (same as write_reg)
  const uint16_t cmd_pec = calculate_pec(cmd_array, CMD_LENGTH);

  // prep command + command pec buffer (4 bytes: cmd0, cmd1, pec0, pec1) (same as write_reg)
  uint8_t cmd_buf[4];
  cmd_buf[0] = cmd0;
  cmd_buf[1] = cmd1;
  cmd_buf[2] = cmd_pec & 0xFF;      // pec lsb first
  cmd_buf[3] = (cmd_pec >> 8) & 0xFF;

  // pack command into 16-bit words for SPI (same as write_reg)
  uint16_t cmd_words[2];
  cmd_words[0] = cmd_buf[0] | (cmd_buf[1] << 8);
  cmd_words[1] = cmd_buf[2] | (cmd_buf[3] << 8);

  // receive buffer for data + pec (data_len + 2 bytes = (data_len + 2 + 1)/2 word)
  uint16_t rx_word_count = (data_len + 2 + 1) / 2;  // Round up
  uint16_t rx_words[rx_word_count];
  uint16_t dummy_tx[rx_word_count];  // Dummy TX data (SPI is full-duplex)
  for (uint16_t i = 0; i < rx_word_count; i++) {
    dummy_tx[i] = 0x0000;  // Send zeros while receiving
  }

  // Send command + command PEC, receive data + data PEC
  // First send command (2 words), while receiving initial data
  uint16_t cmd_rx[2];
  spiTransmitAndReceiveData(ISO_SPI_MODULE, &iso_spi_config, 2, cmd_words, cmd_rx);

  // Continue receiving data + PEC by sending dummy data
  spiTransmitAndReceiveData(ISO_SPI_MODULE, &iso_spi_config, rx_word_count, dummy_tx, rx_words);

  // Unpack received words into bytes
  uint8_t rx_buf[data_len + 2];
  for (uint8_t i = 0; i < data_len + 2; i += 2) {
    if (i + 1 < data_len + 2) {
      rx_buf[i] = rx_words[i / 2] & 0xFF;      // LSB
      rx_buf[i + 1] = (rx_words[i / 2] >> 8) & 0xFF;  // MSB
    } else {
      rx_buf[i] = rx_words[i / 2] & 0xFF;  // last byte if odd
    }
  }

  // move received data to user data buffer (excluding pec)
  for (uint8_t i = 0; i < data_len; i++) {
    data[i] = rx_buf[i];
  }

  // take received pec (last 2 bytes)
  uint8_t received_pec0 = rx_buf[data_len];
  uint8_t received_pec1 = rx_buf[data_len + 1];
  uint16_t received_pec = received_pec0 | (received_pec1 << 8);

  // calculate pec for received data to validate (same as write_reg)
  const uint16_t calculated_data_pec = calculate_pec(data, data_len);
  
  // validate pec (same as write_reg)
  if (calculated_data_pec != received_pec) {
    return false; // pec mismatch - data corruption detected any other flags needed here?
  }

  return true;
}