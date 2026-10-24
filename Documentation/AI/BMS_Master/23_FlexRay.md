# FlexRay Controller + FlexRay Transfer Unit (FTU) — TMS570LS12x/11x
**Context:** FlexRay v2.1 Rev.A communication controller (Bosch E-Ray architecture): time-triggered, up to 10 Mbit/s per channel, dual channel (A/B), static + dynamic segments, up to 128 message buffers in 8 KB message RAM (e.g. 128 buffers × 48 B payload, or 30 × 254 B), one configurable receive FIFO. Needs an external FlexRay bus driver. **Not a BMS-typical interface** — this file is structural (concepts, addresses, state machine, access flow, register map); full field-level programming of the ~100 protocol config registers is in TRM ch.26 §26.19.
**The FlexRay module is accessed through the FTU:** the FTU (power domain PD4) must be powered/enabled before any FlexRay register access (PMM handoff), and FlexRay pins muxed in IOMM.

## Addresses
FlexRay module registers `0xFFF7C800–0xFFF7CFFF` (2 KB) · FTU registers `0xFFF7A000–0xFFF7A1FF` (512 B) · FTU RAM (transfer-status/mirror area) `0xFF500000` (init with MSINENA bit 13) · clocks: VCLKA2 = sample-clock/protocol clock source (system clock handoff). **Min VBUS clock** (§26.20) is set by worst-case: concurrent tasks × (6 + payload words) time-steps per message-RAM transfer; check for your payload/slot config.

## Protocol controller concepts
- Cycle = static segment (fixed slots) + dynamic segment (minislots) + symbol window + network idle time (NIT). Global time in macroticks (MT) built from microticks (µT); `T0C/T1C` timers, `STPW1/2` stop watch.
- Sample clock rates (`PRTC1.BRP`): 10 / 5 / 2.5 Mbit/s (1 µT = 25 ns at 5 Mbit/s example ⇒ 1 sample/µT).
- Clock sync: rate/offset correction (`RCV`, `OCV` read-only values; `SFS` sync frame status; `ACS` aggregated channel status).
- Network management vector (`NEMC`), frame/symbol processing, MHD (message handler) config `MHDC`.

## Register map (offset from 0xFFF7C800)
10 TEST1 · 14 TEST2 · 1C LCK · **20 EIR** (error interrupt) · **24 SIR** (status interrupt) · 28 EILS · 2C SILS · 30 EIES/34 EIER · 38 SIES/3C SIER · 40 ILE · 44 T0C · 48 T1C · 4C STPW1 · 50 STPW2 · **80 SUCC1** (contains the POC `CMD` field, plus coldstart/sync/channel-enable and `PBSY` busy bits — exact bit positions: TRM §26.19.2.3.1) · 84 SUCC2 · 88 SUCC3 · 8C NEMC · 90 PRTC1 · 94 PRTC2 · 98 MHDC · A0–C8 GTUC1–11 (global-timing: macrotick, cycle length, slot lengths) · **100 CCSV** (CC status: `POCS` protocol state, slot/freeze/halt flags) · 104 CCEV · 110 SCV · 114 MTCCV · 118 RCV · 11C OCV · 120 SFS · 124 SWNIT · 128 ACS · 130–1A8 message-buffer config/slot tables … · 300 MRC · 304 FRF · 308 FRFM · 30C FCL · 310 MHDS · 314 LDTS · 318 FSR · 31C MHDF · 320–32C TXRQ1–4 · 330–33C NDAT1–4 · 340–34C MBSC1–4 · 3F0 CREL · 3F4 ENDN · **500–508 WRHS1–3 (write header) · 510 IBCM · 514 IBCR · 700–708 RDHS1–3 · 70C MBS · 710 OBCM · 714 OBCR**. (Data-section windows WRDS/RDDS sit between these header registers; confirm offsets in TRM §26.19.2.8/9.)
**FTU registers (offset 0xFFF7A000):** 00/04 GSN0/1 · 10/14 GCS/GCR · 18 TSCB · 1C LTBCC · 20 LTBSM · 24 TBA · 28 NTBA · 2C BAMS · 30/34 SAMP/EAMP (memory protection window) · 40–4C TSMO1–4 · 50–5C TCCO1–4 · 60 TOOFF · 70 PEADR · 74 TEIF · 78/7C TEIRES/TEIRER · 80–BC TTSMS/R, TTCCS/R (software transfer triggers) · C0–DC ETESMS/R · E0–FC CESMS/R · 100–… TSMIES/R, TCCIES/R (transfer interrupts). FTU auto-copies message buffers ↔ system RAM (like a DMA specialized for FlexRay).

## POC (protocol operation control) states and commands (write `SUCC1.CMD`; poll `SUCC1.PBSY`=0 to know it was accepted)
1 CONFIG · 2 READY · 3 WAKEUP · 4 RUN · 5 ALL_SLOTS · 6 HALT · 7 FREEZE · 8 SEND_MTS · 9 ALLOW_COLDSTART · Ah RESET_STATUS_INDICATORS · Bh MONITOR_MODE · Ch CLEAR_RAMS · Fh LOOPBACK (0 = no command). `CCSV.POCS` reports the current protocol state (default-config, config, ready, wakeup, startup, normal active/passive, halt, monitor, loopback — numeric codes in TRM §26.19.2.4.1).
Typical bring-up: reset → **DEFAULT_CONFIG → CONFIG** (only state where protocol config registers are writable) → write PRTC1/2, GTUC1–11, SUCC1–3 (channels, coldstart role, sync node), MHDC, message-buffer configs → **READY** → (coldstart node: ALLOW_COLDSTART then **RUN**) / wakeup via WAKEUP → NORMAL_ACTIVE once schedule locked.

## Message buffers
Each buffer config: frame ID (slot), cycle counter filter (cycle repetition + base), channel A/B/both, payload length, receive/transmit/FIFO role, header CRC, mode (single-shot/continuous), interrupt enables. Payload stored in message RAM; CPU accesses via **Input Buffer (IBF)** and **Output Buffer (OBF)** with command-request handshake:
- **Write (TX or config):** write header words `WRHS1–3` (+ data section), set `IBCM` (what to transfer: header/data/status bits) then `IBCR` (buffer number) → hardware copies to message RAM; wait `IBCR.IBSYS` clear; for transmit set TXRQ.
- **Read (RX):** `OBCM` + `OBCR` (buffer number, request view) → read `RDHS1–3`, data section, `MBS`; use `NDAT1–4` (new data) and `MBSC1–4` (status changed) flags to find updated buffers. FIFO: `FRF`/`FRFM` filter, `FCL` fill level.
- With FTU: configure transfer base addresses/mirrors in FTU so buffers are copied to/from system RAM automatically (TSMO/TCCO flags + interrupts).

## Interrupts
`EIR` (error: parity, CRC, syntax, content, transmit-across-boundary, …), `SIR` (status: cycle start, RX/TX interrupts, wakeup, slot status, NIT, error-mode change); mapped to line 0/1 via `EILS`/`SILS`, enabled with `EIES/SIES`, global `ILE`. FTU has separate TEIF/TSMI/TCCI interrupts.

## Gotchas
- Config registers only writable in CONFIG (after DEFAULT_CONFIG → CONFIG command); `LCK` register key protects config.
- RAMs must be cleared (`CLEAR_RAMS` command or MSINENA init) before use.
- FTU must be powered/enabled or FlexRay register accesses fail.
- Cycle timing (GTUC values) must match all nodes exactly; wrong values → no sync.
- Coldstart nodes only: ALLOW_COLDSTART + SUCC1.CSA/TXST/TXSY.
