// #include "iso_spi_driver.h"


// uint16_t calculate_pec(uint8_t *data, uint8_t len)
// {
//     uint16_t pec = 0x0010;   // PEC initial value
//     uint8_t i, bit;

//     for (i = 0; i < len; i++)
//     {
//         uint8_t byte = data[i];

//         for (bit = 0; bit < 8; bit++)
//         {
//             uint8_t din = (byte >> 7) & 0x01;
//             uint8_t feedback = din ^ ((pec >> 14) & 0x01);

//             pec <<= 1;
//             if (feedback)
//             {
//                 pec ^= 0x4599;
//             }

//             byte <<= 1;
//         }
//     }

//     return (pec & 0x7FFF);
// }