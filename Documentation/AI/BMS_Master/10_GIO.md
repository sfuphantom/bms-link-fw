# GIO (General-Purpose I/O) — TMS570LS12x/11x
**Context:** Up to 8 ports (A–H) × 8 pins = 64 GIO terminals, individually input/output, push-pull/open-drain, programmable pulls. Up to 32 terminals (ports A–D) can raise interrupts via the VIM. Number of ports actually implemented and which pins are interrupt-capable: see datasheet. This device's register set exposes **ports A and B** at the offsets below (C–H follow the same pattern on larger devices; on this device ports beyond A/B are generally not in the GIO register frame).
**Base address:** `0xFFF7BC00`. Registers accept 8/16/32-bit access. Pin must also be muxed to GIO in IOMM (see IOMM handoff) and peripheral clock/reset released.

## Register map
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

## I/O quick start
1. Release peripheral reset / enable clock for GIO (PCR/PENA in system module — device datasheet lists the peripheral select). 2. `GIOGCR0 = 1`. 3. Per pin: `GIODIR` 0 = input / 1 = output. 4. Input: `GIOPULDIS` 0 = pull enabled (then `GIOPSL` 0 = pull-down, 1 = pull-up) or 1 = pull disabled. Output: optional `GIOPDR`=1 for open drain. 5. Drive with `GIODSET`/`GIODCLR` (atomic, preferred over read-modify-write of GIODOUT) or `GIODOUT`. 6. Read with `GIODIN`.
Pull truth table (input pins only): PULDIS=0,PSL=0 → pull-down; PULDIS=0,PSL=1 → pull-up; PULDIS=1 → none. Output pins have pulls disabled.

## Interrupts
- Enable pin: `GIOENASET`. Edge: `GIOPOL` (1 rising, 0 falling); both edges: set `GIOINTDET`. Priority line: `GIOLVLSET` (high/level A) or `GIOLVLCLR` (low/level B). VIM channel numbers for A/B are in the datasheet; lower VIM channel = higher priority.
- ISR: **read `GIOOFF1` (high-level) or `GIOOFF2` (low-level)**: gives pin index (0 none; 1–8 = A0–A7, 9–16 = B0–B7, …) and **clears the flag**. Keep reading until 0 to service all pending in one ISR. (In emulation mode reading doesn't clear; use GIOEMUx to peek.)
- In low-power mode (GIO clocks off), GIOPOL selects **level** (1 = high, 0 = low) instead of edge to wake the device.

## Gotchas
- `GIOGCR0.RESET` must be 1 before any other GIO register works. Reset state ≠ low-power.
- Use SET/CLR registers; GIODSET/GIODCLR reads mirror GIODOUT.
- Pin mux (IOMM) and peripheral clock gating are separate from GIO config.
