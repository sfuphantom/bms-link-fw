# SCI (UART) and SCI/LIN — TMS570LS12x/11x
**Context:** Two modules share one register layout.
- **SCI/LIN module** (base `0xFFF7E400`): UART ("SCI compatibility mode", `LIN MODE=0`) OR LIN 2.x master/slave (`LIN MODE=1`). Pins LINTX / LINRX.
- **SCI module** (base `0xFFF7E500`): UART only (no LIN registers, no SCIGCR2/LINx regs). Pins SCITX / SCIRX.
No hardware flow control (RTS/CTS) — do in software with a GPIO. UART frame is a plain async NRZ: idle high, start bit low, **LSB first**.

## Frame format (UART mode)
1 start + 1–8 data bits (SCIFORMAT.CHAR = bits−1) + optional address bit (address-bit multiprocessor mode only) + optional parity + 1 or 2 stop bits. Both TX and RX use the same format.

## Register map (offset, both modules unless noted)
| Off | Reg | Notes |
|---|---|---|
| 00 | SCIGCR0 | `RESET`[0]: 1 = module out of reset (privileged write). Must be 1 before other regs writable |
| 04 | SCIGCR1 | main control (below) |
| 08 | SCIGCR2 | **SCI/LIN only**: CC[17], SC[16] checksum; GEN WU[8]; POWERDOWN[0] |
| 0C/10 | SCISETINT / SCICLEARINT | interrupt enable set / clear (same bit positions as SCIFLR) |
| 14/18 | SCISETINTLVL / SCICLEARINTLVL | route each source to INT0 (0) or INT1 (1) line |
| 1C | SCIFLR | flags |
| 20/24 | SCIINTVECT0/1 | pending-interrupt offset (read clears most; **TXRDY not cleared by vector read**) |
| 28 | SCIFORMAT | `LENGTH`[18:16] (LIN response bytes−1), `CHAR`[2:0] (bits−1) |
| 2C | BRS | baud: P[23:0], M[27:24], U[30:28] |
| 30 | SCIED | emulation RX buffer (read without clearing RXRDY) |
| 34 | SCIRD | RX data [7:0] (reading clears RXRDY) |
| 38 | SCITD | TX data [7:0] |
| 3C–5C | SCIPIO0–8 | pin GIO control (below) |
| 60–7C | LINCOMPARE, LINRD0/1, LINMASK, LINID, LINTD0/1, MBRS | **SCI/LIN only** |
| 90 | IODFTCTRL | I/O error-enable / test (bit/phys-bus error test) |

