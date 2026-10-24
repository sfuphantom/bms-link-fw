# ePWM (Enhanced PWM) — TMS570LS12x/11x
**Context:** Up to 7 identical ePWM modules (ePWM1–7), each = 1 PWM channel with two outputs **EPWMxA / EPWMxB**, built from 8 submodules in series: Time-Base (TB) → Counter-Compare (CC) → Action-Qualifier (AQ) → Dead-Band (DB) → PWM-Chopper (PC) → Trip-Zone (TZ), plus Event-Trigger (ET, interrupts + ADC SOC) and Digital-Compare (DC, trip/filter from comparator-style inputs). Clock = **VCLK4**. All registers are **16-bit**. Typical uses: motor/DC-DC drive, gate-drive PWM, triggering ADC sampling at precise points in a PWM cycle.
**Addresses:** ePWM1 `0xFCF78C00`, +0x100 per instance up to ePWM7 (full list in datasheet/memory map). Pins muxed via IOMM (see IOMM handoff); module sync/trip/ADC-trigger routing also configured in IOMM (EPWMSYNCI/O, TZ1–3, ADC SOC mux).

## Register map (16-bit; offset from module base)
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

## Time-Base (TBCTL @02h, reset 0x0083)
`FREE_SOFT`[15:14] emulation (2/3 = free run) · `PHSDIR`[13] count dir after sync (up-down mode only) · `CLKDIV`[12:10] /1…/128 (2ⁿ) · `HSPCLKDIV`[9:7] /1,/2(reset),/4,/6,/8,/10,/12,/14 · `SWFSYNC`[6] force sync · `SYNCOSEL`[5:4] 0 = pass SYNCI, 1 = CTR=0, 2 = CTR=CMPB, 3 = disabled · `PRDLD`[3] 0 = TBPRD shadowed, 1 = immediate · `PHSEN`[2] load TBCTR from TBPHS on sync · `CTRMODE`[1:0] 0 up, 1 down, 2 up-down, **3 = stop/freeze (reset default → counter doesn't run until you change it)**.
- **TBCLK = VCLK4 / (HSPCLKDIV_div × CLKDIV_div)**.
- Period: up or down count `T_pwm = (TBPRD + 1) × T_TBCLK`; up-down `T_pwm = 2 × TBPRD × T_TBCLK`.
- Sync chain: ePWM1 → ePWM2 → …; phase offsets via TBPHS (lead/lag); eCAP can join the chain.

## Counter-Compare (CMPCTL @0Ch)
`SHDWAFULL`[8], `SHDWBFULL`[9] (RO) · `SHDWAMODE`[4] / `SHDWBMODE`[6]: 0 = shadowed (double-buffered), 1 = immediate · `LOADAMODE`[1:0] / `LOADBMODE`[3:2]: 0 = on CTR=0, 1 = on CTR=PRD, 2 = either, 3 = freeze. Duty (up-count): `CMPA / (TBPRD+1)`.

## Action-Qualifier (AQCTLA @14h, AQCTLB @1Ah)
Each field 2 bits: 0 = do nothing, 1 = clear (low), 2 = set (high), 3 = toggle.
`ZRO`[1:0] on CTR=0 · `PRD`[3:2] on CTR=PRD · `CAU`[5:4] CTR=CMPA counting up · `CAD`[7:6] CMPA counting down · `CBU`[9:8] CMPB up · `CBD`[11:10] CMPB down.
Common patterns: **single-edge up-count PWM, active high A:** `ZRO=Set(2)`, `CAU=Clear(1)`. Up-down symmetric: `CAU=Set`, `CAD=Clear`. SW force: AQSFRC (one-shot), AQCSFRC (continuous: 0 none, 1 low, 2 high; shadowed).

## Dead-Band (DBCTL @1Ch)
`HALFCYCLE`[15] · `IN_MODE`[5:4]: 0 = A is source for both RED & FED, 1 = B→RED/A→FED, 2 = A→RED/B→FED, 3 = B for both · `POLSEL`[3:2]: 0 = active-high, 1 = ALC (A inverted), 2 = AHC (B inverted), 3 = active-low · `OUT_MODE`[1:0]: 0 bypass, 1 disable RED, 2 disable FED (A delayed rising), 3 full. Delay `= DBxED × T_TBCLK` (10-bit counts, or ×2 clock with HALFCYCLE).
**Complementary pair with dead time:** `OUT_MODE=3, IN_MODE=0, POLSEL=2 (AHC)`.

## Trip-Zone (TZSEL @26h, TZCTL @2Ah, TZEINT @28h, TZFLG @2Eh, TZCLR @2Ch, TZFRC @32h)
Trip sources: TZ1–TZ3 (pins/ESM/clock-monitor, routed in IOMM), digital-compare events. Per-cycle (CBC) or one-shot (OSHT) selection in TZSEL. `TZCTL`: `TZA`[1:0] and `TZB`[3:2] action on output: 0 = Hi-Z, 1 = force high, 2 = force low, 3 = no action. `DCAEVT1`[5:4], `DCAEVT2`[7:6], `DCBEVT1`[9:8], `DCBEVT2`[11:10] similar. Clear via TZCLR (CBC clears itself each cycle). **Use TZ (one-shot) as a hardware kill switch.**

## Event-Trigger (ETSEL @30h, ETPS @36h, ETFLG/34h, ETCLR/3Ah, ETFRC/38h)
`ETSEL`: `INTEN`[3], `INTSEL`[2:0] (1 = CTR=0, 2 = CTR=PRD, 3 = either, 4 = CTRU=CMPA, 5 = CTRD=CMPA, 6 = CTRU=CMPB, 7 = CTRD=CMPB) · `SOCAEN`[11], `SOCASEL`[10:8] (0 = DCAEVT1.soc, else same codes as INTSEL) · `SOCBEN`[15], `SOCBSEL`[14:12].
`ETPS`: `INTPRD`[1:0] (0 off, 1 = every event, 2 = every 2nd, 3 = every 3rd) · `INTCNT`[3:2] RO · `SOCAPRD`[9:8], `SOCACNT`[11:10], `SOCBPRD`[13:12], `SOCBCNT`[15:14]. Clear flag via ETCLR before re-arming (INT bit). **ePWM SOCA/SOCB can trigger ADC conversions** (select as ADC group trigger source in ADC xSRC/IOMM).

## PWM-Chopper (PCCTL @3Eh)
High-frequency carrier modulation (CHPEN, CHPFREQ, CHPDUTY, OSHTWTH one-shot width) for pulse-transformer drive.

## Digital Compare (DC)
DCTRIPSEL, DCACTL/DCBCTL, DCFCTL (blanking window + filter), DCCAPCTL/DCCAP (capture counter at event), DCFOFFSET/WINDOW — builds trip events (DCAEVT1/2, DCBEVT1/2) from TZ/comparator inputs with optional blanking windows.

## Start-up recipe (simple PWM, 50 kHz @ VCLK4=…; compute your own)
1. Enable ePWM clock domain (`CLKCNTL.PENA`, `CDDIS` bit for VCLK4 enabled) and pin mux. 2. `TBCTL`: CLKDIV/HSPCLKDIV, `CTRMODE=3` (freeze) while configuring. 3. `TBPRD`, `TBPHS`, `TBCTR=0`. 4. `CMPCTL` shadow modes; `CMPA`. 5. `AQCTLA/B`. 6. (Dead-band / TZ / ET as needed.) 7. `CTRMODE=0` (up) to start; update duty by writing `CMPA` (shadowed; takes effect on next load event).

## Gotchas
- Default `CTRMODE=3` = counter frozen.
- Shadowed registers (TBPRD, CMPA/B, AQCSFRC) only update at load events unless `PRDLD`/`SHDW*MODE` set to immediate.
- TZ regs are privileged-only and writes are EALLOW-like protected by privilege.
- Up-down mode period doesn't use `+1`.
- ET interrupt must be cleared with ETCLR, and (for TZ) TZCLR.
