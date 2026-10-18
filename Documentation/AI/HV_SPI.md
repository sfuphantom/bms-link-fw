# ADC Handoff — ADS7044 (12-bit, 1 MSPS SAR ADC)  [uploaded as "HV.pdf"]

The file name suggests a high-voltage measurement use, but the document is only the generic ADC datasheet. It has no isolation or high-voltage content. Items under "Design notes" are my own engineering inferences, not datasheet statements.

## What it is
- 12-bit SAR ADC, up to 1 MSPS, differential input, SPI-like read-only serial interface (no MOSI/command input).
- **The analog supply AVDD is also the reference.** No separate reference pin.

## Limits and operating ranges
| Item | Value |
|---|---|
| AVDD (analog supply + reference) | 1.65–3.6 V |
| DVDD (digital I/O supply, independent of AVDD) | 1.65–3.6 V |
| Absolute max, AVDD/DVDD | −0.3 to 3.9 V |
| Absolute max, AINP/AINM | −0.3 V to AVDD + 0.3 V |
| Absolute max, digital inputs | −0.3 V to DVDD + 0.3 V |
| Operating temperature | −40 to 125 °C |
| AVDD current at 1 MSPS, 3 V | about 240 µA typ (graph), 300 µA max |
| Power | ≤ 900 µW at 1 MSPS, 3 V; ≤ 90 µW at 100 kSPS; 261 µW typ at 1 MSPS, 1.8 V; current scales roughly linearly with sample rate |

## Pins
| Function | X2QFN (RUG) pin | VSSOP (DCU) pin |
|---|---|---|
| CS (active low, frames one conversion) | 1 | 4 |
| SDO (data out) | 2 | 3 |
| SCLK | 3 | 2 |
| DVDD | 4 | 1 |
| GND | 5 | 8 |
| AVDD | 6 | 7 |
| AINP | 7 | 6 |
| AINM | 8 | 5 |

## Analog input and transfer function
- Differential: converts AINP − AINM. Full-scale span is −AVDD to +AVDD, so **FSR = 2 × AVDD** and **1 LSB = FSR / 4096 = AVDD / 2048**.
- Each input pin individually must stay within −0.1 V to AVDD + 0.1 V (that is, roughly 0 to AVDD).
- Output is 12-bit **two's complement**:
| AINP − AINM | Code |
|---|---|
| ≥ AVDD − 1 LSB | 0x7FF (positive full scale) |
| 1 LSB to 2 LSB | 0x001 |
| 0 to 1 LSB | 0x000 |
| −(AVDD − 1 LSB) to −(AVDD − 2 LSB) | 0x801 |
| ≤ −(AVDD − 1 LSB) | 0x800 (negative full scale) |
- LSB size: 1.611 mV at AVDD = 3.3 V; 1.465 mV at 3.0 V.
- Input stage: sampling capacitor 15 pF, series switch about 50 Ω, plus an on-chip 10 pF between the two inputs. Needs a driver and RC filter (below).

## Accuracy (AVDD = 3 V unless noted)
- INL ±1 LSB max (±2 at 1.8 V); DNL ±1 LSB max (typ ±0.5); no missing codes.
- Offset: uncalibrated ±12 LSB max; **calibrated ±3 LSB max**. Offset drift 5 ppm/°C.
- Gain error ±0.1% FS max (±0.2% at 1.8 V); gain drift 2 ppm/°C. CMRR 53 dB at 2 kHz.
- SNR 71 dB typ, THD −85 dB, SINAD 71 dB, SFDR 85 dB (2 kHz input). Full-power bandwidth 25 MHz.
- DC histogram at a fixed input: sigma about 0.14 LSB (very quiet).

## Serial interface (how to read it)
- One CS low period = one conversion + readout. **CS falling edge samples the input.**
- Frame = 14 SCLK cycles. SDO bits, MSB first: `0, 0, D11 … D0`. The first 0 appears at the CS falling edge, then each next bit is launched on an SCLK **falling** edge. After the 14th clock SDO stays low until CS rises, then SDO goes high-impedance.
- **Read with SPI mode 0** (SCLK idle low, sample on rising edge), 14 clocks. If the MCU only does 16-bit transfers, read 16 bits and take `(word >> 2) & 0xFFF`, then sign-extend 12 bits. A 16-clock frame lowers the maximum throughput below 1 MSPS.
- There is no data-input pin: the host only drives CS and SCLK and reads SDO.

