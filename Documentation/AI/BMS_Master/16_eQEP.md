# eQEP (Enhanced Quadrature Encoder Pulse) — TMS570LS12x/11x
**Context:** 2 modules (eQEP1/2). Decode incremental-encoder signals **QEPA / QEPB (90° quadrature), QEPI (index), QEPS (strobe)** into a 32-bit position counter with direction, plus unit timer, capture timer and watchdog for speed measurement. Not typically relevant to BMS (motor/rotary position only). Clock **VCLK4**. Base: eQEP1 `0xFCF79900`, eQEP2 `0xFCF79A00`. Pins muxed via IOMM.

## Register map (offset)
00 QPOSCNT (32) · 04 QPOSINIT · 08 QPOSMAX · 0C QPOSCMP · 10 QPOSILAT (latch @ index) · 14 QPOSSLAT (latch @ strobe) · 18 QPOSLAT (latch @ unit timeout / read) · 1C QUTMR (unit timer) · 20 QUPRD (unit period) · 24 QWDPRD (16-bit watchdog period) · 26 QWDTMR · **28 QEPCTL · 2A QDECCTL · 2C QPOSCTL · 2E QCAPCTL** · 30 QFLG · 32 QEINT · 34 QFRC · 36 QCLR · 38 QCTMR (capture timer) · 3A QEPSTS (reset 0x8E) · 3C QCTMRLAT · 3E QCPRD · 42 QCPRDLAT.
(Doc lists QCPRD at 3Eh and QCPRDLAT at 42h; 40h is skipped.)

## Quadrature decoder (QDECCTL 2Ah)
`QSRC`[15:14] 0 = quadrature count (QCLK/QDIR from A/B decode), 1 = direction-count (A = clock, B = direction), 2 = up-count (frequency measurement), 3 = down-count · `SOEN`[13]/`SPSEL`[12] position-compare sync output on index/strobe pin · `XCR`[11] 0 = 2× resolution (count both edges of QCLK), 1 = 1× · `SWAP`[10] swap A/B · `IGATE`[9] gate index with strobe · `QAP/QBP/QIP/QSP`[8:5] invert A/B/I/S.
Quadrature mode counts **4× the line rate** (both edges of both channels): counts/rev = 4 × lines.
Direction: A leads B = clockwise/forward (counter up), B leads A = reverse (counter down).

## Control (QEPCTL 28h)
`FREE_SOFT`[15:14] · **`PCRM`[13:12]** counter reset mode: 0 on index event, 1 at max position (QPOSMAX), 2 on first index event, 3 on unit-timeout · `SEI`[11:10] strobe init (2 = rising QEPS; 3 = direction-dependent) · `IEI`[9:8] index init (2 = rising QEPI, 3 = falling) · `SWI`[7] SW init (QPOSCNT = QPOSINIT) · `SEL`[6] strobe latch edge · `IEL`[5:4] index latch (1 rising, 2 falling, 3 software marker) · **`QPEN`[3] enable position counter (0 = reset flags/regs)** · `QCLM`[2] 0 = latch on CPU read of QPOSCNT, 1 = latch on unit timeout · `UTE`[1] unit timer enable · `WDE`[0] watchdog enable.
Counter range 0…`QPOSMAX` (e.g. 1000-line encoder: QPOSMAX = 3999). Overflow/underflow wrap with flags.

## Position-compare (QPOSCTL 2Ch)
`PCSHDW`[15], `PCLOAD`[14], `PCPOL`[13], `PCE`[12] enable, `PCSPW`[11:0] sync-pulse width = (n+1)×4×VCLK4 cycles. Match with QPOSCMP raises `PCM` flag/optional sync pulse.

## Capture unit (QCAPCTL 2Eh) — low-speed (period) measurement
`CEN`[15] enable · `CCPS`[6:4] capture timer clock = VCLK4/2ⁿ (n=0–7) · `UPPS`[3:0] unit-position event = QCLK/2ⁿ (n = 0–0xB → /1…/2048). Each UPEVNT latches QCTMR → QCTMRLAT and the elapsed period → QCPRDLAT.

## Speed formulas
- **High speed (unit timer):** `v(k) = (x(k) − x(k−1)) / T`, T = unit period `QUPRD / VCLK4`. Position counts read from QPOSLAT (latched at unit timeout, QCLM=1).
- **Low speed:** `v(k) = X / (t(k) − t(k−1))`, X = unit position (UPPS), Δt from capture timer (QCPRDLAT × CCPS prescale / VCLK4).

## Flags (QFLG 30h; QEINT 32h enable; QFRC 34h force; QCLR 36h clear-by-write-1)
0 `INT` · 1 `PCE` position counter error · 2 `PHE` quadrature phase error · 3 `QDC` direction change · 4 `WTO` watchdog timeout · 5 `PCU` underflow · 6 `PCO` overflow · 7 `PCR` compare ready · 8 `PCM` compare match · 9 `SEL` strobe latch · 10 `IEL` index latch · 11 `UTO` unit timeout. Clear individual flag **and INT** to re-arm.
QEPSTS (3Ah, reset 0x8E) reports first-index/direction/error status (UPEVNT, FIDF, QDF, QDLF, COEF, CDEF, FIMF, PCEF).

## Start-up
Enable clock domain (VCLK4) + pin mux → `QDECCTL` (QSRC=0, XCR per resolution, polarity) → `QPOSMAX` → `QPOSINIT`/`QPOSCNT` → `QEPCTL` (`PCRM`, `QPEN=1`, `UTE=1`, `WDE=1` as needed) → `QUPRD`, `QCAPCTL` → `QEINT` + VIM.

## Gotchas
- `QPEN=0` holds the counter in reset.
- Index-based reset needs IEI/PCRM set; otherwise counter free-runs 0…QPOSMAX.
- Phase error flag indicates missing/ simultaneous A/B transitions (bad wiring/noise).
