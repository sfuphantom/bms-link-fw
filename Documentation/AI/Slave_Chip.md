# BMS IC Handoff — LTC6811-1 / LTC6811-2 (12-cell battery stack monitor)

Source: 92-page datasheet (Rev C). This file keeps what is needed to design with and talk to the chip: specs, pin setup, serial protocol, command codes, register maps, timing, diagnostics, balancing, temperature sensing. Dropped: package, ordering, typical performance plots, LTC6804 comparison, most application circuits. Command hex values and PEC examples below were computed and checked (PEC of 0x0001 gives 0x3D6E as the datasheet states). Items marked **(inferred)** are not stated in the datasheet.

## 1. What it does
- Measures up to **12 series cells** with a 16-bit delta-sigma ADC, plus 5 GPIO analog inputs, internal die temperature, supply rails and the sum of all cells. All 12 cells in about 290 µs in the fastest practical case.
- **Passive balancing** switches on every cell, optional discharge timer and per-cell PWM.
- Two variants, same silicon, different bus wiring:
  - **LTC6811-1: daisy-chain.** Two isoSPI ports (A toward the host, B toward the next device). Only broadcast commands (no addresses).
  - **LTC6811-2: addressable, parallel.** One isoSPI port, 4 address pins (A0–A3), supports both broadcast and addressed commands.
- Interface to the host: **isoSPI** (2-wire isolated differential, 1 Mbps, up to 100 m twisted pair, typically driven by an LTC6820 or another LTC6811-1) or plain **4-wire SPI**, chosen by the ISOMD pin.

## 2. Key specs
| Item | Value |
|---|---|
| ADC resolution | 16 bit, **0.1 mV/LSB**, cell range 0–5 V |
| Total measurement error (TME), normal and filtered modes | at 3.3 V: ±0.2 mV typ, ±1.2 mV max (25 °C), ±2.2 mV over temperature. At 4.2 V: ±0.3 typ, ±1.6 mV max (25 °C), **±2.8 mV over temperature** |
| TME, fast mode (27 kHz) | at 4.2 V up to ±8.3 mV over temperature (poor, avoid for protection) |
| Sum of all cells error | ±0.25% max (normal/filtered) |
| V+ supply | 11 to 55 V, must be ≥ top cell voltage − 0.3 V; V+ to C6 ≤ 40 V for TME specs |
| VREG | 5 V (4.5–5.5 V), from the DRIVE pin through an external NPN (collector V+, emitter VREG) or an external 5 V supply |
| Cell input range | each Cn within C(n−1) to C(n−1) + 5 V; C0 near V− |
| Supply current | Sleep about 4 µA; standby about 13 µA (V+) + 40 µA (VREG); measuring about 11.5 mA (VREG); isoSPI ready/active adds about 4.5/5.8 mA at 2 kΩ bias |
| 2nd reference VREF2 | 3.000 V ±5 mV, drives several 10 kΩ thermistors; bypass 1 µF |
| 1st reference VREF1 | about 3.2 V, 1 µF bypass, **no DC load allowed** |
| Discharge switch resistance | 10 Ω typ, 25 Ω max (at 3.6 V cell) |
| Thermal shutdown | about 150 °C die: resets config registers and turns all discharge switches off |
| Operating temperature | I grade −40 to 85 °C; H grade −40 to 125 °C |
| SPI | SCK period ≥ 1 µs (1 MHz max); SDI setup/hold 25 ns; CSB high ≥ 0.65 µs; CSB fall → first SCK rise ≥ 1 µs |
| Digital levels (SPI pins) | VIH ≥ 2.3 V, VIL ≤ 0.8 V; config pins VIH ≥ 2.7 V, VIL ≤ 1.2 V |

