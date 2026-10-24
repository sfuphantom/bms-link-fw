# HTU (High-End Timer Transfer Unit) — TMS570LS12x/11x
**Context:** Dedicated mini-DMA that moves data between **N2HET RAM** and main RAM (or flash/peripheral address space) without CPU access, triggered by N2HET instruction requests. Not a general DMA — it only talks to its paired N2HET (HTU1↔N2HET1, HTU2↔N2HET2). 8 "double control packets" (DCP0–7), each with two buffers A and B (CP A / CP B). Use with 09_N2HET handoff. 32- or 64-bit transactions.

## Addresses
HTU1 registers `0xFFF7A400`, HTU2 `0xFFF7A500`. Control-packet RAM (parity protected): **HTU1 `0xFF4E0000`, HTU2 `0xFF4C0000`** (initialize via system RAM auto-init: MSINENA bit 4 for HTU1, bit 16 for HTU2).

## Control packets (RAM, 4 words per DCP; DCPx at `base + 0x10·x`)
| Off | Reg | Content |
|---|---|---|
| +00 | IFADDRA | initial main-memory address of buffer A (32-bit) |
| +04 | IFADDRB | initial main-memory address of buffer B |
| +08 | IHADDRCT | `DIR`[23] (0 = N2HET→memory, 1 = memory→N2HET) · `SIZE`[22] (0 = 32-bit, 1 = 64-bit) · `ADDMH`[21] (0 = N2HET address +16 B per element, 1 = +8 B — use with 64-bit to load instruction blocks) · `ADDMF`[20] (0 = post-increment main address, 1 = constant) · `TMBA`[19:18], `TMBB`[17:16] transfer mode (0 = one-shot, 1 = circular; **auto-switch mode** also exists — see mode table) · `IHADDR`[12:2] initial N2HET word address |
| +0C | ITCOUNT | `IETCOUNT`[20:16] elements per frame · `IFTCOUNT`[7:0] frames per buffer |
Current (working) values at **`base + 0x100 + 0x10·x`:** CFADDRA (+00), CFADDRB (+04), CFCOUNT (+08: `CFTCTA`[23:16], `CFTCTB`[7:0] = frames done, updated at frame end). Clear these before starting.
64-bit transfer from an N2HET instruction transfers Control Field + Data Field together (CF at word n, DF at n+1); pointing at PF transfers PF+CF.

## Buffer modes
| CP A mode | CP B mode | Behavior |
|---|---|---|
| one-shot | (unused) | buffer A fills once; buffer-full interrupt says done |
| auto-switch | one-shot | doubles the buffer size (A then B) |
| circular | circular | CPU toggles between A/B at will; the "frozen" buffer is read while HTU fills the other; no interrupts needed (BFINTFL flags circular overrun) |
| auto-switch | auto-switch | A fills, interrupt, HTU switches to B while CPU reads A, and so on (ping-pong) |

## Registers (offset from base)
00 **HTU GC** (`VBUSHOLD`[24], **`HTUEN`[16]**, `DEBM`[8] 1 = keep running in debug, `HTURES`[0] soft reset) · 04 **CPENA** (bits 2x/2x+1 control DCP x) · 08/0C/10/14 BUSY0–3 · 18 ACPE (active CP + error) · 20 RLBECTRL (request-lost / bus-error interrupt enables) · 24 BFINTS / 28 BFINTC (buffer-full int set/clear) · 2C INTMAP (line A/B routing) · 34 INTOFF0 / 38 INTOFF1 (pending interrupt: `INTYPE` + `CPOFF` code; CPOFF Ah = DCP5/CP A, etc.) · 3C BIM (buffer init mode) · 40 RLOSTFL · 44 BFINTFL · 48 BERINTFL · 4C/50 MP1S/MP1E · 54 DCTRL · 58 WPR · 5C WMR · 60 ID · 64 PCR / 68 PAR (parity) · 70 MPCS · 74/78 MP0S/MP0E (memory-protection regions).

### CPENA encoding (write / read)
Write: `01` = enable CP A, disable CP B simultaneously · `10` = enable CP B, disable CP A · `11` = disable both · `00` = no change. Read: `00` DCP disabled · `01` A enabled · `10` B enabled. Switching A↔B keeps pending requests; 00→01/10 clears old pending requests. Certain conditions (request lost, bus error, mem-protection violation, one-shot end) auto-disable a DCP (bits set to 00).

## Setup sequence
1. Init HTU RAM (MSINENA). 2. For each DCP: IFADDRA/B, IHADDRCT, ITCOUNT; clear CFADDRA/B, CFCOUNT. 3. Clear BFINTFL/RLOSTFL. 4. Configure N2HET program to raise **requests** (HTU request lines) at the right instruction (`HETREQENS`, `HETREQDS`). 5. Interrupt mapping/enables (BFINTS, RLBECTRL, INTMAP) + VIM. 6. `GC.HTUEN=1`. 7. `CPENA` to start CP A (and/or B).
Circular ping-pong readout: switch CPENA, wait BUSY=0 for the frozen CP, read CFTCT for fill level, check RLOSTFL/BER, read buffer, reset CF* and clear BFINTFL.

## Request lost / quiet requests / protection
- **Request lost:** a new N2HET request arrives while the DCP is still transferring (frame not finished) → RLOSTFL; DCP can auto-disable. **Quiet requests** (don't count) used for control.
- Arbitration: lower DCP number = higher priority; transfers can be interrupted at frame boundaries.
- Memory protection: window MP0 (+ MP1) restricts main-memory addresses; violation → flag + DCP disabled. Parity on packet RAM.
- BER (bus error) flag if the memory transaction fails.

## Gotchas
- HTU needs `HTUEN` and the **N2HET program to issue requests**; no requests → nothing moves.
- Initialize control-packet RAM (parity) before use.
- `VBUSHOLD` can hog the VBUS — leave 0 unless needed.
- N2HET addresses in IHADDR are word addresses of N2HET RAM; element index step depends on ADDMH/SIZE.
