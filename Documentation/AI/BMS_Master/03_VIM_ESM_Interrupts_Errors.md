# VIM (Vectored Interrupt Manager) + ESM (Error Signaling Module) — TMS570LS12x/11x
**Context:** The VIM is the interrupt controller between all peripheral interrupt requests and the Cortex-R4F (IRQ/FIQ, optional hardware vectoring). The ESM collects safety-related errors (ECC/parity faults, clock/PLL failures, lock-step compare errors, etc.) and responds with interrupts and/or the external **ERROR pin**. Peripheral-to-channel (INT_REQ) and ESM-channel assignments are device-specific and live in the **datasheet**, not in the TRM.

# VIM
**Base `0xFFFFFE00`; vector table RAM `0xFFF82000`; parity registers frame `0xFFFFFD00`.**

## Structure
- 128 request inputs (INT_REQ0–126; 127 reserved) → programmable channel mapping (`CHANMAPx[6:0]`, 4 per 32-bit `CHANCTRL[0..31]` register) → per-channel enable → FIQ/IRQ level select → priority encoder. **Lower channel number = higher priority** within IRQ and within FIQ; FIQ beats IRQ.
- **Channel 0 = ESM high-level interrupt, Channel 1 = other NMI. Both hard-wired, always FIQ, cannot be masked, cannot be remapped.** Channel 127 has no vector — don't use.
- Same request may map to several channels (priority can then be changed dynamically via enable/clear). Default after reset: INT_REQn → CHANn.
- Phantom vector = entry 0 (spurious/no-pending).

## Vector table (VIM RAM, 128 × 32-bit)
`0xFFF82000` phantom · `0xFFF82004` channel 0 · `0xFFF82008` channel 1 · … · `0xFFF821FC` channel 126. Entry = ISR address (`ISR addr = 0xFFF82000 + 4×(channel+1)`). Parity-protected: **use only 32-bit accesses**. Set `FBPARERR` (fall-back ISR address) **before** filling the table. Initialize VIM RAM via system RAM auto-init (MSINENA bit 2) if parity is enabled.

## Registers (offset from 0xFFFFFE00)
00 IRQINDEX · 04 FIQINDEX · 10–1C FIRQPR0–3 (1 = FIQ, 0 = IRQ per channel; ch0/1 ignored) · 20–2C INTREQ0–3 (pending, read-only; unaffected by enable) · **30–3C REQENASET0–3 · 40–4C REQENACLR0–3** (channel enable set/clear; 32 channels per register) · 50–5C WAKEENASET0–3 · 60–6C WAKEENACLR0–3 (wake-from-low-power enables) · **70 IRQVECREG · 74 FIQVECREG** (address of pending ISR) · 78 CAPEVT (RTI capture sources) · 80+ CHANCTRL0–31 (channel map). Parity frame (`0xFFFFFD00`): `+0xEC` PARFLG · `+0xF0` PARCTL · `+0xF4` ADDERR · `+0xF8` FBPARERR (privileged writes).

## Enabling interrupts
1. Map/leave default channel; write ISR address to the vector table entry. 2. `FIRQPR` choose IRQ vs FIQ. 3. `REQENASET` bit for channel. 4. Enable at CPU: clear I (0x80) / F (0x40) bits in CPSR (privileged). 5. Peripheral enables its own interrupt.

## Two handling styles
- **Register-vectored (default):** exception vector 0x18 (IRQ) / 0x1C (FIQ) holds `LDR PC,[PC,#-0x1B0]` → loads **IRQVECREG (0xFFFFFE70) / FIQVECREG (0xFFFFFE74)** → jumps to ISR.
- **Hardware-vectored (VIC port):** set CP15 SCTLR bit 24 (VE) = 1; CPU takes the vector straight from VIM (IRQ only).
- Index style: read `IRQINDEX`/`FIQINDEX` (channel+1) and dispatch from a table (legacy/fast FIQ).
- The peripheral clears its own flag; VIM does not clear it. Reading IRQVECREG/IRQINDEX marks the interrupt serviced.
- ISRs may be ARM or Thumb mode vectors per ISR address LSB convention of the compiler (TI uses `#pragma INTERRUPT`).

# ESM
**Base `0xFFFFF500`.** 128 error channels in 3 groups:
- **Group 1 (channels 0–63, low severity):** configurable interrupt (level high/low) and configurable influence on ERROR pin.
- **Group 2 (32, high severity):** predefined → NMI-like high-level interrupt (VIM ch 0) + ERROR pin (not maskable).
- **Group 3 (32, high severity):** no interrupt; CPU abort + ERROR pin (CPU diagnostics).

## Registers
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

## Behavior
- **ERROR pin is active-low;** Hi-Z during PORRST. On any enabled error it is driven low and stays low for `t = T_VCLK × (ESMLTCPR + 1)`, restarted by each new failure. To release it: write **`0x5` to ESMEKR** (during/after low time) — otherwise it stays low until power-on reset. If a second failure arrives after a release request, write `5h` then `0` to EKR so the pin pulses again.
- **Force-error test:** require `ESMEPSR=1`; write **`0xA` to ESMEKR** to force the pin low; write `0` to return to normal (low-time counter then runs). (The doc text and figures differ on 5h/Ah for forcing; the EKEY field description says `5h` = reset pin, `Ah` = force error.)
- Flags: status bits latch until cleared (write 1). Warm reset (RST) keeps ESMSR1/SR4/SSR2/SR3/EPSR; ESMSR2 is cleared by RST (use ESMSSR2) and cleared when its vector is read via ESMIOFFHR. PORRST clears everything.
- **ISR:** high-level (VIM ch 0): read `ESMIOFFHR` → offset identifies channel (group2 first, then group1 high-level); low-level interrupt arrives on a datasheet-defined VIM channel: read `ESMIOFFLR`. Then clear status (ESMSR1/4).

## Recommended init
1. After power-up, force/test ERROR pin with external monitor. 2. Initialize VIM RAM: map ESM low and high ISRs to their VIM channels. 3. Enable in VIM and CPU. 4. Assign group1 channels to high/low (`ESMILSR1/4`). 5. Enable interrupts (`ESMIESR1/4`) and ERROR-pin influence (`ESMIEPSR`…). 6. Set `ESMLTCPR`. 7. Clear stale flags from previous reset (read ESMSR*, ESMSSR2).

## Gotchas
- Group 2/3 events can't be masked; handle them (reset or fail-safe).
- Reading ESMIOFFLR/ESMIOFFHR does not clear group1 status flags.
- VIM channel 0 is the ESM high-level interrupt — an unhandled ESM NMI loops forever.
- Parity/ECC faults in peripheral RAMs, VIM, flash, clock monitor, and CCM compare all surface in the ESM (check datasheet channel table).
