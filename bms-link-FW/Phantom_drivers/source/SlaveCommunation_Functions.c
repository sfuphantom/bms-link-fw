/*
 * SlaveCommunation_Functions.c
 *
 *  Created on: Jun 7, 2026
 *      Author: tanjo
 */


#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include "ltc6811_commands.h"
#include "SlaveCommunation_Hardware.h"
#include "SPI_helpers.h"
#include "SlaveCommunication_Drivers.h"
#include "SlaveCommunation_Functions.h"
#include "PhantomAlgorithms.h"
#include "PhantomTimers.h"

static ConfigReg_t ConfigRegData[NUMBER_OF_SLAVE_BOARDS_TOTAL];         // what is written next
static ConfigReg_t ConfigRegWritten[NUMBER_OF_SLAVE_BOARDS_TOTAL];      // what was written last
static StatusReg_t StatusRegData[NUMBER_OF_SLAVE_BOARDS_TOTAL];
static uint16_t    Ref2ndData[NUMBER_OF_SLAVE_BOARDS_TOTAL];            // raw ADC, comes from the AUX registers

// The registers are copied straight to and from the SPI bytes, so they must be exactly that big
typedef char ConfigSizeCheck[(sizeof(ConfigReg_t) == BYTES_PER_REG_GROUP) ? 1 : -1];
typedef char StatusSizeCheck[(sizeof(StatusReg_t) == NUMBER_OF_STAT_REG_GROUPS_PER_BOARD * BYTES_PER_REG_GROUP) ? 1 : -1];
typedef char BoardMaskCheck[(NUMBER_OF_SLAVE_BOARDS_TOTAL <= 32) ? 1 : -1];

//---------------------------------------------------------------------------------------------------------
// ADC value -> real units
//---------------------------------------------------------------------------------------------------------
static float Slave_ADC2Volt(const uint16_t ADCVaule){
    return SLAVE_ADC2VOLTS(ADCVaule);
}
static void ADC2Volts_arr(const uint16_t ADCVaule_in[], float Volts_out[], const size_t NumberOfElements){
    size_t i;
    for(i=0;i<NumberOfElements;i++){
        Volts_out[i] = Slave_ADC2Volt(ADCVaule_in[i]);
    }
}
float Slave_ADC2Celsius(const uint16_t ADC){
    return SLAVE_ADC2TEMP(ADC);
}
float Slave_SC_ADC2Volt(const uint16_t ADC_Word){
    return Slave_ADC2Volt(ADC_Word) * 20;
}
//---------------------------------------------------------------------------------------------------------
// Register fields
//---------------------------------------------------------------------------------------------------------
uint16_t Config_getVUV(const ConfigReg_t * const c){
    return (uint16_t)(((uint16_t)c->f.cfgr2.vuv_hi << 8U) | c->f.vuv_lo);
}
uint16_t Config_getVOV(const ConfigReg_t * const c){
    return (uint16_t)(((uint16_t)c->f.vov_hi << 4U) | c->f.cfgr2.vov_lo);
}
uint16_t Config_getDCC(const ConfigReg_t * const c){
    return (uint16_t)(((uint16_t)c->f.cfgr5.dcc_hi << 8U) | c->f.dcc_lo);
}
void Config_setVUV(ConfigReg_t * const c, const uint16_t value){
    c->f.vuv_lo         = (uint8_t)(value & 0xFFU);
    c->f.cfgr2.vuv_hi   = (value >> 8U) & 0x0FU;
}
void Config_setVOV(ConfigReg_t * const c, const uint16_t value){
    c->f.cfgr2.vov_lo   = value & 0x0FU;
    c->f.vov_hi         = (uint8_t)((value >> 4U) & 0xFFU);
}
void Config_setDCC(ConfigReg_t * const c, const uint16_t value){
    c->f.dcc_lo         = (uint8_t)(value & 0xFFU);
    c->f.cfgr5.dcc_hi   = (value >> 8U) & 0x0FU;
}

uint16_t Status_SC(const StatusReg_t * const s)   { return read_u16_LE(s->f.sc); }
uint16_t Status_ITMP(const StatusReg_t * const s) { return read_u16_LE(s->f.itmp); }
uint16_t Status_VA(const StatusReg_t * const s)   { return read_u16_LE(s->f.va); }
uint16_t Status_VD(const StatusReg_t * const s)   { return read_u16_LE(s->f.vd); }

