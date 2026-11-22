# MAXKGO LTC6811 12S Smart Slave BMS: Handoff Notes

Compiled 2026-11-05. Sources: MAXKGO product page, Sequre mirror page, ENNOID-BMS repo/datasheet, LTC6811 datasheet excerpts.
Every value is tagged **[Listing]** (seller's claim), **[ENNOID]** (open-source parent design), **[LTC6811 DS]** (chip datasheet), **[Calc]** (my math) or **[Unverified]** (from memory / not confirmed).

---

## 1. Bottom line on the datasheet

**MAXKGO publishes no board-specific datasheet** for this slave board. Checked: the product page, the MAXKGO Download Area, and a web search. The Download Area only has user manuals for other BMS models, the PC/Android tools, the USB driver, and a protocol doc.

Closest documents:

| What | Link | Notes |
|---|---|---|
| Product page (the only spec source from the seller) | https://maxkgo.com/products/maxkgo-ltc6811-12s-smart-vesc-slave-bms-li-ion-lifepo4-lto-battery-protection-board | SKU BMS00CV12, board "LTC6811-12W-AL V1.2" |
| Sequre (sister brand) listing, same board, more detail (dimensions, weights, pack-config tables) | https://sequremall.com/products/bms | |
| **ENNOID-BMS Datasheet.pdf** (open-source design the MKBMS is based on; includes the LTC6811-12S slave) | https://cadlab.io/project/1987/master/files (file: Datasheet.pdf) | Direct download: https://cadlab.io/download-file/1464/diy_ftree/master/RGF0YXNoZWV0LnBkZg%3D%3D |
| ENNOID-BMS GitHub (schematics for `Slaves/LTC6811/12W`) | https://github.com/EnnoidMe/ENNOID-BMS | Check here for the real MOSFET/resistor BOM |
| MAXKGO Download Area (tools, protocol) | https://maxkgo.com/pages/maxkgo-download-area | |
| Chip datasheet (NOT the board; reference only) | LTC6811-1/-2 on analog.com | |

Seller contact for a real datasheet or schematic: support@maxkgo.com, info@maxkgo.com.

**Caveat:** the MAXKGO board is a derivative of ENNOID's 12W slave, not necessarily identical. Treat ENNOID's schematic as likely, not guaranteed, to match.

---

## 2. Identity

- Product: MAXKGO LTC6811 12S Smart Slave BMS (MKBMS slave)
- PCB: LTC6811-12W-AL V1.2 (aluminum-alloy PCB) [Listing]
- Chip: LTC6811-1 (daisy-chain isoSPI version, by ENNOID design) [ENNOID]
- Size 100 x 60 mm, board weight 31.8 g [Sequre listing]
- Price seen: $82.53 (list $98.00), 20 in stock on 2026-11-05
- Compatible with: MKBMS master boards, ENNOID BMS (firmware and tool) [Listing]
- Despite "VESC" in the URL, the listing never says it works with VESC directly. It is an ENNOID/MKBMS slave.
- Note: the listing title says "LTC6211" in one place; that is a typo for LTC6811.

---

## 3. Balancing / drain specs

| Item | Value | Source |
|---|---|---|
| Balance resistor | **33 ohm on all 12 channels** | [Listing] |
| Stated capacity | "0.125 A" | [Listing]; ambiguous, see below |
| Balancing type | Passive (dissipative), controlled by the LTC6811's internal S-pin switches | [ENNOID], [LTC6811 DS] |
| Balance "MOSFET" | **No discrete MOSFETs listed.** The LTC6811 has an internal N-channel switch per cell, on-resistance up to ~10 ohm | [LTC6811 DS via Macnica summary] |
| External FETs | Not mentioned in the listing; would be needed for higher drain current | [Unverified] |
| Per-cell PWM duty control | Supported by the chip | [LTC6811 DS] |

### Drain current vs cell voltage (calculated)

I = V / (33 ohm + R_switch). Switch resistance is between ~0 and the 10 ohm max, so a range is given.

| Cell voltage | Best case (33 ohm) | Worst case (43 ohm) | Power in resistor (best case) |
|---|---|---|---|
| 2.5 V (LTO / discharged LFP) | 76 mA | 58 mA | 0.19 W |
| 3.2 V (LiFePO4 nominal) | 97 mA | 74 mA | 0.31 W |
| 3.65 V (LiFePO4 full) | 111 mA | 85 mA | 0.40 W |
| 3.7 V (Li-ion nominal) | 112 mA | 86 mA | 0.41 W |
| 4.2 V (Li-ion full) | 127 mA | 98 mA | 0.53 W |

- **Max drain speed: roughly 0.1 A per cell at ~4.2 V.** This matches the "0.125 A" claim, which is almost certainly the max balance current (4.2 V / 33 ohm = 127 mA), not a resistor rating.
- Drain rate falls linearly with cell voltage (it is just a resistor), so it is much slower on LFP/LTO than on 4.2 V Li-ion.
- **[Unverified] Resistor wattage is not stated.** At 4.2 V one resistor dissipates ~0.5 W. If the resistors are 0.125 W parts (a possible misreading of "0.125A"), continuous balancing would overheat them. Check the schematic/BOM before relying on continuous balancing.
- Heat: if all 12 channels balance at once at 4.2 V that is about 6 W total on a 100x60 mm board. The aluminum PCB helps, but expect the firmware to limit simultaneous channels or use temperature cutback. **[Unverified]**, confirm in firmware settings.

### Drain speed while charging

- Balancing current is **independent of charge current.** The resistors drain the same ~V/33 ohm regardless of what the charger is doing.
- ENNOID's documentation says its BMS "can trigger the passive balancing function during charging for cells above the configured limit." [ENNOID README] The MKBMS runs ENNOID-compatible firmware, so expect the same, but MAXKGO does not document its own firmware behavior. **[Unverified for MAXKGO firmware]**
- Practical effect: at ~0.1 A bleed, balancing is only useful for small imbalances (tens of mAh/hour of correction). It cannot absorb a significant share of a multi-amp charge current. Cells at the top need the charger to taper or stop (CC/CV or BMS charge cut-off).
- The LTC6811 also suspends/interleaves discharge around cell measurements ("Discharge Control During Cell Measurements" in its datasheet). **[Unverified]** how the firmware handles it.

---

## 4. Limits of the board

### Voltage
| Item | Value | Source |
|---|---|---|
| Cells per slave | 5 to 12 in series | [Listing] |
| Powered by | The monitored cells themselves (self-powered) | [Listing] |
| Cell measurement range | 0 to 5 V per cell | [ENNOID], [LTC6811 DS] |
| Measurement accuracy | 1.2 mV (max total error) | [Listing], [LTC6811 DS] |
| Chip supply abs-max (V+ to V-) | 75 V | [LTC6811 DS] |
| Per-cell abs-max (C(n) to C(n-1)) | 8 V | [LTC6811 DS] |
| Isolation | "1000 VDC+" scalable and isolated | [Listing]; test conditions not given |
| Min operating pack voltage | Not stated. LTC6811 supply minimum is **[Unverified]** around 11 V, so a 5S pack at low charge may be marginal | |
| Chemistries | Li-ion, LiFePO4, LTO (cell range is wide enough, but thresholds are set in the master/firmware) | [Listing] |

### Temperature
| Item | Value | Source |
|---|---|---|
| Operating range | **-40 to 120 C** | [Listing] |
| Sensors | 1 + 4 per slave; channel 1 is the slave's own on-board sensor, 4 external NTC inputs (NTCs NOT included) | [Listing] |
| NTC type / beta | Not stated. Check the schematic (typically 10k NTC) | [Unverified] |

### Current
- **The slave carries no pack current.** It only taps cells (sense wires) and balances at ~0.1 A per cell.
- Pack current limits belong to the master/contactors/shunt. For reference, the MKBMS-LV Master Mini listing claims 30-150 V, 500 A continuous, 2500 A pulsed. [Sequre listing]
- Sleep current: 6 uA. [Listing]

---

## 5. Timing

| Item | Value | Source |
|---|---|---|
| Measure all 12 cells | 290 us (fastest ADC mode) | [LTC6811 DS] |
| Chip watchdog | Enters sleep if no valid command for ~2 s | [LTC6811 DS via UW Midsun notes] |
| Wake-up | Toggle CS (isoSPI wake pulse) before sending commands | [LTC6811 DS] |
| isoSPI speed | Up to 1 Mb/s, twisted pair, up to 100 m | [LTC6811 DS] |
| Discharge timer (DCTO) | Chip has a programmable software discharge timeout (roughly 0.5 to 120 min) that stops balancing if the host goes silent | [LTC6811 DS]; exact steps **[Unverified]** |
| Firmware polling/balance cycle | Not documented by MAXKGO | [Unverified] |

---

## 6. Connectors and wiring

- Balance and temperature: Molex SL horizontal connectors (male + female + crimps included) [Listing]
- Package includes: 2x GH1.25 double-head 2P same-direction cable, 1x HY2.0 5P cable, 1x HY2.0 13P cable, 4x nylon standoffs, 4x nylon nuts [Listing]
- 13P cable is for cell taps. The listing does not give the pinout; see the ENNOID datasheet slave section.
- ENNOID note: when monitoring 13 cells on the 12W-type slave, pins 13/14/15 are tied together at the last positive terminal. **That note concerns other slave variants; confirm pinout for this exact board before wiring.** [ENNOID]
- Daisy-chain via isoSPI. Compatible with MKBMS/ENNOID masters.

### Multi-slave pack examples (from listing)
- 23S = 12S + 11S (2 slaves)
- 35S = 12S + 12S + 11S (3 slaves)
- Rule: for uneven cell counts, **only the last slave in the chain** should have fewer cells connected.
- 8 cells per board works (the chip supports it); unused channels must be wired correctly per the schematic. **[Unverified]**

---

## 7. Things to know / gotchas

1. **No official board datasheet.** Anything beyond the listing comes from ENNOID or the LTC6811 chip datasheet.
2. **Drain is slow by design**: about 0.1 A max. Fine for maintaining balance on small packs, slow for rescuing badly imbalanced large cells. For big cells, ask about an active balancer or external-FET balancing.
3. **"0.125 A capacity" is ambiguous**; I read it as max balance current. Resistor wattage is the real thermal limit and is unconfirmed.
4. **Temperature sensors are not included.** Buy 4x NTCs (check type first).
5. **It's only a slave.** It needs an MKBMS or ENNOID master to do anything, and the master handles contactors, shunt and current.
6. **Self-powered by the cells** means it draws from the pack at all times (6 uA asleep, more awake); allow for this in long storage.
7. **Firmware/tool:** MAXKGO ENNOID-BMS-Tool V5.04 (HV) and V5.02 (standard) plus MAXKGO-TOOL 5.02 (Bluetooth models) are on the Download Area page. Check which matches your master. The Download Area also lists a BMS protocol doc (Google Doc) and CP210x USB drivers.
8. **Listing inconsistencies:** typo "LTC6211"; the Sequre page says "up to 9 temperature sensors" and "5 to 18 cells" for the *family*, while the 6811 variant is 1+4 sensors and 12 cells.
9. **Isolation claim (1000 VDC+)** is unqualified; do not treat it as a certified rating.
10. Warranty/return policies are on maxkgo.com footer pages.

---

## 8. Open questions to resolve (ask seller or read the schematic)

- [ ] Balance resistor wattage and package size
- [ ] Is anything besides the LTC6811 internal switch used for balancing (external MOSFETs)?
- [ ] NTC type and beta value expected
- [ ] Minimum pack voltage for reliable operation
- [ ] Exact cell-tap connector pinout (13P) and wiring for fewer than 12 cells
- [ ] Firmware balance behavior: simultaneous channels, temperature cutback, balance-while-charging threshold
- [ ] Does the board match ENNOID's `Slaves/LTC6811/12W` schematic exactly?

---

## 9. Key links

- Product: https://maxkgo.com/products/maxkgo-ltc6811-12s-smart-vesc-slave-bms-li-ion-lifepo4-lto-battery-protection-board
- Download Area: https://maxkgo.com/pages/maxkgo-download-area
- Sequre mirror listing: https://sequremall.com/products/bms
- ENNOID-BMS (CADLAB): https://cadlab.io/project/1987/master/files
- ENNOID-BMS (GitHub): https://github.com/EnnoidMe/ENNOID-BMS
- ENNOID BMS thread: https://endless-sphere.com/forums/viewtopic.php?f=14&t=92952
- MAXKGO support: support@maxkgo.com / info@maxkgo.com
