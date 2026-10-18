# Charger Handoff — 3.3 kW On-Board Charger (HK-MF series) + BMS↔Charger CAN Protocol

Two source documents merged:
1. Hardware spec: HK-MF-48-40 / HK-MF-72-40 / HK-MF-108-32, 3.3 kW on-board charger (OBC).
2. "BMS: CAN BUS COMMUNICATION SPECIFICATION" (doc 0010, v0A): the CAN protocol.

**Caveats the reader must know**
- The CAN spec copy is incomplete: its pages are numbered "x/10" but only 7 were supplied. Messages 3–9 are missing.
- The hardware doc never names message IDs. That the HK-MF speaks this exact protocol is an assumption (it is the standard charger/BMS protocol for this class). Verify on the real unit.
- Source contradictions are listed at the end.

---

## 1. Charger hardware

### Models
| Model | Platform | Output voltage range | Max output current |
|---|---|---|---|
| HK-MF-48-40 | 48 V | 35–70 V | 40 A |
| HK-MF-72-40 | 72 V | 50–107 V | 40 A |
| HK-MF-108-32 | 108 V | 80–161 V | 32 A |

The model must match the pack's voltage range (the charger shuts off outside its range).

### Output (DC to pack)
- Power: 3300 W at 220 VAC, 1650 W at 110 VAC. Output is power-limited, so full current is only reachable when V×I ≤ 3.3 kW (derived: 40 A up to about 82 V, 32 A up to about 103 V; halve these at 110 VAC).
- Mode CV/CC. CV accuracy ±1%, CC accuracy ±2%, ripple ±5%. Voltage rise time < 5 s with overshoot < 10%.
- Shut-off response: current < 10% within 300 ms, 0 A within 500 ms.
- Output connector: pin 1 = +, pin 2 = −, rated 40 A, 6 mm² wire.
- Efficiency: ≥ 94% (output table); input table says ≥ 93% at full load.

### Input (AC)
- 90–265 VAC, 47–63 Hz, ≤ 16 A, PF ≥ 0.98 above 1650 W, standby ≤ 5 W, inrush ≤ 24 A.
- AC connector: pin 1 = N, pin 2 = PE, pin 3 = L, 16 A, 2.5–4 mm² wire. Use a 3-core cable with ground.

### Auxiliary 12 V
- 12 V CV output, 5.5 A nominal, ≤ 66 W, ±2%, ripple ≤ 1%.

### Control interface
- Low-voltage connector pins used: **pin 10 = +12 V (5 A), pin 11 = CAN_H, pin 12 = CAN_L, pin 17 = 12 V GND**; all other pins NC.
- Table lists: 12 V wake-up input ≤ 10 mA; 12 V wake-up signal output max 0.2 A; "12 V CV: sleep current ≤ 1 mA, peak ≤ 5 A".
- CAN 2.0B. Supports UDS diagnostics and CAN wake-up.

### Protections
| Protection | Behavior |
|---|---|
| Input over-voltage | trips at AC 270 ±5 V |
| Input under-voltage | trips at AC 85 ±5 V |
| Output over/under-voltage | stops output when above max or below min of the model's range ±5 V |
| Over-temperature | output derates from 85 °C internal, shuts off at 90 °C, restarts automatically after cooling |
| Output short circuit | stops output |
| Reverse polarity | protected |
| Ground fault | ≤ 100 mΩ ground resistance requirement |
| CAN loss | stops output automatically (see timeout below) |

### Environment / install (only the parts that affect integration)
- Operating −40 to 60 °C, IP67, up to 5000 m, fan-cooled, ≤ 65 dB, mass ≤ 5 kg.
- Keep ≥ 50 mm clear of obstacles and never block the air inlet/outlet. Best mount: connector side up. Side/vertical mounting is acceptable. Connector side down is forbidden.
- Safety notes: never plug/unplug connectors while powered, never open the housing powered, battery nominal voltage must match the charger model, keep the DC cable short (voltage drop affects charging), do not charge damaged or non-rechargeable batteries.

---

## 2. CAN protocol (BMS ↔ Charger)

### Bus
- 250 kbps, CAN 2.0B **extended (29-bit) IDs**, J1939-style.
- 29-bit ID layout: Priority(3) | R(1)=0 | DP(1)=0 | PF(8) | PS(8, destination) | SA(8, source). Priority is 6 for every message.
- Multi-byte fields are **big-endian (high byte first)**. All messages use 8 data bytes and a 1000 ms period.

### Node source addresses
| Node | SA |
|---|---|
| BMS | 0xF4 |
| Charger (CCS) | 0xE5 |
| Charger 2 / charging-station charger (CCS2) | 0xE6 |
| Broadcast (BCA) | 0x50 |
| Motor controller | 0xEF |

