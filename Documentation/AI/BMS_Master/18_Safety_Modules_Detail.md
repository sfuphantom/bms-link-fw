# Safety / Self-Test Modules (register level): CCM-R4F, STC, PBIST, DCC, CRC — TMS570LS12x/11x
**Context:** The device is a lock-step dual Cortex-R4F safety MCU (1oo1D). These modules provide boot-time and run-time diagnostics. All errors surface through the ESM (see 03_VIM_ESM handoff). Typical safety boot order: PLL/clock setup → PBIST (RAM) → STC (CPU LBIST) → RAM auto-init → CCM self-test → enable ECC → application. Control registers are privileged-write.

---
# CCM-R4F — CPU lock-step compare (base `0xFFFFF600`)
- Compares ~900 signals from the two cores (master delayed 2 cycles, checker input delayed 2 cycles → temporal diversity). Comparison starts 6 CPU cycles after reset release, runs without software. Mismatch → ESM flag "CCM-R4F compare" (and also "self-test error" path for redundancy).
- Both CPUs' registers must be initialized to identical values before use (otherwise false compare errors after reset: init all core registers incl. banked/FPU at start-up).
- **Registers:** `00 CCMSR` (`CMPE`[16] compare error, `STC`[8] self-test complete, `STET`[1] self-test error type, `STE`[0] self-test error) · **`04 CCMKEYR` (`MKEY`[3:0]): `0x0` lock-step, `0x6` self-test, `0x9` error-forcing, `0xF` self-test error-forcing** (write other values → undefined; privileged write).
- **Self-test** (MKEY=6): runs Compare Match test (all-1s, all-0s, 0xA…, 0x5… patterns, 4 cycles) then Compare Mismatch test (one bit flipped per position, 2×N patterns). Compare error output to ESM is suppressed during test; faults → ESM "CCM-R4F self-test". Stays in self-test mode after completion — **you must write MKEY=0 to return to lock step**. Poll `CCMSR.STC`.
- Error-forcing modes deliberately generate a compare error to test the ESM path.

---
# STC — CPU self-test (LBIST) (base `0xFFFFE600`)
- Deterministic logic BIST on the CPU; **24 intervals**; STCCLK = HCLK / `STCCLKDIV.CLKDIV` (STCCLKDIV in SYS2 module, offset 08h; set before run). Core is isolated from the bus during test; DMA accesses to TCM held until done. **STC always ends by resetting the CPU** (pass or fail) → check `STCGSTAT` on restart.
- **Registers:** 00 `STCGCR0` (`INTCOUNT`[31:16] number of intervals to run, 1–24; `RS_CNT`[0] 1 = restart from interval 0, 0 = continue; self-clears) · 04 `STCGCR1` (`STC_ENA`[3:0] **0xA = start**) · 08 `STCTPR` (timeout preload, VBUS cycles) · 0C `STC_CADDR` (current ROM address) · 10 `STCCICR` (current interval) · **14 `STCGSTAT` (`TEST_DONE`[0], `TEST_FAIL`[1], timeout/error bits)** · 18 `STCFSTAT` (fail status; failing interval) · 1C–28 `CPU1_CURMISR3..0` · 2C–38 `CPU2_CURMISR3..0` · 3C `STCSCSCR` (signature compare self-check).
- **Flow:** clear `SYSESR` flags → `STCCLKDIV` → `STCGCR0` (intervals, RS_CNT) → `STCTPR` → `STCGCR1 = 0xA`. CPU resets, then in startup code: read `STCGSTAT` (TEST_DONE=1, TEST_FAIL=0 → pass), clear flags, continue boot. Check that the reset cause in SYSESR shows the CPU-reset-by-STC and **don't re-run the test again from that reset path**.
- If running in slices during normal operation, save/restore CPU registers (R0–R15, PC, CPSR, CP15 MPU/ACR/ECC enable, CP13 FPU regs, breakpoint/watchpoint regs) around the reset.
- Duration/coverage trade-off: more intervals = higher coverage (full 24 intervals gives the quoted coverage).

---
# PBIST — memory BIST (base `0xFFFFE400`)
- Runs on-chip ROM algorithms (e.g., March13N, Down1A, Map Column) over RAM groups (list of groups/algorithms: Tables 2-5/2-6 of TRM and device datasheet). **Destroys contents of tested memory** → run before app data exists (or only on RAMs not yet used). Needs PLL stable; PBIST ROM clock = HCLK ÷ ratio (`MSTGCR[9:8]`, e.g. 1 → 1:2).
- **Registers:** 160 `RAMT` (RGS/RDS of failing RAM) · 164 `DLR` (datalogger; 0x14 = ROM mode) · 16C `STR` (run/resume: 2 = resume) · **180 `PACT`** (1 = clocks; 3 = clocks + ROM clock) · 184 `PBISTID` · 188 `OVER` (0 = use RINFO/ALGO from application, 1 = override) · 190 `FSRF0` (fail flag) · 198/19C `FSRC0/1` (fail count) · 1A0/1A4 `FSRA0/1` (first fail address) · 1A8/1B0 `FSRDL0/1` (fail data) · **1C0 `ROM`** (0x3 = load algorithm & RAM info from ROM) · 1C4 `ALGO` (bit mask of algorithms) · 1C8/1CC `RINFOL/U` (bit mask of RAM groups; 28 groups on this device).
- **System-module registers involved:** `MSIENA` (bit 0 enables PBIST), `MSTGCR` (`[3:0]` 0xA = enable self-test, 0x5 = disable; `[9:8]` ROM clock ratio), `MSTDONE`/`MSTCGSTAT.MSTDONE` (poll for completion).
- **Flow (RAM group 3 example, HCLK=160 MHz):** `MSTGCR[9:8]=1` → `MSIENA=1` → `MSTGCR[3:0]=0xA` → wait ≥32 VCLK → `PACT=1` → `OVER=0` → `ALGO=0x54` (algos 3,5,7: March13N, Down1A, Map column) → `RINFOL=0x4`, `RINFOU=0` (group 3) → `ROM=0x3` → `DLR=0x14` → `STR=0x2` → poll MSTDONE → check `FSRF0`: 0 = pass → `PACT=0`, `MSTGCR[3:0]=0x5`; 1 = fail → read RAMT/FSRC/FSRA/FSRDL, optionally resume (STR=2).
- To test all groups, set all RAM-group and algorithm bits (Example 2 in TRM).

