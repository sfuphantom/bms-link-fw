# N2HET (High-End Timer) — TMS570LS12x/11x
**Context:** N2HET1 and N2HET2 are programmable timer coprocessors (own 96-bit-instruction RAM + tiny ISA, 30 instructions) driving up to 32 pins each for PWM, capture, pulse measurement, event counting, GPIO sequences, and HW-angle (HWAG, motor/engine) functions. They run a user program in a fixed-period loop independent of the CPU. Typical BMS uses: PWM, precise pulse timing, input capture, extra GPIO, ADC trigger source, SPI/ADC triggers. HTU (High-End Timer Transfer Unit) moves N2HET data to/from system RAM without CPU.
**Not in this manual:** instruction opcode/bit encodings — use TI HET assembler/HALCoGen to generate the RAM image.

## Addresses
| | N2HET1 | N2HET2 |
|---|---|---|
| Control registers | `0xFFF7B800` | `0xFFF7B900` |
| Instruction RAM | `0xFF460000` | `0xFF440000` |
| Parity RAM | `0xFF462000` | `0xFF442000` |
Up to 128 instruction words (address 0x000–0x1FF possible in map; actual count per datasheet). Each instruction = Program (PF), Control (CF), Data (DF) 32-bit fields at `base + 0x10×n + {0, 4, 8}`; `+0xC` reserved (don't access). Only **32-bit writes** to RAM; reads 8/16/32. CF+DF consecutive reads by one master = atomic 64-bit read (3 shadow regs; 4th concurrent master → address error). RAM is 4 banks (n mod 4); N2HET has priority over host (host stalls on conflict).

## Execution model
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

## Register map (offset from control base)
00 HETGCR · 04 HETPFR · 08 HETADDR (current address) · 0C HETOFF1 / 10 HETOFF2 (interrupt offset, level 1/2) · 14 HETINTENAS / 18 HETINTENAC · 1C HETEXC1 / 20 HETEXC2 (exception ctl) · 24 HETPRY · 28 HETFLG · 2C HETAND · 34 HETHRSH · 38 HETXOR · 3C HETREQENS / 40 HETREQENC / 44 HETREQDS (DMA/HTU request enable/dest) · **4C HETDIR · 50 HETDIN · 54 HETDOUT · 58 HETDSET · 5C HETDCLR · 60 HETPDR (open drain) · 64 HETPULDIS · 68 HETPSL** · 74 HETPCR / 78 HETPAR / 7C HETPPR (parity) · 80 HETSFPRLD / 84 HETSFENA (input suppression filters) · 8C HETLBPSEL / 90 HETLBPDIR (loopback) · 94 HETPINDIS.
HWAG regs (angle generator): HWAPINSEL, HWAGCR0–2, HWAENASET/CLR, HWALVLSET/CLR, HWAFLG, HWAOFF0/1, HWAACNT, HWAPCNT1, HWAPCNT, HWASTWD, HWATHNB, HWATHVL, HWAFIL, HWAFIL2, HWAANGI.

## HETGCR (00h)
`HET_PIN_ENA`[24] enable output buffers (with DIR=out, PINDIS.x=1, nDIS=1) · `MP`[22:21] RAM master priority (0 HTU lower, 1 HTU higher, 2 round-robin) · `PPF`[18] protect program fields (read-only for masters) · `IS`[17] ignore suspend · **`CMS`[16] 1 = clock master — must be 1 for a single/standalone N2HET** · **`TO`[0] 1 = run, 0 = stop**.

## Start-up sequence (typical)
1. Pin mux/IOMM and `HETDIR` for outputs (also `HET_PIN_ENA`/PINDIS as needed). 2. `HETPFR` hr/lr. 3. Load program image into instruction RAM (32-bit writes; also parity RAM init if parity on). 4. Interrupt/request enables. 5. `HETGCR = CMS | HET_PIN_ENA | TO`.
GPIO use: `HETDIR` (1 = output), `HETDOUT`, `HETDSET`/`HETDCLR` (atomic), `HETDIN`, `HETPDR` open-drain, `HETPULDIS`/`HETPSL` pulls.

## Interrupts / DMA / HTU
Instruction-level interrupts raised by program (priority level 1 or 2 via HETPRY); `HETOFF1/2` return pending instruction offset (read clears). Exception conditions: program overflow, APCNT under/overflow, parity, etc. (HETEXC1/2). Requests to DMA (via DMA request lines) or HTU selectable per instruction with `HETREQENS`, destination `HETREQDS`. Hardware priority scheme exists between interrupt levels.

## Other
- Suppression filter per input pin (HETSFENA, preload HETSFPRLD) for noise.
- Loop-back pairs for self-test; pin disable feature (nDIS) forces outputs Hi-Z.
- Edge detection and HR capture/compare timing details are instruction-specific; examples exist for PWM, pulse gen, pulse measurement (WCAP).
- HWAG: angle tick generator with tooth period tracking, acceleration/deceleration — only for crank/motor applications (skip for BMS).

## Gotchas
- `CMS` must be set for single-N2HET systems; `TO` only starts the timer after program is loaded.
- Don't write 8/16-bit to RAM; don't touch reserved +0xC word.
- Missing time slots → overflow; widen lr before hr.
- Output buffers need `HET_PIN_ENA` and direction + pin mux.
- Program (and HR clock) are tied to VCLK2, not VCLK.