// Per cell the flags are UV (even bit) and OV (odd bit). offset 0 gives the UV flags, 1 the OV flags.
static uint16_t cellFlags(const StatusReg_t * const s, const uint8_t offset){
    const uint32_t both = (uint32_t)s->f.uvov[0] | ((uint32_t)s->f.uvov[1] << 8U) | ((uint32_t)s->f.uvov[2] << 16U);
    uint16_t flags = 0;
    uint8_t cell;

    for(cell = 0; cell < CELLS_PER_SLAVE_BOARD; cell++){
        flags |= (uint16_t)(((both >> (2U*cell + offset)) & 0x1U) << cell);
    }
    return flags;
}
uint16_t Status_UV_flags(const StatusReg_t * const s){ return cellFlags(s, 0U); }
uint16_t Status_OV_flags(const StatusReg_t * const s){ return cellFlags(s, 1U); }
//---------------------------------------------------------------------------------------------------------
const StatusReg_t* GetStatusRegData(void){
    return StatusRegData;
}
const ConfigReg_t* GetConfigRegData(void){
    return ConfigRegData;
}

// Writes known bytes into the unions and checks that the bit fields land where the chip expects them
static bool layoutOk(void){
    ConfigReg_t c;
    StatusReg_t s;

    ZERO_STRUCT(c);
    ZERO_STRUCT(s);

    c.bytes[0] = 0xB5U;     // GPIO 10110, REFON 1, DTEN 0, ADCOPT 1
    c.bytes[2] = 0xA5U;     // VOV[3:0] 1010, VUV[11:8] 0101
    c.bytes[5] = 0x3CU;     // DCTO 0011, DCC[11:8] 1100
    s.f.uvov[0] = 0x00U;
    s.bytes[11] = 0x92U;    // REV 1001, MUXFAIL 1, THSD 0

    return (c.f.cfgr0.gpio == 0x16U) && (c.f.cfgr0.refon == 1U) && (c.f.cfgr0.DTEN == 0U) && (c.f.cfgr0.adcopt == 1U)
        && (c.f.cfgr2.vov_lo == 0xAU) && (c.f.cfgr2.vuv_hi == 0x5U)
        && (c.f.cfgr5.dcto == 0x3U)   && (c.f.cfgr5.dcc_hi == 0xCU)
        && (s.f.flags.REV == 0x9U)    && (s.f.flags.MUXFAIL == 1U) && (s.f.flags.THSD == 0U);
}
//---------------------------------------------------------------------------------------------------------
// Config
//---------------------------------------------------------------------------------------------------------

// What a read gives back differs from what was written for these: DTEN is the pin,
// GPIO is the pin level and DCTO is the time that is left.
static bool configMatches(const ConfigReg_t * const wrote, const ConfigReg_t * const read){
    ConfigReg_t a = *wrote;
    ConfigReg_t b = *read;

    a.f.cfgr0.DTEN = 0;     b.f.cfgr0.DTEN = 0;
    a.f.cfgr0.gpio = 0;     b.f.cfgr0.gpio = 0;
    a.f.cfgr5.dcto = 0;     b.f.cfgr5.dcto = 0;

    return arr_u8_equal(a.bytes, b.bytes, BYTES_PER_REG_GROUP);
}

static SlaveStatus_t readConfig(ConfigReg_t * const out){
    uint8_t raw[NUMBER_OF_CONFIG_BYTES_SLAVES];
    const uint16_t cmds[NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD] = {LTC6811_RDCFGA};

    const SlaveStatus_t status = ReadMultiRegGroups(cmds, NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD, raw);

    if(status == SLAVE_OK){
        memcpy(out, raw, sizeof(raw));
    }
    return status;
}

