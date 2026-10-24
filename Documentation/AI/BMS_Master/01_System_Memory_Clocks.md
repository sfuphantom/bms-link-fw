# System: Memory Map, Clocks, PLL, Reset, RAM Init — TMS570LS12x/11x
**Context:** Dual ARM Cortex-R4F in lock-step (safety MCU), big/little config per device. 1.25 MB flash, 192 KB data RAM (ECC), 32-bit address space. Everything below is from the system module (SYS), SYS2, and PCR register frames.

## Register frames (base addresses)
| Frame | Base |
|---|---|
| **SYS** (primary system control) | `0xFFFFFF00` |
| **SYS2** (secondary: PLLCTL3, VCLKACON1, CLK2CNTRL, CLKSLIP…) | `0xFFFFE100` |
| **PCR** (peripheral central resource: protection, power-down) | `0xFFFFE000` |
| Flash wrapper | `0xFFF87000` |
| ESM `0xFFFFF500` · RTI+DWWD `0xFFFFFC00` · VIM `0xFFFFFE00` (VIM RAM `0xFFF82000`) · VIM parity `0xFFFFFD00` | |
| DMA regs `0xFFFFF000` (DMA RAM `0xFFF80000`) · PBIST `0xFFFFE400` · STC `0xFFFFE600` · DCC1 `0xFFFFEC00` · DCC2 `0xFFFFF400` · CCM-R4 `0xFFFFF600` · RAM ECC even/odd `0xFFFFF800`/`0xFFFFF900` | |
| CRC controller frame | `0xFE000000` |

## Memory map (default)
| Region | Range | Notes |
|---|---|---|
| Flash (instruction, TCM) | `0x00000000–0x0013FFFF` (1.25 MB) | **reset vector at 0x00000000** |
| Data RAM | `0x08000000–0x0802FFFF` (192 KB) | ECC region at `0x08400000+` |
| Flash mirror | `0x20000000` | |
| EMIF CS2/3/4 (async) | `0x60000000 / 0x64000000 / 0x68000000` | |
| EMIF CS0 SDRAM | `0x80000000–0x87FFFFFF` | |
| Flash OTP/ECC/EEPROM (bus2) | `0xF0000000…`, EEPROM bank `0xF0200000` | OTP incl. LPO trim at `0xF00801B4` |
| Peripheral frame 2 (ePWM/eCAP/eQEP, EMIF regs) | `0xFCF78000–0xFCFFFFFF` | ePWM1–7 `0xFCF78C00 + 0x100·n`; eCAP1–6 `0xFCF79300 + 0x100·n`; eQEP1/2 `0xFCF79900/9A00` |
| Peripheral frame 1 (below) | `0xFFF7xxxx` | |
- BMMCR1 `MEM SWAP` can swap flash and RAM address windows (RAM at 0x0, flash at 0x08000000).
- Accessing reserved regions → abort.

### Peripheral register bases (frame 1)
HTU1/2 `0xFFF7A400/A500` · **N2HET1/2 `0xFFF7B800/B900`** · **GIO `0xFFF7BC00`** · **ADC1/2 `0xFFF7C000/C200`** · FlexRay `0xFFF7C800` · I2C `0xFFF7D400` · **DCAN1/2/3 `0xFFF7DC00/DE00/E000`** · **LIN (SCI/LIN) `0xFFF7E400`** · **SCI `0xFFF7E500`** · **MibSPI1 `0xFFF7F400`, SPI2 `F600`, MibSPI3 `F800`, SPI4 `FA00`, MibSPI5 `FC00`**.
### Peripheral RAM bases
MibSPI5 RAM `0xFF0A0000` · MibSPI3 RAM `0xFF0C0000` · MibSPI1 RAM `0xFF0E0000` · DCAN3 RAM `0xFF1A0000` · DCAN2 `0xFF1C0000` · DCAN1 `0xFF1E0000` · ADC2 RAM `0xFF3A0000` · ADC1 RAM `0xFF3E0000` · N2HET2 RAM `0xFF440000` · N2HET1 RAM `0xFF460000` · HTU2 RAM `0xFF4C0000` · HTU1 RAM `0xFF4E0000` · FlexRay TU RAM `0xFF500000`. (Peripheral RAMs have parity/ECC — see RAM init.)