## 3. Pins and hardware setup that affect software
- **ISOMD:** tie to **VREG** = isoSPI on port A (pins 41–44 become IMA/IPA etc.); tie to **V−** = 4-wire SPI on pins CSB, SCK, SDI, SDO. (In SPI mode on a -1, port B still runs isoSPI to the next device.)
- **SDO** is open-drain: needs about a 5 kΩ pull-up. **WDT** is an open-drain output (low while the watchdog is alive); can be left open or pulled up with 1 MΩ.
- **A0–A3 (LTC6811-2 only):** tie each to VREG or V− to set the 4-bit address (0–15).
- **DTEN:** tie to VREG to enable the programmable discharge timer; tie to V− to disable.
- **DRIVE, VREG:** NPN pass transistor between V+ and VREG; 1 µF on VREG.
- **VREF1, VREF2:** 1 µF each. **V−** pins (two) must be shorted externally.
- **GPIO1–5:** digital I/O or analog inputs (0 to 5 V). GPIO3–5 can become an I2C or SPI master.
- **isoSPI bias:** IBIAS resistor divider sets the pulse current (current = 20 × IBIAS current); ICMP sets the receiver threshold (half the ICMP voltage). Typical total RBIAS 2k–20k. Termination and transformer choice are application details in the datasheet (pages 75–86).

## 4. Operating states, wake-up, watchdog
- **Core states:** SLEEP → STANDBY (wake-up) → REFUP (references on, set by REFON = 1) → MEASURE (during ADC conversions) and back. Extended Balancing state keeps PWM discharge alive after the watchdog expires if the discharge timer is enabled.
- **isoSPI states:** IDLE → READY (wake-up detected) → ACTIVE (data moving). Returns to IDLE after **tIDLE ≈ 5.5 ms** (4.3–6.7 ms) without activity on port A.
- **Wake-up:** a differential pulse of ≥ 200 mV lasting ≥ 240 ns on the isoSPI port, or in SPI mode toggling CSB low then high. Time to ready: **tREADY ≤ 10 µs** if the core is in STANDBY, **tWAKE ≤ 400 µs** if in SLEEP. The first CSB toggle should be a dummy transaction.
- **Waking a daisy chain:** device N wakes after the previous one passes the wake pulse, so total time is N × tREADY (or N × tWAKE). Robust method: send a dummy command (e.g. RDCFGA) or toggle CSB once per device, spaced longer than tREADY/tWAKE but shorter than tIDLE; or send another dummy byte after N × tWAKE. Method 1 fails if some devices in the chain are still READY and others IDLE.
- **Reference start-up:** tREFUP ≈ 3.5 ms (2.7–4.4 ms) added to the first conversion when starting from STANDBY. Setting **REFON = 1** keeps references on and removes that delay (cost about 0.45 mA).
- **Watchdog:** if no valid command (with correct PEC) arrives for **about 2 s (1.8–2.2 s)**, the chip returns to SLEEP, resets CFGR0–3, and, unless the discharge timer is running, also CFGR4–5 and the S-control registers (so all balancing stops). The host should send a valid command (e.g. RDCFGA) at least every ~1 s.
- **Discharge timer** (DTEN pin high and DCTO ≠ 0): balancing switches stay on for the programmed time even after the watchdog fires (CFGR0–3 still reset). DCTO codes (minutes): 0 = off, 1 = 0.5, 2 = 1, 3 = 2, 4 = 3, 5 = 4, 6 = 5, 7 = 10, 8 = 15, 9 = 20, A = 30, B = 40, C = 60, D = 75, E = 90, F = 120. Reading CFGR5 shows time left (in ranges).
- After sleep/power-up all measurement registers read 0xFFFF and the PWM register reads all 1s.

## 5. Serial protocol (works the same on SPI and isoSPI)
- Bytes are sent **MSB first**. **CSB stays low for a whole command sequence** including the data that follows. On a write, data is latched on CSB rising edge. SDO changes on SCK falling edge; SDI sampled on SCK rising edge (mode 0 or 3 timing both fit; the on-chip SPI master uses mode 3. I recall vendor reference code uses mode 3, verify).
- **Command word** = 2 bytes + 2 PEC bytes: `CMD0, CMD1, PEC0, PEC1`.
  - `CMD1 = CC[7:0]`, `CMD0 = 0b00000 CC[10:8]` for a **broadcast** command.
  - **Addressed** command (-2 only): `CMD0 = 1 a3 a2 a1 a0 CC[10:8]` i.e. `0x80 | (addr << 3) | (CC >> 8)`. Only the device whose A3–A0 pins match responds.