---
# DCC1 / DCC2 — dual-clock comparator (bases `0xFFFFEC00` / `0xFFFFF400`)
- Counts **clock0** (reference, with counter0 + tolerance-window `valid0`) vs **clock1** (clock under test, counter1). Both counters start together; when counter0 reaches 0, valid0 starts counting; clock1's counter1 must hit 0 **at the same time as counter0 or while valid0 is counting**. Error if counter1 reaches 0 before counter0 (clock1 too fast / clock0 stuck) or doesn't reach 0 by the time valid0 expires (clock1 too slow / stuck). Error freezes counters (read them for diagnosis).
- **Modes:** continuous (reloads seeds after each OK window) or single-shot (`SINGLESHOT` = 0xA stops when counter0+valid0 reach 0, 0xB when counter1 reaches 0 — "counter1 stop" mode used to measure an unknown frequency).
- **Registers:** 00 `DCCGCTRL` (`DCCENA`[3:0] **0xA enable / 5 disable (stops counters)** · `ERRENA`[7:4] 0xA = error interrupt enable, 5 = off · `SINGLESHOT`[11:8] 0xA = single-shot, 5 = continuous · `DONEENA`[15:12] 0xA = done interrupt) · 04 `DCCREV` · 08 `DCCCNT0SEED` · 0C `DCCVALID0SEED` · 10 `DCCCNT1SEED` · 14 `DCCSTAT` (`ERR`[0], `DONE`[1]) · 18 `DCCCNT0` · 1C `DCCVALID0` · 20 `DCCCNT1` · 24 `DCCCNT1CLKSRC` · 28 `DCCCNT0CLKSRC` (KEY[15:12] = 0xA to select source; clock-source numbers per datasheet).
- **Seeding:** pick `seed0` so that `T_window = seed0 / f0`; `seed1 = f1_expected × seed0 / f0`; `valid0` = tolerance in clock0 cycles (± tolerance window). Both clocks must be running for the duration. Errors raise ESM (DCC1/2 channels per datasheet).
- Typical use: monitor PLL vs OSCIN, HF LPO vs OSCIN; or measure oscillator frequency once at boot.

---
# CRC controller (base `0xFE000000`)
- 2 channels (CH1, CH2) computing **PSA signatures** (64-bit maximal-length, parallel signature analysis) over 8/16/32/64-bit data; compares to preloaded reference per sector. Modes (CRC_CTRL2 `CHx_MODE`): **0 Data capture (no compression, raw data register)**, **1 AUTO** (CRC controller + DMA do everything, interrupts on sector/block done/fail), **2 Semi-CPU** (DMA moves data, CPU checks signature), **3 Full-CPU** (CPU writes data and reads signature; no interrupts/DMA).
- **Registers:** 00 `CRC_CTRL0` (`CH1_PSA_SWREST`[0], `CH2_PSA_SWREST`[8]) · 08 `CRC_CTRL1` (`PWDN`[0]) · 10 `CRC_CTRL2` (`CH1_MODE`[1:0], `CH1_TRACEEN`[4], `CH2_MODE`[9:8], `CH2_TRACEEN`[12]) · 18 `CRC_INTS` / 20 `CRC_INTR` (int enable set/reset) · 28 `CRC_STATUS` · 30 `CRC_INT_OFFSET` (codes: 1/2 CRC fail ch1/ch2, 9/Ah compression complete, 11h/12h overrun, 19h/1Ah underrun, 21h/22h timeout) · 38 `CRC_BUSY` · per channel (ch1 at 40h–7Ch, ch2 at 80h–BCh): `PCOUNT_REG` (pattern count per sector) · `SCOUNT_REG` (sector count) · `CURSEC_REG` · `WDTOPLD` (DMA-request watchdog timeout) · `BCTOPLD` (block-complete timeout) · `PSA_SIGREGL/H` (signature) · `CRC_REGL/H` (reference/CRC value) · `PSA_SECSIGREGL/H` (sector signature) · `RAW_DATAREGL/H`.
- Full-CPU use: reset PSA (`CHx_PSA_SWREST`), set mode 3, write data patterns to the PSA data port, read `PSA_SIGREGL/H`, compare against known-good signature.

## Gotchas
- STC always resets the CPU; ensure startup handles that without looping.
- CCM stays in self-test mode until MKEY=0 is written.
- PBIST overwrites RAM — only on unused RAM or before init.
- DCC needs both clocks running; error state freezes counters until DCC is disabled/restarted (write 5 to DCCENA then re-enable).
- ESM flags from all of these must be checked/cleared after tests.