## Clock sources (numbers used in all source-select fields)
0 OSCIN (main oscillator 5–20 MHz; only input to PLLs) · 1 PLL1 (can spread-spectrum) · 2 reserved (never use) · 3 EXTCLKIN1 · 4 LF LPO (~80 kHz, RTI wake-up) · 5 HF LPO (~10 MHz, clock-monitor reference) · 6 PLL2 (no modulation, async domains) · 7 EXTCLKIN2.
After reset: OSCIN is default for GCLK/HCLK; **VCLK = OSCIN/2**. If oscillator invalid at reset, clock detect falls back to HF LPO. PLL disabled by default. LPO trim: read OTP word `0xF00801B4`, bits[31:16] → write to `LPOMONCTL[15:0]`.

## Clock domains
| Domain | Default src | Select reg | Notes |
|---|---|---|---|
| GCLK (CPU) / GCLK2 (lock-step, 2 cycles delayed) | OSCIN | GHVSRC | = HCLK freq, disabled via CDDIS bit0 |
| HCLK | OSCIN | GHVSRC | flash, TCRAM, ESM, DMA; CDDIS bit1 |
| VCLK | OSCIN | GHVSRC | **peripheral bus + register interfaces (SPI/SCI/GIO/ADC/etc.)**; HCLK/1–16 via CLKCNTL.VCLKR; bit2 |
| VCLK2 | OSCIN | GHVSRC | **N2HET, HTU**; HCLK/1–16 via CLKCNTL.VCLK2R; must be integer multiple of VCLK; bit3 |
| VCLK3 | OSCIN | GHVSRC | EMAC/EMIF slaves; bit8 |
| VCLK4 | OSCIN | GHVSRC | ePWM/eCAP/eQEP; bit9 |
| VCLKA1 | VCLK | VCLKASRC | **DCAN1–3 baud generation**; may be as fast as HCLK; bit4 |
| VCLKA2 | VCLK | VCLKASRC | FlexRay; bit5 |
| VCLKA4 | VCLK | VCLKACON1 | EMAC clock, optional ÷1–8 (default ÷2); bit11 |
| RTICLK | VCLK | RCLKSRC | RTI timebase; if not VCLK must be ≤ VCLK/3 (use RTI1DIV); bit6 |
CDDIS/CDDISSET/CDDISCLR disable domains (a domain turns off only when all modules permit). CSDIS/CSDISSET/CSDISCLR disable/enable sources (bit n = source n; clearing bit enables).

