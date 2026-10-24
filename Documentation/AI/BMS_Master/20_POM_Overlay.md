# POM (Parameter Overlay Module) — TMS570LS12x/11x
**Context:** Lets the CPU's accesses to read-only flash be redirected to RAM (internal or external via EMIF) so calibration parameters/code can be changed live (development/calibration use). Not used in normal BMS firmware. Lives in power domain **PD2** (see PMM handoff). Registers base **`0xFFA04000`** (privileged).

## Rules
- Up to **32 regions**; each region replaces a flash address range with an overlay-memory range of the same size. Region size 64 B … 256 KB (powers of 2). Region start addresses (program and overlay) are 23-bit (8 MB window) and **must be multiples of the region size**.
- **Only the main CPU is redirected.** Any other bus master (DMA, etc.) accessing an overlaid flash range → deadlock/hang; other masters must access the overlay RAM directly.
- Overlay memory is read-only through POM; write to the RAM at its normal address.
- **Disable CPU ECC (CP15) while overlay is enabled,** otherwise ECC errors (overlay RAM has no matching ECC).
- **Do not enable POM while flash/RAM are swapped** (`BMMCR1.MEM SWAP`).
- Wait states inserted automatically if overlay memory is slower.
- **Bus-contention timeout:** when overlaying flash, another master accessing the TCM flash can hang the bus. Enable `POMGLBCTRL.ETO = 0xA`; if the POM read can't finish within **32 HCLK**, flag `POMFLG.TO` sets and the CPU gets a prefetch/data abort. Abort handlers must check `TO`, clear it, and treat it as POM contention.

## Registers (offset)
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

## Typical sequence
1. Ensure PD2 on; disable ECC in CP15; load overlay data into target RAM. 2. Program region(s): PROGSTART, OVLSTART, REGSIZE. 3. `POMGLBCTRL`: set `OTADDR`, `ETO=0xA`, `ON/OFF=0xA`. 4. Install abort handlers that check/clear `POMFLG.TO`. 5. To disable: `ON/OFF` ≠ 0xA.

## Gotchas
- Size 0 disables a region; non-aligned starts misbehave.
- DMA/other masters must not touch overlaid flash.
- Overlay doesn't provide ECC → CPU ECC must be off.