SlaveStatus_t Write_CFGR(void){
    ConfigReg_t out[NUMBER_OF_SLAVE_BOARDS_TOTAL];
    uint8_t raw[NUMBER_OF_CONFIG_BYTES_SLAVES];
    const uint16_t cmds[NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD] = {LTC6811_WRCFGA};
    uint8_t i;

    memcpy(out, ConfigRegData, sizeof(out));
    for(i = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        out[i].f.cfgr0.DTEN = 0;                    // read only, write 0
    }
    memcpy(raw, out, sizeof(raw));

    const SlaveStatus_t status = WriteMultiRegGroups(cmds, NUMBER_OF_CONFIG_REG_GROUPS_PER_BOARD, raw);

    if(status == SLAVE_OK){
        memcpy(ConfigRegWritten, ConfigRegData, sizeof(ConfigRegWritten));
    }
    return status;
}

SlaveStatus_t Read_CFGR(void){
    return readConfig(ConfigRegData);
}

SlaveStatus_t WriteThenRead_CFGR(void){
    ConfigReg_t readBack[NUMBER_OF_SLAVE_BOARDS_TOTAL];
    SlaveStatus_t status = SLAVE_ERR_MISMATCH;
    uint8_t attempt, board;

    for(attempt = 0; attempt < SLAVE_NUMBER_OF_FAILS_ALLOWED; attempt++){
        status = Write_CFGR();
        if(status == SLAVE_OK){ status = readConfig(readBack); }

        if(status == SLAVE_OK){
            for(board = 0; board < NUMBER_OF_SLAVE_BOARDS_TOTAL; board++){
                if(!configMatches(&ConfigRegWritten[board], &readBack[board])){
                    status = Slave_NoteError(SLAVE_ERR_MISMATCH);
                    break;
                }
            }
        }

        if(status == SLAVE_OK){ return SLAVE_OK; }
        if(status != SLAVE_ERR_PEC && status != SLAVE_ERR_MISMATCH){ return status; }
    }
    return status;
}

// A chip that went to sleep (watchdog) lost its config. A chain that does not answer is treated as asleep too.
SlaveStatus_t checkAsleep(uint32_t * const asleepBoards){
    ConfigReg_t readBack[NUMBER_OF_SLAVE_BOARDS_TOTAL];
    uint32_t mask = 0;
    uint8_t board;

    SlaveStatus_t status = readConfig(readBack);

    if(status == SLAVE_OK){
        for(board = 0; board < NUMBER_OF_SLAVE_BOARDS_TOTAL; board++){
            if(!configMatches(&ConfigRegWritten[board], &readBack[board])){
                mask |= (uint32_t)1U << board;
            }
        }
        if(mask != 0U){ status = Slave_NoteError(SLAVE_ERR_ASLEEP); }
    }
    else if(status == SLAVE_ERR_PEC){
        mask   = (NUMBER_OF_SLAVE_BOARDS_TOTAL == 32U) ? 0xFFFFFFFFU : (((uint32_t)1U << NUMBER_OF_SLAVE_BOARDS_TOTAL) - 1U);
        status = Slave_NoteError(SLAVE_ERR_ASLEEP);
    }

    if(asleepBoards != NULL){ *asleepBoards = mask; }
    return status;
}

SlaveStatus_t wakeup_sleep_and_SetConfig(void){
    const SlaveStatus_t status = wakeup_sleep();

    return (status == SLAVE_OK) ? Write_CFGR() : status;
}

SlaveStatus_t wakeIfAsleep(void){
    const SlaveStatus_t status = checkAsleep(NULL);

    if(status != SLAVE_ERR_ASLEEP){ return status; }

    if(wakeup_sleep() != SLAVE_OK){ return SLAVE_ERR_ASLEEP; }
    return WriteThenRead_CFGR();
}

SlaveStatus_t KeepAlive(void){
    return SendCmd(DUMMY_CMD);
}

SlaveStatus_t waitDummyCMD(const uint32_t WaitPeriod_ms, const uint32_t WaitPeriod_us, uint16_t WaitSends){
    for(;WaitSends!=0;WaitSends--){
        const SlaveStatus_t status = KeepAlive();

        if(status != SLAVE_OK){ return status; }
        delay_ms_us(WaitPeriod_ms,WaitPeriod_us);
    }
    return SLAVE_OK;
}

