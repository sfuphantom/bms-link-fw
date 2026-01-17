#ifndef LTC6811_COMMANDS_H

#define LTC6811_COMMANDS_H

//-------------------------------------------------------------
// CONFIGURATION COMMANDS (Supported by LTC6811-1)
//-------------------------------------------------------------
#define LTC6811_WRCFGA        0x0001  // Write Config Register Group A
#define LTC6811_RDCFGA        0x0002  // Read  Config Register Group A
#define LTC6811_WRCFGB        0x0204  // Write Config Register Group B
#define LTC6811_RDCFGB        0x0206  // Read  Config Register Group B

//-------------------------------------------------------------
// CELL VOLTAGE REGISTER READ COMMANDS (A–D)
//-------------------------------------------------------------
#define LTC6811_RDCVA         0x0004  // Read Cell Voltages A (C1–C3)
#define LTC6811_RDCVB         0x0006  // Read Cell Voltages B (C4–C6)
#define LTC6811_RDCVC         0x0008  // Read Cell Voltages C (C7–C9)
#define LTC6811_RDCVD         0x000A  // Read Cell Voltages D (C10–C12)

//-------------------------------------------------------------
// AUX (GPIO) REGISTER READ COMMANDS (A–B)
//-------------------------------------------------------------
#define LTC6811_RDAUXA        0x000C  // Read Auxiliary A (GPIO1–GPIO3)
#define LTC6811_RDAUXB        0x000E  // Read Auxiliary B (GPIO4–GPIO5, VREF2)

//-------------------------------------------------------------
// STATUS REGISTER READ COMMANDS (A–B)
//-------------------------------------------------------------
#define LTC6811_RDSTATA       0x0010  // Read Status A  (SC, ITMP, VA)
#define LTC6811_RDSTATB       0x0012  // Read Status B  (VD + Flags)

//-------------------------------------------------------------
// CLEAR COMMANDS
//-------------------------------------------------------------
#define LTC6811_CLRCELL       0x0711  // Clear Cell Voltage Registers
#define LTC6811_CLRAUX        0x0712  // Clear Auxiliary Registers
#define LTC6811_CLRSTAT       0x0713  // Clear Status Registers

//-------------------------------------------------------------
// S-CONTROL COMMANDS
//-------------------------------------------------------------
#define LTC6811_WRSCTRL       0x0014  // Write S-Control Register Group
#define LTC6811_RDSCTRL       0x0016  // Read  S-Control Register Group
#define LTC6811_STSCTRL       0x0019  // Start S-Control Pulse
#define LTC6811_CLRSCTRL      0x0018  // Clear S-Control Registers

//-------------------------------------------------------------
// COMM REGISTER COMMANDS (GPIO I2C/SPI Master)
//-------------------------------------------------------------
#define LTC6811_WRCOMM        0x0070  // Write COMM Register (6 bytes)
#define LTC6811_RDCOMM        0x0072  // Read  COMM Register (6 bytes)
#define LTC6811_STCOMM        0x0073  // Start COMM Transaction

//-------------------------------------------------------------
// ADC CONVERSION COMMANDS (Only modes implemented in 6811-1)
//-------------------------------------------------------------
// Cell ADC Conversions
#define LTC6811_ADCV          0x0260  // Start Cell Voltage ADC Conversion
#define LTC6811_CVST          0x0207  // Cell Converter Self-Test

// AUX (GPIO) Conversions
#define LTC6811_ADAX          0x0460  // Start GPIO/2nd Ref ADC Conversion
#define LTC6811_AXST          0x0467  // GPIO Converter Self-Test

// Status Conversions
#define LTC6811_ADSTAT        0x0468  // Start Status ADC Conversion
#define LTC6811_STATST        0x0469  // Status Converter Self-Test

// Combined Measurements
#define LTC6811_ADCVAX        0x046F  // Cell + GPIO1/2 Combined Measurement

// Open-Wire Detection
#define LTC6811_ADOW          0x0228  // Open-Wire Cell Voltage Measurement


#endif //LTC6811_COMMANDS_H