- **PEC (CRC-15):** polynomial x¹⁵+x¹⁴+x¹⁰+x⁸+x⁷+x⁴+x³+1 (0x4599), **initial value 0x0010**, computed over all bytes in order, then shifted left by 1 so the 16-bit result has a 0 at the LSB. Sent **high byte first** (PEC0 = bits 15–8). Command PEC covers CMD0 and CMD1; data PEC covers the 6 data bytes of one device. A command with a wrong PEC is ignored and does not reset the watchdog.
- **Read a register group:** send the command (4 bytes), then clock out per device: 6 data bytes + 2 PEC bytes. The chip also appends its own computed PEC, so the host must verify it.
- **Write a register group:** send the command, then per device 6 data bytes + 2 PEC bytes; data takes effect when CSB goes high.
- **Daisy chain (-1):** one broadcast command, then the devices behave as a shift register. For a **write**, send the **top (farthest) device's data first**, the bottom device's last. For a **read**, the bottom (primary) device's 8 bytes come first, then the next device, and so on. Total transfer = 8 × N bytes after the command.
- **Parallel (-2):** use addressed commands for reads. Broadcast commands are fine for starts/clears/writes of identical data; do not use broadcast reads or polling with parallel devices.
- **Polling for conversion done:** either wait the conversion time, or after the command keep CSB low and clock SCK: SDO is low while busy, high when done; or send **PLADC** and clock. In a daisy chain the bottom device reports "all done" and its status is valid only after N clocks. Over isoSPI, send isoSPI pulses (e.g. clock the LTC6820's SCK) to read the status.

### Python reference for PEC
```python
def pec15(data):                 # data: iterable of bytes
    rem = 16
    for b in data:
        addr = ((rem >> 7) ^ b) & 0xFF
        x = addr << 7
        for _ in range(8):
            x = ((x << 1) ^ 0x4599) if (x & 0x4000) else (x << 1)
        rem = ((rem << 8) ^ (x & 0xFFFF)) & 0x7FFFFF
    return (rem * 2) & 0xFFFF   # send as [pec >> 8, pec & 0xFF]
```
Check values: `pec15([0x00,0x01]) == 0x3D6E`, `pec15([0x00,0x02]) == 0x2B0A`, `pec15([0x03,0x60]) == 0xF46C`.

## 6. Commands
`CC[10:0]` 11-bit command code; `CMD0 = CC>>8`, `CMD1 = CC & 0xFF`. Ready-to-send bytes (broadcast) are `CMD0 CMD1 PEC0 PEC1`.

### Fixed commands
| Command | CC | Bytes to send (broadcast) | Function |
|---|---|---|---|
| WRCFGA | 0x001 | 00 01 3D 6E | write Configuration group A |
| RDCFGA | 0x002 | 00 02 2B 0A | read Configuration group A |
| RDCVA | 0x004 | 00 04 07 C2 | read cell voltages C1–C3 |
| RDCVB | 0x006 | 00 06 9A 94 | read C4–C6 |
| RDCVC | 0x008 | 00 08 5E 52 | read C7–C9 |
| RDCVD | 0x00A | 00 0A C3 04 | read C10–C12 |
| RDAUXA | 0x00C | 00 0C EF CC | read GPIO1–3 |
| RDAUXB | 0x00E | 00 0E 72 9A | read GPIO4, GPIO5, 2nd reference |
| RDSTATA | 0x010 | 00 10 ED 72 | read SC, ITMP, VA |
| RDSTATB | 0x012 | 00 12 70 24 | read VD, OV/UV flags, MUXFAIL, THSD |
| WRSCTRL | 0x014 | 00 14 5C EC | write S-control group |
| RDSCTRL | 0x016 | 00 16 C1 BA | read S-control group |
| WRPWM | 0x020 | 00 20 00 00 | write PWM group |
| RDPWM | 0x022 | 00 22 9D 56 | read PWM group |
| STSCTRL | 0x019 | 00 19 8E 4E | start S-control pulsing and poll |
| CLRSCTRL | 0x018 | 00 18 05 7C | clear S-control group |
| CLRCELL | 0x711 | 07 11 C9 C0 | set cell registers to 0xFF |
| CLRAUX | 0x712 | 07 12 DF A4 | clear aux registers |
| CLRSTAT | 0x713 | 07 13 54 96 | clear status registers (OV/UV flags, MUXFAIL, THSD set to 1) |
| PLADC | 0x714 | 07 14 F3 6C | poll ADC status |
| DIAGN | 0x715 | 07 15 78 5E | MUX decoder diagnostic (sets or clears MUXFAIL) |
| WRCOMM | 0x721 | 07 21 24 B2 | write COMM group (I2C/SPI master data) |
| RDCOMM | 0x722 | 07 22 32 D6 | read COMM group |
| STCOMM | 0x723 | 07 23 B9 E4 | start I2C/SPI master transaction |
Also valid (forward-compat, mostly unused): WRCFGB 0x024, RDCFGB 0x026, RDCVE 0x009, RDCVF 0x00B, RDAUXC 0x00D, RDAUXD 0x00F, WRPSB 0x01C, RDPSB 0x01E (these return zeros or are for the LTC6812/13).

### ADC start commands (build the code from fields)
`MD[1:0]` mode, `DCP` discharge permitted, `CH` cell selection, `CHG` GPIO selection, `CHST` status selection, `ST` self-test, `PUP` pull-up.
| Command | Code | What it measures |
|---|---|---|
| ADCV | `0x260 + (MD<<7) + (DCP<<4) + CH` | cell voltages |
| ADOW | `0x228 + (MD<<7) + (PUP<<6) + (DCP<<4) + CH` | cell voltages with 100 µA open-wire current |
| CVST | `0x207 + (MD<<7) + (ST<<5)` | cell self-test |
| ADOL | `0x201 + (MD<<7) + (DCP<<4)` | overlap: cell 7 on both ADCs |
| ADAX | `0x460 + (MD<<7) + CHG` | GPIO1–5 and 2nd reference |
| ADAXD | `0x400 + (MD<<7) + CHG` | same with digital redundancy check |
| AXST | `0x407 + (MD<<7) + (ST<<5)` | GPIO self-test |
| ADSTAT | `0x468 + (MD<<7) + CHST` | SC, ITMP, VA, VD |
| ADSTATD | `0x408 + (MD<<7) + CHST` | same with digital redundancy |
| STATST | `0x40F + (MD<<7) + (ST<<5)` | status self-test |
| ADCVAX | `0x46F + (MD<<7) + (DCP<<4)` | all cells + GPIO1, GPIO2 combined |
| ADCVSC | `0x467 + (MD<<7) + (DCP<<4)` | all cells + sum of cells combined |
Examples (all cells, DCP = 0): ADCV normal 7 kHz = **0x360** (bytes `03 60 F4 6C`); ADCV with discharge permitted = 0x370 (`03 70 AF 42`); ADCV fast 27 kHz = 0x2E0 (`02 E0 38 06`); ADCV filtered 26 Hz = 0x3E0 (`03 E0 B0 4A`); ADAX 7 kHz all GPIO = 0x560 (`05 60 D3 A0`); ADSTAT 7 kHz all = 0x568 (`05 68 3B AE`); ADCVSC 7 kHz = 0x567 (`05 67 74 5A`); ADCVAX 7 kHz = 0x56F (`05 6F 9C 54`).
(For addressed use on a -2, change CMD0 as in section 5 and recompute PEC.)

### Field values
| Field | Values |
|---|---|
| MD[1:0] with ADCOPT = 0 (CFGR0 bit 0) | 00 = 422 Hz, 01 = **27 kHz (fast)**, 10 = **7 kHz (normal)**, 11 = **26 Hz (filtered)** |
| MD[1:0] with ADCOPT = 1 | 00 = 1 kHz, 01 = 14 kHz, 10 = 3 kHz, 11 = 2 kHz |
| DCP | 0 = discharge switches forced off during the cell measurement; 1 = discharge allowed to continue |
| CH | 000 all cells; 001 cells 1 and 7; 010 cells 2 and 8; 011 cells 3 and 9; 100 cells 4 and 10; 101 cells 5 and 11; 110 cells 6 and 12 |
| CHG | 000 all GPIO1–5 plus 2nd reference; 001–101 GPIO1–5; 110 2nd reference only |
| CHST | 000 SC+ITMP+VA+VD; 001 SC; 010 ITMP; 011 VA; 100 VD |
| PUP | 0 = 100 µA pull-down, 1 = 100 µA pull-up (open-wire test) |
| ST | 01 = self-test pattern 1, 10 = pattern 2 |

### Conversion times (total)
| Mode | 27 kHz | 14 kHz | 7 kHz | 3 kHz | 2 kHz | 1 kHz | 422 Hz | 26 Hz |
|---|---|---|---|---|---|---|---|---|
| All 12 cells (or all GPIO) | 1.1 ms | 1.3 ms | 2.3 ms | 3.0 ms | 4.4 ms | 7.2 ms | 12.8 ms | 201 ms |
| One channel pair (CH ≠ 0) or single GPIO | 201 µs | 230 µs | 405 µs | 501 µs | 754 µs | 1.2 ms | 2.2 ms | 34 ms |
| Status group SC+ITMP+VA+VD | 748 µs | 865 µs | 1.6 ms | 2.0 ms | 3.0 ms | 4.8 ms | 8.5 ms | 134 ms |
Add tREFUP (about 3.5 ms) when starting from STANDBY with REFON = 0. Time skew between the cell 12 and cell 1 samples: about 670 µs typ in normal mode (ADCV), 233 µs in fast mode. The CH pairs (1&7, 2&8, …) are measured together, which is why a single-pair conversion covers two cells.

## 7. Register groups
Every group is 6 bytes. A cell/GPIO value is **16-bit unsigned, little-endian (low byte first)**, units of 100 µV. After power-up, sleep or a clear command they read **0xFFFF**.

| Group | Byte0–1 | Byte2–3 | Byte4–5 |
|---|---|---|---|
| Cell A (RDCVA) | C1V | C2V | C3V |
| Cell B (RDCVB) | C4V | C5V | C6V |
| Cell C (RDCVC) | C7V | C8V | C9V |
| Cell D (RDCVD) | C10V | C11V | C12V |
| Aux A (RDAUXA) | G1V | G2V | G3V |
| Aux B (RDAUXB) | G4V | G5V | REF (2nd reference) |
| Status A (RDSTATA) | SC | ITMP | VA |
| Status B (RDSTATB) | VD | OV/UV flags (byte 2) and (byte 3) | OV/UV flags (byte 4) and REV/MUXFAIL/THSD (byte 5) |

Conversions: cell/GPIO voltage = value × 100 µV. **Sum of cells = SC × 20 × 100 µV (= SC × 2 mV).** Die temperature °C = ITMP × 100 µV / 7.5 mV/°C − 273 (= ITMP / 75 − 273). VREG (analog supply) = VA × 100 µV (normal 4.5–5.5 V). VREGD (digital) = VD × 100 µV (normal 2.7–3.6 V). 2nd reference should read 2.99–3.01 V (29900–30100 counts), else the ADC is out of tolerance.

**Status B bytes 2–5 (flags):** byte2 = C4OV C4UV C3OV C3UV C2OV C2UV C1OV C1UV (bit7 down to bit0); byte3 = C8OV C8UV C7OV C7UV C6OV C6UV C5OV C5UV; byte4 = C12OV C12UV C11OV C11UV C10OV C10UV C9OV C9UV; byte5 = REV[3:0], RSVD, RSVD, **MUXFAIL**, **THSD** (bits 7–4, 3–2, 1, 0). Flags update on every cell conversion and compare against VUV/VOV below; CLRSTAT sets all flags to 1, MUXFAIL = 1 and THSD = 1. THSD clears after reading Status B.

### Configuration group A (WRCFGA / RDCFGA)
| Byte | bit7 … bit0 |
|---|---|
| CFGR0 | GPIO5 GPIO4 GPIO3 GPIO2 GPIO1 REFON DTEN(read only) ADCOPT |
| CFGR1 | VUV[7:0] |
| CFGR2 | VOV[3:0] (bits 7–4), VUV[11:8] (bits 3–0) |
| CFGR3 | VOV[11:4] |
| CFGR4 | DCC8 DCC7 DCC6 DCC5 DCC4 DCC3 DCC2 DCC1 |
| CFGR5 | DCTO[3:0] (bits 7–4), DCC12 DCC11 DCC10 DCC9 (bits 3–0) |
- **GPIOx:** write 1 = pull-down off (set to 1 for analog use or to float), 0 = pull-down on; on read, shows the pin level. **REFON:** 1 = keep references on until watchdog timeout. **ADCOPT:** selects the alternate MD mode set. **DCCx:** 1 = turn on balance switch for cell x.
- **Under-voltage threshold** = (VUV + 1) × 16 × 100 µV (1.6 mV steps, default VUV = 0). **Over-voltage threshold** = VOV × 16 × 100 µV (default VOV = 0).
- Example: OV 4.2 V → VOV = 2625 = 0xA41 (exactly 4.200 V); UV 2.5 V → VUV = 0x619 (2.4992 V) or 0x61A (2.5008 V). CFGR bytes for GPIO off, REFON = 1, OV 4.2 V, UV 0x619, no balancing: `FC 19 16 A4 00 00` (PEC `BC 4C`). To avoid the whole thing being overwritten, always read-modify-write if other bits (balance) are in use.

### Other groups
- **PWM group** (WRPWM/RDPWM): 4-bit value per cell (PWM1–PWM12; nibbles packed two per byte starting PWM1 low nibble of byte 0). Default 0xF (always on).
- **S-control group** (WRSCTRL/RDSCTRL): 4-bit SCTLx per cell for S-pin pulse patterns (used with the LT8584 active balancer). STSCTRL starts pulsing.
- **COMM group:** three data bytes with control nibbles ICOMn/FCOMn for the I2C/SPI master on GPIO3–5 (GPIO4/5 = SDA/SCL for I2C; GPIO3/4/5 = CSBM/SDIOM/SCKM for SPI mode 3). GPIOs are open-drain, so external pull-ups are required and the GPIO bits in CFGR0 must be written 1 first. Send WRCOMM, then STCOMM (clock count sets the transfer length), then RDCOMM to read back.

## 8. Typical software flow
1. **Init:** wake the chain (dummy transfer per device), write CFGR (REFON = 1, thresholds, ADCOPT as needed), read it back to verify.
2. **Cell read loop:** `ADCV (MD=7 kHz, DCP=0, CH=0)` → wait about 2.3 ms or poll → `RDCVA/B/C/D` → check each PEC → convert. A value of 0xFFFF means no valid data.
3. **Temperature read:** `ADAX` → wait → `RDAUXA/B`. Needs VREF2 on **(inferred: references must be up, so REFON = 1)**.
4. **Keep-alive:** a valid command at least every ~1 s or the watchdog resets configuration (balancing stops).
5. **Balancing:** set DCCx bits (and DCTO if using the timer) with WRCFGA, keep refreshing; stop it while the BMS shutdown circuit is open (an FSAE rule).
6. **Diagnostics periodically:** see section 9.
7. If the first measurement after idle is off, run one extra single-channel ADCV (CH = 001) first and wait about 6 × the input RC time constant before the all-channel ADCV (datasheet's mux-settling recommendation for RC-filtered inputs).

## 9. Diagnostics
- **Self-test** (CVST, AXST, STATST): fills registers with alternating-bit patterns: ST = 01 → 0x9565 (27 kHz), 0x9553 (14 kHz), 0x9555 (all other modes); ST = 10 → 0x6A9A (27 kHz), 0x6AAC (14 kHz), 0x6AAA (others). Verifies digital filter and memory only.
- **Overlap** (ADOL): measures cell 7 on both ADCs; ADC2 result lands where C7 normally is, ADC1 result where C8 is; compare the two.
- **2nd reference** (ADAX, REF register): must be 2.99–3.01 V.
- **Sum of cells** (ADCVSC/ADSTAT): compare SC × 2 mV against the sum of the 12 readings.
- **Digital redundancy** (ADAXD, ADSTATD): a mismatch writes a fault code **0xFF0X** (above the normal 0x0000–0xDFFF range) instead of data; low nibble flags which result nibble disagreed.
- **MUX check (DIAGN):** about 400 µs (REFUP) or 4.5 ms (STANDBY); then read STBR5 MUXFAIL (0 = pass).
- **Open wire (ADOW):** (1) run 12-cell ADOW with PUP = 1 at least twice, read cells → CELLPU[n]. (2) Same with PUP = 0 → CELLPD[n]. (3) CELLΔ[n] = CELLPU[n] − CELLPD[n]. (4) For n = 1…11: if CELLΔ[n+1] < −400 mV then C(n) is open; if CELLPU[1] = 0 then C(0) open; if CELLPD[12] = 0 then C(12) open. For input capacitance C on the pin, run 1 + ceil(C/10 nF) ADOW commands per step (or use filtered mode).
- **Thermal shutdown** flag THSD in Status B; supplies VA and VD should be inside their normal ranges.

## 10. Balancing details
- Each cell has an internal NMOS between S(n) and C(n−1), **25 Ω max on-resistance**. **Always add an external series resistor** so most heat is outside the package; keep internal balance current **≤ 60 mA**; monitor die temperature (ITMP) because heat limits balancing.
- Sizing (datasheet formulas): balance current = (% SOC imbalance × capacity) / hours; balance resistor = nominal cell voltage / balance current. The cell RC measurement filter resistor is part of the discharge path, so keep it small (about 10 Ω).
- Above 60 mA use the S pin to drive an external MOSFET (internal pull-up PMOS with 1 kΩ series). At 1 A or more, active balancing is suggested.
- **DCP bit** controls whether the discharge switch of a cell (and its neighbors) turns off during that cell's measurement.
- **PWM:** after the watchdog expires, PWM operation takes over (DCC bits must be 1): period **30 s**, duty programmable in 1/15 steps (about 2 s, 6.67%), channels staggered by 62.5 ms. Before the watchdog expires the DCC bits drive the switches directly.

## 11. Temperature sensing
- Per the datasheet: 10 kΩ NTC (at 25 °C) with a 10 kΩ bias resistor from VREF2 (3 V) to the GPIO gives about **1.5 V at 25 °C** and roughly **−1%/°C** around normal cell temperatures. VREF2 is built to bias several such probes. GPIO inputs read 0–5 V. A multiplexer on GPIO1 with I2C control on GPIO4/5 can expand to 16 more analog inputs.

## 12. Design implications to keep in mind
- Worst-case TME of **±2.8 mV over temperature at 4.2 V (normal/filtered mode)** should be subtracted from cell OV/UV thresholds if rules require accuracy to be accounted for (FSAE EV.8.4.2). Use normal or filtered mode for protection decisions, not fast mode.
- The V+ range of 11–55 V covers a full 12S block (12 × 4.2 V = 50.4 V).
- With -1 chaining over isoSPI, all devices measure on one command and the host reads 8 bytes per device per group; with -2, each device needs its own addressed read.