## SCIGCR1 (04h) key bits
`TXENA`25 · `RXENA`24 · `CONT`17 (don't halt on emulation suspend) · `LOOPBACK`16 · `STOP EXT FRAME`13 (LIN) · `HGEN CTRL`12 (LIN: 1 = ID-SlaveTask filtering, recommended) · `CTYPE`11 (LIN: 1 = enhanced checksum) · `MBUF MODE`10 · `ADAPT`9 (LIN auto baud) · `SLEEP`8 (SCI multiprocessor sleep) · **`SWnRST`7 (active-low software reset; 0 = in reset, set to 1 LAST)** · `LIN MODE`6 · **`CLOCK`5 (1 = internal clock; in LIN mode this bit = master(1)/slave(0))** · `STOP`4 (1 = two stop bits) · `PARITY`3 (0 odd, 1 even) · `PARITY ENA`2 (in LIN: ID-parity check) · `TIMING MODE`1 (**1 = asynchronous**, 0 = isosynchronous) · `COMM MODE`0 (0 idle-line, 1 address-bit multiprocessor; in LIN: ID4/ID5 length control).
(Typical plain UART: CLOCK=1, TIMING MODE=1, COMM MODE=0, LIN MODE=0, STOP/PARITY as needed, RXENA/TXENA=1.)

## Baud rate (BRS @2Ch) — asynchronous mode
```
SCICLK    = VCLK / (P + 1 + M/16)
baud      = SCICLK / 16  =  VCLK / (16 × (P + 1 + M/16))
P = 0:      baud = VCLK / 32     (max baud)
```
- P is 24-bit integer prescaler, M is 4-bit fractional (0–15), U is 3-bit "superfractional" modulation (BRS[30:28], 0–7 in table, doc range 0–2h listed for LIN) that adds 1 VCLK period to selected bit slots to average out error (extra resolution; async mode only).
- Instantaneous bit time = `[16(P+1+M/16)+d]·T_VCLK`.
- Example with **VCLK = 50 MHz**: 115200 baud → P=26 (error +0.47%); 57600 → P=53; 38400 → P=80; 19200 → P=162; 9600 → P=325; 10400 (LIN) → P=299.
- Choose P/M to keep error low (<~2% for UART; LIN slaves need tighter / auto-adapt). `MBRS` (13-bit) sets max baud prescaler in LIN mode.
- LIN mode uses only asynchronous baud values.

## UART configuration sequence
1. `SCIGCR0.RESET=1`. 2. Clear `SCIGCR1.SWnRST=0`. 3. Frame bits in SCIGCR1 (STOP, PARITY, PARITY ENA, COMM MODE, TIMING MODE=1, CLOCK=1) and `SCIFORMAT.CHAR=7` (8-bit). 4. **`SCIPIO0`: TX FUNC[2]=1 and RX FUNC[1]=1** (else pins are GPIO) and set pin mux in IOMM if needed. 5. `BRS`. 6. Optional: LOOPBACK, CONT. 7. `RXENA`/`TXENA`=1. 8. Interrupt enables/levels (SCISETINT, SCISETINTLVL). 9. **`SWnRST=1`**.
Rules: Data written to SCITD before TXENA=1 is not sent. Receiver needs RXENA=1 and RX FUNC=1. Receiver waits for a valid idle period before receiving (`SCIFLR.IDLE`=0 means idle detected/ready).

## Data flow
- **TX:** wait `TXRDY`(bit8)=1 → write SCITD. `TX EMPTY`(11)=1 when SCITD and shift reg both empty (use to know last byte is on the wire, e.g. before switching an RS-485/half-duplex driver). When done with TX interrupts, clear `TX INT`/`TX DMA` or TXENA — TXRDY stays set.
- **RX:** `RXRDY`(9)=1 → read SCIRD (clears it). Errors set with the frame: `FE`(26) framing, `OE`(25) overrun, `PE`(24) parity; `BRKDT`(0) break detected; `BUSY`(3) receiving; `WAKEUP`(1). Error/status flags are generally cleared by writing 1 to the bit in SCIFLR (some also clear by reading SCIRD or SCIINTVECT0/1 — RXRDY clears on SCIRD read; TXRDY does not clear via vector read).
- Modes: single-buffer (MBUF MODE=0, per-byte) or multi-buffer (MBUF MODE=1: hardware sends/receives `LENGTH+1` bytes via LINTD0/1 / LINRD0/1 — mainly useful in LIN).
- DMA: `SET TX DMA`[16], `SET RX DMA`[17], `SET RX DMA ALL`[18]. 
- Interrupt enable bits (SCISETINT; mirror SCIFLR): `BRKDT`0, `WAKEUP`1, `TIMEOUT`4, `TOAWUS`6, `TOA3WUS`7, `TX INT`8, `RX INT`9, `ID INT`13, `RX DMA`17/`TX DMA`16, error: `PE`24, `OE`25, `FE`26, `NRE`27, `ISFE`28, `CE`29, `PBE`30, `BE`31 (BE..NRE, ISFE LIN-only).

## SCIFLR bit summary
31 BE · 30 PBE · 29 CE · 28 ISFE · 27 NRE (LIN only) · 26 FE · 25 OE · 24 PE · 14 ID RX · 13 ID TX (LIN) · 12 RXWAKE (address bit received) · 11 TX EMPTY · 10 TXWAKE (next frame = address) · 9 RXRDY · 8 TXRDY · 7 TOA3WUS · 6 TOAWUS · 4 TIMEOUT (LIN bus idle) · 3 BUSY · 2 IDLE · 1 WAKEUP · 0 BRKDT.

## Multiprocessor / sleep (UART mode)
Idle-line mode (COMM MODE=0): an address frame is one preceded by ≥10 idle bits (fewer = data frame); Address-bit mode (COMM MODE=1): 9th bit =1 marks address (TXWAKE / RXWAKE). `SLEEP`=1 makes receiver ignore data frames until an address frame.

## Pin GPIO control (SCIPIO0–8, bit2=TX pin, bit1=RX pin)
PIO0 FUNC (1=SCI function) · PIO1 DIR · PIO2 IN (RO) · PIO3 OUT · PIO4 SET · PIO5 CLR · PIO6 PDR (open drain) · PIO7 PD (pull disable) · PIO8 PSL (pull select). (SCI module pins SCITX/SCIRX; SCI/LIN pins LINTX/LINRX.)

## LIN mode (SCI/LIN module only; `LIN MODE=1`)
- LIN 2.x single-master/multi-slave, break/sync/ID header + 1–8 byte response + checksum. Hardware does header generation, sync-field baud adaption (`ADAPT`), ID parity, checksum (classic or enhanced via `CTYPE`), timeouts (no-response, bus idle >4 s, wake-up 150 ms / 3×, 1.5 s).
- **Config:** as UART plus `LIN MODE=1`, `CLOCK`=1 master/0 slave, `BRS`, `MBRS`, `MBUF MODE`=1 (recommended), `LINMASK` (RX ID MASK[23:16], TX ID MASK[7:0]; set both 0xFF), `HGEN CTRL=1`, then SWnRST=1.
- **Master transmit header:** CPU writes **`LINID[7:0]` (ID BYTE)** → hardware sends break+sync+protected ID. For a TX response write `LINTD0/1` bytes (TD0 first) with `SCIFORMAT.LENGTH` set; checksum auto-sent (or `SCIGCR2.SC`). Slave receive/transmit decided by match of received ID vs `LINID[15:8]` (ID-SlaveTask byte) and masks; flags `ID RX`/`ID TX`, received ID in `LINID[23:16]`.
- RX response in `LINRD0/1`; `SCIGCR2.CC` triggers checksum compare in extended-frame mode. Errors: BE, PBE, CE, ISFE, NRE, FE, OE, PE.
- Wake-up: `GEN WU`(SCIGCR2[8]) sends wakeup; `POWERDOWN`(SCIGCR2[0]) enters low-power.

## Gotchas
- `SWnRST` must be 0 while configuring, 1 to run; module stuck in reset otherwise.
- Pin function bits (TX FUNC/RX FUNC) default to GPIO.
- TX data is 8-bit register; set CHAR=7 for 8N1.
- Baud error from integer-only P is large at high baud: use M/U.
- Reading SCIFLR does not clear flags; write 1 to clear error flags (see per-flag rules).
- Multi-buffer RX sets RXRDY only after the full programmed frame arrives.
