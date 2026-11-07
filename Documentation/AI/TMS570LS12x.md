# TMS570LS12x/11x Technical Reference — Combined Handoff (BMS_Master)

TMS570LS12x/11x Technical Reference (BMS_Master.pdf, 2030 pp.)
**What this is:** Condensed, AI-oriented extracts of a 16/32-bit Cortex-R4F safety MCU reference manual (Hercules TMS570LS12x/11x). Manufacturer/package/marketing content removed; register addresses, bit fields, formulas, init sequences and gotchas kept. All base addresses are 32-bit, **big-endian** MCU. Peripheral clocks must be enabled (`SYS.CLKCNTL.PENA=1`) and pins muxed (IOMM) before any peripheral works.

## Read order for a typical embedded task
| Section | Read when you need… |
|---|---|
| §01 | memory map, base addresses, PLL/clock domains, RAM init, reset causes |
| §11 | pin function selection, kick unlock, SPI/SCI/I2C pin muxing |
| §02 | periodic tick, watchdog |
| §03 | interrupts, ISR vectoring, error pin/ESM |
| §10 | GPIO |
| §05 | CAN bus (3 controllers) |
| §06 | SPI (5 modules) incl. multi-buffer mode |
| §07 | UART (SCI), LIN |
| §04 | I2C |
| §08 | analog inputs (2 × 12-bit) |
| §12 | DMA channels / requests |
| §09 | N2HET timer, PWM, capture (HTU only mentioned) |
| §13 | flash wait states/ECC (safety modules: see §18, §19) |
| §14 | PWM generation, dead-band, trip-zone, ADC triggering |
| §15 | input capture / period+duty measurement / single APWM |
| §16 | quadrature encoder position/speed |
| §17 | N2HET→RAM transfer unit |
| §18 | register-level CCM, STC (LBIST), PBIST, DCC, CRC |
| §19 | RAM ECC wrapper and power domains (which peripherals live in PD3/PD5) |
| §20 | flash→RAM calibration overlay |
| §21 | external SDRAM/async memory |
| §22 | Ethernet MAC/MDIO, descriptors, init |
| §23 | FlexRay controller + FTU (structural) |

## Depth levels
- **Full register-level:** 01–08, 10–12, 14–22.
- **Structural/medium:** 09 (N2HET: instruction opcodes not in manual), 23 (FlexRay: states, access flow, register map; protocol config field values not expanded), 13 (flash only, moderate).
- **Whole manual now covered**; a few sub-topics inside files are summarized (noted in each file).

## Known caveats
- Peripheral-to-VIM-channel mapping, DMA request-line mapping, ESM channel list, pin/ball names, flash wait-state-vs-HCLK table and max clocks are in the **device datasheet**, not this TRM.
- Figures were interpreted from the surrounding text; verify timing diagrams on hardware.