### Message summary
| Msg | CAN ID | Direction | Period | Content |
|---|---|---|---|---|
| 1 | 0x1806E5F4 | BMS → charger | 1000 ms | voltage/current setpoint + control (the essential command) |
| 10–14 | 0x1806E6F4 | BMS → charging-station charger | 1000 ms | extended battery info; **byte 8 = page 1…5** selects which message |
| 2 | 0x18FF50E5 | charger → broadcast | 1000 ms | output V/I + status flags |
Messages 10–14 are marked "only available for charging station", so a plain on-board charger likely only needs Message 1.

### Message 1 — BMS → charger, ID 0x1806E5F4
| Byte | Field | Encoding |
|---|---|---|
| 1–2 | Max allowed charging voltage | 0.1 V/count (3201 → 320.1 V) |
| 3–4 | Max allowed charging current | 0.1 A/count (582 → 58.2 A) |
| 5 | Control | 0 = charger on / charging, 1 = battery protection, charger closes output |
| 6–8 | Reserved | 0 |
(The source labels bytes 3–4 "VOL_SET", a copy-paste typo; they are current.)
Worked example (derived): 100.8 V, 10.0 A, charge enabled → `03 F0 00 64 00 00 00 00`.

### Messages 10–14 — ID 0x1806E6F4, distinguished by byte 8
| Page (byte 8) | Msg | Bytes |
|---|---|---|
| 1 | 10 | 1–2 max charge V (0.1 V); 3–4 max charge I (0.1 A); 5 control (same as Msg 1); 6 max allowed discharge current (10 A/count, 2 → 20 A); 7 reserved |
| 2 | 11 | 1–2 battery nominal Ah (0.1 Ah); 3–4 battery actual Ah (0.1 Ah); 5–6 single-cell OV protection voltage (1 mV); 7 number of cells (1–255, 0 = invalid, see page 5) |
| 3 | 12 | 1–2 cell max V (1 mV); 3–4 cell min V (1 mV); 5–6 single-cell UV protection voltage (1 mV); 7 battery state: bit0 = over-voltage flag, bit1 = under-voltage flag, 0x00 = normal |
| 4 | 13 | 1–2 pack total V (0.1 V); 3–4 actual current (0.1 A; the top bit is a direction flag: 0 = charging, 1 = discharging, my reading of "max byte means mark"); 5 SOC 0–100%; 6 max temp; 7 min temp (temperature bytes are °C + 100: 0 → −100 °C, 125 → 25 °C) |
| 5 | 14 | 1–2 number of cells as 16-bit (0 = invalid); bytes 3–7 unused |

### Message 2 — charger → broadcast, ID 0x18FF50E5
| Byte | Field | Encoding |
|---|---|---|
| 1–2 | Output voltage | 0.1 V/count |
| 3–4 | Output current | 0.1 A/count (same direction flag as above) |
| 5 | Status flags | bits below (the source calls them "Byte0–Byte4", but they are bits of byte 5) |
| 6–8 | Reserved | |

Status bits (0 = normal in all cases):
- bit0 hardware failure
- bit1 charger over-temperature protection
- bit2 input voltage wrong (charger will not run)
- bit3 start state: 0 = charger sees battery voltage and enters start, 1 = stays closed (reverse-polarity protection)
- bit4 communication timeout

### Operating behavior
1. The BMS must send Message 1 (and 10+11+12 if used) **every 1 s**. The charger runs at the V/I in the latest message.
2. If no message arrives within **5 s**, the charger enters communication-error state and **closes its output**.
3. The charger broadcasts Message 2 every 1 s (for displays or logging).
4. To stop charging: send control = 1, or simply stop sending (timeout).
5. Sensible BMS use: set voltage = (cell count × 4.2 V cell max), current = BMS-allowed limit, and drop the current or set control = 1 on any BMS fault.

---

## 3. Contradictions and open questions
- **Baud rate:** CAN spec says 250 kbps; hardware doc says 125/250/500 selectable (section 4.4) but only 250/500 (section 7). Confirm the unit's configured rate.
- **Termination:** hardware section 4.4 lists a 120 Ω terminal resistance; section 7 says "no terminating resistor". Measure CAN_H–CAN_L resistance on the unit.
- **Pin 10 "+12 V":** the doc does not say whether it is an input (control-board supply, which fits "sleep current ≤ 1 mA") or the 12 V auxiliary output. No wake-up pin is listed. Verify before wiring.
- Messages 3–9 are not in the supplied copy; the protocol version (0A) is old.
