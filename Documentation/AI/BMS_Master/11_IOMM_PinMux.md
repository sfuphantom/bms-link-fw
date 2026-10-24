# IOMM (I/O Multiplexing and Control) — TMS570LS12x/11x
**Context:** Many device pins carry several functions. The IOMM selects which one drives each *output* pin (and, on the 144-pin QFP, which pad feeds some *inputs*). Package-dependent: the 337-BGA needs no input muxing; the 144-QFP has input muxing for the signals listed below. Ball/pin numbers and defaults are in the datasheet; this file gives the mechanism and the PINMMR bits found in the TRM. **DCAN1–3, SCI/LIN RX/TX (LIN module), and ADC inputs are dedicated and do not appear in the output-mux table; SCI (the second UART) and several SPI/I2C signals do.**

## Registers (base `0xFFFFEA00`, privileged writes only)
- `0x00` REVISION · `0x20` ENDIAN · **`0x38` KICK_REG0** · **`0x3C` KICK_REG1** · `0xE0` ERR_RAW_STATUS · `0xE4` ERR_ENABLED_STATUS · `0xE8` ERR_ENABLE · … 
- **PINMMR0 starts at `0xFFFFEB10`** (PINMMRn = `0xFFFFEB10 + 4n`; e.g. PINMMR10 = `0xFFFFEB38`). 4 byte-lanes per register; each **byte controls one terminal** and each bit in that byte selects one candidate function.

## Unlock (required before any PINMMR write)
PINMMRs are **locked after every reset**. Write `KICK0 = 0x83E70B13`, then `KICK1 = 0x95A4F1E0`. Lock again by writing any other value to either kick register (recommended after configuring). Writes while locked are silently ignored (no error). Reads always work.
Errors (address error to unimplemented IOMM location, protection error for non-privileged write) go to ESM group 1 channel 37.

## One-hot select rules
- Each terminal's byte: set **exactly one** bit for the desired function and clear the others.
- If **no** bit is set, or **more than one** is set → the terminal falls back to its **default** function (the one whose bit is 1 at reset).
- Example (PINMMR10, ball N17 on 337BGA): default EMIF_nCS[0] = bit16; for N2HET2[7] clear bit16 and set bit18.
- **Inputs are broadcast** to every module wired to that terminal — keep unused modules from reacting (e.g., a GIO interrupt on a pin being used as SPI).

## 144-QFP input muxing (A = output-control bit that enables the function on the shared pin; B = input-select bit)
Input comes from the dedicated pad when `NOT(A) OR (A AND B)`; from the muxed pin when `A AND NOT(B)`.
| Signal | A (PINMMR) | B (PINMMR) |
|---|---|---|
| SPI4SIMO | 5[9] | 23[16] |
| SPI4SOMI | 5[17] | 23[24] |
| SPI4CLK | 5[1] | 23[8] |
| SPI4NENA | 4[17] | 24[0] |
| SPI4NCS[0] | 4[25] | 24[8] |
| N2HET1[17] | 20[17] | 24[16] |
| N2HET1[19] | 8[9] | 24[24] |
| N2HET1[23] | 12[17] | 25[8] |
| N2HET1[25] | 7[9] | 25[16] |
| N2HET1[27] | 0[26] | 25[24] |
| N2HET1[29] | 0[18] | 26[0] |
| N2HET1[31] | 9[10] | 26[8] |
| GIOB[2] | 29[16] | 29[16] |

## PINMMR bits for communication/ADC functions (function → register[bit]; each is one alternative on some terminal; verify default/ball in datasheet)
- **SCI (UART2):** SCIRX = PINMMR7[17] (shares terminal with N2HET1[6]=7[16], EPWM5A=7[18]) · SCITX = PINMMR8[1] (shares with N2HET1[13]=8[0], EPWM5B=8[2]).
- **SPI4:** SPI4CLK 5[1] · SPI4SIMO 5[9] · SPI4SOMI 5[17] · SPI4NENA 4[17] · SPI4NCS[0] 4[25].
- **MibSPI3:** CLK 33[24] · SIMO[0] 33[16] · SOMI[0] 33[8] · NENA 9[8] · NCS[0] 9[16] · NCS[1] 7[8] · NCS[2] 0[24] · NCS[3] 0[16] · NCS[4] 1[9] · NCS[5] 9[9].
- **MibSPI5:** CLK 13[16] · SIMO[0] 13[8] · SOMI[0] 13[0] · SOMI[1] 12[28] · SOMI[2] 13[12] · NENA 12[24] · NCS[0] 27[0].
- **MibSPI1:** NCS[0] 13[24] · SOMI[1] 13[25] · SIMO[1] 14[1] · NENA 12[16] · NCS[1] 20[16] · NCS[2] 8[8] · NCS[3] 9[24] · NCS[4] 8[17] · NCS[5] 11[25]. (CLK/SIMO[0]/SOMI[0] default-dedicated.)
- **SPI2:** NENA 29[0] · NCS[1] 29[1].
- **I2C:** I2C_SCL = 0[17] · I2C_SDA = 0[25].
- **ADC external triggers:** AD1EVT = 10[0] · AD2EVT = 9[17].
- **N2HET1 pins on shared terminals:** [0] 5[0] · [1] 4[16] · [2] 5[8] · [3] 4[24] · [5] 5[16] · [6] 7[16] · [8] 14[0] · [11] 1[8] · [13] 8[0] · [15] 8[16] · [17] 20[17] · [19] 8[9] · [21] 9[25] · [23] 12[17] · [24] 11[24] · [25] 7[9] · [27] 0[26] · [29] 0[18] · [31] 9[10].
(Full per-terminal table incl. EMIF/EMAC/ePWM/eCAP/eQEP/N2HET2 alternatives: TRM section 4.8.)

## Other special controls in IOMM (rarely needed)
ADC trigger-event selection (ADC1/2 event/G1/G2 sources among N2HET/ePWM/GIO…), ePWM→ADC SOC routing, N2HET external-fault interrupt, clock enables/time-base sync for ePWM/eCAP/eQEP, EMIF fast mode and clock select, Ethernet mode select (MII/RMII).

## Gotchas
- Forgetting the kick sequence → pin mux never changes and nothing errors.
- All-clear or multi-set bit patterns select the default, which may not be what you expect.
- SCI/SPI/I2C/ADC-event pin functions often share terminals with N2HET/EMIF/ePWM; check the pin isn't also driven by another enabled module.
- Pin direction/pull for pins used as peripherals is controlled by the peripheral's own pin-function registers (SPIPC*, SCIPIO*, I2CPFNC, GIO…), not by IOMM.