## Contents
- [01. System: Memory Map, Clocks, PLL, Reset, RAM Init](#01-system-memory-map-clocks-pll-reset-ram-init)
- [02. RTI (Real-Time Interrupt) + Digital Windowed Watchdog](#02-rti-real-time-interrupt--digital-windowed-watchdog)
- [03. VIM (Vectored Interrupt Manager) + ESM (Error Signaling Module)](#03-vim-vectored-interrupt-manager--esm-error-signaling-module)
- [04. I2C](#04-i2c)
- [05. DCAN (CAN 2.0A/B)](#05-dcan-can-20ab)
- [06. MibSPI / SPI](#06-mibspi--spi)
- [07. SCI (UART) and SCI/LIN](#07-sci-uart-and-scilin)
- [08. ADC (12-bit SAR)](#08-adc-12-bit-sar)
- [09. N2HET (High-End Timer)](#09-n2het-high-end-timer)
- [10. GIO (General-Purpose I/O)](#10-gio-general-purpose-io)
- [11. IOMM (I/O Multiplexing and Control)](#11-iomm-io-multiplexing-and-control)
- [12. DMA Controller](#12-dma-controller)
- [13. Flash Controller (F021 FMC)](#13-flash-controller-f021-fmc)
- [14. ePWM (Enhanced PWM)](#14-epwm-enhanced-pwm)
- [15. eCAP (Enhanced Capture)](#15-ecap-enhanced-capture)
- [16. eQEP (Enhanced Quadrature Encoder Pulse)](#16-eqep-enhanced-quadrature-encoder-pulse)
- [17. HTU (High-End Timer Transfer Unit)](#17-htu-high-end-timer-transfer-unit)
- [18. Safety / Self-Test Modules (register level): CCM-R4F, STC, PBIST, DCC, CRC](#18-safety--self-test-modules-register-level-ccm-r4f-stc-pbist-dcc-crc)
- [19. TCRAM (data RAM wrapper) + PMM (power domains)](#19-tcram-data-ram-wrapper--pmm-power-domains)
- [20. POM (Parameter Overlay Module)](#20-pom-parameter-overlay-module)
- [21. EMIF (External Memory Interface)](#21-emif-external-memory-interface)
- [22. EMAC + MDIO (10/100 Ethernet MAC)](#22-emac--mdio-10100-ethernet-mac)
- [23. FlexRay Controller + FlexRay Transfer Unit (FTU)](#23-flexray-controller--flexray-transfer-unit-ftu)

---

## 01. System: Memory Map, Clocks, PLL, Reset, RAM Init

**Context:** Dual ARM Cortex-R4F in lock-step (safety MCU), big/little config per device. 1.25 MB flash, 192 KB data RAM (ECC), 32-bit address space. Everything below is from the system module (SYS), SYS2, and PCR register frames.

#### Register frames (base addresses)
| Frame | Base |
|---|---|
| **SYS** (primary system control) | `0xFFFFFF00` |
| **SYS2** (secondary: PLLCTL3, VCLKACON1, CLK2CNTRL, CLKSLIP…) | `0xFFFFE100` |
| **PCR** (peripheral central resource: protection, power-down) | `0xFFFFE000` |
| Flash wrapper | `0xFFF87000` |
| ESM `0xFFFFF500` · RTI+DWWD `0xFFFFFC00` · VIM `0xFFFFFE00` (VIM RAM `0xFFF82000`) · VIM parity `0xFFFFFD00` | |
| DMA regs `0xFFFFF000` (DMA RAM `0xFFF80000`) · PBIST `0xFFFFE400` · STC `0xFFFFE600` · DCC1 `0xFFFFEC00` · DCC2 `0xFFFFF400` · CCM-R4 `0xFFFFF600` · RAM ECC even/odd `0xFFFFF800`/`0xFFFFF900` | |
| CRC controller frame | `0xFE000000` |

#### Memory map (default)
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

##### Peripheral register bases (frame 1)
HTU1/2 `0xFFF7A400/A500` · **N2HET1/2 `0xFFF7B800/B900`** · **GIO `0xFFF7BC00`** · **ADC1/2 `0xFFF7C000/C200`** · FlexRay `0xFFF7C800` · I2C `0xFFF7D400` · **DCAN1/2/3 `0xFFF7DC00/DE00/E000`** · **LIN (SCI/LIN) `0xFFF7E400`** · **SCI `0xFFF7E500`** · **MibSPI1 `0xFFF7F400`, SPI2 `F600`, MibSPI3 `F800`, SPI4 `FA00`, MibSPI5 `FC00`**.
##### Peripheral RAM bases
MibSPI5 RAM `0xFF0A0000` · MibSPI3 RAM `0xFF0C0000` · MibSPI1 RAM `0xFF0E0000` · DCAN3 RAM `0xFF1A0000` · DCAN2 `0xFF1C0000` · DCAN1 `0xFF1E0000` · ADC2 RAM `0xFF3A0000` · ADC1 RAM `0xFF3E0000` · N2HET2 RAM `0xFF440000` · N2HET1 RAM `0xFF460000` · HTU2 RAM `0xFF4C0000` · HTU1 RAM `0xFF4E0000` · FlexRay TU RAM `0xFF500000`. (Peripheral RAMs have parity/ECC — see RAM init.)

#### Clock sources (numbers used in all source-select fields)
0 OSCIN (main oscillator 5–20 MHz; only input to PLLs) · 1 PLL1 (can spread-spectrum) · 2 reserved (never use) · 3 EXTCLKIN1 · 4 LF LPO (~80 kHz, RTI wake-up) · 5 HF LPO (~10 MHz, clock-monitor reference) · 6 PLL2 (no modulation, async domains) · 7 EXTCLKIN2.
After reset: OSCIN is default for GCLK/HCLK; **VCLK = OSCIN/2**. If oscillator invalid at reset, clock detect falls back to HF LPO. PLL disabled by default. LPO trim: read OTP word `0xF00801B4`, bits[31:16] → write to `LPOMONCTL[15:0]`.

#### Clock domains
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

#### Key SYS registers (offsets from 0xFFFFFF00)
30 CSDIS · 34 CSDISSET · 38 CSDISCLR · 3C CDDIS · 40 CDDISSET · 44 CDDISCLR · **48 GHVSRC** (`GHVWAKE`[27:24] wake source, `HVLPM`[19:16], `GHVSRC`[3:0] current source for GCLK/HCLK/VCLK/VCLK2) · 4C VCLKASRC (VCLKA1S[3:0], VCLKA2S[11:8]) · 50 RCLKSRC · **54 CSVSTAT** (RO, bit n = clock source n valid; a domain is gated while its source isn't valid) · 58 MSTGCR · 5C MINITGCR · 60 MSINENA · 64 MSTFAIL · 68 MSTCGSTAT (`MINIDONE`[8], `MSTDONE`[0]) · 6C MINISTAT · **70 PLLCTL1 · 74 PLLCTL2** · 88 LPOMONCTL · A0 GPREG1 · A8 IMPFASTS / AC IMPFTADD (imprecise fault) · B0–BC SSIR1–4 (software interrupts) · C0 RAMGCR · C4 BMMCR1 · CC CPURSTCR · **D0 CLKCNTL** (`VCLK2R`[27:24], `VCLKR`[19:16], **`PENA`[8] peripheral enable — must be 1 before touching any peripheral**) · D4 ECPCNTL · DC DEVCR1 · E0 SYSECR · **E4 SYSESR** (reset cause flags: OSCRST[14], WDRST[13], …) · E8 SYSTASR · EC GLBSTAT · F0 DEVID · F4 SSIVEC · F8 SSIF.
SYS2: PLLCTL3 (PLL2) · STCLKDIV · CLK2CNTRL · VCLKACON1 · CLKSLIP · EFC_CTLREG.
VCLK divider change order: when **increasing** frequency change VCLK2R first then VCLKR; when **decreasing** frequency change VCLKR first then VCLK2R (keeps VCLK2 an integer multiple of VCLK).

#### PLL1 math
`fPLLCLK = fCLKIN / NR × NF / OD / R`
- NR = REFCLKDIV[5:0]+1 (internal ref fINTCLK = fCLKIN/NR must be ≥1 MHz). NF 1–256 encoded in PLLMUL[15:0] as `(NF−1)×256` (NF=90 → 0x5900). OD = ODPLL[2:0]+1 (1–8). R = PLLDIV[4:0]+1 (1–32).
- **VCO output (fCLKIN/NR×NF) must be 150–550 MHz.**
- Modulation (optional): SPRATE (NS = SPRATE+1), SPR_AMOUNT (NV), MULMOD, FM ENA bit in PLLCTL2. Avoid modulation if CAN/UART precision matters; use VCLKA from OSCIN or PLL2.
- **Worked example (20 MHz crystal → 180 MHz GCLK/HCLK):** NR=5, NF=90, OD=2, R=1 → 360 MHz VCO÷2 = 180 MHz. `PLLCTL1 = 0x00045900`, `PLLCTL2 = 0x04C7325B` (0x84C7325B with FM enabled). Field values used: REFCLKDIV=4, PLLMUL=0x5900, ODPLL=1, PLLDIV=0.
- **Lock time:** `512·T_OSCIN + 1024·NR·T_OSCIN` + 6·T_OSCIN to enable.
- **PLL bring-up:** (1) program PLLCTL1/2, (2) `CSDISCLR` bit1 (enable PLL1; PLL2 = bit 6 after PLLCTL3), (3) wait `CSVSTAT` bit1 = 1, (4) *before switching the domain source*, set flash wrapper wait-states/RWAIT for the new HCLK (see flash handoff), (5) write `GHVSRC` = 1 (PLL1), (6) set CLKCNTL VCLKR/VCLK2R. Change ODPLL only before enabling async domains (it gates PLL CLK).
- Monitors: oscillator clock detect (CLKDET), PLL slip detector (→ reset/ESM), external clock monitor, DCC1/DCC2 dual-clock comparators for frequency checks.

#### RAM / peripheral memory auto-initialization (required before using ECC/parity RAM)
1. Enable ECC/parity checking first (if used). 2. `MINITGCR[3:0] = 0xA` (key). 3. Set bits in `MSINENA` for each memory. 4. Poll `MINISTAT` bit n per module, or poll `MSTCGSTAT.MINIDONE[8]` for all. 5. `MINITGCR = 0x5` (disable key; any value ≠0xA).
**MSINENA bit map:** 0 = system data RAM(s) 0x08000000–0x0802FFFF · 1 = DMA RAM · 2 = VIM RAM · 3 = N2HET1 RAM · 4 = HTU1 RAM · 5 = DCAN1 RAM · 6 = DCAN2 RAM · 7 = MibSPI1 RAM · 8 = ADC1 RAM · 10 = DCAN3 RAM · 11 = MibSPI3 RAM · 12 = MibSPI5 RAM · 13 = FlexRay TU RAM · 14 = ADC2 RAM · 15 = N2HET2 RAM · 16 = HTU2 RAM.
(Individual modules such as MibSPI and ADC also auto-init after their own reset release.)

#### Resets / exceptions
Reset causes (incl. oscillator/PLL failure, digital windowed watchdog, power-on, external, CPU/software) are latched in `SYSESR` (OSCRST[14], WDRST[13], other bits per register); check there after reset. Aborts: prefetch, precise data, imprecise data (IMPFASTS/IMPFTADD show address). System software interrupts via SSIR1–4 (SSIVEC reads the pending vector).

#### Gotchas
- **`CLKCNTL.PENA=1` first**, otherwise peripheral register access aborts.
- Don't use clock source 2.
- Flash wait states must be raised before raising HCLK.
- CAN timing: VCLKA must be unmodulated and VCLK ≥ CAN_CLK.
- N2HET/HTU run on VCLK2; ePWM on VCLK4; RTI on RTICLK.
- After reset RAM contents have invalid ECC → run auto-init before enabling ECC reads.

---

## 02. RTI (Real-Time Interrupt) + Digital Windowed Watchdog

**Context:** RTI = OS tick / periodic timer with two counter blocks and four compare channels (interrupt or DMA request each). Contains the **DWD/DWWD watchdog** (digital windowed watchdog) that can reset or NMI the CPU. Base address **`0xFFFFFC00`** (privileged writes for control regs). Clock = RTICLK (default VCLK; if another source, RTICLK ≤ VCLK/3 — set via RCLKSRC.RTI1DIV in SYS).

#### Counter blocks (0 and 1)
Each block: 32-bit up counter (UC) + 32-bit free-running counter (FRC) + compare-up register (CPUC).
`UC` counts RTICLK; when `UC == CPUC` → `FRC++` and `UC = 0`. So
`f_FRC = f_RTICLK / (CPUC + 1)` (CPUC=0 → /2³²+1: avoid CPUC=0).
FRC overflow raises overflow interrupt (not for OS tick). Block 0 can instead be clocked by FlexRay NTU (TBEXT) — ignore for BMS.
Capture: external events (VIM capture sources 0/1) latch UC/FRC into CAUC/CAFRC (select in RTICAPCTRL).

#### Compare channels (4)
`RTICOMPy` is compared against FRC0 or FRC1 (`RTICOMPCTRL`: COMPSEL0[0], COMPSEL1[4], COMPSEL2[8], COMPSEL3[12]; 0 = FRC0, 1 = FRC1). On match: INTy flag set → VIM interrupt (REQ[2..5]) or DMA request (REQ 12,13,18,19 if enabled) and `RTICOMPy += RTIUDCPy` automatically (periodic tick).
`T_period = T_RTICLK × (CPUC+1) × UDCP`.
**Periodic tick recipe:** `RTICPUC0 = prescale−1`; `RTICOMP0 = RTIFRC0 + period`; `RTIUDCP0 = period`; `RTICOMPCTRL` COMPSEL0=0; `RTISETINTENA |= 1<<0`; `RTIGCTRL.CNT0EN=1`. ISR: clear flag by writing 1 to `RTIINTFLAG` bit0.

#### Register map (offset)
00 RTIGCTRL (`CNT0EN`[0], `CNT1EN`[1], `COS`[15], `NTUSEL`[19:16]) · 04 RTITBCTRL (`TBEXT`[0], `INC`[1]) · 08 RTICAPCTRL (CAPCNTR0[0], CAPCNTR1[1]) · 0C RTICOMPCTRL · 10 RTIFRC0 · 14 RTIUC0 · 18 RTICPUC0 · 20 RTICAFRC0 · 24 RTICAUC0 · 30 RTIFRC1 · 34 RTIUC1 · 38 RTICPUC1 · 40 RTICAFRC1 · 44 RTICAUC1 · 50 RTICOMP0 · 54 RTIUDCP0 · 58 RTICOMP1 · 5C RTIUDCP1 · 60 RTICOMP2 · 64 RTIUDCP2 · 68 RTICOMP3 · 6C RTIUDCP3 · 70 RTITBLCOMP · 74 RTITBHCOMP · **80 RTISETINTENA · 84 RTICLEARINTENA · 88 RTIINTFLAG** · 90 RTIDWDCTRL · 94 RTIDWDPRLD · 98 RTIWDSTATUS · 9C RTIWDKEY · A0 RTIDWDCNTR · A4 RTIWWDRXNCTRL · A8 RTIWWDSIZECTRL · AC RTIINTCLRENABLE · B0–BC RTICOMP0CLR–3CLR.
Bits in SETINTENA/CLEARINTENA/INTFLAG: compare INT0–INT3 = bits 0–3; `TBINT`[16]; `OVL0INT`[17]; `OVL1INT`[18]; SETDMA0–3 (DMA enables) at bits 8–11 (SETDMA3 = bit 11). Clear INTFLAG bits by writing 1. Writes to reserved offsets can clear a pending RTI interrupt.

#### Watchdog (DWD) and windowed watchdog (DWWD)
- Disabled after power-on reset. **Enable by writing `0xA98559DA` to `RTIDWDCTRL`** (reset value `0x5312ACED` = disabled). Once enabled it cannot be disabled by software.
- Timeout: `t_exp = (DWDPRLD + 1) × 2¹³ / f_RTICLK`, `RTIDWDPRLD[11:0]` (0–4095). Preload can only be changed while the watchdog is disabled.
- **Service ("pet") sequence:** write **`0xE51A`** then **`0xA35C`** to `RTIWDKEY` (two separate writes, in order). Any other value / wrong order → reset or NMI (e.g., `0x2345`). Key register reset value is A35C.
- **Windowed (DWWD):** window = last X% of the timeout; servicing **outside the window** (too early) or not at all (too late) triggers the reaction. `RTIWWDSIZECTRL`: `0x5`=100% (default, = plain timeout watchdog), `0x50`=50%, `0x500`=25%, `0x5000`=12.5%, `0x50000`=6.25%, `0x500000`=3.125%. `RTIWWDRXNCTRL.WWDRXN`: `0x5` (default) = **reset**; other value = NMI to CPU. Window size and reaction can be changed after enable; take effect after next service.
- Status: `RTIWDSTATUS` — `DWWD ST`[5] window violation, `END TIME VIOL`[4], `START TIME VIOL`[3], plus key-sequence and timeout flags; clear by writing 1. In NMI mode the NMI handler must clear flags then service the watchdog.
- Reset cause shows in SYS `SYSESR.WDRST`[13].
- Halt-debug: counters stop unless `COS=1`; watchdog counter also halts per debug rules.

#### Low-power
RTI clocked from LF LPO (80 kHz) can wake the device from low-power mode via compare interrupts.

#### Gotchas
- Control writes are privileged-mode only.
- Don't set CPUC=0.
- Compare must be re-armed by UDCP (automatic) — if UDCP=0 period is 2³² counts.
- Watchdog enable key is a 32-bit write; sequence keys are 16-bit values in two writes.
- If using non-VCLK RTICLK, honor RTICLK ≤ VCLK/3.

---

## 03. VIM (Vectored Interrupt Manager) + ESM (Error Signaling Module)

**Context:** The VIM is the interrupt controller between all peripheral interrupt requests and the Cortex-R4F (IRQ/FIQ, optional hardware vectoring). The ESM collects safety-related errors (ECC/parity faults, clock/PLL failures, lock-step compare errors, etc.) and responds with interrupts and/or the external **ERROR pin**. Peripheral-to-channel (INT_REQ) and ESM-channel assignments are device-specific and live in the **datasheet**, not in the TRM.

### VIM
**Base `0xFFFFFE00`; vector table RAM `0xFFF82000`; parity registers frame `0xFFFFFD00`.**

#### Structure
- 128 request inputs (INT_REQ0–126; 127 reserved) → programmable channel mapping (`CHANMAPx[6:0]`, 4 per 32-bit `CHANCTRL[0..31]` register) → per-channel enable → FIQ/IRQ level select → priority encoder. **Lower channel number = higher priority** within IRQ and within FIQ; FIQ beats IRQ.
- **Channel 0 = ESM high-level interrupt, Channel 1 = other NMI. Both hard-wired, always FIQ, cannot be masked, cannot be remapped.** Channel 127 has no vector — don't use.
- Same request may map to several channels (priority can then be changed dynamically via enable/clear). Default after reset: INT_REQn → CHANn.
- Phantom vector = entry 0 (spurious/no-pending).

#### Vector table (VIM RAM, 128 × 32-bit)
`0xFFF82000` phantom · `0xFFF82004` channel 0 · `0xFFF82008` channel 1 · … · `0xFFF821FC` channel 126. Entry = ISR address (`ISR addr = 0xFFF82000 + 4×(channel+1)`). Parity-protected: **use only 32-bit accesses**. Set `FBPARERR` (fall-back ISR address) **before** filling the table. Initialize VIM RAM via system RAM auto-init (MSINENA bit 2) if parity is enabled.

#### Registers (offset from 0xFFFFFE00)
00 IRQINDEX · 04 FIQINDEX · 10–1C FIRQPR0–3 (1 = FIQ, 0 = IRQ per channel; ch0/1 ignored) · 20–2C INTREQ0–3 (pending, read-only; unaffected by enable) · **30–3C REQENASET0–3 · 40–4C REQENACLR0–3** (channel enable set/clear; 32 channels per register) · 50–5C WAKEENASET0–3 · 60–6C WAKEENACLR0–3 (wake-from-low-power enables) · **70 IRQVECREG · 74 FIQVECREG** (address of pending ISR) · 78 CAPEVT (RTI capture sources) · 80+ CHANCTRL0–31 (channel map). Parity frame (`0xFFFFFD00`): `+0xEC` PARFLG · `+0xF0` PARCTL · `+0xF4` ADDERR · `+0xF8` FBPARERR (privileged writes).

#### Enabling interrupts
1. Map/leave default channel; write ISR address to the vector table entry. 2. `FIRQPR` choose IRQ vs FIQ. 3. `REQENASET` bit for channel. 4. Enable at CPU: clear I (0x80) / F (0x40) bits in CPSR (privileged). 5. Peripheral enables its own interrupt.

#### Two handling styles
- **Register-vectored (default):** exception vector 0x18 (IRQ) / 0x1C (FIQ) holds `LDR PC,[PC,#-0x1B0]` → loads **IRQVECREG (0xFFFFFE70) / FIQVECREG (0xFFFFFE74)** → jumps to ISR.
- **Hardware-vectored (VIC port):** set CP15 SCTLR bit 24 (VE) = 1; CPU takes the vector straight from VIM (IRQ only).
- Index style: read `IRQINDEX`/`FIQINDEX` (channel+1) and dispatch from a table (legacy/fast FIQ).
- The peripheral clears its own flag; VIM does not clear it. Reading IRQVECREG/IRQINDEX marks the interrupt serviced.
- ISRs may be ARM or Thumb mode vectors per ISR address LSB convention of the compiler (TI uses `#pragma INTERRUPT`).

### ESM
**Base `0xFFFFF500`.** 128 error channels in 3 groups:
- **Group 1 (channels 0–63, low severity):** configurable interrupt (level high/low) and configurable influence on ERROR pin.
- **Group 2 (32, high severity):** predefined → NMI-like high-level interrupt (VIM ch 0) + ERROR pin (not maskable).
- **Group 3 (32, high severity):** no interrupt; CPU abort + ERROR pin (CPU diagnostics).

#### Registers
| Addr | Reg | Purpose |
|---|---|---|
| F500 | ESMEEPAPR1 (aka ESMIEPSR1) | enable ERROR-pin influence, group1 ch 0–31 |
| F504 | ESMDEPAPR1 (aka ESMIEPCR1) | disable ERROR-pin influence, ch 0–31 |
| F508 / F50C | ESMIESR1 / ESMIECR1 | interrupt enable set/clear (ch 0–31) |
| F510 / F514 | ESMILSR1 / ESMILCR1 | level set (high priority) / clear (low), ch 0–31 |
| F518 | ESMSR1 | status group1 ch 0–31 (write 1 to clear) |
| F51C | ESMSR2 | status group2 |
| F520 | ESMSR3 | status group3 |
| F524 | ESMEPSR | ERROR pin status (1 = pin not active/high) |
| F528 / F52C | ESMIOFFHR / ESMIOFFLR | pending high/low-level interrupt offset (channel+1; 0 = none) |
| F530 / F534 | ESMLTCR / ESMLTCPR | low-time counter / preload |
| F538 | ESMEKR | error key |
| F53C | ESMSSR2 | group2 shadow status (survives warm reset) |
| F540–F54C | ESMIEPSR4 / IEPCR4 / IESR4 / IECR4 | group1 ch 32–63: pin-influence set/clr, int enable set/clr |
| F550 / F554 | ESMILSR4 / ESMILCR4 | level set/clear ch 32–63 |
| F558 | ESMSR4 | status group1 ch 32–63 |

#### Behavior
- **ERROR pin is active-low;** Hi-Z during PORRST. On any enabled error it is driven low and stays low for `t = T_VCLK × (ESMLTCPR + 1)`, restarted by each new failure. To release it: write **`0x5` to ESMEKR** (during/after low time) — otherwise it stays low until power-on reset. If a second failure arrives after a release request, write `5h` then `0` to EKR so the pin pulses again.
- **Force-error test:** require `ESMEPSR=1`; write **`0xA` to ESMEKR** to force the pin low; write `0` to return to normal (low-time counter then runs). (The doc text and figures differ on 5h/Ah for forcing; the EKEY field description says `5h` = reset pin, `Ah` = force error.)
- Flags: status bits latch until cleared (write 1). Warm reset (RST) keeps ESMSR1/SR4/SSR2/SR3/EPSR; ESMSR2 is cleared by RST (use ESMSSR2) and cleared when its vector is read via ESMIOFFHR. PORRST clears everything.
- **ISR:** high-level (VIM ch 0): read `ESMIOFFHR` → offset identifies channel (group2 first, then group1 high-level); low-level interrupt arrives on a datasheet-defined VIM channel: read `ESMIOFFLR`. Then clear status (ESMSR1/4).

#### Recommended init
1. After power-up, force/test ERROR pin with external monitor. 2. Initialize VIM RAM: map ESM low and high ISRs to their VIM channels. 3. Enable in VIM and CPU. 4. Assign group1 channels to high/low (`ESMILSR1/4`). 5. Enable interrupts (`ESMIESR1/4`) and ERROR-pin influence (`ESMIEPSR`…). 6. Set `ESMLTCPR`. 7. Clear stale flags from previous reset (read ESMSR*, ESMSSR2).

#### Gotchas
- Group 2/3 events can't be masked; handle them (reset or fail-safe).
- Reading ESMIOFFLR/ESMIOFFHR does not clear group1 status flags.
- VIM channel 0 is the ESM high-level interrupt — an unhandled ESM NMI loops forever.
- Parity/ECC faults in peripheral RAMs, VIM, flash, clock monitor, and CCM compare all surface in the ESM (check datasheet channel table).

---

## 04. I2C

**Context:** One I2C module (base **`0xFFF7D400`**), Philips/NXP I2C-compatible, master/slave, 7-bit or 10-bit addressing, standard/fast mode. Clock from VCLK (VBUS_CLK). Registers are 16-bit (upper half reads 0). Pins SDA/SCL are open-drain (external pull-ups required); mux in IOMM.

#### Clocking
```
ModuleClk = VCLK / (I2CPSC + 1)          must be 6.7 – 13.3 MHz
MasterClk (SCL) = ModuleClk / ((I2CCKL + d) + (I2CCKH + d))
d = 7 if I2CPSC=0, 6 if I2CPSC=1, 5 if I2CPSC>1
```
- `I2CPSC` is only latched when `IRS`=0→1 (prescaler must be written while in reset). Actual SCL is slower than formula (rise/fall time, sync latency, clock stretching).
- Duty: set CKL ≈ CKH (for fast-mode use CKL > CKH ≈ 1.6:1 to meet tLOW/tHIGH).

#### Register map (offset)
00 I2COAR (own addr) · 04 I2CIMR (int mask) · 08 I2CSTR (status) · 0C I2CCKL · 10 I2CCKH · 14 I2CCNT (byte count) · 18 I2CDRR (RX data) · 1C I2CSAR (slave addr) · 20 I2CDXR (TX data) · **24 I2CMDR (mode)** · 28 I2CIVR (int vector) · 2C I2CEMDR · 30 I2CPSC · 34/38 PID1/2 · 3C I2CDMACR · 48 I2CPFNC (pin function: 1 = I2C, 0 = GPIO) · 4C PDIR · 50 DIN · 54 DOUT · 58 DSET · 5C DCLR · 60 PDR (open drain) · 64 PDIS (pull disable) · 68 PSEL (pull select) · 6C SRS (slew rate).

#### I2CMDR (24h) bits
15 `NACKMOD` (1 = send NACK on next received byte) · 14 `FREE` · 13 **`STT`** (start; hw-cleared after START sent) · 11 **`STP`** (stop; hw-cleared) · 10 **`MST`** (1 = master) · 9 **`TRX`** (1 = transmit) · 8 `XA` (1 = 10-bit addr) · 7 `RM` (repeat mode, master) · 6 `DLB` (digital loopback) · 5 **`IRS`** (**0 = module in reset, 1 = running**) · 4 `STB` (start-byte mode) · 3 `FDF` (free data format) · 2:0 `BC` (bits per byte; 0 = 8).

#### I2CSTR (08h) flags
14 `SDIR` slave transmitter · 13 `NACKSNT` · 12 `BB` bus busy · 11 `RSFULL` RX overrun · 10 `XSMT` TX underflow (0 = underflow) · 9 `AAS` addressed-as-slave · 8 `AD0` general call · 5 `SCD` STOP detected · 4 `TXRDY` (DXR empty, ready for next byte) · 3 `RXRDY` (DRR has data) · 2 `ARDY` (registers ready: CNT reached 0) · 1 `NACK` (no ACK received) · 0 `AL` arbitration lost. Flags clear by writing 1 (or on data register access for TXRDY/RXRDY).
I2CIMR enables per flag: bit6 AASEN, 5 SCDEN, 4 TXRDYEN, 3 RXRDYEN, 2 ARDYEN, 1 NACKEN, 0 ALEN. `I2CIVR` returns code of highest-priority pending interrupt.

#### Master transmit (typical)
1. `IRS=0`; set `I2CPSC`, `I2CCKL`, `I2CCKH`, `I2COAR` (if slave too), `I2CPFNC=1`; 2. `I2CMDR.IRS=1`; 3. `I2CSAR` = slave addr (7-bit, right-justified, **not shifted**); 4. `I2CCNT` = N bytes; 5. `I2CMDR = MST|TRX|IRS|STT|STP` (STP = auto-stop when CNT reaches 0; omit for repeated start); 6. for each byte wait `TXRDY` (or interrupt/DMA) → write `I2CDXR`; 7. wait `SCD`/`BB=0`. Check `NACK` and `AL`.
Back-to-back master transfers: **wait until both BB and MST reset** before starting the next, or the second START won't issue.
#### Master receive
Set `TRX=0`, `MST=1`, `STT`, `STP` (with I2CCNT = N); wait `RXRDY` → read `I2CDRR` N times. NACK on the last byte is generated automatically when CNT-driven STP is used. To read an unknown-length message use **repeat mode (`RM=1`)** and manually set `STP=1` after reading byte (size−1), since RX is double-buffered. Read from a register address: write address with TRX=1, STT (no STP), then new `STT` with TRX=0 (repeated START), `STP`.
#### Slave
`I2COAR` holds own address; module sets `AAS` on match (and on general call → `AD0`). Slave RX: read DRR on RXRDY. Slave TX: `SDIR`=1, write DXR on TXRDY.

#### Misc
- Arbitration loss clears MST (sets AL); SCL clock sync handled in hardware; input noise filter present.
- DMA requests for TX/RX via `I2CDMACR`; GPIO mode via PFNC/PDIR/DOUT…; PDR/PDIS/PSEL for pull/open-drain; `I2CSRS` slew rate.
- Low-power mode and "free run" for debug (`FREE`).

#### Gotchas
- `IRS` must be 1 to operate; reset it to change `I2CPSC`.
- Module clock must be 6.7–13.3 MHz (choose PSC from VCLK).
- `STT`/`STP` are self-clearing — don't poll them for completion; use `SCD`/`BB`/`ARDY`.
- Address register uses the 7-bit address (no R/W bit); R/W comes from `TRX`.
- Without external pull-ups the bus won't work.

---

## 05. DCAN (CAN 2.0A/B)

**Context:** 3 identical CAN controllers (DCAN1/2/3). ISO 11898-1, CAN 2.0 A/B (no CAN-FD), up to 1 Mbit/s. Needs an external CAN transceiver on CAN_TX/CAN_RX. 64 message objects per module (numbers 1–64 / 0x01–0x40). Message objects live in Message RAM and are reachable ONLY via interface register sets IF1/IF2 (IF3 = read-only observation) while the module is running.

#### Base addresses (32-bit regs)
DCAN1 `0xFFF7DC00` · DCAN2 `0xFFF7DE00` · DCAN3 `0xFFF7E000`

#### Register map (offset from base)
| Off | Reg | Purpose |
|---|---|---|
| 00 | CTL | control |
| 04 | ES | error/status (read clears some flags) |
| 08 | ERRC | TX/RX error counters |
| 0C | BTR | bit timing (writable only if Init=1 AND CCE=1) |
| 10 | INT | pending interrupt IDs (Int0ID [15:0], Int1ID [23:16]) |
| 14 | TEST | test modes (silent, loopback) |
| 1C | PERR | parity error code |
| 80 | ABOTR | auto-bus-on timer |
| 84/88–94 | TXRQX / TXRQ12,34,56,78 | transmit-request flags (1 bit/object) |
| 98/9C–A8 | NWDATX / NWDAT12..78 | new-data flags |
| AC/B0–BC | INTPNDX / INTPND12..78 | interrupt-pending flags |
| C0/C4–D0 | MSGVALX / MSGVAL12..78 | message-valid flags |
| D8–E4 | INTMUX12..78 | per-object routing to int line 0 or 1 |
| 100/104/108/10C/110/114 | IF1CMD/MSK/ARB/MCTL/DATA(A)/DATB | interface set 1 |
| 120/124/128/12C/130/134 | IF2CMD/MSK/ARB/MCTL/DATA(A)/DATB | interface set 2 |
| 140–154 | IF3OBS/MSK/ARB/MCTL/DATA/DATB | IF3 (auto-updated observation/DMA path) |
| 160–16C | IF3UPD12..78 | IF3 update enable |
| 1E0 / 1E4 | TIOC / RIOC | CAN_TX / CAN_RX pin as GIO (Func,Dir,Out,In,OD,PU,PD) |

#### Clocking
- Two clocks: **VCLK** (module/bus) and **VCLKA** (async, becomes CAN_CLK for bit timing; source picked in system module "Peripheral Asynchronous Clock Source" register).
- If FMPLL spread-spectrum is on, do NOT feed VCLKA from the PLL — use OSCIN-derived clock.
- Must hold: VCLK ≥ CAN_CLK. CAN_CLK ≥ 8 MHz for 1 Mbit/s. Bit time must be 8–25 tq.

#### Bit timing (BTR @0x0C)
Fields (all stored as value−1 except BRPE): `BRPE[19:16]` prescaler ext, `TSeg2[14:12]`, `TSeg1[11:8]`, `SJW[7:6]`, `BRP[5:0]`.
- tq = (BRPE:BRP + 1) / CAN_CLK  (prescaler range 1–1024)
- Bit time = Sync(1) + Prop + PhaseSeg1 + PhaseSeg2 = (TSeg1+1)+(TSeg2+1)+1 tq
- Program: `TSeg1 = Prop+Phase1−1`, `TSeg2 = Phase2−1`, `SJW = SJW−1`. TSeg1 valid 1–15, TSeg2 0–7, SJW 0–3. SJW = min(4, Phase1) recommended. Phase2 ≥ node info-processing time (0–2 tq).
- Reset value 0x2301 = 500 kbit/s @ 8 MHz CAN_CLK. (Check: 8 MHz/2 = 4 MHz tq → 8 tq/bit = 500k.)
- If sample point differs across nodes, pick the config with highest oscillator tolerance.

#### Init sequence
1. (Optional) hardware-zero Message RAM via system-module MINITGCR + MSINENA (clears RAM + parity bits).
2. Config bit timing: `CTL.Init=1` → `CTL.CCE=1` → wait until Init reads 1 → write BTR → clear CCE, then Init → wait until Init reads 0. (Bad BTR values can prevent leaving init.) After reset module is already in Init=1, bus-off also forces Init=1.
3. Pins: ensure muxing (IOMM) sets CAN_TX/RX functional; TIOC/RIOC.Func are forced to 1 (functional) when Init=0. External pull-up on CAN_TX recommended so bus isn't disturbed during reset.
4. Configure message objects through IF1/IF2 (below). All objects deactivated (MsgVal=0) after reset — reconfigure while MsgVal=0 or via IF write.
5. Enable interrupts as needed (`CTL.IE0/IE1`, `SIE`, `EIE`), clear `Init`.

#### CTL (0x00) bits
`WUBA`25 wake on bus activity · `PDR`24 local power-down req · `DE3/DE2/DE1`20/19/18 DMA enable per IF · `IE1`17 int line 1 · `InitDbg`16 · `SWR`15 SW reset (set Init first, then SWR) · `PMD`[13:10] parity (0x5=disabled, others=enabled) · `ABO`9 auto bus-on · `IDS`8 debug mode interrupt tx/rx · `Test`7 · `CCE`6 · `DAR`5 disable auto-retransmit · `EIE`3 error ints (PER,BOff,EWarn) · `SIE`2 status-change ints (WakeUpPnd,RxOK,TxOK,LEC) · `IE0`1 int line 0 · `Init`0 (reset=1).
- Bus-off: module sets Init=1. Clearing Init starts recovery: waits for 129×11 recessive bits, then resets counters. Each 11-recessive sequence writes LEC=Bit0 (monitor progress). Auto-Bus-On (ABO=1 + ABOTR 32-bit timer) restarts automatically.

#### ES (0x04) status
`PDA`10 · `WakeUpPnd`9 · `PER`8 · `BOff`7 · `EWarn`6 (counter ≥96) · `EPass`5 (error-passive; no interrupt) · `RxOK`4 · `TxOK`3 · `LEC`[2:0]: 0 none, 1 stuff, 2 form, 3 ack, 4 bit1, 5 bit0, 6 CRC, 7 no-event (set after ES read).
- **Reading ES clears WakeUpPnd, PER, RxOK, TxOK and sets LEC=7**, and clears the status interrupt (Int0ID=0x8000). Debug/suspend disables this auto-clear.

#### Message object (accessed via IFx)
IFxARB: `MsgVal`31 · `Xtd`30 (1=29-bit) · `Dir`29 (**1=transmit, 0=receive**) · `ID[28:0]`. **11-bit IDs go in ID[28:18]** (i.e. write `id<<18`); ID[17:0] ignored/zero.
IFxMSK: `MXtd`31 · `MDir`30 · `Msk[28:0]` (1 = must match, 0 = don't care; reset all 1s). Mask used only if `UMask`=1 in MCTL.
IFxMCTL: `NewDat`15 · `MsgLst`14 · `IntPnd`13 · `UMask`12 · `TxIE`11 · `RxIE`10 · `RmtEn`9 · `TxRqst`8 · `EoB`7 (1=single object or last of FIFO) · `DLC`[3:0].
IFxDATA = data bytes 0–3, IFxDATB = bytes 4–7.

##### IFxCMD (0x100 / 0x120) — transfer trigger
Bits: `WR_RD`23 (1=IFx→RAM write, 0=RAM→IFx read) · `Mask`22 · `Arb`21 · `Control`20 · `ClrIntPnd`19 · `TxRqst/NewDat`18 · `DataA`17 · `DataB`16 · `Busy`15 (RO) · `DMAActive`14 · `MsgNum`[7:0] (1–0x40; out-of-range wraps modulo — don't rely on it).
- **Writing MsgNum starts the transfer** (so write all other bits first, or in the same 32-bit write). Busy=1 for 4–14 VBUS cycles; IFx regs are write-protected while Busy. Poll Busy=0 before touching IFx again.
- Read with ClrIntPnd=1 / TxRqst-NewDat=1 clears IntPnd / NewDat atomically (MCTL returned shows state before clearing).
- If CMD.TxRqst/NewDat=1 on a write, the object's TxRqst is set regardless of MCTL.

##### Configure TX object (data frames)
MsgVal=1, Dir=1, EoB=1, ID/Xtd per app, DLC+data per app, TxIE per app, RmtEn=1 only if it should auto-answer remote frames, UMask=0 if no remote frames allowed. Set TxRqst only after data valid (can do in same write via CMD.TxRqst bit). Write cmd with WR_RD=1, Arb|Control|DataA|DataB (+Mask if used).
Update a TX object: write new data with Control=0 or with TxRqst; priority between TX objects = lower message number wins.

##### Configure RX object (data frames)
MsgVal=1, Dir=0, EoB=1, ID/Xtd (acceptance ID), Mask+UMask=1 for group filtering, DLC, RxIE per app, NewDat=0, TxRqst=0 (setting TxRqst on an RX object sends a **remote frame** with that ID).
- With a "don't care" mask, ARB ID bits get overwritten by the received frame's ID.
- Read RX: write CMD (read, Control|Arb|DataA|DataB, ClrIntPnd=1, TxRqst/NewDat=1) with MsgNum → check `MCTL.NewDat`, `MsgLst` (overrun), then take DLC/data.
- **FIFO**: give several RX objects identical ID/mask; set EoB=0 on all except the highest-numbered one (EoB=1). Messages fill lowest number first.
- Acceptance filtering scans objects in ascending number; first match wins.

#### Interrupts
- Two lines: DCAN0INT (IE0) and DCAN1INT (IE1). Message-object ints routable to either via INTMUX (1 bit per object). **Status-change (SIE) and error (EIE) ints go only to line 0.**
- INT register: Int0ID = 0x8000 means status/error pending (highest priority); 1–0x40 = object number (lower number = higher priority). Cleared by clearing object IntPnd (use CMD.ClrIntPnd) or reading ES.
- Object ints require IntPnd plus TxIE (after successful TX) or RxIE (after successful RX).

#### Misc features
- **Auto retransmission** on by default (DAR=0). **Auto-bus-on** via ABOTR.
- **Power-down:** local (PDR→PDA ack; WUBA wakes on dominant bit, but the waking frame is lost) and global. 
- **Test modes (TEST reg, Test=1 in CTL):** Silent, Loopback, External loopback, Loopback+Silent, direct TX pin control, direct Message RAM access.
- **Parity** on Message RAM (PMD field; PERR reg shows location; PER in ES).
- **DMA:** DE1/2/3 + IFxCMD.DMAActive for IF1/IF2/IF3 transfers.
- CAN_TX/CAN_RX usable as GPIO via TIOC/RIOC when Func=0 (only writable while Init=1).

#### Gotchas
- BTR and TIOC/RIOC writes only work while Init=1 (BTR also needs CCE=1).
- Don't clear Init to "restart" quickly from bus-off — recovery sequence cannot be shortened.
- ES read has side effects (don't poll it casually in debugger or ISR twice).
- Message RAM is not CPU-accessible during operation except through IFx.

---

## 06. MibSPI / SPI

**Context:** 5 SPI modules: MibSPI1, SPI2, MibSPI3, SPI4, MibSPI5. Each = classic SPI ("compatibility mode") plus optional multi-buffer ("Mib") mode with its own RAM, transfer groups (TGs), and triggers. Word length 2–16 bits, 4 independent data formats per module, up to 6–8 chip selects (device-dependent), optional SPIENA handshake pin, optional parallel (multi-SIMO/SOMI) mode (not covered here). Master or slave.

#### Base addresses (registers)
MibSPI1 `0xFFF7F400` · SPI2 `0xFFF7F600` · MibSPI3 `0xFFF7F800` · SPI4 `0xFFF7FA00` · MibSPI5 `0xFFF7FC00`. MibSPI RAM base: **MibSPI1 `0xFF0E0000`, MibSPI3 `0xFF0C0000`, MibSPI5 `0xFF0A0000`** (2 KB each; TXRAM +0x000–0x1FF, RXRAM +0x200–0x3FF). Registers accept 8/16/32-bit writes; write 0 to reserved bits.

#### Register map (offset)
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

#### Pin roles
- SPICLK: master drives, slave receives. SIMO: master out / slave in. SOMI: slave out / master in.
- **CLKMOD=1** (internal clock): SPIENA = input, SPICS = outputs. **CLKMOD=0**: SPIENA output, SPICS inputs.
- Master: `MASTER=1`, CLKMOD usually 1. **Slave: MASTER=0 AND CLKMOD=0** (anything else undefined).
- SPIENA = optional "slave ready" handshake. SPICS = chip selects (idle level from SPIDEF).
- SPIPC0: `SOMIFUN/SIMOFUN` [31:24]/[23:16] (bit 0 of each is mirrored in bits 11/10 — **SOMIFUN0[11], SIMOFUN0[10] must always be 1 for any SPI transfer**), `CLKFUN`[9], `ENAFUN`[8], `SCSFUN`[7:0]. 0=GIO, 1=SPI function. Also configure IOMM pin mux (see §11).
- Slave with SPICS functional and CS high → output Hi-Z and shifting disabled.

#### Clocking modes (set per data format via POLARITY/PHASE in SPIFMTx)
| TI POLARITY | TI PHASE | Equivalent standard (CPOL/CPHA) mode | Behavior |
|---|---|---|---|
| 0 | 0 | = standard Mode 1 | clock idles low; data out on rising edge, latched on falling edge (as TI describes it) |
| 0 | 1 | = standard Mode 0 | out half-cycle early/on falling, latched on rising |
| 1 | 0 | = standard Mode 3 | idle high; out falling, latched rising |
| 1 | 1 | = standard Mode 2 | out half-cycle early/on rising, latched falling |
**Caution:** TI's PHASE bit is inverted relative to the common CPHA convention (PHASE=0 here = CPHA=1). To get standard Mode 0 (CPOL=0, CPHA=0) use POLARITY=0, PHASE=1; for standard Mode 3 use POLARITY=1, PHASE=0. (Doc text: PHASE=0 → first bit sent on first clock edge, received on second edge; PHASE=1 → clock delayed half cycle, first bit output before first edge and received on first edge.)
- Changing polarity/phase on the fly: only while no slave is selected; in slave mode with 3/4-pin+ENA, can't switch on-the-fly (reset + reconfigure).

#### SPIFMTx (50h–5Ch)
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

#### SPIDAT1 (3Ch) — compat-mode TX word + control
`[28] CSHOLD` keep CS asserted after transfer until next control+data word loaded (if next CSNR same → CS extended) · `[26] WDEL` apply WDELAY after word · `[25:24] DFSEL` pick SPIFMT0–3 · `[23:16] CSNR` chip-select bitmask (bit n ↔ CS[n]; unimplemented bits read 0) · `[15:0] TXDATA`.
- **Writing only the upper 16 control bits (or fewer) does NOT start a transfer**; writing TXDATA (16-bit write to low half or full 32-bit write) does. Useful for pre-setting format/polarity.
- SPIEN must be 1 before writing. SPIEN=0 clears shift regs, SPIFLG, SPIBUF, TXDATA.
- Write when `TXFULL=0` (or on TXINT).
- **CSNR polarity:** the active/inactive level of CS pins when idle comes from **SPIDEF[7:0]** (1=CS pin high when idle, master only). Typical active-low slaves: SPIDEF=0xFF idle, and CSNR bit n selects CS[n] to be driven active. Verify on a scope.

#### SPIBUF (40h) — RX word + status
`[31] RXEMPTY` · `[30] RXOVR` · `[29] TXFULL` · `[28] BITERR` (TX pin read-back mismatch) · `[27] DESYNC` (master: slave ENA dropped early/late) · `[26] PARITYERR` · `[25] TIMEOUT` (ENA never asserted) · `[24] DLENERR` (CS/ENA deasserted before full word) · `[23:16] LCSNR` last CS used · `[15:0] RXDATA`.
- Reading RXDATA clears RXEMPTY→1 flag logic, RXOVR, BITERR etc., and RXINTFLG. Use SPIEMU to peek without clearing.
- Overrun goes into the internal RXBUF; need two SPIBUF reads to reach the overwritten word.

#### SPIDELAY (48h) — master timing (VCLK units)
`[31:24] C2TDELAY` CS-active→first clock: `(C2TDELAY+2)×Tvclk` · `[23:16] T2CDELAY` last bit→CS inactive: `(T2CDELAY+1)×Tvclk` · `[15:8] T2EDELAY` data finished→ENA inactive timeout: `T2EDELAY / SPIclk` · `[7:0] C2EDELAY` CS active→ENA active timeout: `C2EDELAY / SPIclk`. (Phase=1 adds ½ SPICLK to the CS→clock delay.)

#### Interrupts / flags
- SPIINT0 enables: `TXINTENA`[9], `RXINTENA`[8] (**compat mode only**; not generated in multibuffer mode), `RXOVRNINTENA`[6], `BITERRENA`[4], `DESYNCENA`[3], `PARERRENA`[2], `TIMEOUTENA`[1], `DLENERRENA`[0], `DMAREQEN`[16] (set only after SPIEN=1).
- SPIFLG mirrors those flags + `BUFINITACTIVE`[24]. SPILVL routes each to INT0/INT1 line. INTVECT0/1 read gives pending source (read clears).
- TX DMA req each time word moves into shift reg; RX DMA req each time word lands in SPIBUF.

#### Compatibility-mode sequence
1. `SPIGCR0.nRESET=1`. 2. SPIPC0 function bits. 3. SPIGCR1 CLKMOD/MASTER. 4. SPIFMTx. 5. SPIDELAY, SPIDEF. 6. SPIINT0 / SPILVL. 7. `SPIGCR1.SPIEN=1`. 8. Loop: wait TXFULL=0 → write SPIDAT1 (CSNR, DFSEL, CSHOLD, data) → wait RXEMPTY=0 (or RXINT) → read SPIBUF.
(Do all config with SPIEN=0; order then doesn't matter.) LOOPBACK=1 (master+CLKMOD=1 only) gives internal self-test.

#### Multi-buffer (MibSPI) mode
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

#### Gotchas
- RX data/TXDATA right-aligned; CHARLEN 2–16.
- Slave mode needs both CLKMOD and MASTER = 0.
- BUFINITACTIVE stays 1 while nRESET=0; RAM not usable until nRESET=1 then init completes.
- RXINT/TXINT not generated in multibuffer mode — use TG interrupts/flags.
- GIO mode not allowed with LOOPBACK.

---

## 07. SCI (UART) and SCI/LIN

**Context:** Two modules share one register layout.
- **SCI/LIN module** (base `0xFFF7E400`): UART ("SCI compatibility mode", `LIN MODE=0`) OR LIN 2.x master/slave (`LIN MODE=1`). Pins LINTX / LINRX.
- **SCI module** (base `0xFFF7E500`): UART only (no LIN registers, no SCIGCR2/LINx regs). Pins SCITX / SCIRX.
No hardware flow control (RTS/CTS) — do in software with a GPIO. UART frame is a plain async NRZ: idle high, start bit low, **LSB first**.

#### Frame format (UART mode)
1 start + 1–8 data bits (SCIFORMAT.CHAR = bits−1) + optional address bit (address-bit multiprocessor mode only) + optional parity + 1 or 2 stop bits. Both TX and RX use the same format.

#### Register map (offset, both modules unless noted)
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

#### SCIGCR1 (04h) key bits
`TXENA`25 · `RXENA`24 · `CONT`17 (don't halt on emulation suspend) · `LOOPBACK`16 · `STOP EXT FRAME`13 (LIN) · `HGEN CTRL`12 (LIN: 1 = ID-SlaveTask filtering, recommended) · `CTYPE`11 (LIN: 1 = enhanced checksum) · `MBUF MODE`10 · `ADAPT`9 (LIN auto baud) · `SLEEP`8 (SCI multiprocessor sleep) · **`SWnRST`7 (active-low software reset; 0 = in reset, set to 1 LAST)** · `LIN MODE`6 · **`CLOCK`5 (1 = internal clock; in LIN mode this bit = master(1)/slave(0))** · `STOP`4 (1 = two stop bits) · `PARITY`3 (0 odd, 1 even) · `PARITY ENA`2 (in LIN: ID-parity check) · `TIMING MODE`1 (**1 = asynchronous**, 0 = isosynchronous) · `COMM MODE`0 (0 idle-line, 1 address-bit multiprocessor; in LIN: ID4/ID5 length control).
(Typical plain UART: CLOCK=1, TIMING MODE=1, COMM MODE=0, LIN MODE=0, STOP/PARITY as needed, RXENA/TXENA=1.)

#### Baud rate (BRS @2Ch) — asynchronous mode
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

#### UART configuration sequence
1. `SCIGCR0.RESET=1`. 2. Clear `SCIGCR1.SWnRST=0`. 3. Frame bits in SCIGCR1 (STOP, PARITY, PARITY ENA, COMM MODE, TIMING MODE=1, CLOCK=1) and `SCIFORMAT.CHAR=7` (8-bit). 4. **`SCIPIO0`: TX FUNC[2]=1 and RX FUNC[1]=1** (else pins are GPIO) and set pin mux in IOMM if needed. 5. `BRS`. 6. Optional: LOOPBACK, CONT. 7. `RXENA`/`TXENA`=1. 8. Interrupt enables/levels (SCISETINT, SCISETINTLVL). 9. **`SWnRST=1`**.
Rules: Data written to SCITD before TXENA=1 is not sent. Receiver needs RXENA=1 and RX FUNC=1. Receiver waits for a valid idle period before receiving (`SCIFLR.IDLE`=0 means idle detected/ready).

#### Data flow
- **TX:** wait `TXRDY`(bit8)=1 → write SCITD. `TX EMPTY`(11)=1 when SCITD and shift reg both empty (use to know last byte is on the wire, e.g. before switching an RS-485/half-duplex driver). When done with TX interrupts, clear `TX INT`/`TX DMA` or TXENA — TXRDY stays set.
- **RX:** `RXRDY`(9)=1 → read SCIRD (clears it). Errors set with the frame: `FE`(26) framing, `OE`(25) overrun, `PE`(24) parity; `BRKDT`(0) break detected; `BUSY`(3) receiving; `WAKEUP`(1). Error/status flags are generally cleared by writing 1 to the bit in SCIFLR (some also clear by reading SCIRD or SCIINTVECT0/1 — RXRDY clears on SCIRD read; TXRDY does not clear via vector read).
- Modes: single-buffer (MBUF MODE=0, per-byte) or multi-buffer (MBUF MODE=1: hardware sends/receives `LENGTH+1` bytes via LINTD0/1 / LINRD0/1 — mainly useful in LIN).
- DMA: `SET TX DMA`[16], `SET RX DMA`[17], `SET RX DMA ALL`[18]. 
- Interrupt enable bits (SCISETINT; mirror SCIFLR): `BRKDT`0, `WAKEUP`1, `TIMEOUT`4, `TOAWUS`6, `TOA3WUS`7, `TX INT`8, `RX INT`9, `ID INT`13, `RX DMA`17/`TX DMA`16, error: `PE`24, `OE`25, `FE`26, `NRE`27, `ISFE`28, `CE`29, `PBE`30, `BE`31 (BE..NRE, ISFE LIN-only).

#### SCIFLR bit summary
31 BE · 30 PBE · 29 CE · 28 ISFE · 27 NRE (LIN only) · 26 FE · 25 OE · 24 PE · 14 ID RX · 13 ID TX (LIN) · 12 RXWAKE (address bit received) · 11 TX EMPTY · 10 TXWAKE (next frame = address) · 9 RXRDY · 8 TXRDY · 7 TOA3WUS · 6 TOAWUS · 4 TIMEOUT (LIN bus idle) · 3 BUSY · 2 IDLE · 1 WAKEUP · 0 BRKDT.

#### Multiprocessor / sleep (UART mode)
Idle-line mode (COMM MODE=0): an address frame is one preceded by ≥10 idle bits (fewer = data frame); Address-bit mode (COMM MODE=1): 9th bit =1 marks address (TXWAKE / RXWAKE). `SLEEP`=1 makes receiver ignore data frames until an address frame.

#### Pin GPIO control (SCIPIO0–8, bit2=TX pin, bit1=RX pin)
PIO0 FUNC (1=SCI function) · PIO1 DIR · PIO2 IN (RO) · PIO3 OUT · PIO4 SET · PIO5 CLR · PIO6 PDR (open drain) · PIO7 PD (pull disable) · PIO8 PSL (pull select). (SCI module pins SCITX/SCIRX; SCI/LIN pins LINTX/LINRX.)

#### LIN mode (SCI/LIN module only; `LIN MODE=1`)
- LIN 2.x single-master/multi-slave, break/sync/ID header + 1–8 byte response + checksum. Hardware does header generation, sync-field baud adaption (`ADAPT`), ID parity, checksum (classic or enhanced via `CTYPE`), timeouts (no-response, bus idle >4 s, wake-up 150 ms / 3×, 1.5 s).
- **Config:** as UART plus `LIN MODE=1`, `CLOCK`=1 master/0 slave, `BRS`, `MBRS`, `MBUF MODE`=1 (recommended), `LINMASK` (RX ID MASK[23:16], TX ID MASK[7:0]; set both 0xFF), `HGEN CTRL=1`, then SWnRST=1.
- **Master transmit header:** CPU writes **`LINID[7:0]` (ID BYTE)** → hardware sends break+sync+protected ID. For a TX response write `LINTD0/1` bytes (TD0 first) with `SCIFORMAT.LENGTH` set; checksum auto-sent (or `SCIGCR2.SC`). Slave receive/transmit decided by match of received ID vs `LINID[15:8]` (ID-SlaveTask byte) and masks; flags `ID RX`/`ID TX`, received ID in `LINID[23:16]`.
- RX response in `LINRD0/1`; `SCIGCR2.CC` triggers checksum compare in extended-frame mode. Errors: BE, PBE, CE, ISFE, NRE, FE, OE, PE.
- Wake-up: `GEN WU`(SCIGCR2[8]) sends wakeup; `POWERDOWN`(SCIGCR2[0]) enters low-power.

#### Gotchas
- `SWnRST` must be 0 while configuring, 1 to run; module stuck in reset otherwise.
- Pin function bits (TX FUNC/RX FUNC) default to GPIO.
- TX data is 8-bit register; set CHAR=7 for 8N1.
- Baud error from integer-only P is large at high baud: use M/U.
- Reading SCIFLR does not clear flags; write 1 to clear error flags (see per-flag rules).
- Multi-buffer RX sets RXRDY only after the full programmed frame arrives.

---

## 08. ADC (12-bit SAR)

**Context:** Two 12-bit SAR ADC cores (ADC1, ADC2) sharing analog pins/reference/supply. **ADC1 = 24 channels (AD1IN[23:0]); ADC2 = 16 channels, all of which are shared pins with ADC1** (exact pin mapping: see datasheet). Each core: 3 conversion groups (Event, Group1, Group2), 64-word results RAM per ADC (parity-protected), SW or HW triggering, DMA/interrupt support, self-test and calibration logic.
Base addresses: **ADC1 `0xFFF7C000`, ADC2 `0xFFF7C200`**. All regs 32-bit; write reserved bits as 0.

#### Key properties
- Resolution: **10-bit is default**, 12-bit via `ADOPMODECR.10_12_BIT=1`.
- `fADCLK = fVCLK / (PS+1)`, PS = ADCLOCKCR[4:0] (÷1–÷32). Max ADCLK is in the datasheet (not in this manual).
- Acquisition (sample) time per group: `tACQ = GxSAMP[11:0] + 2` ADCLK cycles (range 2–4098). Min time depends on source impedance — datasheet/app note SPNA118. High-impedance sources (e.g., divider resistors for cell voltage / NTC) need long sample time.
- If both ADCs sample the same channel, their sample windows must be identical or separated by ≥2 ADCLK.
- Groups: **Group1 & Group2 = SW or HW triggered; Event group = HW trigger only.** Up to 8 HW trigger sources per group (device-specific list in datasheet; e.g., N2HET, RTI, ADxEVT pin). Trigger select in ADEVSRC / ADG1SRC / ADG2SRC (source 0–7, edge/both-edges, polarity: default falling).
- Channels convert in **ascending channel number** within a group (sequential mode), or user-defined order via Enhanced Channel Selection (lookup table; `ADxCHNSELMODECTRL`).
- Group mode (xMODECR `MODE`): single sequence or continuous. All three can't be continuous (Group2 is forced single).

#### Register map (offset from base)
| Off | Reg | Off | Reg |
|---|---|---|---|
| 00 | ADRSTCR (`0`=release from reset, `1`=reset) | 70 / 74 | ADG1SR / ADG2SR status |
| 04 | ADOPMODECR | 78 / 7C / 80 | ADEVSEL / ADG1SEL / ADG2SEL channel select (bitmask) |
| 08 | ADCLOCKCR | 84 | ADCALR (calibration/offset correction) |
| 0C | ADCALCR | 88 / 8C | ADSMSTATE / ADLASTCONV |
| 10/14/18 | ADEVMODECR / ADG1MODECR / ADG2MODECR | 90–AC | ADEVBUFFER_0–7 (FIFO read window) |
| 1C/20/24 | ADEVSRC / ADG1SRC / ADG2SRC | B0–CC | ADG1BUFFER_0–7 |
| 28/2C/30 | xINTENA (event/G1/G2) | D0–EC | ADG2BUFFER_0–7 |
| 34/38/3C | xINTFLG | F0/F4/F8 | EV/G1/G2 EMUBUFFER (read w/o advancing) |
| 40/44/48 | xTHRINTCR (threshold count) | FC–118 | ADEVT pin GIO (DIR,OUT,IN,SET,CLR,PDR,PDIS,PSEL) |
| 4C/50/54 | xDMACR | 11C–124 | xSAMPDISEN (sample-cap discharge) |
| 58 | ADBNDCR (BNDA[24:16], BNDB[8:0]) | 128–164 | ADMAGINT1–3 CR/MASK, ENASET/CLR, FLG, OFF |
| 5C | ADBNDEND (BNDEND[2:0]) | 168–170 | x FIFO reset |
| 60/64/68 | ADEVSAMP / ADG1SAMP / ADG2SAMP | 174–17C | xRAMWRADDR |
| 6C | ADEVSR | 180/184/188 | ADPARCR / ADPARADDR / ADPWRUPDLYCTRL |
| 190–198 | x CHNSELMODECTRL | 19C–1B0 | x CURRCOUNT / MAXCOUNT |

#### Important bit fields
- **ADOPMODECR (reset 0x00140000):** `10_12_BIT`[31] (1=12-bit) · `COS`[24] emulation · `CHN_TEST_EN`[20:17] (reset 0xA; input impedance/self-test mode) · `RAM_TEST_EN`[16] · `POWER_DOWN`[8] · `IDLE_PWRDN`[4] · **`ADC_EN`[0] must be 1**.
- **GxMODECR (Group1 at 14h; Group2 at 18h; Event 10h):** `G1_DATA_FMT`[9:8] (12-bit mode: 00 = 12-bit, 01 = >>2 → 10-bit, 10 = >>4 → 8-bit, FIFO reads only) · `G1_CHID`[5] (1 = channel number in result bits [14:10]) · `OVR_G1_RAM_IGN`[4] (allow overwrite) · `G1_HW_TRIG`[3] · `G1_8BIT`[2] (10-bit mode only: >>2) · `G1_MODE`[1] (1 = continuous) · `FRZ_G1`[0] (freeze when higher group preempts). Bit 16 `No_Reset_on_ChnSel` (don't reset results memory when new channel select is written; default: any nonzero write to GxSEL resets that group's memory).
- **ADG1SR (70h):** `G1_MEM_EMPTY`[3] (RO, FIFO mode) · `G1_BUSY`[2] (always 1 in continuous) · `G1_STOP`[1] (frozen) · **`G1_END`[0]** (sequence done; cleared by reading a FIFO result, writing GxSEL, writing 1, or ADC_EN=0).
- **Interrupt/flag regs (GxINTENA/GxINTFLG):** `G1_END_INT_EN`[3] etc.; also threshold (`G1_THRESH_INT`) and memory-overrun (`G1_MEM_OVERRUN`[1] in INTFLG, RO) interrupts.
- **ADBNDCR/ADBNDEND memory partition:**
  - Event buffers = `2×BNDA`; Group1 buffers = `2×(BNDB−BNDA)`; Group2 buffers = `Total − 2×BNDB`.
  - `BNDEND[2:0]` total = 0 → 16 words, 1 → 32, 2 → **64 (max)**. Layout order in RAM: Event, Group1, Group2.
- **Result word:** data in bits [11:0] (right-aligned; FIFO read applies DATA_FMT); bits [14:10] hold channel ID if CHID=1 (else forced 0).

#### Basic single-sequence recipe (example: channels 0,2,4,8 in Group1)
1. `ADRSTCR = 0` (release reset). 2. `ADOPMODECR.ADC_EN = 1` (and `10_12_BIT` as needed). 3. `ADCLOCKCR.PS`. 4. `ADG1SAMP`. 5. (Configure `ADBNDCR/ADBNDEND` so Group1 has enough buffers; set `ADG1MODECR` mode/format/trigger/ints.) 6. **Write channel mask to `ADG1SEL` (=0x115 for ch 0,2,4,8)** — in SW-trigger mode the conversion starts immediately on the write. 7. Wait `ADG1SR.G1_END`=1 (or interrupt/DMA). 8. Read results via `ADG1BUFFER` (FIFO, oldest first; LDM-friendly address window) or directly from results RAM.
HW trigger: set `G1_HW_TRIG=1`, select source/edge in `ADG1SRC`, then write channels to `ADG1SEL`; conversion starts on trigger. Continuous mode needs one trigger only.

#### Reading results
- **FIFO mode:** read any address in ADxBUFFER_0..7 window; returns oldest unread; applies DATA_FMT/CHID; clears G1_END on read; updates MEM_EMPTY.
- **Direct RAM:** read results RAM by address (no pop, no formatting; mask bits yourself). RAM auto-initializes after reset; parity-checked (ADPARCR/ADPARADDR).
- DMA: per-result request or fixed block of results (xDMACR). Threshold interrupts: after N results (xTHRINTCR).

#### Other features
- **Magnitude threshold interrupts** (ADMAGINT1–3): compare a channel's result against threshold with mask; flags in ADMAGINTFLG, offset register gives pending index.
- **Self-test mode:** drives channel to detect open/short-to-VCC/short-to-GND on inputs (uses CHN_TEST_EN). **Calibration mode:** ADCALCR/ADCALR offset-error correction, mid-point calibration.
- **Power-down:** `POWER_DOWN` (core only) / `IDLE_PWRDN` (enhanced, auto when idle); `ADPWRUPDLYCTRL` sets power-up delay.
- **Sample-cap discharge** (xSAMPDISEN) removes charge from previous high-voltage channel when switching.
- **ADEVT pin** doubles as GPIO (ADEVTDIR/OUT/IN/SET/CLR/PDR/PDIS/PSEL) or HW trigger input.

#### Gotchas
- Default resolution is 10-bit, not 12.
- Writing a non-zero value to GxSEL triggers/restarts that group's conversions and (by default) resets its memory.
- With Group1/Group2 both enabled: whichever was triggered first is serviced first; priority/freeze governed by `FRZ_Gx`.
- Read ADxBUFFER (FIFO) only for the formatted, popped results; direct RAM reads show raw 16-bit words.
- BNDA/BNDB are in units of **2 buffers**.

---

## 09. N2HET (High-End Timer)

**Context:** N2HET1 and N2HET2 are programmable timer coprocessors (own 96-bit-instruction RAM + tiny ISA, 30 instructions) driving up to 32 pins each for PWM, capture, pulse measurement, event counting, GPIO sequences, and HW-angle (HWAG, motor/engine) functions. They run a user program in a fixed-period loop independent of the CPU. Typical BMS uses: PWM, precise pulse timing, input capture, extra GPIO, ADC trigger source, SPI/ADC triggers. HTU (High-End Timer Transfer Unit) moves N2HET data to/from system RAM without CPU.
**Not in this manual:** instruction opcode/bit encodings — use TI HET assembler/HALCoGen to generate the RAM image.

#### Addresses
| | N2HET1 | N2HET2 |
|---|---|---|
| Control registers | `0xFFF7B800` | `0xFFF7B900` |
| Instruction RAM | `0xFF460000` | `0xFF440000` |
| Parity RAM | `0xFF462000` | `0xFF442000` |
Up to 128 instruction words (address 0x000–0x1FF possible in map; actual count per datasheet). Each instruction = Program (PF), Control (CF), Data (DF) 32-bit fields at `base + 0x10×n + {0, 4, 8}`; `+0xC` reserved (don't access). Only **32-bit writes** to RAM; reads 8/16/32. CF+DF consecutive reads by one master = atomic 64-bit read (3 shadow regs; 4th concurrent master → address error). RAM is 4 banks (n mod 4); N2HET has priority over host (host stalls on conflict).

#### Execution model
- Program = sequential instructions executed once per **loop resolution period (LRP)**, then wraps to instruction 0 and waits for next LRP clock. Each instruction has a 9-bit "next address" (and some have a 9-bit conditional address) → flow control without branches.
- Worst-case path must finish within one LRP (VCLK2 cycles = time slots); otherwise program overflow exception.
- Clocks: all from **VCLK2**. `hr` (HR prescaler 1–64, HETPFR[5:0]=hr−1), `lr` (loop-res prescaler 1,2,4…128, HETPFR[10:8]=log2(lr)).
  `HRP = hr·T_VCLK2`, `LRP = lr·HRP`, `time slots ts = hr·lr`.
  Example: VCLK2=32 MHz, hr=2, lr=128 → HRP=62.5 ns, LRP=8 µs, 256 slots, `HETPFR=0x00000701`.
  Prefer raising `lr` first (raising `hr` lowers pin resolution).
- Counters: two 25-bit + three 32-bit registers; each pin has a 7-bit HR counter (extends resolution; each pin may execute at most one HR instruction per loop).
- Double-buffering trick: MOV32/MOV64 conditionally copy host-written buffer fields into ECMP data/control fields after a match (MOV64 can also swap pin action/branch to implement toggling).
- Instruction mnemonics present in this TRM: CNT, ECNT, SCNT, PCNT, APCNT, ACNT, RCNT, PWCNT, ECMP, MCMP, ACMP, SCMP, WCAP, WCAPE, MOV32, MOV64, ADM32, DADM64, RADM64, ADC, ADCNST, AND, SHFT, BR, DJZ, (+ others, 30 total). Simple examples: **timer = CNT; PWM = CNT + ECMP/MCMP**.
- Automatic read-clear: control bit C26 on an instruction clears its data field when the host reads it (for capture handshaking).
- Host can stop/start (`HETGCR.TO`), single-step/breakpoint, inspect RAM.

#### Register map (offset from control base)
00 HETGCR · 04 HETPFR · 08 HETADDR (current address) · 0C HETOFF1 / 10 HETOFF2 (interrupt offset, level 1/2) · 14 HETINTENAS / 18 HETINTENAC · 1C HETEXC1 / 20 HETEXC2 (exception ctl) · 24 HETPRY · 28 HETFLG · 2C HETAND · 34 HETHRSH · 38 HETXOR · 3C HETREQENS / 40 HETREQENC / 44 HETREQDS (DMA/HTU request enable/dest) · **4C HETDIR · 50 HETDIN · 54 HETDOUT · 58 HETDSET · 5C HETDCLR · 60 HETPDR (open drain) · 64 HETPULDIS · 68 HETPSL** · 74 HETPCR / 78 HETPAR / 7C HETPPR (parity) · 80 HETSFPRLD / 84 HETSFENA (input suppression filters) · 8C HETLBPSEL / 90 HETLBPDIR (loopback) · 94 HETPINDIS.
HWAG regs (angle generator): HWAPINSEL, HWAGCR0–2, HWAENASET/CLR, HWALVLSET/CLR, HWAFLG, HWAOFF0/1, HWAACNT, HWAPCNT1, HWAPCNT, HWASTWD, HWATHNB, HWATHVL, HWAFIL, HWAFIL2, HWAANGI.

#### HETGCR (00h)
`HET_PIN_ENA`[24] enable output buffers (with DIR=out, PINDIS.x=1, nDIS=1) · `MP`[22:21] RAM master priority (0 HTU lower, 1 HTU higher, 2 round-robin) · `PPF`[18] protect program fields (read-only for masters) · `IS`[17] ignore suspend · **`CMS`[16] 1 = clock master — must be 1 for a single/standalone N2HET** · **`TO`[0] 1 = run, 0 = stop**.

#### Start-up sequence (typical)
1. Pin mux/IOMM and `HETDIR` for outputs (also `HET_PIN_ENA`/PINDIS as needed). 2. `HETPFR` hr/lr. 3. Load program image into instruction RAM (32-bit writes; also parity RAM init if parity on). 4. Interrupt/request enables. 5. `HETGCR = CMS | HET_PIN_ENA | TO`.
GPIO use: `HETDIR` (1 = output), `HETDOUT`, `HETDSET`/`HETDCLR` (atomic), `HETDIN`, `HETPDR` open-drain, `HETPULDIS`/`HETPSL` pulls.

#### Interrupts / DMA / HTU
Instruction-level interrupts raised by program (priority level 1 or 2 via HETPRY); `HETOFF1/2` return pending instruction offset (read clears). Exception conditions: program overflow, APCNT under/overflow, parity, etc. (HETEXC1/2). Requests to DMA (via DMA request lines) or HTU selectable per instruction with `HETREQENS`, destination `HETREQDS`. Hardware priority scheme exists between interrupt levels.

#### Other
- Suppression filter per input pin (HETSFENA, preload HETSFPRLD) for noise.
- Loop-back pairs for self-test; pin disable feature (nDIS) forces outputs Hi-Z.
- Edge detection and HR capture/compare timing details are instruction-specific; examples exist for PWM, pulse gen, pulse measurement (WCAP).
- HWAG: angle tick generator with tooth period tracking, acceleration/deceleration — only for crank/motor applications (skip for BMS).

#### Gotchas
- `CMS` must be set for single-N2HET systems; `TO` only starts the timer after program is loaded.
- Don't write 8/16-bit to RAM; don't touch reserved +0xC word.
- Missing time slots → overflow; widen lr before hr.
- Output buffers need `HET_PIN_ENA` and direction + pin mux.
- Program (and HR clock) are tied to VCLK2, not VCLK.

---

## 10. GIO (General-Purpose I/O)

**Context:** Up to 8 ports (A–H) × 8 pins = 64 GIO terminals, individually input/output, push-pull/open-drain, programmable pulls. Up to 32 terminals (ports A–D) can raise interrupts via the VIM. Number of ports actually implemented and which pins are interrupt-capable: see datasheet. This device's register set exposes **ports A and B** at the offsets below (C–H follow the same pattern on larger devices; on this device ports beyond A/B are generally not in the GIO register frame).
**Base address:** `0xFFF7BC00`. Registers accept 8/16/32-bit access. Pin must also be muxed to GIO in IOMM (see §11) and peripheral clock/reset released.

#### Register map
| Off | Reg | Off | Reg |
|---|---|---|---|
| 00 | GIOGCR0 (`RESET`[0]=1 to run) | 34 / 54 | GIODIRA / B (1 = output) |
| 08 | GIOINTDET (1 = both edges) | 38 / 58 | GIODINA / B (read pin level) |
| 0C | GIOPOL (1 = rising/high, 0 = falling/low) | 3C / 5C | GIODOUTA / B (output latch) |
| 10 / 14 | GIOENASET / GIOENACLR (int enable per pin, 32 bits = ports A–D) | 40 / 60 | GIODSETA / B (write 1 → drive high) |
| 18 / 1C | GIOLVLSET / GIOLVLCLR (1 = high-priority "level A" line, 0 = low "level B") | 44 / 64 | GIODCLRA / B (write 1 → drive low) |
| 20 | GIOFLG (interrupt flags) | 48 / 68 | GIOPDRA / B (1 = open-drain) |
| 24 / 28 | GIOOFF1 / GIOOFF2 (pending offset, high / low level) | 4C / 6C | GIOPULDISA / B (1 = pull disabled) |
| 2C / 30 | GIOEMU1 / GIOEMU2 (same as OFF, read w/o clearing) | 50 / 70 | GIOPSLA / B (0 = pull-down, 1 = pull-up) |

#### I/O quick start
1. Release peripheral reset / enable clock for GIO (PCR/PENA in system module — device datasheet lists the peripheral select). 2. `GIOGCR0 = 1`. 3. Per pin: `GIODIR` 0 = input / 1 = output. 4. Input: `GIOPULDIS` 0 = pull enabled (then `GIOPSL` 0 = pull-down, 1 = pull-up) or 1 = pull disabled. Output: optional `GIOPDR`=1 for open drain. 5. Drive with `GIODSET`/`GIODCLR` (atomic, preferred over read-modify-write of GIODOUT) or `GIODOUT`. 6. Read with `GIODIN`.
Pull truth table (input pins only): PULDIS=0,PSL=0 → pull-down; PULDIS=0,PSL=1 → pull-up; PULDIS=1 → none. Output pins have pulls disabled.

#### Interrupts
- Enable pin: `GIOENASET`. Edge: `GIOPOL` (1 rising, 0 falling); both edges: set `GIOINTDET`. Priority line: `GIOLVLSET` (high/level A) or `GIOLVLCLR` (low/level B). VIM channel numbers for A/B are in the datasheet; lower VIM channel = higher priority.
- ISR: **read `GIOOFF1` (high-level) or `GIOOFF2` (low-level)**: gives pin index (0 none; 1–8 = A0–A7, 9–16 = B0–B7, …) and **clears the flag**. Keep reading until 0 to service all pending in one ISR. (In emulation mode reading doesn't clear; use GIOEMUx to peek.)
- In low-power mode (GIO clocks off), GIOPOL selects **level** (1 = high, 0 = low) instead of edge to wake the device.

#### Gotchas
- `GIOGCR0.RESET` must be 1 before any other GIO register works. Reset state ≠ low-power.
- Use SET/CLR registers; GIODSET/GIODCLR reads mirror GIODOUT.
- Pin mux (IOMM) and peripheral clock gating are separate from GIO config.

---

## 11. IOMM (I/O Multiplexing and Control)

**Context:** Many device pins carry several functions. The IOMM selects which one drives each *output* pin (and, on the 144-pin QFP, which pad feeds some *inputs*). Package-dependent: the 337-BGA needs no input muxing; the 144-QFP has input muxing for the signals listed below. Ball/pin numbers and defaults are in the datasheet; this file gives the mechanism and the PINMMR bits found in the TRM. **DCAN1–3, SCI/LIN RX/TX (LIN module), and ADC inputs are dedicated and do not appear in the output-mux table; SCI (the second UART) and several SPI/I2C signals do.**

#### Registers (base `0xFFFFEA00`, privileged writes only)
- `0x00` REVISION · `0x20` ENDIAN · **`0x38` KICK_REG0** · **`0x3C` KICK_REG1** · `0xE0` ERR_RAW_STATUS · `0xE4` ERR_ENABLED_STATUS · `0xE8` ERR_ENABLE · … 
- **PINMMR0 starts at `0xFFFFEB10`** (PINMMRn = `0xFFFFEB10 + 4n`; e.g. PINMMR10 = `0xFFFFEB38`). 4 byte-lanes per register; each **byte controls one terminal** and each bit in that byte selects one candidate function.

#### Unlock (required before any PINMMR write)
PINMMRs are **locked after every reset**. Write `KICK0 = 0x83E70B13`, then `KICK1 = 0x95A4F1E0`. Lock again by writing any other value to either kick register (recommended after configuring). Writes while locked are silently ignored (no error). Reads always work.
Errors (address error to unimplemented IOMM location, protection error for non-privileged write) go to ESM group 1 channel 37.

#### One-hot select rules
- Each terminal's byte: set **exactly one** bit for the desired function and clear the others.
- If **no** bit is set, or **more than one** is set → the terminal falls back to its **default** function (the one whose bit is 1 at reset).
- Example (PINMMR10, ball N17 on 337BGA): default EMIF_nCS[0] = bit16; for N2HET2[7] clear bit16 and set bit18.
- **Inputs are broadcast** to every module wired to that terminal — keep unused modules from reacting (e.g., a GIO interrupt on a pin being used as SPI).

#### 144-QFP input muxing (A = output-control bit that enables the function on the shared pin; B = input-select bit)
Input comes from the dedicated pad when `NOT(A) OR (A AND B)`; from the muxed pin when `A AND NOT(B)`.
| Signal | A (PINMMR) | B (PINMMR) |
|---|---|---|
| SPI4SIMO | 5[9] | 23[16] |
| SPI4SOMI | 5[17] | 23[24] |
| SPI4CLK | 5[1] | 23[8] |
| SPI4NENA | 4[17] | 24[0] |
| SPI4NCS[0] | 4[25] | 24[8] |
| N2HET1[17] | 20[17] | 24[16] |
| N2HET1[19] | 8[9] | 24[24] |
| N2HET1[23] | 12[17] | 25[8] |
| N2HET1[25] | 7[9] | 25[16] |
| N2HET1[27] | 0[26] | 25[24] |
| N2HET1[29] | 0[18] | 26[0] |
| N2HET1[31] | 9[10] | 26[8] |
| GIOB[2] | 29[16] | 29[16] |

#### PINMMR bits for communication/ADC functions (function → register[bit]; each is one alternative on some terminal; verify default/ball in datasheet)
- **SCI (UART2):** SCIRX = PINMMR7[17] (shares terminal with N2HET1[6]=7[16], EPWM5A=7[18]) · SCITX = PINMMR8[1] (shares with N2HET1[13]=8[0], EPWM5B=8[2]).
- **SPI4:** SPI4CLK 5[1] · SPI4SIMO 5[9] · SPI4SOMI 5[17] · SPI4NENA 4[17] · SPI4NCS[0] 4[25].
- **MibSPI3:** CLK 33[24] · SIMO[0] 33[16] · SOMI[0] 33[8] · NENA 9[8] · NCS[0] 9[16] · NCS[1] 7[8] · NCS[2] 0[24] · NCS[3] 0[16] · NCS[4] 1[9] · NCS[5] 9[9].
- **MibSPI5:** CLK 13[16] · SIMO[0] 13[8] · SOMI[0] 13[0] · SOMI[1] 12[28] · SOMI[2] 13[12] · NENA 12[24] · NCS[0] 27[0].
- **MibSPI1:** NCS[0] 13[24] · SOMI[1] 13[25] · SIMO[1] 14[1] · NENA 12[16] · NCS[1] 20[16] · NCS[2] 8[8] · NCS[3] 9[24] · NCS[4] 8[17] · NCS[5] 11[25]. (CLK/SIMO[0]/SOMI[0] default-dedicated.)
- **SPI2:** NENA 29[0] · NCS[1] 29[1].
- **I2C:** I2C_SCL = 0[17] · I2C_SDA = 0[25].
- **ADC external triggers:** AD1EVT = 10[0] · AD2EVT = 9[17].
- **N2HET1 pins on shared terminals:** [0] 5[0] · [1] 4[16] · [2] 5[8] · [3] 4[24] · [5] 5[16] · [6] 7[16] · [8] 14[0] · [11] 1[8] · [13] 8[0] · [15] 8[16] · [17] 20[17] · [19] 8[9] · [21] 9[25] · [23] 12[17] · [24] 11[24] · [25] 7[9] · [27] 0[26] · [29] 0[18] · [31] 9[10].
(Full per-terminal table incl. EMIF/EMAC/ePWM/eCAP/eQEP/N2HET2 alternatives: TRM section 4.8.)

#### Other special controls in IOMM (rarely needed)
ADC trigger-event selection (ADC1/2 event/G1/G2 sources among N2HET/ePWM/GIO…), ePWM→ADC SOC routing, N2HET external-fault interrupt, clock enables/time-base sync for ePWM/eCAP/eQEP, EMIF fast mode and clock select, Ethernet mode select (MII/RMII).

#### Gotchas
- Forgetting the kick sequence → pin mux never changes and nothing errors.
- All-clear or multi-set bit patterns select the default, which may not be what you expect.
- SCI/SPI/I2C/ADC-event pin functions often share terminals with N2HET/EMIF/ePWM; check the pin isn't also driven by another enabled module.
- Pin direction/pull for pins used as peripherals is controlled by the peripheral's own pin-function registers (SPIPC*, SCIPIO*, I2CPFNC, GIO…), not by IOMM.

---

## 12. DMA Controller

**Context:** 16 channels (this device), up to 32 hardware request lines, priority queue, independent bus master. Moves data between any addresses (peripheral registers, RAM, flash) without CPU: typical uses here are SPI/SCI/ADC/N2HET/I2C/DCAN streams. Which peripheral drives which request line is in the **datasheet** (DMA request map). Registers and control-packet RAM are **privileged-access only**; the DMA itself runs in user mode, so it can't write privileged-only registers.
**Addresses:** control registers `0xFFFFF000`; control-packet RAM `0xFFF80000` (parity-protected, init via MSINENA bit 1 — see §01). 8/16/32-bit access.

#### Concepts
- **Element** = one read+write of size RES/WES (8/16/32/64-bit). **Frame** = `IETCOUNT` elements. **Block** = `IFTCOUNT` frames. A request triggers one frame or one whole block (`TTYPE`).
- Each channel n owns control packet n: **primary** (user-programmed) and **working** (hardware copy; at `0x800+`, read-only).
- Requests can come from: software (SWCHENAS), hardware line (HWCHENAS), or chaining from another channel's end.
- Two-level request buffer per channel; a 4th overlapping request is lost. Mixing HW and SW requests on one channel can desync with the peripheral.
- Changing a control packet clears its pending bit and stops an active channel at the next arbitration boundary.

#### Primary control packet (offset from `0xFFF80000 + 0x20×channel`)
| Off | Reg | Content |
|---|---|---|
| 00 | ISADDR | initial source address (absolute) |
| 04 | IDADDR | initial destination address |
| 08 | ITCOUNT | `IFTCOUNT`[28:16] frames (0–0x1FFF) · `IETCOUNT`[12:0] elements per frame |
| 10 | CHCTRL | see below |
| 14 | EIOFF | `EIDXD`[28:16] dest, `EIDXS`[12:0] source element index (**bytes**, not scaled by element size) |
| 18 | FIOFF | `FIDXD`/`FIDXS` frame index offsets (bytes) |
Working: 800 CSADDR · 804 CDADDR · 808 CTCOUNT (current values).

##### CHCTRL (10h)
`CHAIN`[21:16] next channel to trigger at end (value = channel+1; 0 = none) · `RES`[15:14] read element size (0=8b,1=16b,2=32b,3=64b) · `WES`[13:12] write element size · `TTYPE`[8] 0=frame / 1=block per request · `ADDMR`[4:3] read addressing (0 constant, 1 post-increment, 3 indexed) · `ADDMW`[2:1] write addressing · `AIM`[0] auto-initiation (reload from primary after block complete; re-arms for next request).
- Use **constant** address for peripheral data registers, **post-increment** for RAM buffers. Pack/unpack happens automatically when RES ≠ WES.

#### Control registers (offset from 0xFFFFF000)
00 GCTRL (**`DMA_EN`[16]** enable, `DEBUG_MODE`[9:8], `BUS_BUSY`[14] RO, `DMA_RES`[0] soft reset) · 04 PEND (pending channels) · 0C DMASTAT (`STCH[n]` active) · 14 HWCHENAS / 1C HWCHENAR (HW request enable set/reset) · 24 SWCHENAS / 2C SWCHENAR (software trigger set/reset) · 34 CHPRIOS / 3C CHPRIOR (1 = high-priority queue, 0 = low) · 44 GCHIENAS / 4C GCHIENAR (channel interrupt enable) · **54/58/5C/60 DREQASI0–3** (request-line assignment, see below) · 94/98 PAR0/PAR1 (port assignment) · B4 FTCMAP · BC LFSMAP · C4 HBCMAP · CC BTCMAP (map each channel's interrupt to line A/B) · DC/E4 FTCINTENAS/AR · EC/F4 LFSINTENAS/AR · FC/104 HBCINTENAS/AR · 10C/114 BTCINTENAS/AR · 11C GINTFLAG · 124 FTCFLAG · 12C LFSFLAG · 134 HBCFLAG · 13C BTCFLAG · 144 BERFLAG · 14C–158 FTCA/LFSA/HBCA/BTCAOFFSET · 160–16C FTCB/LFSB/HBCB/BTCBOFFSET (interrupt-vector style offsets per line) · 178 PTCRL (port control) · 17C RTCTRL (RAM test) · 180 DCTRL · 184 WPR/188 WMR (watchpoint) · 1A8 DMAPCR (parity ctrl) · 1AC DMAPAR · 1B0 DMAMPCTRL / 1B4 DMAMPST / 1B8–1D4 DMAMPR0S–3E (4 memory-protection regions).

##### Request assignment (DREQASIn)
Each 32-bit reg holds 4 channels as 6-bit fields in **big-endian byte order**: DREQASI0: `CH0ASI`[29:24], `CH1ASI`[21:16], `CH2ASI`[13:8], `CH3ASI`[5:0]; DREQASI1 = ch 4–7 (ch4 in [29:24]), DREQASI2 = ch 8–11, DREQASI3 = ch 12–15. Value 0–0x1F = request line number (0x20–0x3F reserved). **Reset: channel n ← request line n.**

#### Typical setup (peripheral → RAM or RAM → peripheral)
1. Make sure DMA RAM is initialized (MSINENA bit 1) and peripheral clock/pins running.
2. Program primary control packet: ISADDR, IDADDR, ITCOUNT, CHCTRL, EIOFF/FIOFF (when indexed). Peripheral data-register addresses: use the register's address; for 8-bit data (SCI, SPI 16-bit etc.) match RES/WES to register width (watch endianness: big-endian MCU — an 8-bit SCI RX data byte is at the register's last byte `base+0x37` for SCIRD(34h)).
3. `DREQASIn` if remapping lines; `CHPRIOS` if high priority.
4. Interrupts: `FTC/LFS/HBC/BTC INTENAS` + `xMAP` (line A/B) + `GCHIENAS`; enable in VIM.
5. `HWCHENAS` bit n (or `SWCHENAS` for software start); finally `GCTRL.DMA_EN=1` (config must come first).
6. Peripheral side: enable its DMA request (e.g. SCI `SET TX/RX DMA`, SPI `DMAREQEN` + TG/DMA control, ADC `xDMACR`, I2C `I2CDMACR`, DCAN `DEx`/IFxCMD.DMAActive).
7. ISR: read `*AOFFSET`/`*BOFFSET` (channel+1) or flags; clear flag by writing 1.

#### Other features
- **Auto-initiation:** `AIM=1` keeps the channel armed with its original packet (ring-buffer style, used with HBC/BTC interrupts for ping-pong).
- **Channel chaining** via CHAIN (frame-end or block-end per TTYPE).
- **Priority queue:** two queues (high/low) with round-robin inside; **FIFO buffer** (8 × 64-bit) used with data packing.
- **Interrupt types per channel:** FTC (frame complete), LFS (last frame start), HBC (>half block), BTC (block complete), BER (bus error → ESM).
- **Memory protection:** 4 regions with start/end, per-region access; violation → status in DMAMPST + ESM.
- **Parity** on control-packet RAM (DMAPCR, DMAPAR).

#### Gotchas
- Don't enable `DMA_EN` before configuration is complete.
- Element/frame index values are bytes.
- Each peripheral request needs to be enabled at both the peripheral and DMA ends.
- Write privileged only; DMA can't modify privileged peripheral regs.
- Mixing SW and HW triggers on one channel can desync.
- Control-packet RAM must be initialized (ECC/parity) before use.

---

## 13. Flash Controller (F021 FMC)

**Scope:** flash wrapper only. The safety/diagnostic modules (PMM, TCRAM, PBIST, STC, CCM-R4F, DCC, CRC) are documented once, at register level, in §18 and §19.

- Single 3.3 V supply; banks/sectors; ECC (SECDED, address bits included) evaluated **in the CPU for main banks** and in the flash wrapper for the EEPROM-emulation bank (bank 7). Read-while-write across banks. Program/erase done by the F021 Flash API (not in this TRM), not by direct register pokes.
- **Flash wrapper registers base `0xFFF87000`** (privileged writes).
- **Power-up defaults:** 1 data wait state, 0 address wait states, pipeline **disabled**, flash write-protected, active power mode. **Boot code must set wait states/pipeline in `FRDCNTL` *before* raising HCLK** (required values per HCLK are in the datasheet).

#### FRDCNTL (offset 00h)
`RWAIT`[11:8] random/data read wait states (0–15; reset = 1; in pipeline mode there's always ≥1 even if 0) · `ASWSTEN`[4] address-setup wait state (pipeline mode only) · `ENPIPE`[0] pipeline mode enable. (EEPROM bank 7 has an equivalent register.)
**Frequency-up sequence:** set RWAIT/ASWSTEN/ENPIPE for new HCLK → switch clock source/PLL → run. **Down sequence:** reduce clock first, then reduce wait states.

#### ECC / error handling (FEDACCTRL1 etc.)
- `FEDACCTRL1`: `EDACMODE`[19:16] selects error-correction mode for main banks (a specific value, 0x5, covers single-bit errors in OTP/ECC/mirror space — check register description before relying on it); also contains the EDAC enable field and `SUSP_IGNR`[24] (CPU suspend doesn't block error-bit setting when 1).
- Correctable error count/address/position: `FCOR_ERR_CNT/ADD/POS`; uncorrectable: `FUNC_ERR_ADD`; status `FEDACSTATUS`; sector disable `FEDACSDIS`; diagnostic modes (`FDIAGCTRL`, `DIAG_MODE` 1–7) for ECC self-test; address-tag registers for bus-error diagnostic.
- Single-bit → corrected silently (and counted/ESM'd); double-bit → data abort + ESM.
- **Bank/sector info from OTP:** bank-0 info at `0xF0080158` (`NUM_Banks`[7:0], B0–B7 present flags [15:8], sectors in this bank [23:16]); other banks at `0xF0082158, 84158, 86158, 88158, 8A158, 8C158, 8E158` (bank 1–7).
- **LPO trim + max HCLK:** OTP `0xF00801B4`: `HFLPO_TRIM`[31:24], `LFLPO_TRIM`[23:16], `MAX_HCLK`[15:0].
- Power modes & timing: `FBAC` (`BAGP`[15:8] bank active grace period, `VREADST`[7:0], `OTPPROTDIS`[23:16]), `FBFALLBACK`, `FBPRDY`, `FPAC1/2`, `FMAC` (bank select), `FMSTAT` (state machine status); `FBSE/FBBUSY/FBPROT` for the program/erase flow (used by the Flash API).
- ECC bits live in a separate ECC address region (TRM 5.4.1); deliberate-ECC-error test locations are in OTP (TRM 5.4.2.5).

#### Gotchas
- Raise flash wait-states **before** raising HCLK (lower them only after lowering HCLK).

---

## 14. ePWM (Enhanced PWM)

**Context:** Up to 7 identical ePWM modules (ePWM1–7), each = 1 PWM channel with two outputs **EPWMxA / EPWMxB**, built from 8 submodules in series: Time-Base (TB) → Counter-Compare (CC) → Action-Qualifier (AQ) → Dead-Band (DB) → PWM-Chopper (PC) → Trip-Zone (TZ), plus Event-Trigger (ET, interrupts + ADC SOC) and Digital-Compare (DC, trip/filter from comparator-style inputs). Clock = **VCLK4**. All registers are **16-bit**. Typical uses: motor/DC-DC drive, gate-drive PWM, triggering ADC sampling at precise points in a PWM cycle.
**Addresses:** ePWM1 `0xFCF78C00`, +0x100 per instance up to ePWM7 (full list in datasheet/memory map). Pins muxed via IOMM (see §11); module sync/trip/ADC-trigger routing also configured in IOMM (EPWMSYNCI/O, TZ1–3, ADC SOC mux).

#### Register map (16-bit; offset from module base)
| Off | Reg | Off | Reg |
|---|---|---|---|
| 00 TBSTS | status (CTRDIR, SYNCI, CTRMAX) | 2E TZFLG | trip flags |
| 02 TBCTL | time-base control | 30 ETSEL | event select |
| 04 TBPHS | phase | 32 TZFRC | force trip |
| 08 TBPRD | period (shadowed) | 34 ETFLG | event flags |
| 0A TBCTR | counter | 36 ETPS | event prescale |
| 0C CMPCTL | compare ctl | 38 ETFRC | force event |
| 10 CMPA / 16 CMPB | compare (shadowed) | 3A ETCLR | clear event |
| 14 AQCTLA / 1A AQCTLB | action qualifier | 3E PCCTL | chopper |
| 18 AQSFRC / 1E AQCSFRC | SW force | 60 DCACTL / 66 DCBCTL | DC A/B |
| 1C DBCTL | dead-band ctl | 62 DCTRIPSEL / 64 DCFCTL | DC trip/filter select |
| 20 DBFED / 22 DBRED | falling/rising delay (10-bit) | 68 DCFOFFSET / 6C DCFWINDOW / 6A DCCAPCTL | filter offset/window/capture |
| 24 TZDCSEL / 26 TZSEL / 28 TZEINT / 2A TZCTL / 2C TZCLR | trip config | 6E/70/72 DCFOFFSETCNT / DCCAP / DCFWINDOWCNT | |
TZ registers are **privileged-write-only**.

#### Time-Base (TBCTL @02h, reset 0x0083)
`FREE_SOFT`[15:14] emulation (2/3 = free run) · `PHSDIR`[13] count dir after sync (up-down mode only) · `CLKDIV`[12:10] /1…/128 (2ⁿ) · `HSPCLKDIV`[9:7] /1,/2(reset),/4,/6,/8,/10,/12,/14 · `SWFSYNC`[6] force sync · `SYNCOSEL`[5:4] 0 = pass SYNCI, 1 = CTR=0, 2 = CTR=CMPB, 3 = disabled · `PRDLD`[3] 0 = TBPRD shadowed, 1 = immediate · `PHSEN`[2] load TBCTR from TBPHS on sync · `CTRMODE`[1:0] 0 up, 1 down, 2 up-down, **3 = stop/freeze (reset default → counter doesn't run until you change it)**.
- **TBCLK = VCLK4 / (HSPCLKDIV_div × CLKDIV_div)**.
- Period: up or down count `T_pwm = (TBPRD + 1) × T_TBCLK`; up-down `T_pwm = 2 × TBPRD × T_TBCLK`.
- Sync chain: ePWM1 → ePWM2 → …; phase offsets via TBPHS (lead/lag); eCAP can join the chain.

#### Counter-Compare (CMPCTL @0Ch)
`SHDWAFULL`[8], `SHDWBFULL`[9] (RO) · `SHDWAMODE`[4] / `SHDWBMODE`[6]: 0 = shadowed (double-buffered), 1 = immediate · `LOADAMODE`[1:0] / `LOADBMODE`[3:2]: 0 = on CTR=0, 1 = on CTR=PRD, 2 = either, 3 = freeze. Duty (up-count): `CMPA / (TBPRD+1)`.

#### Action-Qualifier (AQCTLA @14h, AQCTLB @1Ah)
Each field 2 bits: 0 = do nothing, 1 = clear (low), 2 = set (high), 3 = toggle.
`ZRO`[1:0] on CTR=0 · `PRD`[3:2] on CTR=PRD · `CAU`[5:4] CTR=CMPA counting up · `CAD`[7:6] CMPA counting down · `CBU`[9:8] CMPB up · `CBD`[11:10] CMPB down.
Common patterns: **single-edge up-count PWM, active high A:** `ZRO=Set(2)`, `CAU=Clear(1)`. Up-down symmetric: `CAU=Set`, `CAD=Clear`. SW force: AQSFRC (one-shot), AQCSFRC (continuous: 0 none, 1 low, 2 high; shadowed).

#### Dead-Band (DBCTL @1Ch)
`HALFCYCLE`[15] · `IN_MODE`[5:4]: 0 = A is source for both RED & FED, 1 = B→RED/A→FED, 2 = A→RED/B→FED, 3 = B for both · `POLSEL`[3:2]: 0 = active-high, 1 = ALC (A inverted), 2 = AHC (B inverted), 3 = active-low · `OUT_MODE`[1:0]: 0 bypass, 1 disable RED, 2 disable FED (A delayed rising), 3 full. Delay `= DBxED × T_TBCLK` (10-bit counts, or ×2 clock with HALFCYCLE).
**Complementary pair with dead time:** `OUT_MODE=3, IN_MODE=0, POLSEL=2 (AHC)`.

#### Trip-Zone (TZSEL @26h, TZCTL @2Ah, TZEINT @28h, TZFLG @2Eh, TZCLR @2Ch, TZFRC @32h)
Trip sources: TZ1–TZ3 (pins/ESM/clock-monitor, routed in IOMM), digital-compare events. Per-cycle (CBC) or one-shot (OSHT) selection in TZSEL. `TZCTL`: `TZA`[1:0] and `TZB`[3:2] action on output: 0 = Hi-Z, 1 = force high, 2 = force low, 3 = no action. `DCAEVT1`[5:4], `DCAEVT2`[7:6], `DCBEVT1`[9:8], `DCBEVT2`[11:10] similar. Clear via TZCLR (CBC clears itself each cycle). **Use TZ (one-shot) as a hardware kill switch.**

#### Event-Trigger (ETSEL @30h, ETPS @36h, ETFLG/34h, ETCLR/3Ah, ETFRC/38h)
`ETSEL`: `INTEN`[3], `INTSEL`[2:0] (1 = CTR=0, 2 = CTR=PRD, 3 = either, 4 = CTRU=CMPA, 5 = CTRD=CMPA, 6 = CTRU=CMPB, 7 = CTRD=CMPB) · `SOCAEN`[11], `SOCASEL`[10:8] (0 = DCAEVT1.soc, else same codes as INTSEL) · `SOCBEN`[15], `SOCBSEL`[14:12].
`ETPS`: `INTPRD`[1:0] (0 off, 1 = every event, 2 = every 2nd, 3 = every 3rd) · `INTCNT`[3:2] RO · `SOCAPRD`[9:8], `SOCACNT`[11:10], `SOCBPRD`[13:12], `SOCBCNT`[15:14]. Clear flag via ETCLR before re-arming (INT bit). **ePWM SOCA/SOCB can trigger ADC conversions** (select as ADC group trigger source in ADC xSRC/IOMM).

#### PWM-Chopper (PCCTL @3Eh)
High-frequency carrier modulation (CHPEN, CHPFREQ, CHPDUTY, OSHTWTH one-shot width) for pulse-transformer drive.

#### Digital Compare (DC)
DCTRIPSEL, DCACTL/DCBCTL, DCFCTL (blanking window + filter), DCCAPCTL/DCCAP (capture counter at event), DCFOFFSET/WINDOW — builds trip events (DCAEVT1/2, DCBEVT1/2) from TZ/comparator inputs with optional blanking windows.

#### Start-up recipe (simple PWM, 50 kHz @ VCLK4=…; compute your own)
1. Enable ePWM clock domain (`CLKCNTL.PENA`, `CDDIS` bit for VCLK4 enabled) and pin mux. 2. `TBCTL`: CLKDIV/HSPCLKDIV, `CTRMODE=3` (freeze) while configuring. 3. `TBPRD`, `TBPHS`, `TBCTR=0`. 4. `CMPCTL` shadow modes; `CMPA`. 5. `AQCTLA/B`. 6. (Dead-band / TZ / ET as needed.) 7. `CTRMODE=0` (up) to start; update duty by writing `CMPA` (shadowed; takes effect on next load event).

#### Gotchas
- Default `CTRMODE=3` = counter frozen.
- Shadowed registers (TBPRD, CMPA/B, AQCSFRC) only update at load events unless `PRDLD`/`SHDW*MODE` set to immediate.
- TZ regs are privileged-only and writes are EALLOW-like protected by privilege.
- Up-down mode period doesn't use `+1`.
- ET interrupt must be cleared with ETCLR, and (for TZ) TZCLR.

---

## 15. eCAP (Enhanced Capture)

**Context:** Up to 6 eCAP modules (eCAP1–6). Each = one capture input pin (ECAPx) with a 32-bit timestamp counter and four 32-bit capture registers (CAP1–CAP4) — OR — a single-channel PWM output (**APWM mode**). Uses: period/duty/pulse-width measurement, speed from tooth sensors, decoding duty-encoded sensors. Clock **VCLK4**; registers 16-bit offsets, 32-bit data regs. Base: eCAP1 `0xFCF79300`, +0x100 per instance (datasheet gives the table). Pin via IOMM.

#### Register map (offset)
00 TSCTR (32-bit timestamp counter) · 04 CTRPHS (phase on sync) · 08 CAP1 (= APRD in APWM) · 0C CAP2 (= ACMP) · 10 CAP3 (= APRD shadow) · 14 CAP4 (= ACMP shadow) · **28 ECCTL2** · **2A ECCTL1** · 2C ECFLG · 2E ECEINT · 30 ECFRC · 32 ECCLR.

#### ECCTL1 (2Ah)
`FREE_SOFT`[15:14] (0 stop immediately, 1 stop at 0, 2/3 free-run) · `PRESCALE`[13:9] input-event divider (0 = bypass, else ÷2N: 1→2, 2→4, 3→6 … up to ÷62) · `CAPLDEN`[8] enable capture-register loads · per event n=1..4: `CAPnPOL` (0 = rising, 1 = falling edge) and `CTRRSTn` (1 = reset TSCTR after event n → **delta/period mode**; 0 = absolute timestamp). Bit layout: CTRRST4[7] CAP4POL[6] CTRRST3[5] CAP3POL[4] CTRRST2[3] CAP2POL[2] CTRRST1[1] CAP1POL[0].

#### ECCTL2 (28h, reset 0x0002)
`APWMPOL`[10] (0 = active high: compare = high time) · **`CAP_APWM`[9]** (0 = capture mode, 1 = APWM mode) · `SWSYNC`[8] · `SYNCO_SEL`[7:6] (0 pass-through, 1 CTR=PRD, 2/3 disabled) · `SYNCI_EN`[5] (load TSCTR from CTRPHS on sync) · **`TSCTRSTOP`[4]** (0 = TSCTR stopped (reset default), 1 = free-running) · `REARM`[3] (write 1: arm one-shot, reset Mod4 counter/flags) · `STOP_WRAP`[2:1] (0..3 = stop/wrap after event 1..4) · `CONT_ONESHT`[0] (0 continuous, 1 one-shot).

#### Flags / interrupts (ECFLG 2Ch RO, ECEINT 2Eh enable, ECFRC 30h force, ECCLR 32h clear-by-write-1)
bit0 `INT` global · 1–4 `CEVT1–4` · 5 `CTROVF` (TSCTR wrapped FFFFFFFF→0) · 6 `CTR_PRD` (APWM) · 7 `CTR_CMP` (APWM). **Clear the individual flag AND `INT` (ECCLR) to re-arm interrupts.**

#### Capture recipes
- **Absolute timestamp:** CAPxPOL per desired edges, all CTRRSTn = 0, `CAPLDEN=1`, `TSCTRSTOP=1`, continuous or one-shot (`STOP_WRAP`). Each edge loads current TSCTR into CAPn in sequence (Mod4 counter cycles 1→4).
- **Period + duty (delta mode):** CAP1 = rising (POL=0, CTRRST1=1), CAP2 = falling (POL=1, CTRRST2=0). Result: CAP1 = time between rising edges (period), CAP2 = rising→falling time (high time); duty = CAP2/CAP1 (read after CEVT2). Frequency = `f_VCLK4 / period_counts`.
- Timestamp resolution = 1/VCLK4; use PRESCALE to divide noisy/fast inputs; 32-bit counter wraps at ~2³²/VCLK4.
- One-shot: arm with `REARM=1`; after `STOP_WRAP`+1 events counter and loads freeze until rearmed.

#### APWM mode (CAP_APWM=1)
CAP1 = period (APRD), CAP2 = compare (ACMP), CAP3/CAP4 = shadow of APRD/ACMP (loaded when TSCTR = period). Output on the same pin (APWMx). Frequency `= f_VCLK4 / (APRD+1)`. Behavior (active high): CMP=0 → 0 % duty; CMP=n → high n cycles; CMP=PRD → high except 1 cycle; **CMP ≥ PRD+1 → 100 %**. `APWMPOL` inverts. Writing the active CAP1/CAP2 also writes shadow regs (use CAP3/CAP4 for glitch-free updates).

#### Gotchas
- `TSCTRSTOP` reset = stopped.
- Clear ECFLG bit + INT via ECCLR or no new interrupts.
- eCAPs sync with ePWM through the sync chain (SYNCI/SYNCO).

---

## 16. eQEP (Enhanced Quadrature Encoder Pulse)

**Context:** 2 modules (eQEP1/2). Decode incremental-encoder signals **QEPA / QEPB (90° quadrature), QEPI (index), QEPS (strobe)** into a 32-bit position counter with direction, plus unit timer, capture timer and watchdog for speed measurement. Not typically relevant to BMS (motor/rotary position only). Clock **VCLK4**. Base: eQEP1 `0xFCF79900`, eQEP2 `0xFCF79A00`. Pins muxed via IOMM.

#### Register map (offset)
00 QPOSCNT (32) · 04 QPOSINIT · 08 QPOSMAX · 0C QPOSCMP · 10 QPOSILAT (latch @ index) · 14 QPOSSLAT (latch @ strobe) · 18 QPOSLAT (latch @ unit timeout / read) · 1C QUTMR (unit timer) · 20 QUPRD (unit period) · 24 QWDPRD (16-bit watchdog period) · 26 QWDTMR · **28 QEPCTL · 2A QDECCTL · 2C QPOSCTL · 2E QCAPCTL** · 30 QFLG · 32 QEINT · 34 QFRC · 36 QCLR · 38 QCTMR (capture timer) · 3A QEPSTS (reset 0x8E) · 3C QCTMRLAT · 3E QCPRD · 42 QCPRDLAT.
(Doc lists QCPRD at 3Eh and QCPRDLAT at 42h; 40h is skipped.)

#### Quadrature decoder (QDECCTL 2Ah)
`QSRC`[15:14] 0 = quadrature count (QCLK/QDIR from A/B decode), 1 = direction-count (A = clock, B = direction), 2 = up-count (frequency measurement), 3 = down-count · `SOEN`[13]/`SPSEL`[12] position-compare sync output on index/strobe pin · `XCR`[11] 0 = 2× resolution (count both edges of QCLK), 1 = 1× · `SWAP`[10] swap A/B · `IGATE`[9] gate index with strobe · `QAP/QBP/QIP/QSP`[8:5] invert A/B/I/S.
Quadrature mode counts **4× the line rate** (both edges of both channels): counts/rev = 4 × lines.
Direction: A leads B = clockwise/forward (counter up), B leads A = reverse (counter down).

#### Control (QEPCTL 28h)
`FREE_SOFT`[15:14] · **`PCRM`[13:12]** counter reset mode: 0 on index event, 1 at max position (QPOSMAX), 2 on first index event, 3 on unit-timeout · `SEI`[11:10] strobe init (2 = rising QEPS; 3 = direction-dependent) · `IEI`[9:8] index init (2 = rising QEPI, 3 = falling) · `SWI`[7] SW init (QPOSCNT = QPOSINIT) · `SEL`[6] strobe latch edge · `IEL`[5:4] index latch (1 rising, 2 falling, 3 software marker) · **`QPEN`[3] enable position counter (0 = reset flags/regs)** · `QCLM`[2] 0 = latch on CPU read of QPOSCNT, 1 = latch on unit timeout · `UTE`[1] unit timer enable · `WDE`[0] watchdog enable.
Counter range 0…`QPOSMAX` (e.g. 1000-line encoder: QPOSMAX = 3999). Overflow/underflow wrap with flags.

#### Position-compare (QPOSCTL 2Ch)
`PCSHDW`[15], `PCLOAD`[14], `PCPOL`[13], `PCE`[12] enable, `PCSPW`[11:0] sync-pulse width = (n+1)×4×VCLK4 cycles. Match with QPOSCMP raises `PCM` flag/optional sync pulse.

#### Capture unit (QCAPCTL 2Eh) — low-speed (period) measurement
`CEN`[15] enable · `CCPS`[6:4] capture timer clock = VCLK4/2ⁿ (n=0–7) · `UPPS`[3:0] unit-position event = QCLK/2ⁿ (n = 0–0xB → /1…/2048). Each UPEVNT latches QCTMR → QCTMRLAT and the elapsed period → QCPRDLAT.

#### Speed formulas
- **High speed (unit timer):** `v(k) = (x(k) − x(k−1)) / T`, T = unit period `QUPRD / VCLK4`. Position counts read from QPOSLAT (latched at unit timeout, QCLM=1).
- **Low speed:** `v(k) = X / (t(k) − t(k−1))`, X = unit position (UPPS), Δt from capture timer (QCPRDLAT × CCPS prescale / VCLK4).

#### Flags (QFLG 30h; QEINT 32h enable; QFRC 34h force; QCLR 36h clear-by-write-1)
0 `INT` · 1 `PCE` position counter error · 2 `PHE` quadrature phase error · 3 `QDC` direction change · 4 `WTO` watchdog timeout · 5 `PCU` underflow · 6 `PCO` overflow · 7 `PCR` compare ready · 8 `PCM` compare match · 9 `SEL` strobe latch · 10 `IEL` index latch · 11 `UTO` unit timeout. Clear individual flag **and INT** to re-arm.
QEPSTS (3Ah, reset 0x8E) reports first-index/direction/error status (UPEVNT, FIDF, QDF, QDLF, COEF, CDEF, FIMF, PCEF).

#### Start-up
Enable clock domain (VCLK4) + pin mux → `QDECCTL` (QSRC=0, XCR per resolution, polarity) → `QPOSMAX` → `QPOSINIT`/`QPOSCNT` → `QEPCTL` (`PCRM`, `QPEN=1`, `UTE=1`, `WDE=1` as needed) → `QUPRD`, `QCAPCTL` → `QEINT` + VIM.

#### Gotchas
- `QPEN=0` holds the counter in reset.
- Index-based reset needs IEI/PCRM set; otherwise counter free-runs 0…QPOSMAX.
- Phase error flag indicates missing/ simultaneous A/B transitions (bad wiring/noise).

---

## 17. HTU (High-End Timer Transfer Unit)

**Context:** Dedicated mini-DMA that moves data between **N2HET RAM** and main RAM (or flash/peripheral address space) without CPU access, triggered by N2HET instruction requests. Not a general DMA — it only talks to its paired N2HET (HTU1↔N2HET1, HTU2↔N2HET2). 8 "double control packets" (DCP0–7), each with two buffers A and B (CP A / CP B). Use with §09. 32- or 64-bit transactions.

#### Addresses
HTU1 registers `0xFFF7A400`, HTU2 `0xFFF7A500`. Control-packet RAM (parity protected): **HTU1 `0xFF4E0000`, HTU2 `0xFF4C0000`** (initialize via system RAM auto-init: MSINENA bit 4 for HTU1, bit 16 for HTU2).

#### Control packets (RAM, 4 words per DCP; DCPx at `base + 0x10·x`)
| Off | Reg | Content |
|---|---|---|
| +00 | IFADDRA | initial main-memory address of buffer A (32-bit) |
| +04 | IFADDRB | initial main-memory address of buffer B |
| +08 | IHADDRCT | `DIR`[23] (0 = N2HET→memory, 1 = memory→N2HET) · `SIZE`[22] (0 = 32-bit, 1 = 64-bit) · `ADDMH`[21] (0 = N2HET address +16 B per element, 1 = +8 B — use with 64-bit to load instruction blocks) · `ADDMF`[20] (0 = post-increment main address, 1 = constant) · `TMBA`[19:18], `TMBB`[17:16] transfer mode (0 = one-shot, 1 = circular; **auto-switch mode** also exists — see mode table) · `IHADDR`[12:2] initial N2HET word address |
| +0C | ITCOUNT | `IETCOUNT`[20:16] elements per frame · `IFTCOUNT`[7:0] frames per buffer |
Current (working) values at **`base + 0x100 + 0x10·x`:** CFADDRA (+00), CFADDRB (+04), CFCOUNT (+08: `CFTCTA`[23:16], `CFTCTB`[7:0] = frames done, updated at frame end). Clear these before starting.
64-bit transfer from an N2HET instruction transfers Control Field + Data Field together (CF at word n, DF at n+1); pointing at PF transfers PF+CF.

#### Buffer modes
| CP A mode | CP B mode | Behavior |
|---|---|---|
| one-shot | (unused) | buffer A fills once; buffer-full interrupt says done |
| auto-switch | one-shot | doubles the buffer size (A then B) |
| circular | circular | CPU toggles between A/B at will; the "frozen" buffer is read while HTU fills the other; no interrupts needed (BFINTFL flags circular overrun) |
| auto-switch | auto-switch | A fills, interrupt, HTU switches to B while CPU reads A, and so on (ping-pong) |

#### Registers (offset from base)
00 **HTU GC** (`VBUSHOLD`[24], **`HTUEN`[16]**, `DEBM`[8] 1 = keep running in debug, `HTURES`[0] soft reset) · 04 **CPENA** (bits 2x/2x+1 control DCP x) · 08/0C/10/14 BUSY0–3 · 18 ACPE (active CP + error) · 20 RLBECTRL (request-lost / bus-error interrupt enables) · 24 BFINTS / 28 BFINTC (buffer-full int set/clear) · 2C INTMAP (line A/B routing) · 34 INTOFF0 / 38 INTOFF1 (pending interrupt: `INTYPE` + `CPOFF` code; CPOFF Ah = DCP5/CP A, etc.) · 3C BIM (buffer init mode) · 40 RLOSTFL · 44 BFINTFL · 48 BERINTFL · 4C/50 MP1S/MP1E · 54 DCTRL · 58 WPR · 5C WMR · 60 ID · 64 PCR / 68 PAR (parity) · 70 MPCS · 74/78 MP0S/MP0E (memory-protection regions).

##### CPENA encoding (write / read)
Write: `01` = enable CP A, disable CP B simultaneously · `10` = enable CP B, disable CP A · `11` = disable both · `00` = no change. Read: `00` DCP disabled · `01` A enabled · `10` B enabled. Switching A↔B keeps pending requests; 00→01/10 clears old pending requests. Certain conditions (request lost, bus error, mem-protection violation, one-shot end) auto-disable a DCP (bits set to 00).

#### Setup sequence
1. Init HTU RAM (MSINENA). 2. For each DCP: IFADDRA/B, IHADDRCT, ITCOUNT; clear CFADDRA/B, CFCOUNT. 3. Clear BFINTFL/RLOSTFL. 4. Configure N2HET program to raise **requests** (HTU request lines) at the right instruction (`HETREQENS`, `HETREQDS`). 5. Interrupt mapping/enables (BFINTS, RLBECTRL, INTMAP) + VIM. 6. `GC.HTUEN=1`. 7. `CPENA` to start CP A (and/or B).
Circular ping-pong readout: switch CPENA, wait BUSY=0 for the frozen CP, read CFTCT for fill level, check RLOSTFL/BER, read buffer, reset CF* and clear BFINTFL.

#### Request lost / quiet requests / protection
- **Request lost:** a new N2HET request arrives while the DCP is still transferring (frame not finished) → RLOSTFL; DCP can auto-disable. **Quiet requests** (don't count) used for control.
- Arbitration: lower DCP number = higher priority; transfers can be interrupted at frame boundaries.
- Memory protection: window MP0 (+ MP1) restricts main-memory addresses; violation → flag + DCP disabled. Parity on packet RAM.
- BER (bus error) flag if the memory transaction fails.

#### Gotchas
- HTU needs `HTUEN` and the **N2HET program to issue requests**; no requests → nothing moves.
- Initialize control-packet RAM (parity) before use.
- `VBUSHOLD` can hog the VBUS — leave 0 unless needed.
- N2HET addresses in IHADDR are word addresses of N2HET RAM; element index step depends on ADDMH/SIZE.

---

## 18. Safety / Self-Test Modules (register level): CCM-R4F, STC, PBIST, DCC, CRC

**Context:** The device is a lock-step dual Cortex-R4F safety MCU (1oo1D). These modules provide boot-time and run-time diagnostics. All errors surface through the ESM (see §03). Typical safety boot order: PLL/clock setup → PBIST (RAM) → STC (CPU LBIST) → RAM auto-init → CCM self-test → enable ECC → application. Control registers are privileged-write.

---
### CCM-R4F — CPU lock-step compare (base `0xFFFFF600`)
- Compares ~900 signals from the two cores (master delayed 2 cycles, checker input delayed 2 cycles → temporal diversity). Comparison starts 6 CPU cycles after reset release, runs without software. Mismatch → ESM flag "CCM-R4F compare" (and also "self-test error" path for redundancy).
- Both CPUs' registers must be initialized to identical values before use (otherwise false compare errors after reset: init all core registers incl. banked/FPU at start-up).
- **Registers:** `00 CCMSR` (`CMPE`[16] compare error, `STC`[8] self-test complete, `STET`[1] self-test error type, `STE`[0] self-test error) · **`04 CCMKEYR` (`MKEY`[3:0]): `0x0` lock-step, `0x6` self-test, `0x9` error-forcing, `0xF` self-test error-forcing** (write other values → undefined; privileged write).
- **Self-test** (MKEY=6): runs Compare Match test (all-1s, all-0s, 0xA…, 0x5… patterns, 4 cycles) then Compare Mismatch test (one bit flipped per position, 2×N patterns). Compare error output to ESM is suppressed during test; faults → ESM "CCM-R4F self-test". Stays in self-test mode after completion — **you must write MKEY=0 to return to lock step**. Poll `CCMSR.STC`.
- Error-forcing modes deliberately generate a compare error to test the ESM path.

---
### STC — CPU self-test (LBIST) (base `0xFFFFE600`)
- Deterministic logic BIST on the CPU; **24 intervals**; STCCLK = HCLK / `STCCLKDIV.CLKDIV` (STCCLKDIV in SYS2 module, offset 08h; set before run). Core is isolated from the bus during test; DMA accesses to TCM held until done. **STC always ends by resetting the CPU** (pass or fail) → check `STCGSTAT` on restart.
- **Registers:** 00 `STCGCR0` (`INTCOUNT`[31:16] number of intervals to run, 1–24; `RS_CNT`[0] 1 = restart from interval 0, 0 = continue; self-clears) · 04 `STCGCR1` (`STC_ENA`[3:0] **0xA = start**) · 08 `STCTPR` (timeout preload, VBUS cycles) · 0C `STC_CADDR` (current ROM address) · 10 `STCCICR` (current interval) · **14 `STCGSTAT` (`TEST_DONE`[0], `TEST_FAIL`[1], timeout/error bits)** · 18 `STCFSTAT` (fail status; failing interval) · 1C–28 `CPU1_CURMISR3..0` · 2C–38 `CPU2_CURMISR3..0` · 3C `STCSCSCR` (signature compare self-check).
- **Flow:** clear `SYSESR` flags → `STCCLKDIV` → `STCGCR0` (intervals, RS_CNT) → `STCTPR` → `STCGCR1 = 0xA`. CPU resets, then in startup code: read `STCGSTAT` (TEST_DONE=1, TEST_FAIL=0 → pass), clear flags, continue boot. Check that the reset cause in SYSESR shows the CPU-reset-by-STC and **don't re-run the test again from that reset path**.
- If running in slices during normal operation, save/restore CPU registers (R0–R15, PC, CPSR, CP15 MPU/ACR/ECC enable, CP13 FPU regs, breakpoint/watchpoint regs) around the reset.
- Duration/coverage trade-off: more intervals = higher coverage (full 24 intervals gives the quoted coverage).

---
### PBIST — memory BIST (base `0xFFFFE400`)
- Runs on-chip ROM algorithms (e.g., March13N, Down1A, Map Column) over RAM groups (list of groups/algorithms: Tables 2-5/2-6 of TRM and device datasheet). **Destroys contents of tested memory** → run before app data exists (or only on RAMs not yet used). Needs PLL stable; PBIST ROM clock = HCLK ÷ ratio (`MSTGCR[9:8]`, e.g. 1 → 1:2).
- **Registers:** 160 `RAMT` (RGS/RDS of failing RAM) · 164 `DLR` (datalogger; 0x14 = ROM mode) · 16C `STR` (run/resume: 2 = resume) · **180 `PACT`** (1 = clocks; 3 = clocks + ROM clock) · 184 `PBISTID` · 188 `OVER` (0 = use RINFO/ALGO from application, 1 = override) · 190 `FSRF0` (fail flag) · 198/19C `FSRC0/1` (fail count) · 1A0/1A4 `FSRA0/1` (first fail address) · 1A8/1B0 `FSRDL0/1` (fail data) · **1C0 `ROM`** (0x3 = load algorithm & RAM info from ROM) · 1C4 `ALGO` (bit mask of algorithms) · 1C8/1CC `RINFOL/U` (bit mask of RAM groups; 28 groups on this device).
- **System-module registers involved:** `MSIENA` (bit 0 enables PBIST), `MSTGCR` (`[3:0]` 0xA = enable self-test, 0x5 = disable; `[9:8]` ROM clock ratio), `MSTDONE`/`MSTCGSTAT.MSTDONE` (poll for completion).
- **Flow (RAM group 3 example, HCLK=160 MHz):** `MSTGCR[9:8]=1` → `MSIENA=1` → `MSTGCR[3:0]=0xA` → wait ≥32 VCLK → `PACT=1` → `OVER=0` → `ALGO=0x54` (algos 3,5,7: March13N, Down1A, Map column) → `RINFOL=0x4`, `RINFOU=0` (group 3) → `ROM=0x3` → `DLR=0x14` → `STR=0x2` → poll MSTDONE → check `FSRF0`: 0 = pass → `PACT=0`, `MSTGCR[3:0]=0x5`; 1 = fail → read RAMT/FSRC/FSRA/FSRDL, optionally resume (STR=2).
- To test all groups, set all RAM-group and algorithm bits (Example 2 in TRM).

---
### DCC1 / DCC2 — dual-clock comparator (bases `0xFFFFEC00` / `0xFFFFF400`)
- Counts **clock0** (reference, with counter0 + tolerance-window `valid0`) vs **clock1** (clock under test, counter1). Both counters start together; when counter0 reaches 0, valid0 starts counting; clock1's counter1 must hit 0 **at the same time as counter0 or while valid0 is counting**. Error if counter1 reaches 0 before counter0 (clock1 too fast / clock0 stuck) or doesn't reach 0 by the time valid0 expires (clock1 too slow / stuck). Error freezes counters (read them for diagnosis).
- **Modes:** continuous (reloads seeds after each OK window) or single-shot (`SINGLESHOT` = 0xA stops when counter0+valid0 reach 0, 0xB when counter1 reaches 0 — "counter1 stop" mode used to measure an unknown frequency).
- **Registers:** 00 `DCCGCTRL` (`DCCENA`[3:0] **0xA enable / 5 disable (stops counters)** · `ERRENA`[7:4] 0xA = error interrupt enable, 5 = off · `SINGLESHOT`[11:8] 0xA = single-shot, 5 = continuous · `DONEENA`[15:12] 0xA = done interrupt) · 04 `DCCREV` · 08 `DCCCNT0SEED` · 0C `DCCVALID0SEED` · 10 `DCCCNT1SEED` · 14 `DCCSTAT` (`ERR`[0], `DONE`[1]) · 18 `DCCCNT0` · 1C `DCCVALID0` · 20 `DCCCNT1` · 24 `DCCCNT1CLKSRC` · 28 `DCCCNT0CLKSRC` (KEY[15:12] = 0xA to select source; clock-source numbers per datasheet).
- **Seeding:** pick `seed0` so that `T_window = seed0 / f0`; `seed1 = f1_expected × seed0 / f0`; `valid0` = tolerance in clock0 cycles (± tolerance window). Both clocks must be running for the duration. Errors raise ESM (DCC1/2 channels per datasheet).
- Typical use: monitor PLL vs OSCIN, HF LPO vs OSCIN; or measure oscillator frequency once at boot.

---
### CRC controller (base `0xFE000000`)
- 2 channels (CH1, CH2) computing **PSA signatures** (64-bit maximal-length, parallel signature analysis) over 8/16/32/64-bit data; compares to preloaded reference per sector. Modes (CRC_CTRL2 `CHx_MODE`): **0 Data capture (no compression, raw data register)**, **1 AUTO** (CRC controller + DMA do everything, interrupts on sector/block done/fail), **2 Semi-CPU** (DMA moves data, CPU checks signature), **3 Full-CPU** (CPU writes data and reads signature; no interrupts/DMA).
- **Registers:** 00 `CRC_CTRL0` (`CH1_PSA_SWREST`[0], `CH2_PSA_SWREST`[8]) · 08 `CRC_CTRL1` (`PWDN`[0]) · 10 `CRC_CTRL2` (`CH1_MODE`[1:0], `CH1_TRACEEN`[4], `CH2_MODE`[9:8], `CH2_TRACEEN`[12]) · 18 `CRC_INTS` / 20 `CRC_INTR` (int enable set/reset) · 28 `CRC_STATUS` · 30 `CRC_INT_OFFSET` (codes: 1/2 CRC fail ch1/ch2, 9/Ah compression complete, 11h/12h overrun, 19h/1Ah underrun, 21h/22h timeout) · 38 `CRC_BUSY` · per channel (ch1 at 40h–7Ch, ch2 at 80h–BCh): `PCOUNT_REG` (pattern count per sector) · `SCOUNT_REG` (sector count) · `CURSEC_REG` · `WDTOPLD` (DMA-request watchdog timeout) · `BCTOPLD` (block-complete timeout) · `PSA_SIGREGL/H` (signature) · `CRC_REGL/H` (reference/CRC value) · `PSA_SECSIGREGL/H` (sector signature) · `RAW_DATAREGL/H`.
- Full-CPU use: reset PSA (`CHx_PSA_SWREST`), set mode 3, write data patterns to the PSA data port, read `PSA_SIGREGL/H`, compare against known-good signature.

#### Gotchas
- STC always resets the CPU; ensure startup handles that without looping.
- CCM stays in self-test mode until MKEY=0 is written.
- PBIST overwrites RAM — only on unused RAM or before init.
- DCC needs both clocks running; error state freezes counters until DCC is disabled/restarted (write 5 to DCCENA then re-enable).
- ESM flags from all of these must be checked/cleared after tests.

---

## 19. TCRAM (data RAM wrapper) + PMM (power domains)

### TCRAM
**Context:** CPU data RAM on the Cortex-R4F **BTCM** interface (flash is on ATCM). Split into two banks, **B0TCM (even) / B1TCM (odd)**, each 36-bit wide (32 data + 4 ECC) → a 64-bit word + 8 ECC bits. Size per datasheet (≤256 KB BTCM). **ECC (SECDED) is evaluated by the CPU; the wrapper stores ECC in RAM and reports errors.** Address parity and redundant address-decode checking too.
**Registers:** even RAM ECC wrapper `0xFFFFF800` (B0), odd `0xFFFFF900` (B1). Same layout in each.

#### Memory map
Data RAM at `0x08000000` (e.g. 192 KB); ECC space **+4 MB** (`0x08400000`). Unimplemented addresses → error response. Access sizes 8/16/32/64-bit (no bit-wise ops). ECC memory writes disabled by default (only for test).

#### Registers (offset)
00 **RAMCTRL** (reset `0x0005000A`): `ECC_DETECT_EN`[3:0] (**0xA = ECC detect enabled (reset default), 0x5 = disabled**) · `ECC_WR_EN`[8] (1 = allow writing ECC memory; test only) · `ADDR_PARITY_DISABLE`[19:16] (0xA disables address-parity check; 5 = enabled) · `ADDR_PARITY_OVERRIDE`[27:24] (0xD inverts parity scheme, test)
04 RAMTHRESHOLD (single-bit error count threshold) · 08 RAMOCCUR (occurrence counter) · 0C RAMINTCTRL (interrupt enables) · **10 RAMERRSTATUS** (`WADDR_PAR_FAIL`[9], `RADDR_PAR_FAIL`[8], `DERR`[5] multi-bit error, `ADDR_COMP_LOGIC_FAIL`[4]; other bits report single-bit/other events (TRM 6.7.5); write 1 to clear) · 14 RAMSERRADDR (single-bit error address) · 1C RAMUERRADDR (uncorrectable) · 30 RAMTEST · 38 RAMADDRDECVECT · 3C RAMPERRADDR · **40 INIT_DOMAIN** (`AUTO_MEM_INIT_DOMAIN`[7:0], reset 0xFF = auto-init enabled for all power domains).
#### Using ECC
1. Run RAM auto-init (§01, MSINENA bit 0) so ECC bits are valid. 2. **Enable ECC in the CPU** via the Cortex-R4F auxiliary control register (see ARM Cortex-R4F TRM; TI start-up code sets the TCM ECC enable bits) — the wrapper only counts/reports. 3. Keep `RAMCTRL.ECC_DETECT_EN = 0xA`. 4. Single-bit errors are corrected by CPU logic and counted (compare to RAMTHRESHOLD → ESM interrupt); double-bit errors → data abort + ESM group-2 error.
Common gotcha: reading uninitialized RAM with ECC enabled raises ECC errors.

---
### PMM — Power Management Module
**Base `0xFFFF0000`.** Core logic split into domains: **PD1 always on** (CPU, flash interface, clock control, basic peripherals, I/O, 64 KB TCRAM) · **PD2:** POM + test logic · **PD3:** N2HET2, HTU2, ADC2, SCI, I2C, DCAN3, SPI4, DCC2 · **PD4:** FlexRay + FTU · **PD5:** EMAC, EMIF, ePWMx, eCAPx, eQEPx · **RAM_PD1 / RAM_PD2:** 64 KB TCRAM each.
**Practical consequence:** if a peripheral in PD3/PD5 "doesn't respond", check whether its domain is powered/clocked (default state of each domain comes from TI-OTP reset configuration; TI can also permanently disable a domain — then you can't turn it on, and **no error is raised**).
- Domain states: Active / Idle (clocks off, still powered) / Off. **Switching is static only:** change a domain's state once at init; to change again, assert a system reset first.
- Registers (offset from base): 00 `LOGICPDPWRCTRL0` (`LOGICPDON0`[27:24]=PD2, `LOGICPDON1`[19:16]=PD3, `LOGICPDON2`[11:8]=PD4, `LOGICPDON3`[3:0]=PD5; **0xA write = command OFF**; another key value = ON, see TRM Table 3-2) · 10 `MEMPDPWRCTRL0` · 20 `PDCLKDIS` / 24 `PDCLKDISSET` / 28 `PDCLKDISCLR` (per-domain clock disable) · 40/44/48/4C `LOGICPDPWRSTAT0–3` (PD2–PD5 status; `LOGICPDPWR_STAT`, `DOMAIN_ON`) · 80/84 `MEMPDPWRSTAT0/1` · A0 `GLOBALCTRL1` · A8 `GLOBALSTAT` · AC `PRCKEYREG` (PSCON diagnostic compare key) · B8/BC `MPDDCSTAT1/2`, C0 `ISODIAGSTAT` (diagnostic status).
- **Power OFF a logic domain (PD2–5):** `PDCLKDIS` disable all clocks → write 0xA to the domain's nibble in `LOGICPDPWRCTRL0` → poll `LOGICPDPWRSTATx` = 00 (off). RAM-only domains skip the clock step.
- **Power ON:** write the ON key to `LOGICPDONx` (or `MEMPDONx`); PMM restarts clocks automatically if "auto clock enable on wake" is selected, else clear `PDCLKDIS` bit **after** polling `LOGICPDPWRSTATx.DOMAIN_ONx` = on. PMM resets the domain when turning it on.
- Two PSCONs (primary + diagnostic, lock-step, with compare modes) control each domain for safety; compare errors → ESM.
- PMM generates nPORRST / nRST / nPRST / nTRST per domain.

---

## 20. POM (Parameter Overlay Module)

**Context:** Lets the CPU's accesses to read-only flash be redirected to RAM (internal or external via EMIF) so calibration parameters/code can be changed live (development/calibration use). Not used in normal BMS firmware. Lives in power domain **PD2** (see §19). Registers base **`0xFFA04000`** (privileged).

#### Rules
- Up to **32 regions**; each region replaces a flash address range with an overlay-memory range of the same size. Region size 64 B … 256 KB (powers of 2). Region start addresses (program and overlay) are 23-bit (8 MB window) and **must be multiples of the region size**.
- **Only the main CPU is redirected.** Any other bus master (DMA, etc.) accessing an overlaid flash range → deadlock/hang; other masters must access the overlay RAM directly.
- Overlay memory is read-only through POM; write to the RAM at its normal address.
- **Disable CPU ECC (CP15) while overlay is enabled,** otherwise ECC errors (overlay RAM has no matching ECC).
- **Do not enable POM while flash/RAM are swapped** (`BMMCR1.MEM SWAP`).
- Wait states inserted automatically if overlay memory is slower.
- **Bus-contention timeout:** when overlaying flash, another master accessing the TCM flash can hang the bus. Enable `POMGLBCTRL.ETO = 0xA`; if the POM read can't finish within **32 HCLK**, flag `POMFLG.TO` sets and the CPU gets a prefetch/data abort. Abort handlers must check `TO`, clear it, and treat it as POM contention.

#### Registers (offset)
| Off | Reg | Fields |
|---|---|---|
| 00 | POMGLBCTRL | `OTADDR`[31:23] overlay target address (reset 0x60 → 0x60000000 EMIF CS2 base region), `ETO`[11:8] 0xA = timeout enabled, `ON/OFF`[3:0] **0xA = POM enabled** (functional) |
| 04 | POMREV | revision |
| 08 | POMCLKCTRL | `CLK GATE OFF`[0] — leave at reset value |
| 0C | POMFLG | `TO`[0] timeout flag |
| 200+0x10·x | POMPROGSTARTx | `STARTADDRESS`[22:0] flash-region start |
| 204+0x10·x | POMOVLSTARTx | overlay-region start (relative to OTADDR target) |
| 208+0x10·x | POMREGSIZEx | `SIZE`[3:0]: 0 = region disabled, 1 = 64 B, 2 = 128 B, … up to 0xD = 256 KB (power-of-2 steps) |
| F00–FD4 | ITCTRL, CLAIMSET/CLR, LOCKACCESS/STATUS, AUTHSTATUS, DEVID, DEVTYPE, PERIPHERAL/COMPONENT IDs | CoreSight-style debug/ID regs |
x = 0…31 (0x200…0x3F8).

#### Typical sequence
1. Ensure PD2 on; disable ECC in CP15; load overlay data into target RAM. 2. Program region(s): PROGSTART, OVLSTART, REGSIZE. 3. `POMGLBCTRL`: set `OTADDR`, `ETO=0xA`, `ON/OFF=0xA`. 4. Install abort handlers that check/clear `POMFLG.TO`. 5. To disable: `ON/OFF` ≠ 0xA.

#### Gotchas
- Size 0 disables a region; non-aligned starts misbehave.
- DMA/other masters must not touch overlaid flash.
- Overlay doesn't provide ECC → CPU ECC must be off.

---

## 21. EMIF (External Memory Interface)

**Context:** 16-bit external bus for **SDR SDRAM** (one chip select, nCS[0]) and **asynchronous devices** (SRAM, NOR flash; up to 3 chip selects nCS[2..4]). Not used in a minimal BMS board. Clock: EMIF_CLK = **VCLK3** domain (default = OSCIN/2; HCLK÷1–16 via system module). Registers at **`0xFCFFE800`** (privileged). Needs PD5 powered (PMM) and pin mux (IOMM; EMIF pins shared with N2HET2/etc.).
**Data memory map:** async CS2 `0x60000000`, CS3 `0x64000000`, CS4 `0x68000000`; SDRAM CS0 `0x80000000–0x87FFFFFF` (see §01). Pins: EMIF_DATA[15:0], EMIF_ADDR[12:0], EMIF_BA[1:0], nCS[0], nCS[4:2], nOE, nWE, nWAIT (extended wait), CLK, CKE, nCAS, nRAS, nDQM[1:0].

#### Supported memories
- **SDRAM:** SDR only, **16-bit bus**, 1/2/4 banks, 8–11 column bits, CAS latency 2 or 3, 3.3 V LVCMOS; self-refresh and power-down supported. No mobile SDRAM.
- **Async:** 8- or 16-bit width (`ASIZE`: 0 = 8-bit, 1 = 16-bit; ASIZE also decides EMIF_A/BA pin functions — see TRM 17.2.6.1), programmable setup/strobe/hold per read/write, turnaround, optional external wait (nWAIT) with timeout, "select strobe" mode.

#### Register map (offset)
08 **SDCR** · 0C **SDRCR** · 10 **CE2CFG** · 14 CE3CFG · 18 CE4CFG · 1C CE5CFG (table lists four async configs; only three chip selects are pinned out) · 20 **SDTIMR** · 3C SDSRETR · 40 INTRAW · 44 INTMSK · 48 INTMSKSET · 4C INTMSKCLR · 68 PMCR (page mode control). Also RCSR/AWCCR (async wait config) near the start of the map.

##### SDCR (08h)
`SR`[31] self-refresh · `PD`[30] power-down · `PDWR`[29] refresh during power-down · `NM`[14] narrow mode (1 = 16-bit SDRAM, 0 = 32-bit) · `CL`[11:9] CAS latency (2 or 3) — **writable only if `BIT11_9LOCK`[8] = 1 in the same write** · `IBANK`[6:4] (0 = 1 bank, 1 = 2, 2 = 4) · `PAGESIZE`[2:0] (0 = 8 col bits/256 elements, 1 = 9, 2 = 10, 3 = 11). **Writing SDCR (bank/page/CL) triggers the SDRAM initialization sequence**; to set `SR` without that, use a **byte write to the upper byte**.
##### SDRCR (0Ch)
`RR`[12:0] refresh period in EMIF_CLK cycles = `t_refresh_interval × f_EMIF_CLK` (e.g. 64 ms/4096 rows = 15.6 µs → 1560 at 100 MHz; use slightly less).
##### SDTIMR (20h) (all values = cycles − 1)
`T_RFC`[31:27], `T_RP`[26:24], `T_RCD`[22:20], `T_WR`[18:16], `T_RAS`[15:12], `T_RC`[11:8], `T_RRD`[6:4] — set from the SDRAM datasheet at the chosen EMIF_CLK.
##### CExCFG (async, 10h/14h/18h)
`SS`[31] select-strobe mode · `EW`[30] extend wait (use nWAIT) · `W_SETUP`[29:26] · `W_STROBE`[25:20] · `W_HOLD`[19:17] · `R_SETUP`[16:13] · `R_STROBE`[12:7] · `R_HOLD`[6:4] (each = EMIF_CLK cycles − 1) · `TA`[3:2] minimum turn-around between accesses · `ASIZE`[1:0] bus width. Timing values come from memory datasheet access times: strobe ≥ access time, setup/hold per address/data setup specs.

#### SDRAM bring-up (example flow)
1. Power PD5, pin-mux EMIF, set VCLK3 / EMIF_CLK. **To change the EMIF clock after init:** put SDRAM in self-refresh first (`SDCR.SR=1` by upper-byte write), change VCLK3 source/divider, then clear SR (byte write). 2. `SDTIMR` and `SDRCR`. 3. `SDCR` with CL/IBANK/PAGESIZE/NM (+ BIT11_9LOCK=1) — this starts the init sequence. 4. SDRAM is usable at `0x80000000`.
#### Async flash/SRAM bring-up
`CExCFG` timing + `ASIZE` + (optional) `EW`/SS; wire RD/nBY to EMIF_nWAIT if using external wait. Access through the chip-select region.
#### Interrupts
INTRAW/INTMSK: asynchronous timeout (`AT`), line trap (`LT`), wait-rise (`WR`) — enable via INTMSKSET.

#### Gotchas
- SDCR writes restart SDRAM init; use byte-write for SR/PD.
- 16-bit data bus only (NM=1 for 16-bit SDRAM).
- EMIF clock = VCLK3; check it is stable before init.
- Chip-select ranges differ from the POM overlay target window (OTADDR default 0x60 → CS2 area).

---

## 22. EMAC + MDIO (10/100 Ethernet MAC)

**Context:** IEEE 802.3 10/100 Mbit MAC with MII or RMII to an external PHY, DMA bus master with 8 TX / 8 RX channels, 8 KB local descriptor RAM (CPPI RAM), statistics counters, MDIO master for PHY management. Not used in a typical BMS. Lives in power domain PD5 (see PMM). Clock: logic on **VCLKA4**; MDIO clock = VCLK3 ÷ divider (≤2.5 MHz, typically 1 MHz). PHY supplies TX/RX clocks (MII: 2.5/25 MHz; **RMII: 50 MHz ref clock for both speeds**).

#### Base addresses
EMAC module `0xFCF78000` · EMAC control module `0xFCF78800` · MDIO `0xFCF78900` · CPPI descriptor RAM 8 KB (start = EMAC RAM base in memory map, 2K×32; descriptors are 16 B → up to 512 packets).

#### Pin muxing (IOMM, one-hot byte per signal; set the listed value in that PINMMR byte)
**MDIO:** MDIO_CLK = PINMMR7[15:8] = 0x04 · MDIO_D = PINMMR8[15:8] = 0x04.
**MII / RMII** (PINMMR byte; first value MII, second RMII):
TXCLK PINMMR17[7:0]=0x02 (MII only) · TXD3 14[7:0]=0x04 · TXD2 13[31:24]=0x04 · TXD1 13[15:8]=0x04 / **0x08** · TXD0 13[7:0]=0x04 / **0x08** · TXEN 13[23:16]=0x04 / **0x08** · COL 20[23:16]=0x04 · CRS(DV) 17[23:16]=0x02 / **0x04** · RXCLK / RMII 50 MHz clk 14[15:8]=0x02 / **0x04** · RXD3 12[31:24]=0x04 · RXD2 12[23:16]=0x04 · RXD1 12[7:0]=0x02 / **0x04** · RXD0 11[31:24]=0x04 / **0x08** · RXDV 19[15:8]=0x02 · RXER 10[7:0]=0x02 / **0x04**. (Unlock IOMM kick registers first.) RMII needs only 7 signals + ref clock; MII/RMII selection also in IOMM (Ethernet mode select).

#### Descriptor format (4 words, 32-bit aligned, in CPPI RAM or other RAM)
| Word | TX | RX |
|---|---|---|
| 0 | Next descriptor pointer (0 = end of queue) | same |
| 1 | Buffer pointer (byte-aligned) | same |
| 2 | `[31:16]` buffer offset · `[15:0]` buffer length | same (length = free space; EMAC writes actual) |
| 3 | `[31:16]` flags · `[15:0]` packet length (on SOP descriptor) | flags + packet length |
TX flags (word 3 bits): `SOP`31, `EOP`30, **`OWNER`29** (1 = owned by EMAC), `EOQ`28, `TDOWNCMPLT`27, `PASSCRC`26 (software supplies CRC).
RX flags: SOP/EOP/OWNER/EOQ/TDOWNCMPLT/PASSCRC plus status: Jabber, Oversize, Fragment, Undersized, Control, Overrun, CodeError, AlignError, CRCError, NoMatch.
C struct: `{pNext, pBuffer, BufOffLen, PktFlgLen}`; `EMAC_DSC_FLAG_SOP 0x80000000, EOP 0x40000000, OWNER 0x20000000, EOQ 0x10000000, TDOWNCMPLT 0x08000000, PASSCRC 0x04000000`.
- Buffer offset only checked on SOP descriptor; legal 0…(length−1). Packet length on SOP descriptor = whole packet length; fragments chained via next pointer.
- **Queues:** software chains descriptors; EMAC clears OWNER on completion; sets **EOQ** on last descriptor when it hits NULL next → if software appended a packet after EMAC already read NULL next, the channel halts and software must restart it by writing HDP (detect EOQ on returned descriptor).
- **TX start:** fill descriptor(s) (OWNER|SOP|EOP, lengths) → write head pointer to `TXnHDP` (n = channel). Completion: ISR reads `TXnCP` / TXINTSTATMASKED, walks list until OWNER=1/EOQ, then acknowledges by writing the completed descriptor address to `TXnCP`. Interrupts are level; clear by writing CP (and EOI via `MACEOIVECTOR`). **RX:** prepare empty buffer descriptors (OWNER=1) and write head to `RXnHDP`; on completion walk list, process, ack via `RXnCP`, re-queue buffers.
- Descriptor memory in EMAC CPPI RAM gives best throughput.

#### Register map highlights
**Control module (0xFCF78800):** 00 REVID · 04 SOFTRESET · 0C INTCONTROL (pacing) · 10 C0RXTHRESHEN · 14 C0RXEN · 18 C0TXEN · 1C C0MISCEN · 40 C0RXTHRESHSTAT · 44 C0RXSTAT · 48 C0TXSTAT · 4C C0MISCSTAT · 70 C0RXIMAX · 74 C0TXIMAX (interrupts per ms pacing).
**MDIO (0xFCF78900):** 00 REVID · **04 CONTROL** (`IDLE`31, **`ENABLE`30**, `HIGHEST_USER_CHANNEL`[28:24], `PREAMBLE`20 (1 = disable preamble), `FAULT`19, `FAULTENB`18, **`CLKDIV`[15:0]** → MDIO_clk = VCLK3/(CLKDIV+1): e.g. 50 MHz → CLKDIV 49–50 for ~1 MHz) · 08 ALIVE · 0C LINK · 10/14 LINKINT RAW/MASKED · 20–2C USERINT RAW/MASKED/MASKSET/MASKCLR · **80 USERACCESS0** (`GO`31, `WRITE`30, `ACK`29, `REGADR`[25:21], `PHYADR`[20:16], `DATA`[15:0]) · 84 USERPHYSEL0 (`LINKSEL`7, `LINKINTENB`6, `PHYADRMON`[4:0]) · 88/8C USERACCESS1/USERPHYSEL1.
**EMAC (0xFCF78000):** 00 TXREVID · 04 TXCONTROL (`TXEN`[0]) · 08 TXTEARDOWN · 10 RXREVID · 14 RXCONTROL (`RXEN`[0]) · 18 RXTEARDOWN · 80–8C TX int stat raw/masked/mask set/clear · 90 MACINVECTOR · 94 MACEOIVECTOR · A0–AC RX int regs · B0–BC MAC int regs (HOSTMASK, STATMASK) · **100 RXMBPENABLE** (`RXPASSCRC`30, `RXQOSEN`29, `RXNOCHAIN`28, `RXCMFEN`24, `RXCSFEN`23, `RXCEFEN`22, `RXCAFEN`21 promiscuous; broadcast/multicast enables + channel select fields below) · 104/108 RXUNICASTSET/CLEAR · 10C RXMAXLEN · 110 RXBUFFEROFFSET · 114 RXFILTERLOWTHRESH · 120–13C RXnFLOWTHRESH · 140–15C RXnFREEBUFFER · **160 MACCONTROL** · 164 MACSTATUS · 168 EMCONTROL · 16C FIFOCONTROL · 170 MACCONFIG · 174 SOFTRESET · 1D0/1D4 MACSRCADDRLO/HI · 1D8/1DC MACHASH1/2 · 1E8 RXPAUSE · 1EC TXPAUSE · 200–2xx statistics (RXGOODFRAMES 200, RXBCASTFRAMES 204, …) · **500/504/508 MACADDRLO/HI/INDEX** · **600–61C TXnHDP · 620–63C RXnHDP · 640–65C TXnCP · 660–67C RXnCP**.
**MACCONTROL (160h) bits:** `RMIISPEED`15 (1 = 100 Mbit RMII) · `RXOFFLENBLOCK`14 · `RXOWNERSHIP`13 · `CMDIDLE`11 · `TXSHORTGAPEN`10 · `TXPTYPE`9 (0 round-robin, 1 fixed priority) · `TXPACE`6 · **`GMIIEN`5 (must be 1 for any TX/RX)** · `TXFLOWEN`4 · lower bits: full-duplex, loopback, RX buffer flow control, per datasheet table 32-xx.

#### Initialization order (EMAC module)
1. Disable control-module interrupts (C0RXTHRESHEN/C0RXEN/C0TXEN/C0MISCEN = 0). 2. Clear MACCONTROL, RXCONTROL, TXCONTROL. 3. Zero all 16 HDP regs. 4. Zero 36 statistics regs. 5. Program MAC address: for each of 8 channels write `MACINDEX`, `MACADDRHI`, `MACADDRLO` (duplicate same address into unused channels; start at ch0). 6. (Flow control: RXnFREEBUFFER/FLOWTHRESH/FILTERLOWTHRESH.) 7. Clear MACHASH1/2. 8. `RXBUFFEROFFSET` (0). 9. `RXUNICASTCLEAR = 0xFF`, then `RXUNICASTSET` for wanted channel(s). 10. `RXMBPENABLE` (broadcast/multicast/promiscuous). 11. `MACCONTROL` config **without GMIIEN**. 12. Clear unused channel int masks (RXINTMASKCLEAR/TXINTMASKCLEAR). 13. Set used channel masks (RXINTMASKSET/TXINTMASKSET) + `MACINTMASKSET` (HOSTMASK, STATMASK). 14. Build descriptor queues. 15. Write RX head to `RXnHDP`. 16. `RXCONTROL.RXEN=1`, `TXCONTROL.TXEN=1`, then `MACCONTROL.GMIIEN=1`. 17. Enable control-module interrupts. Map interrupts in VIM.

#### MDIO bring-up
Set `CLKDIV` (1 MHz) and `PREAMBLE` → `ENABLE=1` → poll `ALIVE`/`LINK` (state machine polls all 32 PHY addresses; each register read can take ~50 µs; **PHY autonegotiation can take up to 3 s** → use timers) → `USERPHYSEL` PHY address (+ link-change interrupt) → to read/write PHY registers: write `USERACCESS` with GO=1, WRITE, REGADR, PHYADR, DATA; wait GO=0 (read data valid when ACK=1). Use PHY result (speed/duplex) to set MACCONTROL.FULLDUPLEX and RMIISPEED.

#### Interrupts
26 events: TXPEND0–7, RXPEND0–7, RXTHRESHPEND0–7 (flow control), STATPEND (stats rollover), HOSTPEND (host/descriptor error). Level-type; clear TX/RX by writing the completion pointers; ack with MACEOIVECTOR. Pacing by INTCONTROL/C0RXIMAX/C0TXIMAX.

#### Gotchas
- `GMIIEN` last; programming MAC address for all 8 channels is mandatory.
- Descriptors must be word-aligned; EMAC needs a dedicated, init'd CPPI RAM (RAM auto-init).
- IOMM unlock + RMII/MII pin mux.
- Short MDIO clock divider or missing PREAMBLE disable can break PHYs.
- DMA masters for EMAC bus matter for performance; keep descriptors in CPPI RAM.

---

## 23. FlexRay Controller + FlexRay Transfer Unit (FTU)

**Context:** FlexRay v2.1 Rev.A communication controller (Bosch E-Ray architecture): time-triggered, up to 10 Mbit/s per channel, dual channel (A/B), static + dynamic segments, up to 128 message buffers in 8 KB message RAM (e.g. 128 buffers × 48 B payload, or 30 × 254 B), one configurable receive FIFO. Needs an external FlexRay bus driver. **Not a BMS-typical interface** — this file is structural (concepts, addresses, state machine, access flow, register map); full field-level programming of the ~100 protocol config registers is in TRM ch.26 §26.19.
**The FlexRay module is accessed through the FTU:** the FTU (power domain PD4) must be powered/enabled before any FlexRay register access (§19 PMM), and FlexRay pins muxed in IOMM.

#### Addresses
FlexRay module registers `0xFFF7C800–0xFFF7CFFF` (2 KB) · FTU registers `0xFFF7A000–0xFFF7A1FF` (512 B) · FTU RAM (transfer-status/mirror area) `0xFF500000` (init with MSINENA bit 13) · clocks: VCLKA2 = sample-clock/protocol clock source (§01). **Min VBUS clock** (§26.20) is set by worst-case: concurrent tasks × (6 + payload words) time-steps per message-RAM transfer; check for your payload/slot config.

#### Protocol controller concepts
- Cycle = static segment (fixed slots) + dynamic segment (minislots) + symbol window + network idle time (NIT). Global time in macroticks (MT) built from microticks (µT); `T0C/T1C` timers, `STPW1/2` stop watch.
- Sample clock rates (`PRTC1.BRP`): 10 / 5 / 2.5 Mbit/s (1 µT = 25 ns at 5 Mbit/s example ⇒ 1 sample/µT).
- Clock sync: rate/offset correction (`RCV`, `OCV` read-only values; `SFS` sync frame status; `ACS` aggregated channel status).
- Network management vector (`NEMC`), frame/symbol processing, MHD (message handler) config `MHDC`.

#### Register map (offset from 0xFFF7C800)
10 TEST1 · 14 TEST2 · 1C LCK · **20 EIR** (error interrupt) · **24 SIR** (status interrupt) · 28 EILS · 2C SILS · 30 EIES/34 EIER · 38 SIES/3C SIER · 40 ILE · 44 T0C · 48 T1C · 4C STPW1 · 50 STPW2 · **80 SUCC1** (contains the POC `CMD` field, plus coldstart/sync/channel-enable and `PBSY` busy bits — exact bit positions: TRM §26.19.2.3.1) · 84 SUCC2 · 88 SUCC3 · 8C NEMC · 90 PRTC1 · 94 PRTC2 · 98 MHDC · A0–C8 GTUC1–11 (global-timing: macrotick, cycle length, slot lengths) · **100 CCSV** (CC status: `POCS` protocol state, slot/freeze/halt flags) · 104 CCEV · 110 SCV · 114 MTCCV · 118 RCV · 11C OCV · 120 SFS · 124 SWNIT · 128 ACS · 130–1A8 message-buffer config/slot tables … · 300 MRC · 304 FRF · 308 FRFM · 30C FCL · 310 MHDS · 314 LDTS · 318 FSR · 31C MHDF · 320–32C TXRQ1–4 · 330–33C NDAT1–4 · 340–34C MBSC1–4 · 3F0 CREL · 3F4 ENDN · **500–508 WRHS1–3 (write header) · 510 IBCM · 514 IBCR · 700–708 RDHS1–3 · 70C MBS · 710 OBCM · 714 OBCR**. (Data-section windows WRDS/RDDS sit between these header registers; confirm offsets in TRM §26.19.2.8/9.)
**FTU registers (offset 0xFFF7A000):** 00/04 GSN0/1 · 10/14 GCS/GCR · 18 TSCB · 1C LTBCC · 20 LTBSM · 24 TBA · 28 NTBA · 2C BAMS · 30/34 SAMP/EAMP (memory protection window) · 40–4C TSMO1–4 · 50–5C TCCO1–4 · 60 TOOFF · 70 PEADR · 74 TEIF · 78/7C TEIRES/TEIRER · 80–BC TTSMS/R, TTCCS/R (software transfer triggers) · C0–DC ETESMS/R · E0–FC CESMS/R · 100–… TSMIES/R, TCCIES/R (transfer interrupts). FTU auto-copies message buffers ↔ system RAM (like a DMA specialized for FlexRay).

#### POC (protocol operation control) states and commands (write `SUCC1.CMD`; poll `SUCC1.PBSY`=0 to know it was accepted)
1 CONFIG · 2 READY · 3 WAKEUP · 4 RUN · 5 ALL_SLOTS · 6 HALT · 7 FREEZE · 8 SEND_MTS · 9 ALLOW_COLDSTART · Ah RESET_STATUS_INDICATORS · Bh MONITOR_MODE · Ch CLEAR_RAMS · Fh LOOPBACK (0 = no command). `CCSV.POCS` reports the current protocol state (default-config, config, ready, wakeup, startup, normal active/passive, halt, monitor, loopback — numeric codes in TRM §26.19.2.4.1).
Typical bring-up: reset → **DEFAULT_CONFIG → CONFIG** (only state where protocol config registers are writable) → write PRTC1/2, GTUC1–11, SUCC1–3 (channels, coldstart role, sync node), MHDC, message-buffer configs → **READY** → (coldstart node: ALLOW_COLDSTART then **RUN**) / wakeup via WAKEUP → NORMAL_ACTIVE once schedule locked.

#### Message buffers
Each buffer config: frame ID (slot), cycle counter filter (cycle repetition + base), channel A/B/both, payload length, receive/transmit/FIFO role, header CRC, mode (single-shot/continuous), interrupt enables. Payload stored in message RAM; CPU accesses via **Input Buffer (IBF)** and **Output Buffer (OBF)** with command-request handshake:
- **Write (TX or config):** write header words `WRHS1–3` (+ data section), set `IBCM` (what to transfer: header/data/status bits) then `IBCR` (buffer number) → hardware copies to message RAM; wait `IBCR.IBSYS` clear; for transmit set TXRQ.
- **Read (RX):** `OBCM` + `OBCR` (buffer number, request view) → read `RDHS1–3`, data section, `MBS`; use `NDAT1–4` (new data) and `MBSC1–4` (status changed) flags to find updated buffers. FIFO: `FRF`/`FRFM` filter, `FCL` fill level.
- With FTU: configure transfer base addresses/mirrors in FTU so buffers are copied to/from system RAM automatically (TSMO/TCCO flags + interrupts).

#### Interrupts
`EIR` (error: parity, CRC, syntax, content, transmit-across-boundary, …), `SIR` (status: cycle start, RX/TX interrupts, wakeup, slot status, NIT, error-mode change); mapped to line 0/1 via `EILS`/`SILS`, enabled with `EIES/SIES`, global `ILE`. FTU has separate TEIF/TSMI/TCCI interrupts.

#### Gotchas
- Config registers only writable in CONFIG (after DEFAULT_CONFIG → CONFIG command); `LCK` register key protects config.
- RAMs must be cleared (`CLEAR_RAMS` command or MSINENA init) before use.
- FTU must be powered/enabled or FlexRay register accesses fail.
- Cycle timing (GTUC values) must match all nodes exactly; wrong values → no sync.
- Coldstart nodes only: ALLOW_COLDSTART + SUCC1.CSA/TXST/TXSY.