SlaveStatus_t initConfig(void){
    uint8_t i;
    for(i = 0; i < NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        ConfigReg_t* const current_Reg = &ConfigRegData[i];

        ZERO_STRUCT(*current_Reg);
        current_Reg->f.cfgr0.adcopt = 0;
        current_Reg->f.cfgr0.refon  = 1;
        current_Reg->f.cfgr0.gpio   = 0x1F;
        Config_setDCC(current_Reg, 0x000);
        Config_setVUV(current_Reg, UNDER_VOLTAGE_CONFIG);
        Config_setVOV(current_Reg, OVER_VOLTAGE_CONFIG);
    }

    return WriteThenRead_CFGR();
}

SlaveStatus_t initLink(void){
    if(!layoutOk()){ return Slave_NoteError(SLAVE_ERR_LAYOUT); }

    pec15_init();

    SlaveStatus_t status = wakeup_sleep();
    if(status == SLAVE_OK){ status = ClearSlaveRegs(); }
    if(status == SLAVE_OK){ status = SetAllPWM_Regs(0x0); }
    if(status == SLAVE_OK){ status = initConfig(); }
    return status;
}
//---------------------------------------------------------------------------------------------
void ReadConfig_DCC(uint16_t * const DCC){
    uint8_t i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         DCC[i] = Config_getDCC(&ConfigRegData[i]);
    }
}
void SetConfig_DCC(const uint16_t * const DCC){
    uint8_t i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         Config_setDCC(&ConfigRegData[i], DCC[i]);
    }
}
void SetAllConfig_DCC(const uint16_t DCC){
    uint8_t i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         Config_setDCC(&ConfigRegData[i], DCC);
    }
}
void ReadConfig_gpio(uint8_t * const gpio){
    uint8_t i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         gpio[i] = ConfigRegData[i].f.cfgr0.gpio;
    }
}
bool ReadConfig_gpio_allZero(void){
    uint8_t i;
    bool allZero = true;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         allZero &= (ConfigRegData[i].f.cfgr0.gpio == 0U);
    }
    return allZero;
}
void SetConfig_gpio(const uint8_t * const gpio){
    uint8_t i;
    for(i=0; i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
         ConfigRegData[i].f.cfgr0.gpio = gpio[i] & 0x1FU;
    }
}
//---------------------------------------------------------------------------------------------------------
// Status and faults
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t Read_STAT(void){
    uint8_t raw[NUMBER_OF_STAT_BYTES_SLAVES];
    const uint16_t cmds[NUMBER_OF_STAT_REG_GROUPS_PER_BOARD] = {LTC6811_RDSTATA, LTC6811_RDSTATB};
    size_t i;

    const SlaveStatus_t status = ReadMultiRegGroups(cmds, NUMBER_OF_STAT_REG_GROUPS_PER_BOARD, raw);
    if(status != SLAVE_OK){ return status; }

    memcpy(StatusRegData, raw, sizeof(raw));

    for(i=0; i<NUMBER_OF_STAT_BYTES_SLAVES; i+=BYTES_PER_REG_GROUP){
        if(arr_u8_all(&raw[i], BYTES_PER_REG_GROUP, isEqual, SPI_DUMMY_BYTE)){
            return Slave_NoteError(SLAVE_ERR_NO_DATA);
        }
    }
    return SLAVE_OK;
}

void getStatus_SC(float * const SC){
    uint8_t i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        SC[i] = Slave_SC_ADC2Volt(Status_SC(&StatusRegData[i]));
    }
}
void getStatus_BoardTempC(float * const BoardTempC){
    uint8_t i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        BoardTempC[i] = Slave_ADC2Celsius(Status_ITMP(&StatusRegData[i]));
    }
}
void getStatus_RefVolt2nd(float * const RefVolt2nd){
    uint8_t i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        RefVolt2nd[i] = Slave_ADC2Volt(Ref2ndData[i]);
    }
}

