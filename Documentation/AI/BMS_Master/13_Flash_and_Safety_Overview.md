# Flash Controller + Safety/Diagnostic Modules (summary level) — TMS570LS12x/11x
**Depth note:** Flash section has register-level detail for what matters at start-up. The safety modules (PMM, PBIST, STC, CCM, DCC, CRC, TCRAM) are **overview only** — enough for an AI to know what exists, where it lives, and the boot-time role. Ask for a deeper handoff on any one of them if the project needs it.

# Flash (F021 flash module controller, FMC)
- Single 3.3 V supply; banks/sectors; ECC (SECDED, address bits included) evaluated **in the CPU for main banks** and in the flash wrapper for the EEPROM-emulation bank (bank 7). Read-while-write across banks. Program/erase done by the F021 Flash API (not in this TRM), not by direct register pokes.
- **Flash wrapper registers base `0xFFF87000`** (privileged writes).
- **Power-up defaults:** 1 data wait state, 0 address wait states, pipeline **disabled**, flash write-protected, active power mode. **Boot code must set wait states/pipeline in `FRDCNTL` *before* raising HCLK** (required values per HCLK are in the datasheet).

## FRDCNTL (offset 00h)
`RWAIT`[11:8] random/data read wait states (0–15; reset = 1; in pipeline mode there's always ≥1 even if 0) · `ASWSTEN`[4] address-setup wait state (pipeline mode only) · `ENPIPE`[0] pipeline mode enable. (EEPROM bank 7 has an equivalent register.)
**Frequency-up sequence:** set RWAIT/ASWSTEN/ENPIPE for new HCLK → switch clock source/PLL → run. **Down sequence:** reduce clock first, then reduce wait states.

## ECC / error handling (FEDACCTRL1 etc.)
- `FEDACCTRL1`: `EDACMODE`[19:16] selects error-correction mode for main banks (a specific value, 0x5, covers single-bit errors in OTP/ECC/mirror space — check register description before relying on it); also contains the EDAC enable field and `SUSP_IGNR`[24] (CPU suspend doesn't block error-bit setting when 1).
- Correctable error count/address/position: `FCOR_ERR_CNT/ADD/POS`; uncorrectable: `FUNC_ERR_ADD`; status `FEDACSTATUS`; sector disable `FEDACSDIS`; diagnostic modes (`FDIAGCTRL`, `DIAG_MODE` 1–7) for ECC self-test; address-tag registers for bus-error diagnostic.
- Single-bit → corrected silently (and counted/ESM'd); double-bit → data abort + ESM.
- **Bank/sector info from OTP:** bank-0 info at `0xF0080158` (`NUM_Banks`[7:0], B0–B7 present flags [15:8], sectors in this bank [23:16]); other banks at `0xF0082158, 84158, 86158, 88158, 8A158, 8C158, 8E158` (bank 1–7).
- **LPO trim + max HCLK:** OTP `0xF00801B4`: `HFLPO_TRIM`[31:24], `LFLPO_TRIM`[23:16], `MAX_HCLK`[15:0].
- Power modes & timing: `FBAC` (`BAGP`[15:8] bank active grace period, `VREADST`[7:0], `OTPPROTDIS`[23:16]), `FBFALLBACK`, `FBPRDY`, `FPAC1/2`, `FMAC` (bank select), `FMSTAT` (state machine status); `FBSE/FBBUSY/FBPROT` for the program/erase flow (used by the Flash API).
- ECC bits live in a separate ECC address region (TRM 5.4.1); deliberate-ECC-error test locations are in OTP (TRM 5.4.2.5).

# PMM — Power Management Module
5 switchable logic power domains (PD1–PD5) + 2 memory-only domains (RAM_PD1/2); PMM controls clocks, resets, and power state per domain via PMC/PSCON. PD1 (CPU/always-on core) not switchable; other domains can be shut down to cut leakage. Registers are memory-mapped in the PMM frame (see datasheet/TRM ch.3).

# TCRAM
Tightly-coupled RAM wrapper with ECC (SECDED) on RAM banks at `0x08000000+`; ECC check bits at `0x08400000+`; even/odd bank ECC controllers `0xFFFFF800 / 0xFFFFF900`. Requires RAM auto-init (System handoff, MSINENA bit 0) before enabling ECC reads.

# PBIST — memory built-in self-test
Run-time programmable BIST engine (small coprocessor + ROM of algorithms) that tests on-chip memories (including its own ROM). Registers at `0xFFFFE400`. Typical flow: select RAM group/algorithm → start → poll done → check fail flags; typically run once at boot before using RAM (RAM contents destroyed on the tested memory; run before app data).

# STC — CPU self-test (LBIST)
Deterministic logic BIST on the Cortex-R4F core; **24 test intervals** on this device; runs all or part, resumable; isolates the core from the bus during test (DMA accesses to TCM held). Registers at `0xFFFFE600`. Boot flow: configure interval count/timeout/ROM clock → trigger → CPU resets itself after run → check the STC global status/result registers after restart.

# CCM-R4F — CPU lock-step compare
Compares outputs of the two lock-stepped Cortex-R4F cores (1oo1D). Mismatch → ESM. Has a self-test and error-forcing mode for boot-time check of the CCM itself. Registers at `0xFFFFF600`.

# DCC1 / DCC2 — dual-clock comparator
Counts clock0 (source under test) against clock1 (reference) over programmable windows; checks the frequency ratio stays within tolerance (counter0 seed + valid0 window, counter1 seed). Modes: continuous or single-shot. Error → ESM (DCC1 `0xFFFFEC00`, DCC2 `0xFFFFF400`). Use to verify PLL/oscillator vs HF LPO / OSCIN.

# CRC controller
Two channels, signature (PSA) of memory data (8/16/32/64-bit), full-CPU or auto (DMA-triggered) mode, compares against golden signature. Base `0xFE000000`. Use for flash/RAM integrity checks.

## Gotchas
- Raise flash wait-states **before** raising HCLK.
- Run PBIST/STC at boot **before** enabling ECC reads of tested RAM (PBIST overwrites memory).
- RAM auto-init required prior to ECC/parity checking.
