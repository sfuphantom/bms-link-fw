# MibSPI / SPI — TMS570LS12x/11x
**Context:** 5 SPI modules: MibSPI1, SPI2, MibSPI3, SPI4, MibSPI5. Each = classic SPI ("compatibility mode") plus optional multi-buffer ("Mib") mode with its own RAM, transfer groups (TGs), and triggers. Word length 2–16 bits, 4 independent data formats per module, up to 6–8 chip selects (device-dependent), optional SPIENA handshake pin, optional parallel (multi-SIMO/SOMI) mode (not covered here). Master or slave.

## Base addresses (registers)
MibSPI1 `0xFFF7F400` · SPI2 `0xFFF7F600` · MibSPI3 `0xFFF7F800` · SPI4 `0xFFF7FA00` · MibSPI5 `0xFFF7FC00`. MibSPI RAM base: **MibSPI1 `0xFF0E0000`, MibSPI3 `0xFF0C0000`, MibSPI5 `0xFF0A0000`** (2 KB each; TXRAM +0x000–0x1FF, RXRAM +0x200–0x3FF). Registers accept 8/16/32-bit writes; write 0 to reserved bits.

## Register map (offset)
| Off | Reg | Off | Reg |
|---|---|---|---|
| 00 SPIGCR0 | nRESET (bit0) | 60 INTVECT0 | int vector line 0 |
| 04 SPIGCR1 | SPIEN[24], LOOPBACK[16], POWERDOWN[8], CLKMOD[1], MASTER[0] | 64 INTVECT1 | int vector line 1 |
| 08 SPIINT0 | int enables, DMAREQEN[16], ENABLEHIGHZ[24] | 68 SPIPC9 | (SPI2 only) |
| 0C SPILVL | int level (line 0/1) per source | 6C SPIPMCTRL | parallel/modulo mode |
| 10 SPIFLG | flags (write 1 to clear where applicable) | 70 MIBSPIE | MSPIENA[0], RXRAM_ACCESS (test) |
| 14 SPIPC0 | pin function (1=SPI, 0=GIO) | 74/78 TGITENST/CR | TG int enable set/clear |
| 18–34 SPIPC1–8 | GIO dir/in/out/set/clr/open-drain/pull | 7C/80 TGITLVST/CR | TG int level set/clear |
| 38 SPIDAT0 | TX data (format fixed to SPIDEF-based; compat mode only) | 84 TGINTFLG | TG int flags |
| 3C SPIDAT1 | TX data + control (CSHOLD,WDEL,DFSEL,CSNR) | 90 TICKCNT | periodic tick trigger |
| 40 SPIBUF | RX data + status | 94 LTGPEND | last-TG end ptr, TG_IN_SERVICE |
| 44 SPIEMU | mirror of SPIBUF, read w/o side effects | 98–D4 TGxCTRL | per-TG control (98h+4x) |
| 48 SPIDELAY | master CS/ENA timing | D8–F4 DMAxCTRL | per-DMA-channel control |
| 4C SPIDEF | idle CS pattern | F8–114 ICOUNT | DMAxCOUNT |
| 50/54/58/5C SPIFMT0–3 | data formats | 118 DMACNTLEN | large DMA count |
| 120–130 | UERRCTRL/STAT/ADDR1/ADDR0, RXOVRN_BUF_ADDR | 134 IOLPBKTSTCR | I/O loopback test |
| 138/13C | EXTENDED_PRESCALE1/2 | | |
TX RAM = RAM base + 0x000–0x1FF; RX RAM = RAM base + 0x200–0x3FF.

## Pin roles
- SPICLK: master drives, slave receives. SIMO: master out / slave in. SOMI: slave out / master in.
- **CLKMOD=1** (internal clock): SPIENA = input, SPICS = outputs. **CLKMOD=0**: SPIENA output, SPICS inputs.
- Master: `MASTER=1`, CLKMOD usually 1. **Slave: MASTER=0 AND CLKMOD=0** (anything else undefined).
- SPIENA = optional "slave ready" handshake. SPICS = chip selects (idle level from SPIDEF).
- SPIPC0: `SOMIFUN/SIMOFUN` [31:24]/[23:16] (bit 0 of each is mirrored in bits 11/10 — **SOMIFUN0[11], SIMOFUN0[10] must always be 1 for any SPI transfer**), `CLKFUN`[9], `ENAFUN`[8], `SCSFUN`[7:0]. 0=GIO, 1=SPI function. Also configure IOMM pin mux (see IOMM handoff).
- Slave with SPICS functional and CS high → output Hi-Z and shifting disabled.

