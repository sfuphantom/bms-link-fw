# ADC (12-bit SAR) — TMS570LS12x/11x
**Context:** Two 12-bit SAR ADC cores (ADC1, ADC2) sharing analog pins/reference/supply. **ADC1 = 24 channels (AD1IN[23:0]); ADC2 = 16 channels, all of which are shared pins with ADC1** (exact pin mapping: see datasheet). Each core: 3 conversion groups (Event, Group1, Group2), 64-word results RAM per ADC (parity-protected), SW or HW triggering, DMA/interrupt support, self-test and calibration logic.
Base addresses: **ADC1 `0xFFF7C000`, ADC2 `0xFFF7C200`**. All regs 32-bit; write reserved bits as 0.

## Key properties
- Resolution: **10-bit is default**, 12-bit via `ADOPMODECR.10_12_BIT=1`.
- `fADCLK = fVCLK / (PS+1)`, PS = ADCLOCKCR[4:0] (÷1–÷32). Max ADCLK is in the datasheet (not in this manual).
- Acquisition (sample) time per group: `tACQ = GxSAMP[11:0] + 2` ADCLK cycles (range 2–4098). Min time depends on source impedance — datasheet/app note SPNA118. High-impedance sources (e.g., divider resistors for cell voltage / NTC) need long sample time.
- If both ADCs sample the same channel, their sample windows must be identical or separated by ≥2 ADCLK.
- Groups: **Group1 & Group2 = SW or HW triggered; Event group = HW trigger only.** Up to 8 HW trigger sources per group (device-specific list in datasheet; e.g., N2HET, RTI, ADxEVT pin). Trigger select in ADEVSRC / ADG1SRC / ADG2SRC (source 0–7, edge/both-edges, polarity: default falling).
- Channels convert in **ascending channel number** within a group (sequential mode), or user-defined order via Enhanced Channel Selection (lookup table; `ADxCHNSELMODECTRL`).
- Group mode (xMODECR `MODE`): single sequence or continuous. All three can't be continuous (Group2 is forced single).

## Register map (offset from base)
| Off | Reg | Off | Reg |
|---|---|---|---|
| 00 | ADRSTCR (`0`=release from reset, `1`=reset) | 70 / 74 | ADG1SR / ADG2SR status |
| 04 | ADOPMODECR | 78 / 7C / 80 | ADEVSEL / ADG1SEL / ADG2SEL channel select (bitmask) |
| 08 | ADCLOCKCR | 84 | ADCALR (calibration/offset correction) |
| 0C | ADCALCR | 88 / 8C | ADSMSTATE / ADLASTCONV |
| 10/14/18 | ADEVMODECR / ADG1MODECR / ADG2MODECR | 90–AC | ADEVBUFFER_0–7 (FIFO read window) |
| 1C/20/24 | ADEVSRC / ADG1SRC / ADG2SRC | B0–CC | ADG1BUFFER_0–7 |
| 28/2C/30 | xINTENA (event/G1/G2) | D0–EC | ADG2BUFFER_0–7 |
| 34/38/3C | xINTFLG | F0/F4/F8 | EV/G1/G2 EMUBUFFER (read w/o advancing) |
| 40/44/48 | xTHRINTCR (threshold count) | FC–118 | ADEVT pin GIO (DIR,OUT,IN,SET,CLR,PDR,PDIS,PSEL) |
| 4C/50/54 | xDMACR | 11C–124 | xSAMPDISEN (sample-cap discharge) |
| 58 | ADBNDCR (BNDA[24:16], BNDB[8:0]) | 128–164 | ADMAGINT1–3 CR/MASK, ENASET/CLR, FLG, OFF |
| 5C | ADBNDEND (BNDEND[2:0]) | 168–170 | x FIFO reset |
| 60/64/68 | ADEVSAMP / ADG1SAMP / ADG2SAMP | 174–17C | xRAMWRADDR |
| 6C | ADEVSR | 180/184/188 | ADPARCR / ADPARADDR / ADPWRUPDLYCTRL |
| 190–198 | x CHNSELMODECTRL | 19C–1B0 | x CURRCOUNT / MAXCOUNT |