static uint16_t boardFaults(const StatusReg_t * const s, const uint16_t ref2nd){
    uint16_t faults = 0;

    // NOTE: these four are set when the chip flag is 0. The datasheet has 1 = over/under/failed, check this.
    if(Status_OV_flags(s) == 0)  { faults |= (uint16_t)(1U << BAD_OV_flags); }
    if(Status_UV_flags(s) == 0)  { faults |= (uint16_t)(1U << BAD_UV_flags); }
    if(!s->f.flags.THSD)         { faults |= (uint16_t)(1U << BAD_THSD); }
    if(!s->f.flags.MUXFAIL)      { faults |= (uint16_t)(1U << BAD_MUXFAIL); }

    if(isNotBetween_CC(Status_ITMP(s), MIN_INTERNAL_DIE_TEMPERATURE_FLAG, MAX_INTERNAL_DIE_TEMPERATURE_FLAG)){
        faults |= (uint16_t)(1U << BAD_ITMP);
    }
    if(isNotBetween_CC(Status_VA(s), MIN_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG, MAX_ANALOG_POWER_SUPPLY_VOLTAGE_FLAG)){
        faults |= (uint16_t)(1U << BAD_VA);
    }
    if(isNotBetween_CC(Status_VD(s), MIN_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG, MAX_DIGITAL_POWER_SUPPLY_VOLTAGE_FLAG)){
        faults |= (uint16_t)(1U << BAD_VD);
    }
    if(isNotBetween_CC(ref2nd, MIN_2ND_REFERENCE_VOLTAGE_FLAG, MAX_2ND_REFERENCE_VOLTAGE_FLAG)){
        faults |= (uint16_t)(1U << BAD_REF2ND);
    }
    return faults;
}

void checkStatFlags_perBoard(uint16_t * const faults){
    uint8_t i;
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        faults[i] = boardFaults(&StatusRegData[i], Ref2ndData[i]);
    }
}

uint32_t checkStatFlags(void){
    uint16_t perBoard[NUMBER_OF_SLAVE_BOARDS_TOTAL];
    uint32_t faults = 0;
    uint8_t i;

    checkStatFlags_perBoard(perBoard);
    for(i=0;i<NUMBER_OF_SLAVE_BOARDS_TOTAL; i++){
        faults |= perBoard[i];
    }
    return faults;
}

SlaveStatus_t ReadStatAndGetFlags(uint32_t * const faults){
    SlaveStatus_t status = MeasureSTATCmd_All(ADC_MEASURE_MODE);

    if(status == SLAVE_OK){ status = Read_STAT(); }
    if(status == SLAVE_OK){ *faults = checkStatFlags(); }
    return status;
}
//---------------------------------------------------------------------------------------------------------
// Commands
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t ClearCellsCMD(void){ return SendCmd(LTC6811_CLRCELL); }
SlaveStatus_t ClearAUXCMD(void){   return SendCmd(LTC6811_CLRAUX); }
SlaveStatus_t ClearStatCMD(void){  return SendCmd(LTC6811_CLRSTAT); }
SlaveStatus_t ClearSCtrlCMD(void){ return SendCmd(LTC6811_CLRSCTRL); }

SlaveStatus_t ClearSlaveRegs(void){
    SlaveStatus_t status = ClearCellsCMD();

    if(status == SLAVE_OK){ status = ClearAUXCMD(); }
    if(status == SLAVE_OK){ status = ClearStatCMD(); }
    if(status == SLAVE_OK){ status = ClearSCtrlCMD(); }
    return status;
}

static uint16_t adcCmd(const uint16_t base, const uint8_t MD, const uint16_t extraBits){
    return (uint16_t)(base | (((uint16_t)MD & 0x03U) << 7U) | extraBits);
}

