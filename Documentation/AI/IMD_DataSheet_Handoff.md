# IMD Handoff: Bender ISOMETER IR155-3203 / IR155-3204 (V004, 06.2024)

Insulation monitoring device (IMD) for unearthed DC HV systems. Reports insulation
resistance and status as a PWM signal on one pin. Source doc: `IR155-32xx-V004_D00115_05_D_XXEN/06.2024`.

## Files

| File | Role |
|---|---|
| `IMD_DataSheetSpec.h` | Raw datasheet numbers only. Prefix `IMD_DS_`. Included only by `IMD_hardware.h`. |
| `FSAE_Rules_2027_EV.h` | Rule numbers only. Prefix `RULE_`. Rule IDs in comments. Included only by `*_hardware.h`. |
| `IMD_hardware.h` | Enum, user placeholders (tolerances, fault limits), derived macros. Includes both spec headers. |
| `IMD.h` / `IMD.c` | eCAP read, state decode, resistance, isolation %, fault logic. Use macros only, no magic numbers. |

Same layout as `BatteryCell_DataSheetSpec.h` / `BatteryCell_Hardware.h`.
Rule: if it is not printed on the datasheet it does not go in `IMD_DataSheetSpec.h`.

## Variants

| Type | Output pin | Idle/high level | Pull resistor |
|---|---|---|---|
| IR155-3204 | M_HS (XK1A pin 5) | Us - 2 V | 2.2 kOhm pull-down to Kl.31 |
| IR155-3203 | M_LS (XK1A pin 6) | Kl.31 + 2 V (inverted) | 2.2 kOhm pull-up to Kl.15 |
| Both | OK_HS (pin 8) | High = no fault | 2.2 kOhm pull-down to Kl.31 |

OK_HS Low means: insulation fault, device error, earth connection fault, undervoltage, or device off.

## Output M: frequency = state

| Freq | State | Duty meaning |
|---|---|---|
| 0 Hz | Short circuit (Hi: short to Kl.15 / Low: IMD off or short to Kl.31) | none |
| 10 Hz | Normal | 5...95 % = resistance (DCP) |
| 20 Hz | Undervoltage detected | 5...95 % = resistance (DCP) |
| 30 Hz | Speed start (SST) | 5...10 % good, 90...95 % bad |
| 40 Hz | Device error | 47.5...52.5 % |
| 50 Hz | Earth connection fault (Kl.31) | 47.5...52.5 % |

Datasheet frequency uncertainty: +/-5 % at each frequency. Code decodes with bounds at the middle between
states (5 / 15 / 25 / 35 / 45 / 55 Hz).

## Duty <-> resistance (10 / 20 Hz)

`RF = 90% x 1200 kOhm / (duty - 5%) - 1200 kOhm`

| Duty | Resistance |
|---|---|
| 5 % | > 50 MOhm (infinite) |
| 7.11 % | 50 MOhm |
| 14.64 % | 10 MOhm (top of measuring range) |
| 50 % | 1200 kOhm |
| 54.09 % | 1000 kOhm (max response value) |
| 88.08 % | 100 kOhm (min / default response value) |
| 95 % | 0 kOhm |

Rows 7.11, 14.64, 54.09, 88.08 are computed from the equation, not printed.

## Accuracy

- Measuring range 0...10 MOhm.
- SST (30 Hz): good if R > 2 x Ran, bad if R < 0.5 x Ran, between is undefined.
- DCP absolute: 0...85 kOhm = +/-20 kOhm.
- DCP relative: 100 kOhm +/-15 % narrowing to +/-7 % at 1.2 MOhm, then widening to +/-15 % at 10 MOhm.
- Absolute uncertainty graph labels: +/-15 kOhm at 100 kOhm, +/-84 kOhm at 1.2 MOhm, +/-1.5 MOhm at 10 MOhm.
- Undervoltage detection: +/-10 % at Un >= 100 V, +/-5 % at Un >= 300 V.
- Response value Ran: 100 kOhm...1 MOhm (default 100 kOhm). Hysteresis 25 %.
- Undervoltage threshold 0...500 V. Default 0 V (inactive) on -3204, 300 V on -3203.

## Timing

- SST first result <= 2 s after power on (typ < 1 s at Un > 100 V).
- DCP starts 2 s after power on, first valid result <= 17.5 s.
- Fault on (tan) and off (tab) times depend on Fave (1...10, factory 10; 10 recommended for EVs):