## Clocking modes (set per data format via POLARITY/PHASE in SPIFMTx)
| TI POLARITY | TI PHASE | Equivalent standard (CPOL/CPHA) mode | Behavior |
|---|---|---|---|
| 0 | 0 | = standard Mode 1 | clock idles low; data out on rising edge, latched on falling edge (as TI describes it) |
| 0 | 1 | = standard Mode 0 | out half-cycle early/on falling, latched on rising |
| 1 | 0 | = standard Mode 3 | idle high; out falling, latched rising |
| 1 | 1 | = standard Mode 2 | out half-cycle early/on rising, latched falling |
**Caution:** TI's PHASE bit is inverted relative to the common CPHA convention (PHASE=0 here = CPHA=1). To get standard Mode 0 (CPOL=0, CPHA=0) use POLARITY=0, PHASE=1; for standard Mode 3 use POLARITY=1, PHASE=0. (Doc text: PHASE=0 → first bit sent on first clock edge, received on second edge; PHASE=1 → clock delayed half cycle, first bit output before first edge and received on first edge.)
- Changing polarity/phase on the fly: only while no slave is selected; in slave mode with 3/4-pin+ENA, can't switch on-the-fly (reset + reconfigure).

## SPIFMTx (50h–5Ch)
| Bits | Field | Meaning |
|---|---|---|
| 31:24 | WDELAY | extra idle (used if WDEL=1): `WDELAY×Tvclk + 2×Tvclk` |
| 23 | PARPOL | 0 even, 1 odd parity |
| 22 | PARITYENA | add/check parity bit after word |
| 21 | WAITENA | master waits for slave ENA low before shifting; timeout (C2EDELAY) → TIMEOUT flag |
| 20 | SHIFTDIR | 0 MSB first, 1 LSB first |
| 19 | HDUPLEX_ENAx | half duplex (SIMO shared); keep 0 normally |
| 18 | DIS_CS_TIMERS | 1 = skip C2TDELAY/T2CDELAY |
| 17 | POLARITY | 1 = clock idles high |
| 16 | PHASE | see above |
| 15:8 | PRESCALE | **SPICLK = VCLK / (PRESCALE+1)**; PRESCALE=0 → VCLK/2 (master only). Extended prescale regs 138h/13Ch exist for lower rates |
| 4:0 | CHARLEN | word length, legal 2–16 (0x02–0x10) |
- TX data must be **right-justified** in 16 bits; RX data is right-justified, zero-padded. Both independent of shift direction/length.
- Master/slave word length must match.

## SPIDAT1 (3Ch) — compat-mode TX word + control
`[28] CSHOLD` keep CS asserted after transfer until next control+data word loaded (if next CSNR same → CS extended) · `[26] WDEL` apply WDELAY after word · `[25:24] DFSEL` pick SPIFMT0–3 · `[23:16] CSNR` chip-select bitmask (bit n ↔ CS[n]; unimplemented bits read 0) · `[15:0] TXDATA`.
- **Writing only the upper 16 control bits (or fewer) does NOT start a transfer**; writing TXDATA (16-bit write to low half or full 32-bit write) does. Useful for pre-setting format/polarity.
- SPIEN must be 1 before writing. SPIEN=0 clears shift regs, SPIFLG, SPIBUF, TXDATA.
- Write when `TXFULL=0` (or on TXINT).
- **CSNR polarity:** the active/inactive level of CS pins when idle comes from **SPIDEF[7:0]** (1=CS pin high when idle, master only). Typical active-low slaves: SPIDEF=0xFF idle, and CSNR bit n selects CS[n] to be driven active. Verify on a scope.

## SPIBUF (40h) — RX word + status
`[31] RXEMPTY` · `[30] RXOVR` · `[29] TXFULL` · `[28] BITERR` (TX pin read-back mismatch) · `[27] DESYNC` (master: slave ENA dropped early/late) · `[26] PARITYERR` · `[25] TIMEOUT` (ENA never asserted) · `[24] DLENERR` (CS/ENA deasserted before full word) · `[23:16] LCSNR` last CS used · `[15:0] RXDATA`.
- Reading RXDATA clears RXEMPTY→1 flag logic, RXOVR, BITERR etc., and RXINTFLG. Use SPIEMU to peek without clearing.
- Overrun goes into the internal RXBUF; need two SPIBUF reads to reach the overwritten word.

## SPIDELAY (48h) — master timing (VCLK units)
`[31:24] C2TDELAY` CS-active→first clock: `(C2TDELAY+2)×Tvclk` · `[23:16] T2CDELAY` last bit→CS inactive: `(T2CDELAY+1)×Tvclk` · `[15:8] T2EDELAY` data finished→ENA inactive timeout: `T2EDELAY / SPIclk` · `[7:0] C2EDELAY` CS active→ENA active timeout: `C2EDELAY / SPIclk`. (Phase=1 adds ½ SPICLK to the CS→clock delay.)

