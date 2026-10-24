# DMA Controller — TMS570LS12x/11x
**Context:** 16 channels (this device), up to 32 hardware request lines, priority queue, independent bus master. Moves data between any addresses (peripheral registers, RAM, flash) without CPU: typical uses here are SPI/SCI/ADC/N2HET/I2C/DCAN streams. Which peripheral drives which request line is in the **datasheet** (DMA request map). Registers and control-packet RAM are **privileged-access only**; the DMA itself runs in user mode, so it can't write privileged-only registers.
**Addresses:** control registers `0xFFFFF000`; control-packet RAM `0xFFF80000` (parity-protected, init via MSINENA bit 1 — see System handoff). 8/16/32-bit access.

## Concepts
- **Element** = one read+write of size RES/WES (8/16/32/64-bit). **Frame** = `IETCOUNT` elements. **Block** = `IFTCOUNT` frames. A request triggers one frame or one whole block (`TTYPE`).
- Each channel n owns control packet n: **primary** (user-programmed) and **working** (hardware copy; at `0x800+`, read-only).
- Requests can come from: software (SWCHENAS), hardware line (HWCHENAS), or chaining from another channel's end.
- Two-level request buffer per channel; a 4th overlapping request is lost. Mixing HW and SW requests on one channel can desync with the peripheral.
- Changing a control packet clears its pending bit and stops an active channel at the next arbitration boundary.

## Primary control packet (offset from `0xFFF80000 + 0x20×channel`)
| Off | Reg | Content |
|---|---|---|
| 00 | ISADDR | initial source address (absolute) |
| 04 | IDADDR | initial destination address |
| 08 | ITCOUNT | `IFTCOUNT`[28:16] frames (0–0x1FFF) · `IETCOUNT`[12:0] elements per frame |
| 10 | CHCTRL | see below |
| 14 | EIOFF | `EIDXD`[28:16] dest, `EIDXS`[12:0] source element index (**bytes**, not scaled by element size) |
| 18 | FIOFF | `FIDXD`/`FIDXS` frame index offsets (bytes) |
Working: 800 CSADDR · 804 CDADDR · 808 CTCOUNT (current values).

### CHCTRL (10h)
`CHAIN`[21:16] next channel to trigger at end (value = channel+1; 0 = none) · `RES`[15:14] read element size (0=8b,1=16b,2=32b,3=64b) · `WES`[13:12] write element size · `TTYPE`[8] 0=frame / 1=block per request · `ADDMR`[4:3] read addressing (0 constant, 1 post-increment, 3 indexed) · `ADDMW`[2:1] write addressing · `AIM`[0] auto-initiation (reload from primary after block complete; re-arms for next request).
- Use **constant** address for peripheral data registers, **post-increment** for RAM buffers. Pack/unpack happens automatically when RES ≠ WES.

## Control registers (offset from 0xFFFFF000)
00 GCTRL (**`DMA_EN`[16]** enable, `DEBUG_MODE`[9:8], `BUS_BUSY`[14] RO, `DMA_RES`[0] soft reset) · 04 PEND (pending channels) · 0C DMASTAT (`STCH[n]` active) · 14 HWCHENAS / 1C HWCHENAR (HW request enable set/reset) · 24 SWCHENAS / 2C SWCHENAR (software trigger set/reset) · 34 CHPRIOS / 3C CHPRIOR (1 = high-priority queue, 0 = low) · 44 GCHIENAS / 4C GCHIENAR (channel interrupt enable) · **54/58/5C/60 DREQASI0–3** (request-line assignment, see below) · 94/98 PAR0/PAR1 (port assignment) · B4 FTCMAP · BC LFSMAP · C4 HBCMAP · CC BTCMAP (map each channel's interrupt to line A/B) · DC/E4 FTCINTENAS/AR · EC/F4 LFSINTENAS/AR · FC/104 HBCINTENAS/AR · 10C/114 BTCINTENAS/AR · 11C GINTFLAG · 124 FTCFLAG · 12C LFSFLAG · 134 HBCFLAG · 13C BTCFLAG · 144 BERFLAG · 14C–158 FTCA/LFSA/HBCA/BTCAOFFSET · 160–16C FTCB/LFSB/HBCB/BTCBOFFSET (interrupt-vector style offsets per line) · 178 PTCRL (port control) · 17C RTCTRL (RAM test) · 180 DCTRL · 184 WPR/188 WMR (watchpoint) · 1A8 DMAPCR (parity ctrl) · 1AC DMAPAR · 1B0 DMAMPCTRL / 1B4 DMAMPST / 1B8–1D4 DMAMPR0S–3E (4 memory-protection regions).

### Request assignment (DREQASIn)
Each 32-bit reg holds 4 channels as 6-bit fields in **big-endian byte order**: DREQASI0: `CH0ASI`[29:24], `CH1ASI`[21:16], `CH2ASI`[13:8], `CH3ASI`[5:0]; DREQASI1 = ch 4–7 (ch4 in [29:24]), DREQASI2 = ch 8–11, DREQASI3 = ch 12–15. Value 0–0x1F = request line number (0x20–0x3F reserved). **Reset: channel n ← request line n.**

## Typical setup (peripheral → RAM or RAM → peripheral)
1. Make sure DMA RAM is initialized (MSINENA bit 1) and peripheral clock/pins running.
2. Program primary control packet: ISADDR, IDADDR, ITCOUNT, CHCTRL, EIOFF/FIOFF (when indexed). Peripheral data-register addresses: use the register's address; for 8-bit data (SCI, SPI 16-bit etc.) match RES/WES to register width (watch endianness: big-endian MCU — an 8-bit SCI RX data byte is at the register's last byte `base+0x37` for SCIRD(34h)).
3. `DREQASIn` if remapping lines; `CHPRIOS` if high priority.
4. Interrupts: `FTC/LFS/HBC/BTC INTENAS` + `xMAP` (line A/B) + `GCHIENAS`; enable in VIM.
5. `HWCHENAS` bit n (or `SWCHENAS` for software start); finally `GCTRL.DMA_EN=1` (config must come first).
6. Peripheral side: enable its DMA request (e.g. SCI `SET TX/RX DMA`, SPI `DMAREQEN` + TG/DMA control, ADC `xDMACR`, I2C `I2CDMACR`, DCAN `DEx`/IFxCMD.DMAActive).
7. ISR: read `*AOFFSET`/`*BOFFSET` (channel+1) or flags; clear flag by writing 1.

## Other features
- **Auto-initiation:** `AIM=1` keeps the channel armed with its original packet (ring-buffer style, used with HBC/BTC interrupts for ping-pong).
- **Channel chaining** via CHAIN (frame-end or block-end per TTYPE).
- **Priority queue:** two queues (high/low) with round-robin inside; **FIFO buffer** (8 × 64-bit) used with data packing.
- **Interrupt types per channel:** FTC (frame complete), LFS (last frame start), HBC (>half block), BTC (block complete), BER (bus error → ESM).
- **Memory protection:** 4 regions with start/end, per-region access; violation → status in DMAMPST + ESM.
- **Parity** on control-packet RAM (DMAPCR, DMAPAR).

## Gotchas
- Don't enable `DMA_EN` before configuration is complete.
- Element/frame index values are bytes.
- Each peripheral request needs to be enabled at both the peripheral and DMA ends.
- Write privileged only; DMA can't modify privileged peripheral regs.
- Mixing SW and HW triggers on one channel can desync.
- Control-packet RAM must be initialized (ECC/parity) before use.
