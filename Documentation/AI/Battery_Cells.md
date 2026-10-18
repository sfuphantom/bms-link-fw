# Cell Handoff — INR-18650-P26A (Li-ion 18650)

Source: 1-page manufacturer datasheet. The datasheet says values are for reference only (no guarantee). Values marked `~` were read by eye off the datasheet graphs (about ±3–5%); everything else is a printed number.

## Identity
- Model: INR-18650-P26A, cylindrical 18650, steel can. Chemistry is not stated (the "INR" designation means lithium-nickel based).
- Mass: 50 g max per cell.

## Printed specs
| Parameter | Value |
|---|---|
| Typical capacity | 2600 mAh (9.5 Wh) |
| Nominal voltage | 3.6 V |
| Charge voltage (max) | 4.2 V |
| Discharge cutoff (min) | 2.5 V |
| Standard charge | 2.6 A (1C), CC-CV to 4.2 V, terminate at 50 mA, about 1.5 h |
| Max charge current | 6.0 A |
| Max discharge current | 35 A (continuous vs pulse NOT specified) |
| AC internal resistance (1 kHz) | 20 mΩ max |
| Charge temperature window | 0 °C to 60 °C |
| Discharge temperature window | −40 °C to 60 °C |
| Energy density | 190 Wh/kg, 535 Wh/L |

**Not in the datasheet (do not invent):** DC internal resistance, SOC-OCV table, thermal resistance or heat data, pulse-current duration limits, absolute-maximum voltage separate from 4.2 V, storage and self-discharge data, safety/abuse data.

## What the graphs show (all discharge to 2.5 V cutoff, 23 °C unless noted)

**Charge profile (CC-CV, 4.2 V, 50 mA cutoff, 23 °C)**
- 2.6 A charge: constant-current phase lasts `~0.9 h` (reaches about 88–90% of capacity), CV taper ends `~1.15 h`; capacity about 100% by `~1.0–1.1 h`. Cell voltage jumps to about 3.5 V almost immediately and rises roughly linearly to 4.2 V at the end of CC.
- 5.2 A charge: CC lasts `~0.44 h` (about 88%), taper ends `~0.65 h`. Voltage is about 0.1–0.15 V higher than the 2.6 A curve during CC.
- Takeaway: charging above 1C saves little time (the CV tail dominates) and the datasheet's "standard" is 2.6 A.

**Discharge vs rate (charged at 2.6 A)**
| Rate | Start V (`~`) | Mid-discharge V (`~`) | Capacity delivered (`~`) |
|---|---|---|---|
| 0.52 A | 4.15 V | 3.65 V @ 1200 mAh | 2550 mAh |
| 2.6 A | 4.10 V | 3.60 V @ 1200 mAh | 2450 mAh |
| 10 A | 3.95 V | 3.45 V @ 1200 mAh | 2520 mAh |
| 20 A | 3.75 V | 3.30 V @ 1200 mAh | 2500 mAh |
- Capacity is nearly rate-independent up to 20 A, but voltage sags about 0.4 V at 20 A versus 0.52 A (consistent with the 20 mΩ max resistance). All curves have a sharp knee near 2300–2400 mAh.

**Discharge vs temperature (2.6 A, charged at 23 °C)**
| Temp | Capacity (`~`) | Voltage behavior |
|---|---|---|
| 60 °C / 45 °C | 2600 mAh | about 4.1 V start, curves overlap |
| 23 °C | 2450 mAh | about 4.1 V start |
| 0 °C | 2200 mAh | about 3.95 V start |
| −20 °C / −30 °C | 2050–2070 mAh | about 3.5–3.7 V start |
| −40 °C | 1900 mAh | starts about 2.9 V, rises to about 3.3 V, then sags |
(The mapping of curve color to temperature is inferred from the legend; treat −20/−30 °C as approximate.)

**Cycle life (charge 2.6 A CC-CV; discharge 10 A or 20 A CC; 23 °C)**
- About 97% capacity at 100 cycles, about 95% at 200, about 91% (10 A) / about 89% (20 A) at 500 cycles.

## Derived numbers (calculated, not printed)
- 1C = 2.6 A; 35 A ≈ 13.5C; 6 A charge ≈ 2.3C.
- Worst-case I²R per cell with 20 mΩ: about 8 W at 20 A, about 24.5 W at 35 A. Use this for thermal sizing.
- Per-cell energy 9.5 Wh; pack energy = 9.5 Wh × cell count.

## BMS-relevant limits to implement
- Cell under-voltage: 2.5 V; over-voltage: 4.2 V. Leave margin for the monitor IC's measurement error.
- Block charging below 0 °C (and above 60 °C); block discharge above 60 °C.
- Charge termination: constant voltage 4.2 V per cell, stop at 50 mA (about C/52).
