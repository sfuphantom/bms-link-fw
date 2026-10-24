# RTI (Real-Time Interrupt) + Digital Windowed Watchdog — TMS570LS12x/11x
**Context:** RTI = OS tick / periodic timer with two counter blocks and four compare channels (interrupt or DMA request each). Contains the **DWD/DWWD watchdog** (digital windowed watchdog) that can reset or NMI the CPU. Base address **`0xFFFFFC00`** (privileged writes for control regs). Clock = RTICLK (default VCLK; if another source, RTICLK ≤ VCLK/3 — set via RCLKSRC.RTI1DIV in SYS).

## Counter blocks (0 and 1)
Each block: 32-bit up counter (UC) + 32-bit free-running counter (FRC) + compare-up register (CPUC).
`UC` counts RTICLK; when `UC == CPUC` → `FRC++` and `UC = 0`. So
`f_FRC = f_RTICLK / (CPUC + 1)` (CPUC=0 → /2³²+1: avoid CPUC=0).
FRC overflow raises overflow interrupt (not for OS tick). Block 0 can instead be clocked by FlexRay NTU (TBEXT) — ignore for BMS.
Capture: external events (VIM capture sources 0/1) latch UC/FRC into CAUC/CAFRC (select in RTICAPCTRL).

## Compare channels (4)
`RTICOMPy` is compared against FRC0 or FRC1 (`RTICOMPCTRL`: COMPSEL0[0], COMPSEL1[4], COMPSEL2[8], COMPSEL3[12]; 0 = FRC0, 1 = FRC1). On match: INTy flag set → VIM interrupt (REQ[2..5]) or DMA request (REQ 12,13,18,19 if enabled) and `RTICOMPy += RTIUDCPy` automatically (periodic tick).
`T_period = T_RTICLK × (CPUC+1) × UDCP`.
**Periodic tick recipe:** `RTICPUC0 = prescale−1`; `RTICOMP0 = RTIFRC0 + period`; `RTIUDCP0 = period`; `RTICOMPCTRL` COMPSEL0=0; `RTISETINTENA |= 1<<0`; `RTIGCTRL.CNT0EN=1`. ISR: clear flag by writing 1 to `RTIINTFLAG` bit0.

## Register map (offset)
00 RTIGCTRL (`CNT0EN`[0], `CNT1EN`[1], `COS`[15], `NTUSEL`[19:16]) · 04 RTITBCTRL (`TBEXT`[0], `INC`[1]) · 08 RTICAPCTRL (CAPCNTR0[0], CAPCNTR1[1]) · 0C RTICOMPCTRL · 10 RTIFRC0 · 14 RTIUC0 · 18 RTICPUC0 · 20 RTICAFRC0 · 24 RTICAUC0 · 30 RTIFRC1 · 34 RTIUC1 · 38 RTICPUC1 · 40 RTICAFRC1 · 44 RTICAUC1 · 50 RTICOMP0 · 54 RTIUDCP0 · 58 RTICOMP1 · 5C RTIUDCP1 · 60 RTICOMP2 · 64 RTIUDCP2 · 68 RTICOMP3 · 6C RTIUDCP3 · 70 RTITBLCOMP · 74 RTITBHCOMP · **80 RTISETINTENA · 84 RTICLEARINTENA · 88 RTIINTFLAG** · 90 RTIDWDCTRL · 94 RTIDWDPRLD · 98 RTIWDSTATUS · 9C RTIWDKEY · A0 RTIDWDCNTR · A4 RTIWWDRXNCTRL · A8 RTIWWDSIZECTRL · AC RTIINTCLRENABLE · B0–BC RTICOMP0CLR–3CLR.
Bits in SETINTENA/CLEARINTENA/INTFLAG: compare INT0–INT3 = bits 0–3; `TBINT`[16]; `OVL0INT`[17]; `OVL1INT`[18]; SETDMA0–3 (DMA enables) at bits 8–11 (SETDMA3 = bit 11). Clear INTFLAG bits by writing 1. Writes to reserved offsets can clear a pending RTI interrupt.

## Watchdog (DWD) and windowed watchdog (DWWD)
- Disabled after power-on reset. **Enable by writing `0xA98559DA` to `RTIDWDCTRL`** (reset value `0x5312ACED` = disabled). Once enabled it cannot be disabled by software.
- Timeout: `t_exp = (DWDPRLD + 1) × 2¹³ / f_RTICLK`, `RTIDWDPRLD[11:0]` (0–4095). Preload can only be changed while the watchdog is disabled.
- **Service ("pet") sequence:** write **`0xE51A`** then **`0xA35C`** to `RTIWDKEY` (two separate writes, in order). Any other value / wrong order → reset or NMI (e.g., `0x2345`). Key register reset value is A35C.
- **Windowed (DWWD):** window = last X% of the timeout; servicing **outside the window** (too early) or not at all (too late) triggers the reaction. `RTIWWDSIZECTRL`: `0x5`=100% (default, = plain timeout watchdog), `0x50`=50%, `0x500`=25%, `0x5000`=12.5%, `0x50000`=6.25%, `0x500000`=3.125%. `RTIWWDRXNCTRL.WWDRXN`: `0x5` (default) = **reset**; other value = NMI to CPU. Window size and reaction can be changed after enable; take effect after next service.
- Status: `RTIWDSTATUS` — `DWWD ST`[5] window violation, `END TIME VIOL`[4], `START TIME VIOL`[3], plus key-sequence and timeout flags; clear by writing 1. In NMI mode the NMI handler must clear flags then service the watchdog.
- Reset cause shows in SYS `SYSESR.WDRST`[13].
- Halt-debug: counters stop unless `COS=1`; watchdog counter also halts per debug rules.

## Low-power
RTI clocked from LF LPO (80 kHz) can wake the device from low-power mode via compare interrupts.

## Gotchas
- Control writes are privileged-mode only.
- Don't set CPUC=0.
- Compare must be re-armed by UDCP (automatic) — if UDCP=0 period is 2³² counts.
- Watchdog enable key is a 32-bit write; sequence keys are 16-bit values in two writes.
- If using non-VCLK RTICLK, honor RTICLK ≤ VCLK/3.