### Timing
| Parameter | Value |
|---|---|
| SCLK frequency | 16 kHz min, 16 MHz max (needed for 1 MSPS) |
| SCLK period | 62.5 ns min; duty 45–55% |
| Conversion time | 12.5 × tSCLK + tSU_CSCK |
| Acquisition time (tACQ) | 200 ns min after conversion |
| Cycle time | 1 µs min (1 MSPS) |
| CS high time | 60 ns min (but acquisition still needs 200 ns) |
| CS fall → first SCLK fall setup (tSU_CSCK) | 15 ns min |
| last SCLK fall → CS rise (tD_CKCS) | 10 ns min |
| SCLK fall → data valid (tD_CKDO) | 30 ns max (50 ns max if AVDD < 1.8 V) |
| CS fall → data enable | 10 ns max |
| CS rise → SDO tri-state | 5 ns max |
- Digital levels follow DVDD: VIH ≥ 0.65 DVDD, VIL ≤ 0.35 DVDD, VOH ≥ DVDD − 0.45 V at 2 mA source, VOL ≤ 0.45 V at 2 mA sink.

## Offset calibration (do this at every power-up)
- The offset register resets to 0 on power-up, leaving ±12 LSB error. Calibrate it for ±3 LSB.
- **On power-up:** the first CS falling edge triggers calibration if CS stays low for **≥ 16 SCLKs** in that first frame (SCLK ≤ 16 MHz when AVDD > 2.25 V, ≤ 12 MHz when AVDD < 2.25 V). SDO stays low during it. Then wait ≥ 200 ns before the first real sample. With fewer than 16 clocks, calibration is skipped.
- **During operation:** send ≥ 32 SCLKs in one frame. The first 14 clocks return the conversion normally, calibration runs from the 17th to the 32nd falling edge, SDO stays low after clock 14. Fewer than 32 clocks = register not updated. Re-run if temperature or AVDD changes significantly.
- Recycling AVDD power resets the register.

## Supply and layout rules
- Decouple AVDD with 1 µF low-ESR ceramic (200 nF minimum) right at the pins; DVDD 1 µF (20 nF min); 220 nF min if both share a source. AVDD should come from a low-noise, low-impedance regulator (the datasheet suggests TPS79101), because **AVDD noise is reference noise**.
- Ground plane under the device, keep analog input traces away from digital lines, no vias between AVDD/DVDD pins and their capacitors, short low-impedance ground connections.
- Lower DVDD and lower SDO load capacitance reduce digital current. Digital supply current is about C × V × f (up to 6 MHz toggling for codes 0xAAA/0x555 at 1 MSPS).

## Input driver reference circuits (from the datasheet application section)
- Input RC filter: R_FLT in series with each input, C_FLT across the inputs. **C_FLT ≥ 300 pF (20× the 15 pF sampling cap), C0G/NP0 type.** Cutoff f = 1 / (2π × 2R_FLT × C_FLT). The driver amplifier's gain-bandwidth should be at least 4× that cutoff.
- Both reference designs use R_FLT = 30 Ω per leg with 1 nF to ground per leg and 2.2 nF across the inputs, driven from AVDD/2 common-mode. Larger R_FLT improves amplifier stability but adds distortion.
- Design A: single-supply OPA316 pair (four 20 kΩ resistors, rail-to-rail, low power); 5 kHz test gave SNR 72.2 dB, THD −85.7 dB.
- Design B: fully differential THS4531A (36 MHz, 10 nV/√Hz, sets common-mode via VOCM = AVDD/2, CMRR ≥ 90 dB); 5 kHz test gave SNR 72.3 dB, THD −87.8 dB at 740 µA; 25 kHz gave 71.6 dB, THD −85 dB.
- Settling and amplifier phase margin (> 40°) should be checked in SPICE.

## Design notes (my inference, not in the datasheet)
- With AINM tied to ground and a positive-only signal on AINP, only codes 0…2047 are used, so effective resolution is 11 bits (1.61 mV/LSB at 3.3 V). Use the differential drive to get the full 12 bits.
- A high-voltage source needs a resistive divider to bring each pin inside 0 to AVDD. Any use on a high-voltage rail also needs isolation (isolated supply and digital isolator) and protection; this chip provides none.
- Because AVDD is the reference, ratiometric accuracy depends on AVDD stability. Absolute accuracy of a divider measurement is limited by the supply, so consider an external reference regulator if you need better than about 0.1% absolute.