| Fave | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| tan (s) | 5 | 7.5 | 7.5 | 10 | 12.5 | 12.5 | 15 | 17.5 | 17.5 | 20 |
| tab (s) | 20 | 20 | 26 | 26 | 26 | 33 | 33 | 33 | 40 | 40 |

- Conditions: Ce = 1 uF, Un = 1000 V.
- Self test: every 5 min, lasts 10 s, outputs are not affected, add 10 s to tan/tab.
- Ce > 1 uF shrinks the range (1 MOhm at 3 uF, tan = 68 s).

## Power and electrical

| Item | Value |
|---|---|
| Supply | 10...36 V DC (Kl.15) |
| Operating current | <= 150 mA |
| Power | < 2 W |
| Inrush | 6 A for 2 ms, Ik max 2 A |
| Supply dropout tolerated | <= 2 ms |
| Load dump | No protection above 50 V, add external |
| HV monitored | DC 0...1000 V, AC 660 V RMS (10 Hz...1 kHz), 1000 V peak |
| Isolation test | AC 3500 V, 1 min |
| Measuring voltage / current | +/-40 V / +/-33 uA at RF = 0 |
| Impedance at 50 Hz, internal R | >= 1.2 MOhm |
| Output load current | 80 mA |
| Turn on / off (to 90 % / 10 %) | <= 125 us / <= 175 us |
| Slew on (10...30 %) / off (70...40 %) | <= 6 V/us / <= 8 V/us |
| Temperature | -40...+105 C |
| Size / mass | 140 x 60 x 15 mm, 52 g +/-2 g |

## Graphs in the datasheet

1. PWM driver, 10/20 Hz: working range 5...95 %, grey outside is invalid. Values in the table above.
2. PWM driver, 30 Hz: valid windows 5...10 % and 90...95 %.
3. PWM driver, 40/50 Hz: valid window 47.5...52.5 %.
4. SST response: edges at 50 kOhm and 200 kOhm around Ran = 100 kOhm (0.5 x and 2 x Ran).
5. Relative uncertainty DCP and absolute uncertainty: breakpoints at 85 kOhm, 100 kOhm, 1.2 MOhm, 10 MOhm (see Accuracy).
6. Output timing diagram: 90 % / 10 % points for turn on/off and slew rates.

## NOT in the datasheet

- **Isolation percent.** Nowhere. The datasheet only gives duty -> resistance (kOhm).
  `IMD_CalcIsolation` uses a linear map `100.53763 - 1.05287 x duty` that is a project choice,
  now `IMD_ISOLATION_FIT_*` in the user placeholders section.
- **PWM resolution.** Not stated. Real resolution depends on the eCAP clock (`ECAP_COUNTS_PER_SEC`)
  and the signal period: duty step = 100 x freq / counts per second.
- Duty and frequency decode tolerance beyond the +/-5 % frequency figure. `IMD_DUTY_TOL_PERCENT` is ours.
- Fault limits (`NORMAL_IMD_RESISTANCE_kOHM` 1E3, `MIN_IMD_RESISTANCE_kOHM` 1.0 kOhm/V, 75 %, 50 % duty). Ours.

## Open items

- Rule EV.8.6.3: IMD response value >= 500 Ohm/V x 600 V = 300 kOhm (`IMD_RULE_MIN_RESISTANCE_kOHM`). The default Ran of 100 kOhm is below this, so order the customer-specific setting.
- Resistance fault = `MIN_IMD_RESISTANCE_kOHM` (kOhm per volt) x 600 V = 600 kOhm (`IMD_FAULT_RESISTANCE_kOHM`).
- The 75 % isolation limit trips at duty ~24.3 % = ~4.4 MOhm. That is stricter than the 600 kOhm and 50 % duty limits, so those two never trigger first.
- `IMD_RESISTANCE_MAX_KOHM` was 1E6 kOhm, now 50 000 kOhm (datasheet "> 50 MOhm"). It only caps the "infinite" value.
- Duty windows now use one symmetric tolerance (0.5 %). Old code had 5.0 / 10.5, 90 / 95.5, 47 / 53.5.
- Undervoltage (20 Hz) is treated as a fault in `IMD_HasFault`.
