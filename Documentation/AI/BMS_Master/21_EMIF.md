# EMIF (External Memory Interface) — TMS570LS12x/11x
**Context:** 16-bit external bus for **SDR SDRAM** (one chip select, nCS[0]) and **asynchronous devices** (SRAM, NOR flash; up to 3 chip selects nCS[2..4]). Not used in a minimal BMS board. Clock: EMIF_CLK = **VCLK3** domain (default = OSCIN/2; HCLK÷1–16 via system module). Registers at **`0xFCFFE800`** (privileged). Needs PD5 powered (PMM) and pin mux (IOMM; EMIF pins shared with N2HET2/etc.).
**Data memory map:** async CS2 `0x60000000`, CS3 `0x64000000`, CS4 `0x68000000`; SDRAM CS0 `0x80000000–0x87FFFFFF` (see System handoff). Pins: EMIF_DATA[15:0], EMIF_ADDR[12:0], EMIF_BA[1:0], nCS[0], nCS[4:2], nOE, nWE, nWAIT (extended wait), CLK, CKE, nCAS, nRAS, nDQM[1:0].

## Supported memories
- **SDRAM:** SDR only, **16-bit bus**, 1/2/4 banks, 8–11 column bits, CAS latency 2 or 3, 3.3 V LVCMOS; self-refresh and power-down supported. No mobile SDRAM.
- **Async:** 8- or 16-bit width (`ASIZE`: 0 = 8-bit, 1 = 16-bit; ASIZE also decides EMIF_A/BA pin functions — see TRM 17.2.6.1), programmable setup/strobe/hold per read/write, turnaround, optional external wait (nWAIT) with timeout, "select strobe" mode.

## Register map (offset)
08 **SDCR** · 0C **SDRCR** · 10 **CE2CFG** · 14 CE3CFG · 18 CE4CFG · 1C CE5CFG (table lists four async configs; only three chip selects are pinned out) · 20 **SDTIMR** · 3C SDSRETR · 40 INTRAW · 44 INTMSK · 48 INTMSKSET · 4C INTMSKCLR · 68 PMCR (page mode control). Also RCSR/AWCCR (async wait config) near the start of the map.

### SDCR (08h)
`SR`[31] self-refresh · `PD`[30] power-down · `PDWR`[29] refresh during power-down · `NM`[14] narrow mode (1 = 16-bit SDRAM, 0 = 32-bit) · `CL`[11:9] CAS latency (2 or 3) — **writable only if `BIT11_9LOCK`[8] = 1 in the same write** · `IBANK`[6:4] (0 = 1 bank, 1 = 2, 2 = 4) · `PAGESIZE`[2:0] (0 = 8 col bits/256 elements, 1 = 9, 2 = 10, 3 = 11). **Writing SDCR (bank/page/CL) triggers the SDRAM initialization sequence**; to set `SR` without that, use a **byte write to the upper byte**.
### SDRCR (0Ch)
`RR`[12:0] refresh period in EMIF_CLK cycles = `t_refresh_interval × f_EMIF_CLK` (e.g. 64 ms/4096 rows = 15.6 µs → 1560 at 100 MHz; use slightly less).
### SDTIMR (20h) (all values = cycles − 1)
`T_RFC`[31:27], `T_RP`[26:24], `T_RCD`[22:20], `T_WR`[18:16], `T_RAS`[15:12], `T_RC`[11:8], `T_RRD`[6:4] — set from the SDRAM datasheet at the chosen EMIF_CLK.
### CExCFG (async, 10h/14h/18h)
`SS`[31] select-strobe mode · `EW`[30] extend wait (use nWAIT) · `W_SETUP`[29:26] · `W_STROBE`[25:20] · `W_HOLD`[19:17] · `R_SETUP`[16:13] · `R_STROBE`[12:7] · `R_HOLD`[6:4] (each = EMIF_CLK cycles − 1) · `TA`[3:2] minimum turn-around between accesses · `ASIZE`[1:0] bus width. Timing values come from memory datasheet access times: strobe ≥ access time, setup/hold per address/data setup specs.

## SDRAM bring-up (example flow)
1. Power PD5, pin-mux EMIF, set VCLK3 / EMIF_CLK. **To change the EMIF clock after init:** put SDRAM in self-refresh first (`SDCR.SR=1` by upper-byte write), change VCLK3 source/divider, then clear SR (byte write). 2. `SDTIMR` and `SDRCR`. 3. `SDCR` with CL/IBANK/PAGESIZE/NM (+ BIT11_9LOCK=1) — this starts the init sequence. 4. SDRAM is usable at `0x80000000`.
## Async flash/SRAM bring-up
`CExCFG` timing + `ASIZE` + (optional) `EW`/SS; wire RD/nBY to EMIF_nWAIT if using external wait. Access through the chip-select region.
## Interrupts
INTRAW/INTMSK: asynchronous timeout (`AT`), line trap (`LT`), wait-rise (`WR`) — enable via INTMSKSET.

## Gotchas
- SDCR writes restart SDRAM init; use byte-write for SR/PD.
- 16-bit data bus only (NM=1 for 16-bit SDRAM).
- EMIF clock = VCLK3; check it is stable before init.
- Chip-select ranges differ from the POM overlay target window (OTADDR default 0x60 → CS2 area).