## Key SYS registers (offsets from 0xFFFFFF00)
30 CSDIS · 34 CSDISSET · 38 CSDISCLR · 3C CDDIS · 40 CDDISSET · 44 CDDISCLR · **48 GHVSRC** (`GHVWAKE`[27:24] wake source, `HVLPM`[19:16], `GHVSRC`[3:0] current source for GCLK/HCLK/VCLK/VCLK2) · 4C VCLKASRC (VCLKA1S[3:0], VCLKA2S[11:8]) · 50 RCLKSRC · **54 CSVSTAT** (RO, bit n = clock source n valid; a domain is gated while its source isn't valid) · 58 MSTGCR · 5C MINITGCR · 60 MSINENA · 64 MSTFAIL · 68 MSTCGSTAT (`MINIDONE`[8], `MSTDONE`[0]) · 6C MINISTAT · **70 PLLCTL1 · 74 PLLCTL2** · 88 LPOMONCTL · A0 GPREG1 · A8 IMPFASTS / AC IMPFTADD (imprecise fault) · B0–BC SSIR1–4 (software interrupts) · C0 RAMGCR · C4 BMMCR1 · CC CPURSTCR · **D0 CLKCNTL** (`VCLK2R`[27:24], `VCLKR`[19:16], **`PENA`[8] peripheral enable — must be 1 before touching any peripheral**) · D4 ECPCNTL · DC DEVCR1 · E0 SYSECR · **E4 SYSESR** (reset cause flags: OSCRST[14], WDRST[13], …) · E8 SYSTASR · EC GLBSTAT · F0 DEVID · F4 SSIVEC · F8 SSIF.
SYS2: PLLCTL3 (PLL2) · STCLKDIV · CLK2CNTRL · VCLKACON1 · CLKSLIP · EFC_CTLREG.
VCLK divider change order: when **increasing** frequency change VCLK2R first then VCLKR; when **decreasing** frequency change VCLKR first then VCLK2R (keeps VCLK2 an integer multiple of VCLK).

## PLL1 math
`fPLLCLK = fCLKIN / NR × NF / OD / R`
- NR = REFCLKDIV[5:0]+1 (internal ref fINTCLK = fCLKIN/NR must be ≥1 MHz). NF 1–256 encoded in PLLMUL[15:0] as `(NF−1)×256` (NF=90 → 0x5900). OD = ODPLL[2:0]+1 (1–8). R = PLLDIV[4:0]+1 (1–32).
- **VCO output (fCLKIN/NR×NF) must be 150–550 MHz.**
- Modulation (optional): SPRATE (NS = SPRATE+1), SPR_AMOUNT (NV), MULMOD, FM ENA bit in PLLCTL2. Avoid modulation if CAN/UART precision matters; use VCLKA from OSCIN or PLL2.
- **Worked example (20 MHz crystal → 180 MHz GCLK/HCLK):** NR=5, NF=90, OD=2, R=1 → 360 MHz VCO÷2 = 180 MHz. `PLLCTL1 = 0x00045900`, `PLLCTL2 = 0x04C7325B` (0x84C7325B with FM enabled). Field values used: REFCLKDIV=4, PLLMUL=0x5900, ODPLL=1, PLLDIV=0.
- **Lock time:** `512·T_OSCIN + 1024·NR·T_OSCIN` + 6·T_OSCIN to enable.
- **PLL bring-up:** (1) program PLLCTL1/2, (2) `CSDISCLR` bit1 (enable PLL1; PLL2 = bit 6 after PLLCTL3), (3) wait `CSVSTAT` bit1 = 1, (4) *before switching the domain source*, set flash wrapper wait-states/RWAIT for the new HCLK (see flash handoff), (5) write `GHVSRC` = 1 (PLL1), (6) set CLKCNTL VCLKR/VCLK2R. Change ODPLL only before enabling async domains (it gates PLL CLK).
- Monitors: oscillator clock detect (CLKDET), PLL slip detector (→ reset/ESM), external clock monitor, DCC1/DCC2 dual-clock comparators for frequency checks.

## RAM / peripheral memory auto-initialization (required before using ECC/parity RAM)
1. Enable ECC/parity checking first (if used). 2. `MINITGCR[3:0] = 0xA` (key). 3. Set bits in `MSINENA` for each memory. 4. Poll `MINISTAT` bit n per module, or poll `MSTCGSTAT.MINIDONE[8]` for all. 5. `MINITGCR = 0x5` (disable key; any value ≠0xA).
**MSINENA bit map:** 0 = system data RAM(s) 0x08000000–0x0802FFFF · 1 = DMA RAM · 2 = VIM RAM · 3 = N2HET1 RAM · 4 = HTU1 RAM · 5 = DCAN1 RAM · 6 = DCAN2 RAM · 7 = MibSPI1 RAM · 8 = ADC1 RAM · 10 = DCAN3 RAM · 11 = MibSPI3 RAM · 12 = MibSPI5 RAM · 13 = FlexRay TU RAM · 14 = ADC2 RAM · 15 = N2HET2 RAM · 16 = HTU2 RAM.
(Individual modules such as MibSPI and ADC also auto-init after their own reset release.)

## Resets / exceptions
Reset causes (incl. oscillator/PLL failure, digital windowed watchdog, power-on, external, CPU/software) are latched in `SYSESR` (OSCRST[14], WDRST[13], other bits per register); check there after reset. Aborts: prefetch, precise data, imprecise data (IMPFASTS/IMPFTADD show address). System software interrupts via SSIR1–4 (SSIVEC reads the pending vector).

## Gotchas
- **`CLKCNTL.PENA=1` first**, otherwise peripheral register access aborts.
- Don't use clock source 2.
- Flash wait states must be raised before raising HCLK.
- CAN timing: VCLKA must be unmodulated and VCLK ≥ CAN_CLK.
- N2HET/HTU run on VCLK2; ePWM on VCLK4; RTI on RTICLK.
- After reset RAM contents have invalid ECC → run auto-init before enabling ECC reads.
