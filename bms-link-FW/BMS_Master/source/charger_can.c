/*
 * charger_can.c
 *
 *  Created on: Feb 28, 2026
 *      Author: joeyw
 */

#include "charger_can.h"
#include <string.h>

static uint16 swap16(uint16 val)
{
    return (uint16)((val >> 8U) | (val << 8U));
}

uint32 Charger_SendCmd(const ChargerCmd_t *cmd)
{
    uint8 buf[8] = {0};
    uint16 v = swap16(cmd->max_voltage_dV);
    uint16 i = swap16(cmd->max_current_dA);

    memcpy(&buf[0], &v, 2U);
    memcpy(&buf[2], &i, 2U);
    buf[4] = cmd->charge_enable & 0x01U;

    return canTransmit(canREG1, CHARGER_TX_BOX, buf);
}

uint32 Charger_GetStatus(ChargerStatus_t *status)
{
    uint8  buf[8] = {0};
    uint32 result;
    uint16 v, i;

    result = canGetData(canREG1, CHARGER_RX_BOX, buf);
    if (result == 0U) return 0U;

    memcpy(&v, &buf[0], 2U);
    memcpy(&i, &buf[2], 2U);

    status->output_voltage_dV = swap16(v);
    status->output_current_dA = swap16(i);
    status->status_flags      = buf[4];

    return result;
}
