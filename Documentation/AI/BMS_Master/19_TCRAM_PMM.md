# TCRAM (data RAM wrapper) + PMM (power domains) — TMS570LS12x/11x

# TCRAM
**Context:** CPU data RAM on the Cortex-R4F **BTCM** interface (flash is on ATCM). Split into two banks, **B0TCM (even) / B1TCM (odd)**, each 36-bit wide (32 data + 4 ECC) → a 64-bit word + 8 ECC bits. Size per datasheet (≤256 KB BTCM). **ECC (SECDED) is evaluated by the CPU; the wrapper stores ECC in RAM and reports errors.** Address parity and redundant address-decode checking too.
**Registers:** even RAM ECC wrapper `0xFFFFF800` (B0), odd `0xFFFFF900` (B1). Same layout in each.

## Memory map
Data RAM at `0x08000000` (e.g. 192 KB); ECC space **+4 MB** (`0x08400000`). Unimplemented addresses → error response. Access sizes 8/16/32/64-bit (no bit-wise ops). ECC memory writes disabled by default (only for test).

## Registers (offset)
00 **RAMCTRL** (reset `0x0005000A`): `ECC_DETECT_EN`[3:0] (**0xA = ECC detect enabled (reset default), 0x5 = disabled**) · `ECC_WR_EN`[8] (1 = allow writing ECC memory; test only) · `ADDR_PARITY_DISABLE`[19:16] (0xA disables address-parity check; 5 = enabled) · `ADDR_PARITY_OVERRIDE`[27:24] (0xD inverts parity scheme, test)
04 RAMTHRESHOLD (single-bit error count threshold) · 08 RAMOCCUR (occurrence counter) · 0C RAMINTCTRL (interrupt enables) · **10 RAMERRSTATUS** (`WADDR_PAR_FAIL`[9], `RADDR_PAR_FAIL`[8], `DERR`[5] multi-bit error, `ADDR_COMP_LOGIC_FAIL`[4]; other bits report single-bit/other events (TRM 6.7.5); write 1 to clear) · 14 RAMSERRADDR (single-bit error address) · 1C RAMUERRADDR (uncorrectable) · 30 RAMTEST · 38 RAMADDRDECVECT · 3C RAMPERRADDR · **40 INIT_DOMAIN** (`AUTO_MEM_INIT_DOMAIN`[7:0], reset 0xFF = auto-init enabled for all power domains).
## Using ECC
1. Run RAM auto-init (System handoff, MSINENA bit 0) so ECC bits are valid. 2. **Enable ECC in the CPU** via the Cortex-R4F auxiliary control register (see ARM Cortex-R4F TRM; TI start-up code sets the TCM ECC enable bits) — the wrapper only counts/reports. 3. Keep `RAMCTRL.ECC_DETECT_EN = 0xA`. 4. Single-bit errors are corrected by CPU logic and counted (compare to RAMTHRESHOLD → ESM interrupt); double-bit errors → data abort + ESM group-2 error.
Common gotcha: reading uninitialized RAM with ECC enabled raises ECC errors.

---
# PMM — Power Management Module
**Base `0xFFFF0000`.** Core logic split into domains: **PD1 always on** (CPU, flash interface, clock control, basic peripherals, I/O, 64 KB TCRAM) · **PD2:** POM + test logic · **PD3:** N2HET2, HTU2, ADC2, SCI, I2C, DCAN3, SPI4, DCC2 · **PD4:** FlexRay + FTU · **PD5:** EMAC, EMIF, ePWMx, eCAPx, eQEPx · **RAM_PD1 / RAM_PD2:** 64 KB TCRAM each.
**Practical consequence:** if a peripheral in PD3/PD5 "doesn't respond", check whether its domain is powered/clocked (default state of each domain comes from TI-OTP reset configuration; TI can also permanently disable a domain — then you can't turn it on, and **no error is raised**).
- Domain states: Active / Idle (clocks off, still powered) / Off. **Switching is static only:** change a domain's state once at init; to change again, assert a system reset first.
- Registers (offset from base): 00 `LOGICPDPWRCTRL0` (`LOGICPDON0`[27:24]=PD2, `LOGICPDON1`[19:16]=PD3, `LOGICPDON2`[11:8]=PD4, `LOGICPDON3`[3:0]=PD5; **0xA write = command OFF**; another key value = ON, see TRM Table 3-2) · 10 `MEMPDPWRCTRL0` · 20 `PDCLKDIS` / 24 `PDCLKDISSET` / 28 `PDCLKDISCLR` (per-domain clock disable) · 40/44/48/4C `LOGICPDPWRSTAT0–3` (PD2–PD5 status; `LOGICPDPWR_STAT`, `DOMAIN_ON`) · 80/84 `MEMPDPWRSTAT0/1` · A0 `GLOBALCTRL1` · A8 `GLOBALSTAT` · AC `PRCKEYREG` (PSCON diagnostic compare key) · B8/BC `MPDDCSTAT1/2`, C0 `ISODIAGSTAT` (diagnostic status).
- **Power OFF a logic domain (PD2–5):** `PDCLKDIS` disable all clocks → write 0xA to the domain's nibble in `LOGICPDPWRCTRL0` → poll `LOGICPDPWRSTATx` = 00 (off). RAM-only domains skip the clock step.
- **Power ON:** write the ON key to `LOGICPDONx` (or `MEMPDONx`); PMM restarts clocks automatically if "auto clock enable on wake" is selected, else clear `PDCLKDIS` bit **after** polling `LOGICPDPWRSTATx.DOMAIN_ONx` = on. PMM resets the domain when turning it on.
- Two PSCONs (primary + diagnostic, lock-step, with compare modes) control each domain for safety; compare errors → ESM.
- PMM generates nPORRST / nRST / nPRST / nTRST per domain.