## Important bit fields
- **ADOPMODECR (reset 0x00140000):** `10_12_BIT`[31] (1=12-bit) · `COS`[24] emulation · `CHN_TEST_EN`[20:17] (reset 0xA; input impedance/self-test mode) · `RAM_TEST_EN`[16] · `POWER_DOWN`[8] · `IDLE_PWRDN`[4] · **`ADC_EN`[0] must be 1**.
- **GxMODECR (Group1 at 14h; Group2 at 18h; Event 10h):** `G1_DATA_FMT`[9:8] (12-bit mode: 00 = 12-bit, 01 = >>2 → 10-bit, 10 = >>4 → 8-bit, FIFO reads only) · `G1_CHID`[5] (1 = channel number in result bits [14:10]) · `OVR_G1_RAM_IGN`[4] (allow overwrite) · `G1_HW_TRIG`[3] · `G1_8BIT`[2] (10-bit mode only: >>2) · `G1_MODE`[1] (1 = continuous) · `FRZ_G1`[0] (freeze when higher group preempts). Bit 16 `No_Reset_on_ChnSel` (don't reset results memory when new channel select is written; default: any nonzero write to GxSEL resets that group's memory).
- **ADG1SR (70h):** `G1_MEM_EMPTY`[3] (RO, FIFO mode) · `G1_BUSY`[2] (always 1 in continuous) · `G1_STOP`[1] (frozen) · **`G1_END`[0]** (sequence done; cleared by reading a FIFO result, writing GxSEL, writing 1, or ADC_EN=0).
- **Interrupt/flag regs (GxINTENA/GxINTFLG):** `G1_END_INT_EN`[3] etc.; also threshold (`G1_THRESH_INT`) and memory-overrun (`G1_MEM_OVERRUN`[1] in INTFLG, RO) interrupts.
- **ADBNDCR/ADBNDEND memory partition:**
  - Event buffers = `2×BNDA`; Group1 buffers = `2×(BNDB−BNDA)`; Group2 buffers = `Total − 2×BNDB`.
  - `BNDEND[2:0]` total = 0 → 16 words, 1 → 32, 2 → **64 (max)**. Layout order in RAM: Event, Group1, Group2.
- **Result word:** data in bits [11:0] (right-aligned; FIFO read applies DATA_FMT); bits [14:10] hold channel ID if CHID=1 (else forced 0).

## Basic single-sequence recipe (example: channels 0,2,4,8 in Group1)
1. `ADRSTCR = 0` (release reset). 2. `ADOPMODECR.ADC_EN = 1` (and `10_12_BIT` as needed). 3. `ADCLOCKCR.PS`. 4. `ADG1SAMP`. 5. (Configure `ADBNDCR/ADBNDEND` so Group1 has enough buffers; set `ADG1MODECR` mode/format/trigger/ints.) 6. **Write channel mask to `ADG1SEL` (=0x115 for ch 0,2,4,8)** — in SW-trigger mode the conversion starts immediately on the write. 7. Wait `ADG1SR.G1_END`=1 (or interrupt/DMA). 8. Read results via `ADG1BUFFER` (FIFO, oldest first; LDM-friendly address window) or directly from results RAM.
HW trigger: set `G1_HW_TRIG=1`, select source/edge in `ADG1SRC`, then write channels to `ADG1SEL`; conversion starts on trigger. Continuous mode needs one trigger only.

## Reading results
- **FIFO mode:** read any address in ADxBUFFER_0..7 window; returns oldest unread; applies DATA_FMT/CHID; clears G1_END on read; updates MEM_EMPTY.
- **Direct RAM:** read results RAM by address (no pop, no formatting; mask bits yourself). RAM auto-initializes after reset; parity-checked (ADPARCR/ADPARADDR).
- DMA: per-result request or fixed block of results (xDMACR). Threshold interrupts: after N results (xTHRINTCR).

## Other features
- **Magnitude threshold interrupts** (ADMAGINT1–3): compare a channel's result against threshold with mask; flags in ADMAGINTFLG, offset register gives pending index.
- **Self-test mode:** drives channel to detect open/short-to-VCC/short-to-GND on inputs (uses CHN_TEST_EN). **Calibration mode:** ADCALCR/ADCALR offset-error correction, mid-point calibration.
- **Power-down:** `POWER_DOWN` (core only) / `IDLE_PWRDN` (enhanced, auto when idle); `ADPWRUPDLYCTRL` sets power-up delay.
- **Sample-cap discharge** (xSAMPDISEN) removes charge from previous high-voltage channel when switching.
- **ADEVT pin** doubles as GPIO (ADEVTDIR/OUT/IN/SET/CLR/PDR/PDIS/PSEL) or HW trigger input.

## Gotchas
- Default resolution is 10-bit, not 12.
- Writing a non-zero value to GxSEL triggers/restarts that group's conversions and (by default) resets its memory.
- With Group1/Group2 both enabled: whichever was triggered first is serviced first; priority/freeze governed by `FRZ_Gx`.
- Read ADxBUFFER (FIFO) only for the formatted, popped results; direct RAM reads show raw 16-bit words.
- BNDA/BNDB are in units of **2 buffers**.
