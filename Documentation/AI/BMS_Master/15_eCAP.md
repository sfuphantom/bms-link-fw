# eCAP (Enhanced Capture) — TMS570LS12x/11x
**Context:** Up to 6 eCAP modules (eCAP1–6). Each = one capture input pin (ECAPx) with a 32-bit timestamp counter and four 32-bit capture registers (CAP1–CAP4) — OR — a single-channel PWM output (**APWM mode**). Uses: period/duty/pulse-width measurement, speed from tooth sensors, decoding duty-encoded sensors. Clock **VCLK4**; registers 16-bit offsets, 32-bit data regs. Base: eCAP1 `0xFCF79300`, +0x100 per instance (datasheet gives the table). Pin via IOMM.

## Register map (offset)
00 TSCTR (32-bit timestamp counter) · 04 CTRPHS (phase on sync) · 08 CAP1 (= APRD in APWM) · 0C CAP2 (= ACMP) · 10 CAP3 (= APRD shadow) · 14 CAP4 (= ACMP shadow) · **28 ECCTL2** · **2A ECCTL1** · 2C ECFLG · 2E ECEINT · 30 ECFRC · 32 ECCLR.

## ECCTL1 (2Ah)
`FREE_SOFT`[15:14] (0 stop immediately, 1 stop at 0, 2/3 free-run) · `PRESCALE`[13:9] input-event divider (0 = bypass, else ÷2N: 1→2, 2→4, 3→6 … up to ÷62) · `CAPLDEN`[8] enable capture-register loads · per event n=1..4: `CAPnPOL` (0 = rising, 1 = falling edge) and `CTRRSTn` (1 = reset TSCTR after event n → **delta/period mode**; 0 = absolute timestamp). Bit layout: CTRRST4[7] CAP4POL[6] CTRRST3[5] CAP3POL[4] CTRRST2[3] CAP2POL[2] CTRRST1[1] CAP1POL[0].

## ECCTL2 (28h, reset 0x0002)
`APWMPOL`[10] (0 = active high: compare = high time) · **`CAP_APWM`[9]** (0 = capture mode, 1 = APWM mode) · `SWSYNC`[8] · `SYNCO_SEL`[7:6] (0 pass-through, 1 CTR=PRD, 2/3 disabled) · `SYNCI_EN`[5] (load TSCTR from CTRPHS on sync) · **`TSCTRSTOP`[4]** (0 = TSCTR stopped (reset default), 1 = free-running) · `REARM`[3] (write 1: arm one-shot, reset Mod4 counter/flags) · `STOP_WRAP`[2:1] (0..3 = stop/wrap after event 1..4) · `CONT_ONESHT`[0] (0 continuous, 1 one-shot).

## Flags / interrupts (ECFLG 2Ch RO, ECEINT 2Eh enable, ECFRC 30h force, ECCLR 32h clear-by-write-1)
bit0 `INT` global · 1–4 `CEVT1–4` · 5 `CTROVF` (TSCTR wrapped FFFFFFFF→0) · 6 `CTR_PRD` (APWM) · 7 `CTR_CMP` (APWM). **Clear the individual flag AND `INT` (ECCLR) to re-arm interrupts.**

## Capture recipes
- **Absolute timestamp:** CAPxPOL per desired edges, all CTRRSTn = 0, `CAPLDEN=1`, `TSCTRSTOP=1`, continuous or one-shot (`STOP_WRAP`). Each edge loads current TSCTR into CAPn in sequence (Mod4 counter cycles 1→4).
- **Period + duty (delta mode):** CAP1 = rising (POL=0, CTRRST1=1), CAP2 = falling (POL=1, CTRRST2=0). Result: CAP1 = time between rising edges (period), CAP2 = rising→falling time (high time); duty = CAP2/CAP1 (read after CEVT2). Frequency = `f_VCLK4 / period_counts`.
- Timestamp resolution = 1/VCLK4; use PRESCALE to divide noisy/fast inputs; 32-bit counter wraps at ~2³²/VCLK4.
- One-shot: arm with `REARM=1`; after `STOP_WRAP`+1 events counter and loads freeze until rearmed.

## APWM mode (CAP_APWM=1)
CAP1 = period (APRD), CAP2 = compare (ACMP), CAP3/CAP4 = shadow of APRD/ACMP (loaded when TSCTR = period). Output on the same pin (APWMx). Frequency `= f_VCLK4 / (APRD+1)`. Behavior (active high): CMP=0 → 0 % duty; CMP=n → high n cycles; CMP=PRD → high except 1 cycle; **CMP ≥ PRD+1 → 100 %**. `APWMPOL` inverts. Writing the active CAP1/CAP2 also writes shadow regs (use CAP3/CAP4 for glitch-free updates).

## Gotchas
- `TSCTRSTOP` reset = stopped.
- Clear ECFLG bit + INT via ECCLR or no new interrupts.
- eCAPs sync with ePWM through the sync chain (SYNCI/SYNCO).