## Interrupts / flags
- SPIINT0 enables: `TXINTENA`[9], `RXINTENA`[8] (**compat mode only**; not generated in multibuffer mode), `RXOVRNINTENA`[6], `BITERRENA`[4], `DESYNCENA`[3], `PARERRENA`[2], `TIMEOUTENA`[1], `DLENERRENA`[0], `DMAREQEN`[16] (set only after SPIEN=1).
- SPIFLG mirrors those flags + `BUFINITACTIVE`[24]. SPILVL routes each to INT0/INT1 line. INTVECT0/1 read gives pending source (read clears).
- TX DMA req each time word moves into shift reg; RX DMA req each time word lands in SPIBUF.

## Compatibility-mode sequence
1. `SPIGCR0.nRESET=1`. 2. SPIPC0 function bits. 3. SPIGCR1 CLKMOD/MASTER. 4. SPIFMTx. 5. SPIDELAY, SPIDEF. 6. SPIINT0 / SPILVL. 7. `SPIGCR1.SPIEN=1`. 8. Loop: wait TXFULL=0 → write SPIDAT1 (CSNR, DFSEL, CSHOLD, data) → wait RXEMPTY=0 (or RXINT) → read SPIBUF.
(Do all config with SPIEN=0; order then doesn't matter.) LOOPBACK=1 (master+CLKMOD=1 only) gives internal self-test.

## Multi-buffer (MibSPI) mode
- Buffers (up to 128 words, 8–16 TGs device-dep) stored in RAM; each TXRAM word = control+data (same layout as SPIDAT1 plus BUFMODE/LOCK), paired RXRAM word with status+RXDATA. SPIDAT0 is not accessible in this mode.
- **Sequence:** nRESET=1 → `MIBSPIE.MSPIENA=1` → pins/mode/format/delay → **poll `SPIFLG.BUFINITACTIVE`=0** before touching RAM/regs (RAM auto-initializes after nRESET=1) → TG interrupts (TGITENST), error ints (SPIINT0) → `SPIEN=1` → program TGxCTRL (PSTART, trigger source/event), `LTGPEND` → fill TXRAM control fields + data → set BUFMODE per buffer → set `TGENA` (with TRIGEVT=ALWAYS it fires immediately).
- **TXRAM word:** `[31:29] BUFMODE` · `[28] CSHOLD` · `[27] LOCK` · `[26] WDEL` · `[25:24] DFSEL` · `[23:16] CSNR` · `[15:0] TXDATA`.
  BUFMODE: 0 disabled · 1 skip until TXFULL · 2 skip until RXEMPTY · 3 skip until both · 4 continuous (always send, overwrite RX) · 5/6/7 suspend-and-wait variants of 1/2/3. Typical: 1 (single-shot per new data), 4 (continuous polling).
- **RXRAM word:** `RXEMPTY, RXOVR, TXFULL, BITERR, DESYNC, PARITYERR, TIMEOUT, DLENERR, LCSNR, RXDATA` (same as SPIBUF).
- **TGxCTRL (98h+4x):** `[31] TGENA` · `[30] ONESHOT` (clears TGENA after one pass) · `[29] PRST` (retrigger resets pointer; level-trig only) · `[28] TGTD` triggered (RO) · `[23:20] TRIGEVT`: 0 never, 1 rising, 2 falling, 3 both, 5 high-level, 6 low-level, 7 always · `[19:16] TRIGSRC`: 0 off, 1–0xE = EXT0–EXT13 (device-specific, e.g. N2HET pin/GIO), 0xF = TICK · `[14:8] PSTART` start buffer index · `[6:0] PCURRENT`.
  Software trigger: TRIGSRC=0, TRIGEVT=7, ONESHOT=1, then set TGENA.
  TG end = next TG's PSTART−1; **LTGPEND** must give end of last TG (if <all TGs used, create a dummy last TG covering the remaining buffers). Lower TG number = higher priority; `LOCK`=1 prevents preemption by higher-priority TG until next unlocked buffer.
- TICKCNT: down-counter with reload; toggles trigger line (`TICKENA`, `RELOAD`, `CLKCTRL`, `TICKVALUE`).
- DMA: up to 8 TX + 8 RX channels, NOBRK buffers for uninterrupted DMA bursts (ICOUNT).
- Parity on multi-buffer RAM (UERR* regs); slave sends wrong parity/zeros on uncorrectable RAM error.

## Gotchas
- RX data/TXDATA right-aligned; CHARLEN 2–16.
- Slave mode needs both CLKMOD and MASTER = 0.
- BUFINITACTIVE stays 1 while nRESET=0; RAM not usable until nRESET=1 then init completes.
- RXINT/TXINT not generated in multibuffer mode — use TG interrupts/flags.
- GIO mode not allowed with LOOPBACK.