SlaveStatus_t MeasureSTATCmd(const uint8_t MD,      // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                             const uint8_t CHST)    //  Status Group Selection
{
    return ConvertADC(adcCmd(LTC6811_ADSTAT, MD, CHST & 0x07U));
}

SlaveStatus_t MeasureSTATCmd_All(const uint8_t MD)
{
    const SlaveStatus_t status = ClearStatCMD();

    return (status == SLAVE_OK) ? MeasureSTATCmd(MD, 0) : status;
}

SlaveStatus_t MeasureCellsCmd(const uint8_t MD  , // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                              const bool DCP    , // Discharge Permit
                              const uint8_t CHG ) // Cell Selection for ADC Conversion
{
    return ConvertADC(adcCmd(LTC6811_ADCV, MD, (DCP ? 0x0010U : 0x0000U) | (CHG & 0x07U)));
}

SlaveStatus_t MeasureCellsCmd_All_NoDis(const uint8_t MD)
{
    const SlaveStatus_t status = ClearCellsCMD();

    return (status == SLAVE_OK) ? MeasureCellsCmd(MD, false, 0) : status;
}

SlaveStatus_t MeasureAUXCmd(const uint8_t MD,     // ADC mode: 0=Fast, 1=Normal, 2=Filtered
                            const uint8_t CHG)    //  GPIO Selection for ADC Conversion
{
    return ConvertADC(adcCmd(LTC6811_ADAX, MD, CHG & 0x07U));
}

SlaveStatus_t MeasureAUXCmd_All(const uint8_t MD)
{
    const SlaveStatus_t status = ClearAUXCMD();

    return (status == SLAVE_OK) ? MeasureAUXCmd(MD, 0) : status;
}

SlaveStatus_t Start_S_CTRL_Pulsing(void){
    return SendCmdAndPoll(LTC6811_STSCTRL);
}
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t Write_S_CTRL(const uint8_t * const nibbles){
    return WriteRegGroup(LTC6811_WRSCTRL, nibbles);
}
SlaveStatus_t Read_S_CTRL(uint8_t * const nibbles){
    return ReadRegGroup(LTC6811_RDSCTRL, nibbles);
}
SlaveStatus_t Write_PWM(const uint8_t * const nibbles){
    return WriteRegGroup(LTC6811_WRPWM, nibbles);
}
SlaveStatus_t Read_PWM(uint8_t * const nibbles){
    return ReadRegGroup(LTC6811_RDPWM, nibbles);
}
SlaveStatus_t WriteThenRead_PWM(const uint8_t * const nibbles){
    return WriteThenReadRegGroup(LTC6811_WRPWM, LTC6811_RDPWM, nibbles);
}
SlaveStatus_t SetAllPWM_Regs(const uint8_t nibble){
    uint8_t nibbles[NUMBER_OF_REG_BYTES_PER_CMD];
    const uint8_t both = (uint8_t)((nibble & 0x0FU) | ((nibble & 0x0FU) << 4U));

    arr_u8_fill(nibbles, NUMBER_OF_REG_BYTES_PER_CMD, both);
    return WriteThenRead_PWM(nibbles);
}
//---------------------------------------------------------------------------------------------------------
// Readings
//---------------------------------------------------------------------------------------------------------
SlaveStatus_t GetVoltageReadings(uint16_t * const data){
    const uint16_t cmds[NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD] = {LTC6811_RDCVA, LTC6811_RDCVB, LTC6811_RDCVC, LTC6811_RDCVD};

    return ReadMultiRegGroups_u16(cmds, NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD, data);
}
SlaveStatus_t GetGPIOReadings_Analog(uint16_t * const data){
    const uint16_t cmds[NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD] = {LTC6811_RDAUXA, LTC6811_RDAUXB};

    return ReadMultiRegGroups_u16(cmds, NUMBER_OF_GPIO_VOLTAGE_REG_GROUPS_PER_BOARD, data);
}

// Only this chip's data is kept, so it is enough to read up to this chip
SlaveStatus_t GetCellReadings_Slave(const uint8_t board, uint16_t * const cells){
    const uint16_t cmds[NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD] = {LTC6811_RDCVA, LTC6811_RDCVB, LTC6811_RDCVC, LTC6811_RDCVD};
    uint8_t group[BYTES_PER_REG_GROUP];
    uint8_t i;

    for(i = 0; i < NUMBER_OF_CELL_VOLTAGE_REG_GROUPS_PER_BOARD; i++){
        SlaveStatus_t status = wakeup_idle();

        if(status == SLAVE_OK){ status = ReadRegGroup_Slave(cmds[i], board, group); }
        if(status != SLAVE_OK){ return status; }

        bytes_to_u16_LE(group, &cells[i * WORDS_PER_REG_GROUP], WORDS_PER_REG_GROUP);
    }
    return SLAVE_OK;
}
//---------------------------------------------------------------------------------------------------------
// AUX data is G1..G5, REF for every chip
static void splitAUX(const uint16_t * const AUXData, uint16_t * const GPIO_Data, uint16_t * const RefVolt2nd){
    uint8_t board, gpio;
    size_t idx = 0;

    for(board=0; board<NUMBER_OF_SLAVE_BOARDS_TOTAL; board++){
        for(gpio=0; gpio<GPIOS_PER_SLAVE_BOARD; gpio++){
            GPIO_Data[board * GPIOS_PER_SLAVE_BOARD + gpio] = AUXData[idx++];
        }
        RefVolt2nd[board] = AUXData[idx++];
    }
}

SlaveStatus_t MeasureCellVoltage_16(uint16_t * const CellVolt){
    SlaveStatus_t status = MeasureCellsCmd_All_NoDis(ADC_MEASURE_MODE);

    if(status == SLAVE_OK){ status = GetVoltageReadings(CellVolt); }
    if(status == SLAVE_OK && arr_u16_any(CellVolt, NUMBER_OF_CELLS_SLAVES, isEqual, ADC_NO_DATA)){
        status = Slave_NoteError(SLAVE_ERR_NO_DATA);
    }
    return status;
}

SlaveStatus_t MeasureRef2ndVoltage(void){
    uint16_t AUXDataOut[NUMBER_OF_AUCILIARY_SLAVES];
    uint16_t GPIO_data_buf[NUMBER_OF_GPIOS_SLAVES];
    uint16_t RefVolt2nd_Data[NUMBER_OF_REF_2ND_SLAVES];

    SlaveStatus_t status = ClearAUXCMD();
    if(status == SLAVE_OK){ status = MeasureAUXCmd(ADC_MEASURE_MODE, 6); }
    if(status == SLAVE_OK){ status = GetGPIOReadings_Analog(AUXDataOut); }
    if(status != SLAVE_OK){ return status; }

    splitAUX(AUXDataOut, GPIO_data_buf, RefVolt2nd_Data);

    if(arr_u16_any(RefVolt2nd_Data, NUMBER_OF_REF_2ND_SLAVES, isEqual, ADC_NO_DATA)){   // the GPIOs are not converted here
        return Slave_NoteError(SLAVE_ERR_NO_DATA);
    }
    memcpy(Ref2ndData, RefVolt2nd_Data, sizeof(Ref2ndData));
    return SLAVE_OK;
}

SlaveStatus_t MeasureAUXVoltage16(uint16_t * const GPIO_Data){
    uint16_t AUXDataOut[NUMBER_OF_AUCILIARY_SLAVES];
    uint16_t RefVolt2nd_Data[NUMBER_OF_REF_2ND_SLAVES];

    SlaveStatus_t status = MeasureAUXCmd_All(ADC_MEASURE_MODE);
    if(status == SLAVE_OK){ status = GetGPIOReadings_Analog(AUXDataOut); }
    if(status == SLAVE_OK && arr_u16_any(AUXDataOut, NUMBER_OF_AUCILIARY_SLAVES, isEqual, ADC_NO_DATA)){
        status = Slave_NoteError(SLAVE_ERR_NO_DATA);
    }
    if(status != SLAVE_OK){ return status; }

    splitAUX(AUXDataOut, GPIO_Data, RefVolt2nd_Data);
    memcpy(Ref2ndData, RefVolt2nd_Data, sizeof(Ref2ndData));
    return SLAVE_OK;
}

SlaveStatus_t MeasureCellVoltage(float * const CellVolt, const size_t NumberOfRealValues){
    uint16_t CellVolt16[NUMBER_OF_CELLS_SLAVES];
    const SlaveStatus_t status = MeasureCellVoltage_16(CellVolt16);

    if(status == SLAVE_OK){
        ADC2Volts_arr(CellVolt16, CellVolt, MIN_VAL(NumberOfRealValues, (size_t)NUMBER_OF_CELLS_SLAVES));
    }
    return status;
}
SlaveStatus_t MeasureAUXVoltage(float * const GPIO_Data, const size_t NumberOfRealValues){
    uint16_t GPIO_Data16[NUMBER_OF_GPIOS_SLAVES];
    const SlaveStatus_t status = MeasureAUXVoltage16(GPIO_Data16);

    if(status == SLAVE_OK){
        ADC2Volts_arr(GPIO_Data16, GPIO_Data, MIN_VAL(NumberOfRealValues, (size_t)NUMBER_OF_GPIOS_SLAVES));
    }
    return status;
}
//---------------------------------------------------------------------------------------------------------
