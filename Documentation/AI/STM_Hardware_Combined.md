# Handoff — STM32H755ZIT6 on NUCLEO-H755ZI-Q (MB1363)

Scope: only the **STM32H755ZIT6** (dual Cortex-M7 480/400 MHz + Cortex-M4 240/200 MHz, 2 MB flash, ~1 MB RAM, LQFP144, internal SMPS) mounted on the **NUCLEO-H755ZI-Q** board (Digi-Key 497-NUCLEO-H755ZI-Q-ND). Files were distilled from: STM32H755xI datasheet DS12919 Rev 3 (246 pp), board user manual UM2408 Rev 6 (MB1363, 57 pp), Nucleo-144 data brief DB3171 Rev 20. Branding, mechanical, compliance, disposal and other-board content were dropped; figure/diagram info was folded into text. **No web sources were used** (see file 99 for clearly-labelled unverified background notes).

## Load only what the task needs
| Section | Load when working on... |
|---|---|
| §01 | powering the board, JP2/JP4/JP5, SMPS vs LDO, can't connect to target |
| §02 | SWD/ST-LINK, CN5, VCP serial console, LEDs, buttons |
| §03 | which pin is on which Zio/morpho/Arduino pin, shield compatibility |
| §04 | clocks (HSE/LSE), USB, Ethernet, motor control, solder bridges |
| §05 | cores, memories, ECC, boot, DMA, bus matrix |
| §06 | VOS/frequency limits, supplies, abs-max ratings, low-power |
| §07a | package pin numbers, analog/special pin functions, 5 V tolerance |
| §07b | AF numbers for GPIO muxing (largest file) |
| §08 | USART/SPI/I2C/CAN/USB/ETH/SDMMC/timers specs |
| §09 | ADC/DAC/COMP/OPAMP/DFSDM/crypto |
| §10 | **start here for firmware**: required init, default pins, traps |
| §99 | background knowledge NOT from the PDFs; verify first |

## Five facts that prevent most mistakes
1. Board default is **SMPS direct supply** -> firmware must call `HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY)`; mismatch bricks debug access until BOOT0 recovery. Max CPU = **400 MHz** (VOS0/480 MHz needs LDO hardware config).
2. HSE is an **8 MHz clock from ST-LINK (bypass)**; board has a 32.768 kHz LSE crystal.
3. Console = **USART3 PD8/PD9** via ST-LINK VCP; Arduino D0/D1 are LPUART1 PB7/PB6.
4. GPIOs reset to analog mode; several MCU pins are shared with on-board functions (LEDs, Ethernet RMII, SWD, SMPS control) — see §03/§04 conflict lists.
5. Power the board **before** plugging a cable into USB CN13; use the JP2 order procedure when powering from VIN/EXT.

## Source-document inconsistencies noticed
- UM2408 7.4.1 says 500 mA USB request; 7.4.6 says 300 mA.
- UM2408 text for VIN refers to "Table 4"; correct table is Table 5. LD6 description in 7.6 mistakenly says "LD5".
- UM2408 labels Zio D68-D70 as I2C_2; datasheet shows PF14/PF15/PB5 only on **I2C4**.
- Default (bold) solder-bridge states were lost in PDF text; defaults are given only where prose states them (e.g., A4/A5 ADC-vs-I2C default is unclear).
- DB3171 lists H755ZI-Q user manual as UM2408 (MB1363); the manual also covers H745/H7A3 — H7A3 has no Ethernet.

## Contents
- [01. NUCLEO-H755ZI-Q — Board: power, jumpers, SMPS/LDO config](#01-nucleo-h755zi-q--board-power-jumpers-smpsldo-config)
- [02. NUCLEO-H755ZI-Q — Board: debug/programming (STLINK-V3E), VCP, LEDs, buttons](#02-nucleo-h755zi-q--board-debugprogramming-stlink-v3e-vcp-leds-buttons)
- [03. NUCLEO-H755ZI-Q — Board connectors: ST Zio (CN7/8/9/10) and ST morpho (CN11/12) pin maps](#03-nucleo-h755zi-q--board-connectors-st-zio-cn78910-and-st-morpho-cn1112-pin-maps)
- [04. NUCLEO-H755ZI-Q — Board: clocks, USB, Ethernet, motor control, solder bridges](#04-nucleo-h755zi-q--board-clocks-usb-ethernet-motor-control-solder-bridges)
- [05. STM32H755ZIT6 — Cores, memory, boot, bus matrix, DMA, interrupts](#05-stm32h755zit6--cores-memory-boot-bus-matrix-dma-interrupts)
- [06. STM32H755ZIT6 — Power, clocks, resets, low-power modes, key electrical limits](#06-stm32h755zit6--power-clocks-resets-low-power-modes-key-electrical-limits)
- [07a. STM32H755ZIT6 (LQFP144) - Pin list: package pin -> name, I/O type, analog/special functions](#07a-stm32h755zit6-lqfp144---pin-list-package-pin---name-io-type-analogspecial-functions)
- [07b. STM32H755ZIT6 (LQFP144) - Alternate function matrix (AF0-AF15)](#07b-stm32h755zit6-lqfp144---alternate-function-matrix-af0-af15)
- [08. STM32H755ZIT6 — Digital communication peripherals and timers](#08-stm32h755zit6--digital-communication-peripherals-and-timers)
- [09. STM32H755ZIT6 — Analog peripherals, crypto/security blocks](#09-stm32h755zit6--analog-peripherals-cryptosecurity-blocks)
- [10. NUCLEO-H755ZI-Q firmware handoff — default hardware state, pin usage, pitfalls](#10-nucleo-h755zi-q-firmware-handoff--default-hardware-state-pin-usage-pitfalls)
- [99. SUPPLEMENTARY — NOT from the supplied PDFs (unverified background knowledge)](#99-supplementary--not-from-the-supplied-pdfs-unverified-background-knowledge)

---

## 01. NUCLEO-H755ZI-Q — Board: power, jumpers, SMPS/LDO config

Scope: the dev board only (ST board ref **MB1363**, variant H755ZIQ, revisions C01 / D01; sold as order code NUCLEO-H755ZI-Q, product ids NUH755ZIQ$AT1/$AT2). Source: UM2408 Rev 6. The target MCU is **STM32H755ZIT6** (dual Cortex-M7 + M4, LQFP144, 2 MB flash, internal SMPS, crypto). Board has Ethernet (RJ45), Micro-AB USB OTG, on-board **STLINK-V3E** debugger, ST Zio (Arduino Uno V3 superset) + ST morpho headers. NUCLEO-H745ZI-Q/H7A3ZI-Q share the PCB but differ (H7A3 has no Ethernet) — ignore their rows.

Conventions: "Jumper JPx ON" = fitted; "SBx ON" = solder bridge closed (or 0 ohm); OFF = open. Default states are shown in **bold** in the original PDF but bold was lost in text extraction; defaults below are those stated in prose.

#### Board hardware block summary (from block diagram)
Two parts: **ST-LINK part** (STM32F723IEK6 "STLINK-V3E", Micro-B USB CN1) and **MCU part** (STM32H755, Zio CN7/8/9/10, morpho CN11/12, 3 LEDs, B1 user, B2 reset, Micro-AB USB CN13, RJ45 CN14). Links between them: SWD, VCP UART (USART3 PD8/PD9), MCO clock (8 MHz), NRST. Ethernet goes MCU --RMII--> LAN8742A PHY (U15) -> RJ45.

#### Power input options (choose with JP2)
JP2 is a 2x4 header selecting the 5 V source feeding the board. Pin pairs:

| JP2 jumper | Source | Connector/pin | Voltage | Max current | Notes |
|---|---|---|---|---|---|
| pins 1-2 (STLK) **default** | ST-LINK USB CN1 (U5V) from PC | CN1 | 5 V | 500 mA (negotiated) | Board powers only after USB enumeration succeeds |
| pins 3-4 (VIN) | External 7-11 V | CN8 pin 15 / CN11 pin 24 | 7-11 V only | 800 mA @7 V; 450 mA for 7<VIN<9 V; 250 mA for 9<VIN<11 V | Goes through on-board 5 V regulator (U12) |
| pins 5-6 (EXT) | External 5 V | CN11 pin 6 (5V_EXT) | 4.75-5.25 V | 500 mA | |
| pins 7-8 (CHGR) | USB charger on CN1 | CN1 | 5 V | not specified | **ST-LINK NOT powered -> no debug/programming** |
| (no JP2; direct 3.3 V) | External 3.3 V | CN8 pin 7 / CN11 pin 16 | 3.0-3.6 V | not specified | **ST-LINK NOT powered -> no debug** |

External supplies must be SELV, limited power, EN 62368-1 compliant.

##### USB enumeration behavior (PC power, JP2 1-2)
- Before enumeration the PC gives only 100 mA, so only the ST-LINK circuit is powered.
- During enumeration the board requests 500 mA (power switch U2 gated by PWR_ENn from the ST-LINK MCU). On success (SetConfiguration) U2 switches ON, **green LD5 (PWR) lights** and the MCU part + shield can draw up to 500 mA. If the host cannot supply it, enumeration fails, U2 stays OFF, LD5 stays OFF -> use an external supply.
- U2 current-limits; overcurrent >500 mA lights **red LD6**.
- Manual inconsistency: section 7.4.6 says 300 mA is requested at enumeration; 7.4.1 says 500 mA. Design for 500 mA budget but expect possible rejection.

##### Debugging while powered from VIN or EXT — mandatory power sequence
1. Fit JP2 5-6 (EXT) or 3-4 (VIN). 2. Connect external source. 3. Power it on (7<VIN<11 V, or 5 V). 4. Confirm LD5 green ON. 5. THEN plug PC USB into CN1.
Wrong order risks USB (U5V) powering the board first: >300 mA draw can damage/limit the PC port, or enumeration is rejected and the board stays unpowered.

##### Power outputs
When powered by USB, VIN or EXT: 5 V available on CN8 pin 9 / CN11 pin 18 for shields. 3.3 V on CN8 pin 7 / CN11 pin 16 is usable as output, limited by regulator U11 (**1300 mA max**).

#### Other power-related jumpers
- **JP4 (IDD)**: ON (default) = MCU powered. Remove jumper and insert an ammeter to measure MCU current (no ammeter -> MCU unpowered). VDD also feeds level shifters U1/U10 (debug/VCP translation): for very-low-power measurement set pull-up/pull-down in software on PA13, PB3, PD8. On **Rev C01** boards VDD_MCU also feeds the Ethernet PHY U15, debug level shifters and pull-ups, so Standby IDD can't be measured unless you remove R4,R43-R53,R59,R64,U1,U10,U15,SB72 (kills Ethernet and 1.8 V debug). **Rev D01** fixed this (IDD jumper wiring, added SB13 to bypass U1) — "no limitation".
- **JP5 (VDD_MCU select)**: pins 1-2 = VDD_MCU 3.3 V (default); pins 2-3 = 1.8 V. If 1.8 V used, ST-LINK level shifters handle debug. Check JP5 before first power-up.
- **JP1 (STLK_RST)**: ON = ST-LINK held in reset (all its GPIOs high-Z, lets an external debugger on CN5 take over; also saves power). OFF (default) = no effect.
- **JP3 (T_NRST)**: ON (default) = board NRST connected to ST-LINK reset control; OFF = disconnected.
- JP6/JP7 (Ethernet RMII TXD1 / MDIO): must be ON when using Ethernet (see §04). JP8 = USB power-enable routing (ON for H755).
- **VDDA/VREF+ (R40)**: R40 ON (default) ties VDDA and VREF+ to VDD. Remove R40 to feed external VREF+ via Zio CN7 pin 6 (AREF).
- **IOREF** selected by SB24/SB13/SB14: (OFF,OFF,ON)=VDD_MCU; (OFF,ON,OFF)=3V3_PER (Rev C only); (ON,OFF,OFF)=3V3.
- VBAT: SB83 ON connects VBAT to VDD_MCU.
- SB1 (3V3_PER, Rev C only) and SB2 (3.3 V regulator ST1L05CPU33R output) — ON connects them to the 3.3 V rail.

#### Internal SMPS / LDO supply configuration  (CRITICAL — must match firmware)
The STM32H755 core supply (VCORE) can come from 4 configurations; hardware is set with solder bridges/resistors (X = fitted/ON, - = not fitted):

| | Config 1: LDO only | **Config 2: SMPS direct (DEFAULT)** | Config 3: SMPS + LDO cascaded | Config 4: Bypass (ext VCORE) |
|---|---|---|---|---|
| SMPS | OFF | ON | ON | OFF |
| LDO | ON | OFF | ON | OFF |
| SB25 | X | - | - | - |
| R33 | X | - | - | X |
| SB74 | X | - | - | X |
| C58 / C54 | 2.2 uF | 100 nF | 2.2 uF | 100 nF |
| SB92 | - | X | X | - |
| SB79 | - | X | X | - |
| SB75 | - | X | - | X |
| SB76 | - | X | X | X |
| R35 | - | X | - | - |
| R34 | - | - | X | - |
| SB97 | - | - | - | X |
| SB95 | - | - | - | X |

**Datasheet implication:** with the SMPS directly supplying VCORE (the board default), **VOS0 (480 MHz M7 / 240 MHz M4) is NOT available**; max is VOS1 = **400 MHz M7 / 200 MHz M4**. VOS0 requires the LDO as VCORE source (Config 1, hardware rework).

**Deadlock warning:** the firmware's supply setting must match the hardware. A mismatch makes the MCU unreachable (ST-LINK cannot connect after reset). In `main.c` `SystemClock_Config()`:
- Default (Config 2, direct SMPS): `HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);`
- LDO hardware (Config 1): `HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);`

**Recovery from mismatch deadlock:** (1) power off; (2) wire CN11 BOOT0 pin (CN11 pin 7) to VDD (CN11 pin 5 = 3V3_VDD; the 1-2 header notes BOOT0 high via jumper between CN11 pins 5 and 7); (3) power on -> boots system-memory bootloader instead of user flash; (4) connect with STM32CubeProgrammer; (5) erase user flash; (6) power off, remove wire; board recovers.

#### External SMPS option (CN6 footprint, unpopulated)
Optional external module (e.g. ST1PS02D1QTR) on CN6: pin1 3V3_VDD; pin2 SMPS_V1 (PG8, D1 voltage select; needs SB34 ON/SB31 OFF); pin3 VOUTVDD (ext 3.3 V for MCU); pin4 SMPS_EN (PG9; SB35 ON/SB41 OFF); pin5 1V2_VOUTCORE (ext 1.2 V VCORE into VCAP); pin6 SMPS_SW (PG10; SB33 ON/SB30 OFF); pin7 GND; pin8 SMPS_PG (PG12; SB42 ON/SB45 OFF). These GPIOs (PG8,PG9,PG10,PG12) are also Zio D50, D8, D49, D7.

#### Safety / handling (brief)
Board is ESD-sensitive; don't touch connections while powered; sharp header pins; not for children; **USB CN13 cannot power the board** — power the board BEFORE connecting a cable to CN13 (otherwise current injection into STM32 I/Os).

---

## 02. NUCLEO-H755ZI-Q — Board: debug/programming (STLINK-V3E), VCP, LEDs, buttons

Source: UM2408 Rev 6 (sections 6-7.7, 7.10) + DB3171. Target MCU: STM32H755ZIT6.

#### On-board STLINK-V3E
- Integrated debugger/programmer (MCU STM32F723IEK6, labelled "STLINK-V3E"); no external probe required. USB 2.0 high-speed-compatible, Micro-B connector **CN1**.
- Interfaces: SWD, JTAG, SWV (serial wire viewer), Virtual COM port (VCP), mass storage (drag-drop flash), debug port. USB re-enumeration capability. Appears as a USB composite device (VID 0x0483; PID 0x374E = V3 without bridge functions, 0x374F = with bridge functions).
- JTAG/SWD target voltage 3 V to 3.6 V on the interface with 5 V-tolerant inputs.
- Windows 7/8 need the ST-LINK driver; Windows 10, Linux, macOS do not. If board is connected before driver install, manually re-associate "Unknown" USB devices in Device Manager.
- Keep ST-LINK firmware updated (in-place upgrade over USB, tool from st.com).
- Programs both cores' flash through the single SWD/JTAG port; the datasheet debug infrastructure allows debugging **one core at a time or both in parallel** (CoreSight; 4 KB embedded trace FIFO).

##### Debug-related MCU pins
| Signal | MCU pin | Notes |
|---|---|---|
| SWDIO / JTMS | PA13 | Shared with ST-LINK; avoid using as GPIO |
| SWCLK / JTCK | PA14 | Shared with ST-LINK; avoid using as GPIO |
| SWO / TRACESWO / JTDO | PB3 | Connected to ST-LINK SWO and CN5 pin 8 (SB49/SB50 selects, see below). Also Zio D23 (CN7 pin 15) |
| NRST | NRST | B2 reset button; JP3 links to ST-LINK |
| VCP (USART3) | PD8 = TX, PD9 = RX | default routed to ST-LINK (SB16, SB17 ON) |
| MCO clock in | PH0/OSC_IN | 8 MHz from ST-LINK by default (SB72 ON) |

SWO routing (SB49, SB50): ON,ON = PB3 to ST-LINK SWO **and** Zio CN7 pin 15; OFF,ON = ST-LINK/CN5 only; ON,OFF = PB3 to Zio pin 15 only (I2S_B_CK/SPI_B_SCK usable); OFF,OFF = PB3 unconnected.

#### Programming/debugging options
1. **Embedded ST-LINK** (normal): jumper JP2 correct (see §01), connect PC USB Type-A -> Micro-B to **CN1**.
2. **External debug tool on CN5 (MIPI-10 / STDC14 footprint)**, two ways:
   - Leave ST-LINK running: power it first until COM LED LD4 lights red, then connect the external tool.
   - Hold ST-LINK in reset: fit **JP1 (STLK_RST)**; all ST-LINK GPIOs go high-Z; then use external tool.

##### CN5 pinout (MIPI-10 pin / STDC14 pin)
| MIPI10 | STDC14 | Signal | Meaning |
|---|---|---|---|
| - | 1,2 | NC | reserved |
| 1 | 3 | T_VCC | target VCC |
| 2 | 4 | T_SWDIO (T_JTMS) | SWDIO or JTAG TMS |
| 3 | 5 | GND | |
| 4 | 6 | T_SWCLK (T_JCLK) | SWCLK or JTAG TCK |
| 5 | 7 | GND | |
| 6 | 8 | T_SWO (T_JTDO) | SWO or JTAG TDO |
| 7 | 9 | T_JRCLK | JTAG RTCK, specific use only |
| 8 | 10 | T_JTDI | JTAG TDI, external tools only |
| 9 | 11 | GNDDetect | plug detect |
| 10 | 12 | T_NRST | target reset |
| - | 13 | T_VCP_RX | VCP RX (must be the UART dedicated to the bootloader) |
| - | 14 | T_VCP_TX | VCP TX |

#### Virtual COM port (USART3)
USART3 on PD8 (TX) / PD9 (RX) is wired through the ST-LINK to the PC as a COM port (default). Solder-bridge options:

| Pin | Function | Virtual COM port (DEFAULT) | Arduino D0/D1 | ST morpho |
|---|---|---|---|---|
| PD8 | USART3_TX | SB103 OFF, SB16 ON, SB15 OFF | SB103 OFF, SB16 OFF, SB15 ON | SB103 ON, SB16 OFF, SB15 OFF |
| PD9 | USART3_RX | SB104 OFF, SB17 ON, SB94 OFF | SB104 OFF, SB17 OFF, SB94 ON | SB104 ON, SB17 OFF, SB94 OFF |

Alternate VCP wiring group (SB16,SB17,SB12,SB64): (ON,ON,OFF,OFF) = PD8/PD9; (OFF,OFF,ON,ON) = PB6/PB7 (then PB6/PB7 unusable on morpho); all OFF = VCP disconnected. Arduino USART group (SB11,SB96,SB15,SB94): (ON,ON,OFF,OFF) = D1/D0 on PB6/PB7 (this is the *default Zio D0/D1 mapping* per the pin table: D1 = PB6 LPUART1_TX, D0 = PB7 LPUART1_RX); (OFF,OFF,ON,ON) = D1/D0 on PD8/PD9; all OFF = disconnected. SB13 (not fitted by default on Rev D01) bypasses U1 level shifter between T_VCP_TX and STLK_VCP_RX.
(The Zio table lists D0/D1 as **LPUART1** on PB7/PB6; the VCP is **USART3** on PD9/PD8.)

#### LEDs
| LED | Colour | Connected to | Notes |
|---|---|---|---|
| LD1 | green | **PB0** (SB54 ON, SB65 OFF — default) or PA5/Zio D13 (SB65 ON, SB54 OFF) | SB54 ON+SB65 ON forbidden. Active HIGH (LED on when pin high). |
| LD2 | yellow | **PE1** (R74 ON) | active high |
| LD3 | red | **PB14** (R75 ON) | active high; conflicts with Zio D65 (PB14) — remove R75 if D65 used |
| LD4 COM | red/green/orange | ST-LINK status | see below |
| LD5 PWR | green | 5 V power good | MCU part powered & +5 V on CN8 pin 9 / CN11 pin 18 |
| LD6 | red | USB overcurrent (>500 mA) on CN1 | external supply needed |
| LD7 | red | USB FS overcurrent on CN13 (host mode, 5 V enabled) | |
| LD8 | green | USB VBUS present on CN13 / power switch ON in host mode | |

LD4 COM states: slow-blink red or off = power-on before USB init; fast-blink red/off = after first PC<->ST-LINK enumeration; red ON = PC<->ST-LINK init complete; green ON = successful target comm init; blinking red/green = communicating with target; green ON = comm finished OK; orange ON = comm failure.

#### Buttons
- **B1 USER (blue)**: PC13 by default (SB82 ON, SB81 OFF; tamper/wake support). Alternative PA0 (SB81 ON, SB82 OFF; wake-up pin; set SB82 OFF if Zio is used). Both OFF = not connected. PA0 is also Zio D32 via SB89 ON (CN10 pin 29).
- **B2 RESET (black)**: tied to NRST; resets the MCU.
- Pre-programmed demo: after power-up LD5 green + LD4 light and LD3 red blinks; pressing B1 changes the blink frequency of LD1-LD3.

---

## 03. NUCLEO-H755ZI-Q — Board connectors: ST Zio (CN7/8/9/10) and ST morpho (CN11/12) pin maps

Source: UM2408 Rev 6 Tables 17 and 19 (H755 column only; H7A3 differences omitted). MCU: STM32H755ZIT6 LQFP144. All I/O are **3.3 V logic** (Arduino shields assume 5 V). Zio = Arduino Uno V3 subset (A0-A5, D0-D15) + extension (A6-A8, D16-D72).

Figure info (extension-connector diagram): CN7 and CN10 are the right-hand 2x(10/17) and 2x17 Zio headers; CN8 and CN9 are the left-hand ones. Odd pins form one row, even pins the other. Zio sockets are female on top, male underneath. CN11/CN12 morpho headers (2x35, outer edges) are **unpopulated by default** (OFF) — solder pin headers to use them; all MCU signals/power are on them. Motor-control shields need CN11 and CN12 fitted (Samtec TSW-135-07-x-D).

#### Arduino compatibility settings
For Arduino Uno V3 shields: **SB39 and SB62 ON, SB40 and SB69 OFF** so A4/A5 are I2C (PB9/PB8) — otherwise A4/A5 are analog (PC2_C / PF11). (Prose in Table 16 lists the ADC choice as the ON,ON,OFF,OFF row; the manual's default is not recoverable from the text — check the board.) Remove R40 before using Arduino shield VREF+ on CN7 pin 6 (AREF).

#### CN8 (left, power + SDMMC) — odd pins power, even pins D43-D50
| Pin | Signal | MCU pin / function |
|---|---|---|
| 1 | NC | |
| 3 | IOREF | 3.3 V ref |
| 5 | NRST | reset |
| 7 | 3V3 | 3.3 V in/out |
| 9 | 5V | 5 V output |
| 11, 13 | GND | |
| 15 | VIN | 7-11 V power input |
| 2 | D43 | PC8 SDMMC_D0 |
| 4 | D44 | PC9 SDMMC_D1 / SAI2_EXTCLK |
| 6 | D45 | PC10 SDMMC_D2 |
| 8 | D46 | PC11 SDMMC_D3 |
| 10 | D47 | PC12 SDMMC_CK |
| 12 | D48 | PD2 SDMMC_CMD |
| 14 | D49 | PG10 I/O (shared with SMPS_SW) |
| 16 | D50 | PG8 I/O (shared with SMPS_V1) |

#### CN9 (left, analog / I2C_B / CAN / USART_B / SAI)
| Pin | Signal | MCU pin / function | Pin | Signal | MCU pin / function |
|---|---|---|---|---|---|
| 1 | A0 | PA3 ADC12_INP15 | 2 | D51 | PD7 USART_B_SCLK (USART2) |
| 3 | A1 | PC0 ADC123_INP10 | 4 | D52 | PD6 USART_B_RX |
| 5 | A2 | PC3_C ADC3_INP1 | 6 | D53 | PD5 USART_B_TX |
| 7 | A3 | PB1 ADC12_INP5 | 8 | D54 | PD4 USART_B_RTS |
| 9 | A4 | PC2_C ADC3_INP0 *or* PB9 I2C1_SDA | 10 | D55 | PD3 USART_B_CTS |
| 11 | A5 | PF11 ADC1_INP2 *or* PB8 I2C1_SCL | 12 | GND | |
| 13 | D72 | PB2 COMP1_INP (NC by default) | 14 | D56 | PE2 SAI_A_MCLK (SAI1_A) |
| 15 | D71 | PE9 COMP2_INP | 16 | D57 | PE4 SAI_A_FS |
| 17 | D70 | PB5 I2C_B_SMBA (I2C4_SMBA AF6; NC by default) | 18 | D58 | PE5 SAI_A_SCK |
| 19 | D69 | PF14 I2C_B_SCL (I2C4_SCL, AF4) | 20 | D59 | PE6 SAI_A_SD |
| 21 | D68 | PF15 I2C_B_SDA (I2C4_SDA, AF4) | 22 | D60 | PE3 SAI_B_SD (SAI1_B) |
| 23 | GND | | 24 | D61 | PF8 SAI_B_SCK |
| 25 | D67 | PD0 CAN_RX (FDCAN1) | 26 | D62 | PF7 SAI_B_MCLK |
| 27 | D66 | PD1 CAN_TX (FDCAN1) | 28 | D63 | PF9 SAI_B_FS |
| 29 | D65 | PB14 I/O (also LD3 red) | 30 | D64 | PD10 I/O (NC by default; = USB_FS_PWR_EN, JP8) |
(Note: the board manual labels the D68-D70 group "I2C_2", but the datasheet AF table shows PF14/PF15/PB5 only offer **I2C4** (SCL/SDA/SMBA) — use I2C4. Arduino D14/D15 = PB9/PB8 = I2C1.)

#### CN7 (right, Arduino SPI/I2C/D8-D15 + I2S/SPI_B)
| Pin | Signal | MCU pin / function | Pin | Signal | MCU pin / function |
|---|---|---|---|---|---|
| 1 | D16 | PC6 I2S_A_MCK (I2S2) | 2 | D15 | PB8 I2C1_SCL (Arduino) |
| 3 | D17 | PB15 I2S_A_SD | 4 | D14 | PB9 I2C1_SDA (Arduino) |
| 5 | D18 | PB13 I2S_A_CK (NC by default; = RMII_TXD1, see SB8/JP6) | 6 | AREF | AVDD/VREF+ |
| 7 | D19 | PB12 I2S_A_WS | 8 | GND | |
| 9 | D20 | PA15 I2S_B_WS (I2S3) | 10 | D13 | PA5 SPI1_SCK (Arduino; also LD1 if SB65 ON) |
| 11 | D21 | PC7 I2S_B_MCK | 12 | D12 | PA6 SPI1_MISO |
| 13 | D22 | PB5 I2S_B_SD / SPI3_MOSI | 14 | D11 | PB5 SPI1_MOSI / TIM3_CH2 (SB56 OFF, SB57 ON for H755) |
| 15 | D23 | PB3 I2S_B_CK / SPI3_SCK (= SWO) | 16 | D10 | PD14 SPI1_CS / TIM4_CH3 |
| 17 | D24 | PA4 SPI3_NSS | 18 | D9 | PD15 TIM4_CH4 |
| 19 | D25 | PB4 SPI3_MISO | 20 | D8 | PG9 I/O (= SMPS_EN) |

#### CN10 (right, analog ref, QSPI, timers, USART_A, TIM1)
| Pin | Signal | MCU pin / function | Pin | Signal | MCU pin / function |
|---|---|---|---|---|---|
| 1 | VDDA | analog VDD | 2 | D7 | PG12 I/O (= SMPS_PG) |
| 3 | AGND | | 4 | D6 | PA8 TIM1_CH1 (= USB_SOF / MCO1) |
| 5 | GND | | 6 | D5 | PE11 TIM1_CH2 |
| 7 | A6 | PF6 ADC3_INP8 | 8 | D4 | PE14 I/O |
| 9 | A7 | PF10 ADC3_INP6 | 10 | D3 | PE13 TIM1_CH3 |
| 11 | A8 | PA2 ADC12_INP14 (NC by default; = RMII_MDIO, JP7) | 12 | D2 | PG14 I/O |
| 13 | D26 | PG6 QSPI_CS (BK1) | 14 | D1 | PB6 LPUART1_TX (Arduino USART_A_TX) |
| 15 | D27 | PB2 QSPI_CLK | 16 | D0 | PB7 LPUART1_RX (Arduino USART_A_RX) |
| 17 | GND | | 18 | D42 | PE8 TIM1_CH1N |
| 19 | D28 | PD13 QSPI_BK1_IO3 | 20 | D41 | PE7 TIM1_ETR |
| 21 | D29 | PD12 QSPI_BK1_IO1 | 22 | GND | |
| 23 | D30 | PD11 QSPI_BK1_IO0 | 24 | D40 | PE10 TIM1_CH2N |
| 25 | D31 | PE2 QSPI_BK1_IO2 | 26 | D39 | PE12 TIM1_CH3N |
| 27 | GND | | 28 | D38 | PE6 I/O |
| 29 | D32 | PA0 TIM2_CH1 (needs SB89 ON; also B1 alt) | 30 | D37 | PE15 TIM1_BKIN1 |
| 31 | D33 | PB0 TIM3_CH3 (= LD1 green when SB54 ON) | 32 | D36 | PB10 TIM2_CH3 |
| 33 | D34 | PE0 TIM4_ETR | 34 | D35 | PB11 TIM2_CH4 |

##### MCU pins that appear on TWO Zio signals (double-connected — don't drive both)
PB2 = D27 / D72; PB5 = D11 / D22 / D70; PE2 = D31 / D56; PE6 = D59 / D38; PB3 = D23 / SWO. Also board-function conflicts: PB14 = D65 / LD3 (remove R75); PA5 = D13 / LD1 (SB65); PG10/PG8/PG9/PG12 = D49/D50/D8/D7 / external-SMPS control; PD10 = D64 / USB power switch; PB13 = D18 / RMII_TXD1; PA2 = A8 / RMII_MDIO; PA8 = D6 / USB_SOF/MCO1.

#### ST morpho CN11 / CN12 pin assignment (H755)
Pins are paired: odd pin / even pin per row. **CN11** (left), **CN12** (right).
| CN11 pin | odd | even | | CN12 pin | odd | even |
|---|---|---|---|---|---|---|
| 1/2 | PC10 | PC11 | | 1/2 | PC9 | PC8 |
| 3/4 | PC12 | PD2 | | 3/4 | PB8 | PC6 |
| 5/6 | 3V3_VDD | 5V_EXT | | 5/6 | PB9 | PC5 |
| 7/8 | BOOT0 | GND | | 7/8 | VREFP | 5V_USB_STLK |
| 9/10 | PF6 | NC | | 9/10 | GND | PD8 |
| 11/12 | PF7 | IOREF | | 11/12 | PA5 | PA12 or NC |
| 13/14 | PA13 (SWDIO) | NRST | | 13/14 | PA6 | PA11 or PE6 |
| 15/16 | PA14 (SWCLK) | 3V3 | | 15/16 | PA7 | PB12 |
| 17/18 | PA15 | 5V | | 17/18 | PB6 | PB11 |
| 19/20 | GND | GND | | 19/20 | PC7 | GND |
| 21/22 | PB7 | GND | | 21/22 | PA9 | PB2 |
| 23/24 | PC13 | VIN | | 23/24 | PA8 | PB1 |
| 25/26 | PC14 | NC | | 25/26 | PB10 | PB15 |
| 27/28 | PC15 | PA0 | | 27/28 | PB4 | PB14 or PE15 |
| 29/30 | PH0 | PA1 | | 29/30 | PB5 or PA4 | PB13 or PE8 |
| 31/32 | PH1 | PA4 | | 31/32 | PB3 | AGND |
| 33/34 | VBAT | PB0 | | 33/34 | PA10 | PC4 |
| 35/36 | PC2 | PC1 | | 35/36 | PA2 | NC |
| 37/38 | PC3 | PC0 | | 37/38 | PA3 | NC |
| 39/40 | PD4 | PD3 | | 39/40 | GND | PE8 |
| 41/42 | PD5 | NC | | 41/42 | PD13 | PF10 |
| 43/44 | PD6 | NC | | 43/44 | PD12 | PE7 |
| 45/46 | PD7 | PE2 | | 45/46 | PD11 | PD14 |
| 47/48 | PE3 | PE4 | | 47/48 | PE10 | PD15 |
| 49/50 | GND | PE5 | | 49/50 | PE12 | PF14 |
| 51/52 | NC | NC | | 51/52 | PE14 | PE9 |
| 53/54 | NC | PF8 | | 53/54 | PE15 | GND |
| 55/56 | PD1 | PF9 | | 55/56 | PE13 | PE11 |
| 57/58 | PD0 | NC | | 57/58 | NC | NC |
| 59/60 | NC | GND | | 59/60 | NC | PF15 |
| 61/62 | PE1 | PE6 | | 61/62 | PG14 | PF11 |
| 63/64 | PG9 | NC | | 63/64 | GND | PE0 |
| 65/66 | PG12 | PG10 | | 65/66 | PD10 | PG8 |
| 67/68 | NC | PG13 | | 67/68 | PG7 | NC |
| 69/70 | PD9 | PG11 | | 69/70 | NC | PG6 |

Notes: BOOT0 default LOW; set HIGH with a jumper between CN11 pins 5 and 7. 5V_USB_STLK (CN12 pin 8) is the raw ST-LINK USB 5 V that rises before the board 5 V. PA13/PA14 are shared with SWD — avoid as GPIO. "Or" pins are chosen by solder bridges (motor-control table: PE6/BKIN2 SB10 ON/SB22 OFF; PE15/BKIN1 SB38 ON/SB37 OFF; PA4/DAC SB77 ON/SB59 OFF; PE8/TIM1_CH1N SB47 ON/SB43 OFF; PA1 and PC1 RMII pins need SB80/SB84 OFF to be free). Clock/oscillator morpho pins: CN11 pin 29 = PH0 (external clock in), pin 25 = PC14 (external LSE clock in).

---

## 04. NUCLEO-H755ZI-Q — Board: clocks, USB, Ethernet, motor control, solder bridges

Source: UM2408 Rev 6 (7.9-7.14), H755 rows only. MCU STM32H755ZIT6. Default SB/JP state = stated in prose; bold defaults were lost in extraction, so "default" is marked only where the text says so.

#### Clocks
**HSE (main, 4-48 MHz capable) — four options:**
1. **MCO from ST-LINK (default):** 8 MHz into PH0/OSC_IN. SB72 ON, SB71 OFF, SB3 & SB4 OFF. (Frequency may change with ST-LINK firmware — RN0093.) In firmware use HSE **bypass** mode at the 8 MHz.
2. On-board crystal X2: 25 MHz, 6 pF, 20 ppm (NDK NX2016SA-25MHz). SB71 & SB90 OFF, SB3 & SB4 ON, C74 & C76 fitted 5.1 pF, SB72 OFF.
3. External oscillator on CN11 pin 29 (PH0): SB71 ON, SB72 OFF, SB3/SB4 removed.
4. HSE unused (PH0/PH1 = GPIO): SB71 & SB90 ON, SB72 OFF, SB3/SB4 removed.
**LSE (32.768 kHz):** on-board crystal X3 (default; NDK NX3215SA, 6 pF, 20 ppm). Or external clock on CN11 pin 25 (PC14): SB66 & SB67 ON, R41 & R42 removed. Or unused (PC14/PC15 GPIO): same SB66/SB67 ON, R41/R42 removed.
Crystal-less USB: MCU can use HSI48 for USB FS (datasheet).

#### USB (user USB, CN13 Micro-AB, OTG full speed)
- Uses **PA11 (DM), PA12 (DP)** via the MCU's internal FS PHY (datasheet names: `OTG_FS_DM/DP`, AF10). PA9 = VBUS sense, PA10 = ID (ID not used on board — ESD part USBLC6-2SC6), PA8 = SOF test point TP4. PA11/PA12 also offer FDCAN1_RX/TX (AF9) if not used for USB.
- VBUS power switch U18 enable = **PD10** (JP8 ON on H745/H755 to connect it), overcurrent flag = **PG7** (SB100 ON).
- LD8 green: VBUS present / host power ON; LD7 red: overcurrent in host mode.
- **CN13 cannot power the board — power board first, then plug USB** (current injection risk). Use external power when using USB host/OTG.
- USB pin solder bridges (USB-connector config | morpho config): PA9 SB21 ON | OFF; PA10 SB29 ON | OFF; PA11 SB26 ON | OFF; PA12 SB27 ON | OFF; PD10 power-enable: JP8 ON (H745/H755) | OFF; PF10 power-enable: SB91 OFF in both H755 configs; PG7 overcurrent SB100 ON | OFF. SB26/SB27 OFF frees PA11/PA12 as GPIO on morpho.
- The board only exposes the full-speed USB on PA11/PA12; no external ULPI (high-speed) PHY is described in the manual.

#### Ethernet (RJ45 CN14, 10/100 Mbit)
- PHY **LAN8742A-CZ-TR (U15)**, RMII interface to MCU; PHY supplies the 50 MHz RMII_REF_CLK to the MCU.
- **JP6 and JP7 must be ON** when using Ethernet.
- RMII pin map (all must stay assigned to Ethernet when used; solder bridges default ON for H755): PA1 RMII_REF_CLK (SB80); PA2 RMII_MDIO (JP7; conflicts Zio A8); PC1 RMII_MDC (SB84); PA7 RMII_CRS_DV (SB52); PC4 RMII_RXD0 (SB63); PC5 RMII_RXD1 (SB48); PG11 RMII_TX_EN (SB36); PG13 RMII_TXD0 (SB46); PB13 RMII_TXD1 (JP6 ON & SB8 OFF for Ethernet; JP6 OFF & SB8 ON to give PB13 to Zio D18). To free these pins for Zio/morpho use set their SB OFF.
- PHY reset: SB98 ON connects MCU NRST to PHY. HAL peripheral: ETH with `ETH_RMII_*` AF11 (PA1/PA2/PC1/PA7/PC4/PC5/PG11/PG13/PB13 all AF11; see 07b).
- Rev C01 only: to reach low-power IDD the PHY must be in power-down (LAN8742A basic control register 0x00, bit 11 = 1) or remove SB13.

#### Motor control shield compatibility
X-NUCLEO-IHM09M1, IHM07M1 (L6230), IHM08M1 (STL220N6F7): fit CN11 and CN12 morpho headers and set bridges: PE6 morpho CN12 pin 14 (BKIN2) SB10 ON/SB22 OFF; PE15 morpho CN12 pin 28 (BKIN1) SB38 ON/SB37 OFF; PA4 morpho CN12 pin 29 (DAC) SB77 ON/SB59 OFF; PE8 morpho CN12 pin 30 (TIM1_CH1N) SB47 ON/SB43 OFF; PA1 (RMII ref clk) SB80 OFF; PC1 (RMII MDC) SB84 OFF (Ethernet is then unusable).

#### Other solder-bridge table (selected, non-obvious)
| Bridge(s) | State -> effect |
|---|---|
| SB103/SB104 | ON: PD8/PD9 to morpho CN11/CN12 (then SB16,SB17,SB15,SB94 must be OFF); OFF: disconnected |
| SB20 (SDMMC_D0), SB9 (SDMMC_D1) | ON: PC8/PC9 to morpho CN12; OFF: disconnected to avoid SDMMC stub |
| SB54/SB65 | LD1: (ON,OFF)=PB0, (OFF,ON)=PA5/D13, (OFF,OFF)=none, (ON,ON) forbidden |
| R74 / R75 | ON = LD2(PE1) / LD3(PB14) connected |
| SB56/SB57 (D11) | H755: (OFF,ON): D11 = PB5 (SPI1_MOSI/TIM3_CH2); H7A3 differs |
| SB73/SB70 (A6) | H755: (ON,OFF): A6 = PF6 |
| SB93/SB58 (A7) | H755: (ON,OFF): A7 = PF10 |
| SB78 (A8) | OFF for H745/H755 (PA2 free of A8) — A8 is "NC by default" because PA2 is RMII_MDIO |
| SB69,SB40 (ADC A4/A5) / SB62,SB39 (I2C A4/A5) | (ON,ON,OFF,OFF): A4/A5 = PC2_C/PF11 analog; (OFF,OFF,ON,ON): A4/A5 = PB9/PB8 I2C |
| SB81/SB82 (B1) | see §02 |
| SB89 | PA0 to Zio CN10 pin 29 (ON/OFF) |
| SB83 | VBAT to VDD_MCU |
| SB98 | MCU NRST to Ethernet PHY |
| RMII signals: SB80,SB84,JP7,SB63,SB48,SB46,SB36 | ON = Ethernet use, pins must not be used on Zio/morpho; OFF = free GPIO. "No default for RMII signals" in manual footnote — H745/H755 boards ship with Ethernet ON |

---

## 05. STM32H755ZIT6 — Cores, memory, boot, bus matrix, DMA, interrupts

Source: DS12919 Rev 3 (STM32H755xI datasheet), sections 3.1-3.16, 3.44. Part: STM32H755ZIT6 = dual-core, **144-pin LQFP (20x20)**, **2 MB flash**, temp range suffix 6 (-40..+85 C ambient, Tj max 125 C), has crypto/hash. (Ordering code letters: Z=144 pins, I=2 MB, T=LQFP, 6=-40..85 C, option A = advanced mission profile VOS1/480 MHz, TR=tape&reel.) 96-bit unique ID. Datasheet does not list register-level info — see RM0399 for registers.

#### Cores
| | Cortex-M7 (CPU1, "CM7") | Cortex-M4 (CPU2, "CM4") |
|---|---|---|
| Max clock | **480 MHz** (VOS0, LDO only) / 400 (VOS1) / 300 (VOS2) / 200 (VOS3) | **240 MHz** (VOS0) / 200 (VOS1) / 150 (VOS2) / 100 (VOS3) |
| FPU | single + **double precision** | single precision |
| Perf | 1027 DMIPS (2.14 DMIPS/MHz), DSP instr. | 300 DMIPS (1.25 DMIPS/MHz), DSP instr. |
| Pipeline/caches | 6-stage dual-issue, branch prediction, Harvard, **16 KB I-cache + 16 KB D-cache**, 64-bit AXI, 64-bit ITCM, 2x32-bit DTCM | 3-stage, branch pred., Harvard, 32-bit I/D/S buses; **ART accelerator** (64 x 256-bit cache lines + 256-bit buffer) for accesses to D1-domain flash/AXI-SRAM/QSPI/FMC |
| MPU | yes, 16 regions (each splittable into 8 subregions, 32 B - 4 GB) | yes, own MPU (16 regions) |
| NVIC | own NVIC1 | own NVIC2; each NVIC: 16 priority levels, up to 150 maskable IRQ channels + core lines |
| SysTick | 24-bit, one per core | one per core |
| Power domain | **D1** (with most high-perf peripherals) | **D2** (with most comms peripherals and timers) |
Cores are binary compatible. Both run boot code simultaneously by default; a flash user option byte can boot one core and clock-gate the other. Cores can be put in separate low-power states. Inter-core management uses two EXTI interrupt lines and shared peripherals/RAM (semaphores etc. -> RM0399).

#### Memory
| Memory | Size | Domain / bus | Notes |
|---|---|---|---|
| Flash | **2 MB**, 2 independent banks (bank = 1 MB user = 8 x 128 KB sectors, + 128 KB system flash, + 2 KB option bytes) | D1, AXI | 266-bit flash word = 256 data bits (32 B, 8x32-bit) + 10 ECC bits; read-while-write across banks. Address 0x0800_0000-0x081F_FFFF. Prog. 100-290 us/flash word, sector erase 1-2 s, mass erase 5-13 s (depends on x8..x64 parallelism); endurance 10 kcycles; retention 20 yr @10 kcycles/55 C. Program voltage >= 1.62 V (x8/x16/x32) or 1.8 V (x64) |
| ITCM RAM | 64 KB | CM7 only (TCM) | 0 wait state, CM7 or MDMA access; **CM4 cannot access** |
| DTCM RAM | 128 KB (2x64 KB, 2x32-bit ports) | CM7 only | 0 wait state; stack/ISR data; usable in parallel by CM7 dual issue |
| AXI SRAM | 512 KB | D1 | 0x2400_0000-0x2407_FFFF (per bus-matrix note) |
| SRAM1 | 128 KB | D2 | |
| SRAM2 | 128 KB | D2 | |
| SRAM3 | 32 KB | D2 | |
| SRAM4 | 64 KB | D3 | accessible when D1/D2 are off; used by low-power peripherals (BDMA etc.) |
| Backup SRAM | 4 KB | VBAT domain | retained in Standby/VBAT; write-protected |
| Backup registers | 32 x 32-bit (128 B) | VBAT domain | not reset by system/power reset or Standby exit |
Total ~1 MB RAM (192 KB TCM + 864 KB user SRAM + 4 KB backup per feature list). All SRAM has hardware **ECC** (SECDED: 7 bits per 32-bit word, 8 bits per 64-bit word for AXI-SRAM and ITCM); single-error correction, double-error detection; software must handle ECC events.
**Security:** RDP (readout protection), PC-ROP, active tamper, secure access mode (ST root secure services in system memory + user-defined secure services in flash; available only to CM7 in secure access mode; secure code runs right after reset and is then inaccessible), secure firmware upgrade.
External memories: **FMC** (SRAM/PSRAM 4 banks/NOR/NAND with ECC up to 8 KB; SDRAM/LPSDR; 8/16/32-bit bus; FMC_CLK max = kernel clock/2, <=125 MHz sync), **Quad-SPI** (single/dual/quad, SDR & DDR, direct/polling/memory-mapped modes, up to 256 MB mapped incl. code execution, up to 133 MHz). On the Nucleo, QSPI signals are on Zio CN10 (D26-D31); no QSPI flash on board.

#### Boot
Boot memory is chosen by the **BOOT0 pin** and the BOOT_ADDx option bytes (any address 0x0000_0000-0x3FFF_FFFF: flash or SRAMs; CM4 cannot boot from ITCM/DTCM). Bootloader lives in system memory and can reprogram flash over **FDCAN, USART, I2C, SPI, USB-DFU** (see AN2606). BOOT0 pin: input max 9 V. BOOT0 low = user flash (default on board); high = system bootloader.

#### Bus matrix / DMA / interrupts
- Interconnect: 1 x 64-bit **AXI** matrix (D1) + 2 x 32-bit AHB matrices (D2, D3), bridges 5x AHB-APB, 2x AXI-AHB. D1: CM7 (AXIM, AHBP, AHBS), MDMA, DMA2D, LTDC, SDMMC1 masters; slaves AXI SRAM, flash A/B, QSPI, FMC, APB3/AHB3. D2: DMA1, DMA2, Ethernet MAC, SDMMC2, USB OTG masters; SRAM1-3, APB1/2, AHB1/2. D3: BDMA; SRAM4, backup SRAM, AHB4/APB4. D1<->D2<->D3 AHB cross-links.
- **DMA:** MDMA (D1, 16 channels, linked-list; reaches TCM via AHBS), DMA1 & DMA2 (D2, dual-port with FIFO, request router/DMAMUX), BDMA (D3, request router). DMAMUX can chain DMA requests from peripheral triggers.
- **DMA2D (Chrom-ART):** fill / copy / pixel-format conversion / blending, 4bpp-32bpp, CLUT, YCbCr for JPEG.
- **EXTI:** 89 lines (28 configurable + 61 direct) can wake cores/domains from Stop. **CRC unit** with programmable polynomial.
- Datasheet's debug section: SWD + JTAG, ETM-style trace port (TRACECLK/TRACED0-3 pins on PE2-PE6 area), SWO, 4 KB ETF trace FIFO, CoreSight; debug one or both cores.
- Datasheet Section 4 defers all peripheral base addresses to RM0399.

---

## 06. STM32H755ZIT6 — Power, clocks, resets, low-power modes, key electrical limits

Source: DS12919 Rev 3 sections 3.5-3.7, 3.19, 6.2-6.3 and selected tables. (Board-level wiring of these rails: see §01.)

#### Supply pins and ranges
| Supply | Range | Purpose |
|---|---|---|
| VDD | 1.62-3.6 V | I/O supply (Nucleo: 3.3 V default, 1.8 V via JP5) |
| VDDLDO | 1.62-3.6 V (<= VDD) | internal LDO regulator input (1.2 V min only during power-up with SMPS feeding LDO) |
| VDDSMPS | 1.62-3.6 V, **must equal VDD** | SMPS step-down input |
| VLXSMPS / VFBSMPS | | SMPS inductor output / feedback (VCORE, or 1.8 V / 2.5 V when SMPS feeds the LDO or external circuitry) |
| VDDA | 1.62-3.6 V (ADC/COMP >=1.62; DAC >=1.8; VREFBUF >=1.8; OPAMP >=2.0 V) | analog |
| VDD33USB | 3.0-3.6 V when USB used | USB PHY supply; VDD50USB (5 V from VBUS) feeds internal regulator to make it; regulator can be bypassed |
| VBAT | 1.2-3.6 V | RTC, backup regs, backup SRAM when VDD absent; with PDR_ON tied to VSS the VBAT function is lost |
| VCAP | VCORE: 1.0/1.1/1.2/1.35 V by VOS | 2.2 uF (+/-20 %, ESR<100 mOhm) per VCAP pin; two pins |
Power-up/down rule: while VDD < 1 V, VDDA/VDD33USB/VDD50USB must stay < VDD + 300 mV; above 1 V they are independent (VDDSMPS always = VDD). During power-down VDD may temporarily be below other rails only if energy into the MCU < 1 mJ. Supervisors: POR, PDR (PDR_ON pin can disable), PVD, BOR (3 thresholds 2.1-2.7 V via option bytes).

#### VCORE source and voltage scaling (VOS) — max frequencies
| Power scale | VCORE (typ) | VCORE from LDO | VCORE from SMPS direct | Max CM7 / CM4 clock | Max Tj |
|---|---|---|---|---|---|
| VOS0 (boost) | 1.35 V | **yes, 480/240 MHz, Tj<=105 C** | **NOT available** | 480 / 240 MHz | 105 C |
| VOS1 | 1.20 V | yes | yes | 400 / 200 | 125 C |
| VOS2 | 1.10 V | yes | yes | 300 / 150 | 125 C (140 for ext. range) |
| VOS3 | 1.00 V | yes | yes | 200 / 100 | 125 C |
Also: AXI/AHB bus clock max = 240 (VOS0), 200 (VOS1), 150 (VOS2), 100 (VOS3) MHz; **APB max 120 / 100 / 75 / 50 MHz**. Timer kernel clock max 240 MHz (up to 480 MHz selectable via TIMPRE/D2PRE), HRTIM kernel up to 480 MHz. Stop-mode scales SVOS3-SVOS5 (SVOS3: wake-capable UART/SPI/I2C/LPTIM stay active; SVOS4/5 peripherals off, wake via GPIO/async IRQ).
**On the Nucleo-H755ZI-Q the default is SMPS direct => CPU max 400 MHz; 480 MHz needs hardware rework to LDO (+ matching firmware).**
SMPS modes: direct VCORE supply (follows run/stop/standby modes); feeds LDO at 1.8 V or 2.5 V; or external supply output (forced high-performance with SDEXTHP=1 in PWR_CR3). Voltage scaling set by PWR_D3CR VOS bits and SYSCFG_PWRCR ODEN (VOS0).

#### Clocks (RCC, in D3)
- Internal: HSI 64 MHz (system starts on it), HSI48 (USB/RNG crystal-less), CSI 4 MHz, LSI 32 kHz. External: HSE 4-48 MHz crystal (4-50 MHz external clock), LSE 32.768 kHz. **3 PLLs** (PLL1 system, PLL2/PLL3 kernel clocks, fractional mode). USB needs a 48 MHz kernel clock (from PLL or HSI48). MCO outputs (PA8 = MCO1 AF0).
- Peripherals with dual clock domains (bus interface vs kernel) keep baud rate while system clock changes.
- Resets: POR, BOR, NRST pin, IWDG1/2 (per domain), WWDG1/2, software, low-power security reset, Standby exit.

#### Low-power modes
CSleep (CPU clock off; WFI/WFE/SLEEPONEXIT), CStop (CPU subsystem clock off), DStop (domain bus matrix clock off), Stop (system clock off), DStandby (domain powered down), Standby (system off). System/domain mapping: Run = D1/D2 in DRun/DStop/DStandby, D3 DRun; Stop = D1/D2 DStop/DStandby, D3 DStop; Standby = all DStandby. A domain only sleeps when its CPU and all its allocated peripherals are idle; the other CPU can re-init clocks after Stop while the slave CPU stays held. Features list: 2.95 uA Standby (backup SRAM off, RTC/LSE on). VBAT mode: external events/RTC alarm do NOT exit it.
Wakeup times (typ/max): Sleep 9/10 CPU cycles; Stop VOS3 HSI flash-normal 4.4/5.6 us, VOS5 CSI 68/75 us; Standby 390/500 us.
Run current (typ, all periph off, LDO, CoreMark from ITCM on M7 / flash on M4, Tj 25 C): VOS0 480/240 MHz = 179 mA; VOS1 400/200 = 132 mA; VOS2 300/150 = 91 mA; VOS3 200/100 = 56 mA.

#### Absolute maximum ratings (do not exceed)
- Any supply (VDD, VDDLDO, VDDSMPS, VDDA, VDD33USB, VBAT) vs VSS: -0.3 to **4.0 V**.
- Input on **FT_xxx pins** (5 V tolerant): up to min(min(VDD,VDDA,VDD33USB,VBAT)+4.0, 6 V); above ~4 V internal pull-up/down must be disabled; an FT_a pin used by analog peripheral max 4 V. **TT_xx pins** (3.3 V only, e.g. PA4, PA5, PA7): max 4.0 V. BOOT0 max 9.0 V. Other pins 4.0 V.
- Current: per I/O sink/source 20 mA (Px_C pins 1 mA); total all I/O 140 mA; total VDD in/VSS out 620 mA; injected -5/+0 mA per pin (0/0 for PA4, PA5); sum injected +/-25 mA.
- Tj max 125 C (suffix 6; 140 C for suffix 3); storage -65..150 C.
#### I/O characteristics
VIL <= 0.3 VDD (or 0.4 VDD - 0.1), VIH >= 0.7 VDD (or 0.47 VDD + 0.25); hysteresis 250 mV; weak pull-up/down 30-50 kOhm (40 typ); pin capacitance ~5 pF. Output: VOL <= 0.4 V @ +8 mA (VDD 2.7-3.6 V); VOH >= VDD-0.4 V @ -8 mA; at 20 mA VOL <= 1.3 V; Fast-mode+ (FT_f) pins sink 20 mA at <= 0.4 V (VDD>=2.3 V). Leakage <= +/-250 nA. All GPIOs high-current capable with speed selection (EMI/power); all pins analog-input after reset except debug.

---

## 07a. STM32H755ZIT6 (LQFP144) - Pin list: package pin -> name, I/O type, analog/special functions

Source: STM32H755xI datasheet DS12919 Rev 3, Table 8 (LQFP144 column only). Digital alternate functions (AFx numbers) are in §07b.

I/O structure codes: FT = 5V-tolerant; FT_a = +analog; FT_h = high-speed low-voltage; FT_u = USB option (VDD33USB); FT_f = FM+ capable (20mA I2C); TT_a = 3.3V-only (NOT 5V tolerant) with analog; B = BOOT0; RST = reset. All GPIO are floating inputs after reset except debug pins; (datasheet 3.8) GPIOs are Analog mode after reset per RM.

| LQFP144 pin | Name | I/O struct | Additional/analog functions | Note |
|---|---|---|---|---|
| 1 | PE2 | FT_ h |  |  |
| 2 | PE3 | FT_ h |  |  |
| 3 | PE4 | FT_ h |  |  |
| 4 | PE5 | FT_ h |  |  |
| 5 | PE6 | FT_ h |  |  |
| 6 | VSS |  |  |  |
| 7 | VDD |  |  |  |
| 8 | VBAT |  |  |  |
| 9 | PC13 | FT | RTC_OUT/RTC_T AMP1/RTC_TS/W KUP4 |  |
| 10 | PC14- OSC32_IN(O SC32_IN)(1) | FT | OSC32_IN |  |
| 11 | PC15- OSC32_OUT (OSC32_OU T)(1) | FT | OSC32_OUT |  |
| 12 | VSS |  |  |  |
| 13 | VDD |  |  |  |
| 14 | VSSSMPS |  |  |  |
| 15 | VLXSMPS |  |  |  |
| 16 | VDDSMPS |  |  |  |
| 17 | VFBSMPS |  |  |  |
| 18 | VSS |  |  |  |
| 19 | VDD |  |  |  |
| 20 | PF6 | FT_ ha | ADC3_INN4, ADC3_INP8 |  |
| 21 | PF7 | FT_ ha | ADC3_INP3 |  |
| 22 | PF8 | FT_ ha | ADC3_INN3, ADC3_INP7 |  |
| 23 | PF9 | FT_ ha | ADC3_INP2 |  |
| 24 | PF10 | FT_ ha | ADC3_INN2, ADC3_INP6 |  |
| 25 | PH0- OSC_IN(PH 0) | FT | OSC_IN |  |
| 26 | PH1- OSC_OUT(P H1) | FT | OSC_OUT |  |
| 27 | NRST | RST |  |  |
| 28 | PC0 | FT_ a | ADC123_INP10 |  |
| 29 | PC1 | FT_ ha | ADC123_INN10, ADC123_INP11, RTC_TAMP3/WK UP6 |  |
| 32 | VDD |  |  |  |
| 33 | VSS |  |  |  |
| 34 | VSSA |  |  |  |
| 35 | VREF+ |  |  |  |
| 36 | VDDA |  |  |  |
| 37 | PA0 | FT_ a | ADC1_INP16, WKUP1 |  |
| 38 | PA1 | FT_ ha | ADC1_INN16, ADC1_INP17 |  |
| 39 | PA2 | FT_ a | ADC12_INP14, WKUP2 |  |
| 40 | PA3 | FT_ ha | ADC12_INP15 |  |
| 41 | VSS |  |  |  |
| 42 | VDD |  |  |  |
| 43 | PA4 | TT_ a | ADC12_INP18, DAC1_OUT1 |  |
| 44 | PA5 | TT_ ha | ADC12_INN18, ADC12_INP19, DAC1_OUT2 |  |
| 45 | PA6 | FT_ a | ADC12_INP3 |  |
| 46 | PA7 | TT_ a | ADC12_INN3, ADC12_INP7, OPAMP1_VINM |  |
| 47 | PC4 | TT_ a | ADC12_INP4, OPAMP1_VOUT, COMP1_INM |  |
| 48 | PC5 | TT_ a | ADC12_INN4, ADC12_INP8, OPAMP1_VINM |  |
| 49 | PB0 | FT_ a | ADC12_INN5, ADC12_INP9, OPAMP1_VINP, COMP1_INP |  |
| 50 | PB1 | TT_ a | ADC12_INP5, COMP1_INM |  |
| 51 | PB2 | FT_ ha | COMP1_INP |  |
| 52 | PF11 | FT_ a | ADC1_INP2 |  |
| 53 | PF14 | FT_f ha | ADC2_INN2, ADC2_INP6 |  |
| 54 | PF15 | FT_f h |  |  |
| 55 | VSS |  |  |  |
| 56 | VDD |  |  |  |
| 57 | PE7 | TT_ ha | OPAMP2_VOUT, COMP2_INM |  |
| 58 | PE8 | TT_ ha | OPAMP2_VINM |  |
| 59 | PE9 | TT_ ha | OPAMP2_VINP, COMP2_INP |  |
| 60 | PE10 | FT_ ha | COMP2_INM |  |
| 61 | PE11 | FT_ ha | COMP2_INP |  |
| 62 | PE12 | FT_ h |  |  |
| 63 | PE13 | FT_ h |  |  |
| 64 | PE14 | FT_ h |  |  |
| 65 | PE15 | FT_ h |  |  |
| 66 | PB10 | FT_f |  |  |
| 67 | PB11 | FT_f |  |  |
| 68 | VCAP |  |  |  |
| 69 | VSS |  |  |  |
| 70 | VDDLDO(4) |  |  |  |
| 71 | VDD |  |  |  |
| 72 | PB12(5) | FT_ u |  |  |
| 73 | PB13(5) | FT_ u | OTG_HS_VBUS |  |
| 74 | PB14 | FT_ u |  |  |
| 75 | PB15 | FT_ u |  |  |
| 76 | PD8 | FT_ h |  |  |
| 77 | PD9 | FT_ h |  |  |
| 78 | PD10 | FT_ h |  |  |
| 79 | VDD |  |  |  |
| 80 | VSS |  |  |  |
| 81 | PD11 | FT_ h |  |  |
| 82 | PD12 | FT_f h |  |  |
| 83 | PD13 | FT_f h |  |  |
| 84 | PD14 | FT_ h |  |  |
| 85 | PD15 | FT_ h |  |  |
| 86 | PG6 | FT_ h |  |  |
| 87 | PG7 | FT_ h |  |  |
| 88 | PG8 | FT_ h |  |  |
| 89 | VSS |  |  |  |
| 90 | VDD50USB (6) |  |  |  |
| 91 | VDD33USB |  |  |  |
| 92 | VDD |  |  |  |
| 93 | PC6 | FT_ h | SWPMI_IO |  |
| 94 | PC7 | FT_ h |  |  |
| 95 | PC8 | FT_ h |  |  |
| 96 | PC9 | FT_f h |  |  |
| 97 | PA8 | FT_f ha |  |  |
| 98 | PA9 | FT_ u | OTG_FS_VBUS |  |
| 99 | PA10 | FT_ u |  |  |
| 100 | PA11 | FT_ u |  |  |
| 101 | PA12 | FT_ u |  |  |
| 102 | PA13(JTMS/ SWDIO) | FT |  |  |
| 103 | VCAP |  |  |  |
| 104 | VSS |  |  |  |
| 105 | VDDLDO(4) |  |  |  |
| 106 | VDD |  |  |  |
| 107 | PA14(JTCK/ SWCLK) | FT |  |  |
| 108 | PA15(JTDI) | FT |  |  |
| 109 | PC10 | FT_ ha |  |  |
| 110 | PC11 | FT_ h |  |  |
| 111 | PC12 | FT_ h |  |  |
| 112 | PD0 | FT_ h |  |  |
| 113 | PD1 | FT_ h |  |  |
| 114 | PD2 | FT_ h |  |  |
| 115 | PD3 | FT_ h |  |  |
| 116 | PD4 | FT_ h |  |  |
| 117 | PD5 | FT_ h |  |  |
| 118 | VSS |  |  |  |
| 119 | VDD |  |  |  |
| 120 | PD6 | FT_ h |  |  |
| 121 | PD7 | FT_ h |  |  |
| 122 | PG9 | FT_ h |  |  |
| 123 | PG10 | FT_ h |  |  |
| 124 | PG11 | FT_ h |  |  |
| 125 | PG12 | FT_ h |  |  |
| 126 | PG13 | FT_ h |  |  |
| 127 | PG14 | FT_ h |  |  |
| 128 | VSS |  |  |  |
| 129 | VDD |  |  |  |
| 130 | PB3(JTDO/T RACESWO) | FT |  |  |
| 131 | PB4(NJTRS T) | FT |  |  |
| 132 | PB5 | FT |  |  |
| 133 | PB6 | FT_f |  |  |
| 134 | PB7 | FT_f a | PVD_IN |  |
| 135 | BOOT0 | B | VPP |  |
| 136 | PB8 | FT_f h |  |  |
| 137 | PB9 | FT_f h |  |  |
| 138 | PE0 | FT_ h |  |  |
| 139 | PE1 | FT_ h |  |  |
| 140 | VCAP |  |  |  |
| 141 | VSS |  |  |  |
| 142 | PDR_ON | FT |  |  |
| 143 | VDDLDO(4) |  |  |  |
| 144 | VDD |  |  |  |

Supply/special pin notes: VDDSMPS (16) must always equal VDD. VDDLDO pins 70/105/143 are the LDO supply (datasheet: 1.62-3.6 V, VDDLDO <= VDD). VCAP pins 68/103/140 = VCORE decoupling (2.2 uF each, ESR < 100 mohm; in bypass mode use 100 nF instead). SMPS pins: 14 VSSSMPS, 15 VLXSMPS (inductor), 16 VDDSMPS (must equal VDD), 17 VFBSMPS (feedback). VDD50USB (90) is the 5 V USB-VBUS-derived input of the internal 3.3 V USB regulator that produces VDD33USB (91). PDR_ON (142): pull high for normal operation; tying it low disables internal reset supervision AND the VBAT function. BOOT0 (135): max 9 V tolerance. PA13/PA14 (102/107) = SWDIO/SWCLK; PB3 (130) = JTDO/TRACESWO (SWO); PB4 (131) = NJTRST; PA15 = JTDI.

---

## 07b. STM32H755ZIT6 (LQFP144) - Alternate function matrix (AF0-AF15)

Source: DS12919 Rev 3 Table 9, filtered to the GPIOs that exist on LQFP144. Only non-empty AFs are listed. Use in HAL as `GPIO_AFx_<PERIPH>` / set via GPIOx_AFR. NOTE: the datasheet table calls the USB-on-PA11/PA12 peripheral `OTG_FS_*` and the ULPI/HS one `OTG_HS_*`. Several pins are on the Nucleo board hard-wired to other things; see §03 and §04 before choosing pins.

AF column meaning (peripheral families per AF number): AF0=SYS; AF1=TIM1/2/16/17/LPTIM1/HRTIM1; AF2=SAI1/TIM3/4/5/12/HRTIM1; AF3=LPUART/TIM8/LPTIM2-5/HRTIM1/DFSDM; AF4=I2C1-4/USART1/TIM15/LPTIM2/DFSDM/CEC; AF5=SPI1-6/CEC; AF6=SPI2/3/SAI1/3/I2C4/UART4/DFSDM; AF7=SPI2/3/6/USART1/2/3/6/UART7/SDMMC1; AF8=SPI6/SAI2/4/UART4/5/8/LPUART/SDMMC1/SPDIFRX1; AF9=SAI4/FDCAN1/2/TIM13/14/QUADSPI/FMC/SDMMC2/LCD/SPDIFRX1; AF10=SAI2/4/TIM8/QUADSPI/SDMMC2/OTG_HS1/OTG_FS2/LCD/CRS; AF11=I2C4/UART7/SWPMI1/TIM1/8/DFSDM/SDMMC2/MDIOS/ETH; AF12=TIM1/8/FMC/SDMMC1/MDIOS/OTG_FS1/LCD; AF13=TIM1/DCMI/LCD/COMP; AF14=UART5/LCD; AF15=SYS

| Pin | Alternate functions |
|---|---|
| PA0 | AF1 TIM2_CH1/TIM2_ETR; AF2 TIM5_CH1; AF3 TIM8_ETR; AF4 TIM15_BKIN; AF7 USART2_CTS/USART2_NSS; AF8 UART4_TX; AF9 SDMMC2_CMD; AF10 SAI2_SD_B; AF11 ETH_MII_CRS |
| PA1 | AF1 TIM2_CH2; AF2 TIM5_CH2; AF3 LPTIM3_OUT; AF4 TIM15_CH1N; AF7 USART2_RTS/USART2_DE; AF8 UART4_RX; AF9 QUADSPI_BK1_IO3; AF10 SAI2_MCLK_B; AF11 ETH_MII_RX_CLK/ETH_RMII_REF_CLK; AF14 LCD_R2 |
| PA2 | AF1 TIM2_CH3; AF2 TIM5_CH3; AF3 LPTIM4_OUT; AF4 TIM15_CH1; AF7 USART2_TX; AF8 SAI2_SCK_B; AF11 ETH_MDIO; AF12 MDIOS_MDIO; AF14 LCD_R1 |
| PA3 | AF1 TIM2_CH4; AF2 TIM5_CH4; AF3 LPTIM5_OUT; AF4 TIM15_CH2; AF7 USART2_RX; AF9 LCD_B2; AF10 OTG_HS_ULPI_D0; AF11 ETH_MII_COL; AF14 LCD_B5 |
| PA4 | AF0 D1PWREN; AF2 TIM5_ETR; AF5 SPI1_NSS/I2S1_WS; AF6 SPI3_NSS/I2S3_WS; AF7 USART2_CK; AF8 SPI6_NSS; AF12 OTG_HS_SOF; AF13 DCMI_HSYNC; AF14 LCD_VSYNC |
| PA5 | AF0 D2PWREN; AF1 TIM2_CH1/TIM2_ETR; AF3 TIM8_CH1N; AF5 SPI1_SCK/I2S1_CK; AF8 SPI6_SCK; AF10 OTG_HS_ULPI_CK; AF14 LCD_R4 |
| PA6 | AF1 TIM1_BKIN; AF2 TIM3_CH1; AF3 TIM8_BKIN; AF5 SPI1_MISO/I2S1_SDI; AF8 SPI6_MISO; AF9 TIM13_CH1; AF10 TIM8_BKIN_COMP12; AF11 MDIOS_MDC; AF12 TIM1_BKIN_COMP12; AF13 DCMI_PIXCLK; AF14 LCD_G2 |
| PA7 | AF1 TIM1_CH1N; AF2 TIM3_CH2; AF3 TIM8_CH1N; AF5 SPI1_MOSI/I2S1_SDO; AF8 SPI6_MOSI; AF9 TIM14_CH1; AF11 ETH_MII_RX_DV/ETH_RMII_CRS_DV; AF12 FMC_SDNWE |
| PA8 | AF0 MCO1; AF1 TIM1_CH1; AF2 HRTIM_CHB2; AF3 TIM8_BKIN2; AF4 I2C3_SCL; AF7 USART1_CK; AF10 OTG_FS_SOF; AF11 UART7_RX; AF12 TIM8_BKIN2_COMP12; AF13 LCD_B3; AF14 LCD_R6 |
| PA9 | AF1 TIM1_CH2; AF2 HRTIM_CHC1; AF3 LPUART1_TX; AF4 I2C3_SMBA; AF5 SPI2_SCK/I2S2_CK; AF7 USART1_TX; AF11 ETH_TX_ER; AF13 DCMI_D0; AF14 LCD_R5 |
| PA10 | AF1 TIM1_CH3; AF2 HRTIM_CHC2; AF3 LPUART1_RX; AF7 USART1_RX; AF10 OTG_FS_ID; AF11 MDIOS_MDIO; AF12 LCD_B4; AF13 DCMI_D1; AF14 LCD_B1 |
| PA11 | AF1 TIM1_CH4; AF2 HRTIM_CHD1; AF3 LPUART1_CTS; AF5 SPI2_NSS/I2S2_WS; AF6 UART4_RX; AF7 USART1_CTS/USART1_NSS; AF9 FDCAN1_RX; AF10 OTG_FS_DM; AF14 LCD_R4 |
| PA12 | AF1 TIM1_ETR; AF2 HRTIM_CHD2; AF3 LPUART1_RTS/LPUART1_DE; AF5 SPI2_SCK/I2S2_CK; AF6 UART4_TX; AF7 USART1_RTS/USART1_DE; AF8 SAI2_FS_B; AF9 FDCAN1_TX; AF10 OTG_FS_DP; AF14 LCD_R5 |
| PA13 | AF0 JTMS-SWDIO |
| PA14 | AF0 JTCK-SWCLK |
| PA15 | AF0 JTDI; AF1 TIM2_CH1/TIM2_ETR; AF2 HRTIM_FLT1; AF4 CEC; AF5 SPI1_NSS/I2S1_WS; AF6 SPI3_NSS/I2S3_WS; AF7 SPI6_NSS; AF8 UART4_RTS/UART4_DE; AF11 UART7_TX |
| PB0 | AF1 TIM1_CH2N; AF2 TIM3_CH3; AF3 TIM8_CH2N; AF6 DFSDM1_CKOUT; AF8 UART4_CTS; AF9 LCD_R3; AF10 OTG_HS_ULPI_D1; AF11 ETH_MII_RXD2; AF14 LCD_G1 |
| PB1 | AF1 TIM1_CH3N; AF2 TIM3_CH4; AF3 TIM8_CH3N; AF6 DFSDM1_DATIN1; AF9 LCD_R6; AF10 OTG_HS_ULPI_D2; AF11 ETH_MII_RXD3; AF14 LCD_G0 |
| PB2 | AF0 RTC_OUT; AF2 SAI1_D1; AF4 DFSDM1_CKIN1; AF6 SAI1_SD_A; AF7 SPI3_MOSI/I2S3_SDO; AF8 SAI4_SD_A; AF9 QUADSPI_CLK; AF10 SAI4_D1; AF11 ETH_TX_ER |
| PB3 | AF0 JTDO/TRACESWO; AF1 TIM2_CH2; AF2 HRTIM_FLT4; AF5 SPI1_SCK/I2S1_CK; AF6 SPI3_SCK/I2S3_CK; AF8 SPI6_SCK; AF9 SDMMC2_D2; AF10 CRS_SYNC; AF11 UART7_RX |
| PB4 | AF0 NJTRST; AF1 TIM16_BKIN; AF2 TIM3_CH1; AF3 HRTIM_EEV6; AF5 SPI1_MISO/I2S1_SDI; AF6 SPI3_MISO/I2S3_SDI; AF7 SPI2_NSS/I2S2_WS; AF8 SPI6_MISO; AF9 SDMMC2_D3; AF11 UART7_TX |
| PB5 | AF1 TIM17_BKIN; AF2 TIM3_CH2; AF3 HRTIM_EEV7; AF4 I2C1_SMBA; AF5 SPI1_MOSI/I2S1_SDO; AF6 I2C4_SMBA; AF7 SPI3_MOSI/I2S3_SDO; AF8 SPI6_MOSI; AF9 FDCAN2_RX; AF10 OTG_HS_ULPI_D7; AF11 ETH_PPS_OUT; AF12 FMC_SDCKE1; AF13 DCMI_D10; AF14 UART5_RX |
| PB6 | AF1 TIM16_CH1N; AF2 TIM4_CH1; AF3 HRTIM_EEV8; AF4 I2C1_SCL; AF5 CEC; AF6 I2C4_SCL; AF7 USART1_TX; AF8 LPUART1_TX; AF9 FDCAN2_TX; AF10 QUADSPI_BK1_NCS; AF11 DFSDM1_DATIN5; AF12 FMC_SDNE1; AF13 DCMI_D5; AF14 UART5_TX |
| PB7 | AF1 TIM17_CH1N; AF2 TIM4_CH2; AF3 HRTIM_EEV9; AF4 I2C1_SDA; AF6 I2C4_SDA; AF7 USART1_RX; AF8 LPUART1_RX; AF11 DFSDM1_CKIN5; AF12 FMC_NL; AF13 DCMI_VSYNC |
| PB8 | AF1 TIM16_CH1; AF2 TIM4_CH3; AF3 DFSDM1_CKIN7; AF4 I2C1_SCL; AF6 I2C4_SCL; AF7 SDMMC1_CKIN; AF8 UART4_RX; AF9 FDCAN1_RX; AF10 SDMMC2_D4; AF11 ETH_MII_TXD3; AF12 SDMMC1_D4; AF13 DCMI_D6; AF14 LCD_B6 |
| PB9 | AF1 TIM17_CH1; AF2 TIM4_CH4; AF3 DFSDM1_DATIN7; AF4 I2C1_SDA; AF5 SPI2_NSS/I2S2_WS; AF6 I2C4_SDA; AF7 SDMMC1_CDIR; AF8 UART4_TX; AF9 FDCAN1_TX; AF10 SDMMC2_D5; AF11 I2C4_SMBA; AF12 SDMMC1_D5; AF13 DCMI_D7; AF14 LCD_B7 |
| PB10 | AF1 TIM2_CH3; AF2 HRTIM_SCOUT; AF3 LPTIM2_IN1; AF4 I2C2_SCL; AF5 SPI2_SCK/I2S2_CK; AF6 DFSDM1_DATIN7; AF7 USART3_TX; AF9 QUADSPI_BK1_NCS; AF10 OTG_HS_ULPI_D3; AF11 ETH_MII_RX_ER; AF14 LCD_G4 |
| PB11 | AF1 TIM2_CH4; AF2 HRTIM_SCIN; AF3 LPTIM2_ETR; AF4 I2C2_SDA; AF6 DFSDM1_CKIN7; AF7 USART3_RX; AF10 OTG_HS_ULPI_D4; AF11 ETH_MII_TX_EN/ETH_RMII_TX_EN; AF14 LCD_G5 |
| PB12 | AF1 TIM1_BKIN; AF4 I2C2_SMBA; AF5 SPI2_NSS/I2S2_WS; AF6 DFSDM1_DATIN1; AF7 USART3_CK; AF9 FDCAN2_RX; AF10 OTG_HS_ULPI_D5; AF11 ETH_MII_TXD0/ETH_RMII_TXD0; AF12 OTG_HS_ID; AF13 TIM1_BKIN_COMP12; AF14 UART5_RX |
| PB13 | AF1 TIM1_CH1N; AF3 LPTIM2_OUT; AF5 SPI2_SCK/I2S2_CK; AF6 DFSDM1_CKIN1; AF7 USART3_CTS/USART3_NSS; AF9 FDCAN2_TX; AF10 OTG_HS_ULPI_D6; AF11 ETH_MII_TXD1/ETH_RMII_TXD1; AF14 UART5_TX |
| PB14 | AF1 TIM1_CH2N; AF2 TIM12_CH1; AF3 TIM8_CH2N; AF4 USART1_TX; AF5 SPI2_MISO/I2S2_SDI; AF6 DFSDM1_DATIN2; AF7 USART3_RTS/USART3_DE; AF8 UART4_RTS/UART4_DE; AF9 SDMMC2_D0; AF12 OTG_HS_DM |
| PB15 | AF0 RTC_REFIN; AF1 TIM1_CH3N; AF2 TIM12_CH2; AF3 TIM8_CH3N; AF4 USART1_RX; AF5 SPI2_MOSI/I2S2_SDO; AF6 DFSDM1_CKIN2; AF8 UART4_CTS; AF9 SDMMC2_D1; AF12 OTG_HS_DP |
| PC0 | AF3 DFSDM1_CKIN0; AF6 DFSDM1_DATIN4; AF8 SAI2_FS_B; AF10 OTG_HS_ULPI_STP; AF12 FMC_SDNWE; AF14 LCD_R5 |
| PC1 | AF0 TRACED0; AF2 SAI1_D1; AF3 DFSDM1_DATIN0; AF4 DFSDM1_CKIN4; AF5 SPI2_MOSI/I2S2_SDO; AF6 SAI1_SD_A; AF8 SAI4_SD_A; AF9 SDMMC2_CK; AF10 SAI4_D1; AF11 ETH_MDC; AF12 MDIOS_MDC |
| PC4 | AF0 C2DSLEEP; AF3 DFSDM1_CKIN2; AF5 I2S1_MCK; AF9 SPDIFRX1_IN3; AF11 ETH_MII_RXD0/ETH_RMII_RXD0; AF12 FMC_SDNE0 |
| PC5 | AF0 C2SLEEP; AF2 SAI1_D3; AF3 DFSDM1_DATIN2; AF9 SPDIFRX1_IN4; AF10 SAI4_D3; AF11 ETH_MII_RXD1/ETH_RMII_RXD1; AF12 FMC_SDCKE0; AF13 COMP1_OUT |
| PC6 | AF1 HRTIM_CHA1; AF2 TIM3_CH1; AF3 TIM8_CH1; AF4 DFSDM1_CKIN3; AF5 I2S2_MCK; AF7 USART6_TX; AF8 SDMMC1_D0DIR; AF9 FMC_NWAIT; AF10 SDMMC2_D6; AF12 SDMMC1_D6; AF13 DCMI_D0; AF14 LCD_HSYNC |
| PC7 | AF0 TRGIO; AF1 HRTIM_CHA2; AF2 TIM3_CH2; AF3 TIM8_CH2; AF4 DFSDM1_DATIN3; AF6 I2S3_MCK; AF7 USART6_RX; AF8 SDMMC1_D123DIR; AF9 FMC_NE1; AF10 SDMMC2_D7; AF11 SWPMI_TX; AF12 SDMMC1_D7; AF13 DCMI_D1; AF14 LCD_G6 |
| PC8 | AF0 TRACED1; AF1 HRTIM_CHB1; AF2 TIM3_CH3; AF3 TIM8_CH3; AF7 USART6_CK; AF8 UART5_RTS/UART5_DE; AF9 FMC_NE2/FMC_NCE; AF11 SWPMI_RX; AF12 SDMMC1_D0; AF13 DCMI_D2 |
| PC9 | AF0 MCO2; AF2 TIM3_CH4; AF3 TIM8_CH4; AF4 I2C3_SDA; AF5 I2S_CKIN; AF8 UART5_CTS; AF9 QUADSPI_BK1_IO0; AF10 LCD_G3; AF11 SWPMI_SUSPEND; AF12 SDMMC1_D1; AF13 DCMI_D3; AF14 LCD_B2 |
| PC10 | AF2 HRTIM_EEV1; AF3 DFSDM1_CKIN5; AF6 SPI3_SCK/I2S3_CK; AF7 USART3_TX; AF8 UART4_TX; AF9 QUADSPI_BK1_IO1; AF12 SDMMC1_D2; AF13 DCMI_D8; AF14 LCD_R2 |
| PC11 | AF2 HRTIM_FLT2; AF3 DFSDM1_DATIN5; AF6 SPI3_MISO/I2S3_SDI; AF7 USART3_RX; AF8 UART4_RX; AF9 QUADSPI_BK2_NCS; AF12 SDMMC1_D3; AF13 DCMI_D4 |
| PC12 | AF0 TRACED3; AF2 HRTIM_EEV2; AF6 SPI3_MOSI/I2S3_SDO; AF7 USART3_CK; AF8 UART5_TX; AF12 SDMMC1_CK; AF13 DCMI_D9 |
| PC13 |  |
| PC14 |  |
| PC15 |  |
| PD0 | AF3 DFSDM1_CKIN6; AF6 SAI3_SCK_A; AF8 UART4_RX; AF9 FDCAN1_RX; AF12 FMC_D2/FMC_DA2 |
| PD1 | AF3 DFSDM1_DATIN6; AF6 SAI3_SD_A; AF8 UART4_TX; AF9 FDCAN1_TX; AF12 FMC_D3/FMC_DA3 |
| PD2 | AF0 TRACED2; AF2 TIM3_ETR; AF8 UART5_RX; AF12 SDMMC1_CMD; AF13 DCMI_D11 |
| PD3 | AF3 DFSDM1_CKOUT; AF5 SPI2_SCK/I2S2_CK; AF7 USART2_CTS/USART2_NSS; AF12 FMC_CLK; AF13 DCMI_D5; AF14 LCD_G7 |
| PD4 | AF2 HRTIM_FLT3; AF6 SAI3_FS_A; AF7 USART2_RTS/USART2_DE; AF12 FMC_NOE |
| PD5 | AF2 HRTIM_EEV3; AF7 USART2_TX; AF12 FMC_NWE |
| PD6 | AF2 SAI1_D1; AF3 DFSDM1_CKIN4; AF4 DFSDM1_DATIN1; AF5 SPI3_MOSI/I2S3_SDO; AF6 SAI1_SD_A; AF7 USART2_RX; AF8 SAI4_SD_A; AF10 SAI4_D1; AF11 SDMMC2_CK; AF12 FMC_NWAIT; AF13 DCMI_D10; AF14 LCD_B2 |
| PD7 | AF3 DFSDM1_DATIN4; AF5 SPI1_MOSI/I2S1_SDO; AF6 DFSDM1_CKIN1; AF7 USART2_CK; AF9 SPDIFRX1_IN1; AF11 SDMMC2_CMD; AF12 FMC_NE1 |
| PD8 | AF3 DFSDM1_CKIN3; AF6 SAI3_SCK_B; AF7 USART3_TX; AF9 SPDIFRX1_IN2; AF12 FMC_D13/FMC_DA13 |
| PD9 | AF3 DFSDM1_DATIN3; AF6 SAI3_SD_B; AF7 USART3_RX; AF12 FMC_D14/FMC_DA14 |
| PD10 | AF3 DFSDM1_CKOUT; AF6 SAI3_FS_B; AF7 USART3_CK; AF12 FMC_D15/FMC_DA15; AF14 LCD_B3 |
| PD11 | AF3 LPTIM2_IN2; AF4 I2C4_SMBA; AF7 USART3_CTS/USART3_NSS; AF9 QUADSPI_BK1_IO0; AF10 SAI2_SD_A; AF12 FMC_A16 |
| PD12 | AF1 LPTIM1_IN1; AF2 TIM4_CH1; AF3 LPTIM2_IN1; AF4 I2C4_SCL; AF7 USART3_RTS/USART3_DE; AF9 QUADSPI_BK1_IO1; AF10 SAI2_FS_A; AF12 FMC_A17 |
| PD13 | AF1 LPTIM1_OUT; AF2 TIM4_CH2; AF4 I2C4_SDA; AF9 QUADSPI_BK1_IO3; AF10 SAI2_SCK_A; AF12 FMC_A18 |
| PD14 | AF2 TIM4_CH3; AF6 SAI3_MCLK_B; AF8 UART8_CTS; AF12 FMC_D0/FMC_DA0 |
| PD15 | AF2 TIM4_CH4; AF6 SAI3_MCLK_A; AF8 UART8_RTS/UART8_DE; AF12 FMC_D1/FMC_DA1 |
| PE0 | AF1 LPTIM1_ETR; AF2 TIM4_ETR; AF3 HRTIM_SCIN; AF4 LPTIM2_ETR; AF8 UART8_RX; AF10 SAI2_MCLK_A; AF12 FMC_NBL0; AF13 DCMI_D2 |
| PE1 | AF1 LPTIM1_IN2; AF3 HRTIM_SCOUT; AF8 UART8_TX; AF12 FMC_NBL1; AF13 DCMI_D3 |
| PE2 | AF0 TRACECLK; AF2 SAI1_CK1; AF5 SPI4_SCK; AF6 SAI1_MCLK_A; AF8 SAI4_MCLK_A; AF9 QUADSPI_BK1_IO2; AF10 SAI4_CK1; AF11 ETH_MII_TXD3; AF12 FMC_A23 |
| PE3 | AF0 TRACED0; AF4 TIM15_BKIN; AF6 SAI1_SD_B; AF8 SAI4_SD_B; AF12 FMC_A19 |
| PE4 | AF0 TRACED1; AF2 SAI1_D2; AF3 DFSDM1_DATIN3; AF4 TIM15_CH1N; AF5 SPI4_NSS; AF6 SAI1_FS_A; AF8 SAI4_FS_A; AF10 SAI4_D2; AF12 FMC_A20; AF13 DCMI_D4; AF14 LCD_B0 |
| PE5 | AF0 TRACED2; AF2 SAI1_CK2; AF3 DFSDM1_CKIN3; AF4 TIM15_CH1; AF5 SPI4_MISO; AF6 SAI1_SCK_A; AF8 SAI4_SCK_A; AF10 SAI4_CK2; AF12 FMC_A21; AF13 DCMI_D6; AF14 LCD_G0 |
| PE6 | AF0 TRACED3; AF1 TIM1_BKIN2; AF2 SAI1_D1; AF4 TIM15_CH2; AF5 SPI4_MOSI; AF6 SAI1_SD_A; AF8 SAI4_SD_A; AF9 SAI4_D1; AF10 SAI2_MCLK_B; AF11 TIM1_BKIN2_COMP12; AF12 FMC_A22; AF13 DCMI_D7; AF14 LCD_G1 |
| PE7 | AF1 TIM1_ETR; AF3 DFSDM1_DATIN2; AF7 UART7_RX; AF10 QUADSPI_BK2_IO0; AF12 FMC_D4/FMC_DA4 |
| PE8 | AF1 TIM1_CH1N; AF3 DFSDM1_CKIN2; AF7 UART7_TX; AF10 QUADSPI_BK2_IO1; AF12 FMC_D5/FMC_DA5; AF13 COMP2_OUT |
| PE9 | AF1 TIM1_CH1; AF3 DFSDM1_CKOUT; AF7 UART7_RTS/UART7_DE; AF10 QUADSPI_BK2_IO2; AF12 FMC_D6/FMC_DA6 |
| PE10 | AF1 TIM1_CH2N; AF3 DFSDM1_DATIN4; AF7 UART7_CTS; AF10 QUADSPI_BK2_IO3; AF12 FMC_D7/FMC_DA7 |
| PE11 | AF1 TIM1_CH2; AF3 DFSDM1_CKIN4; AF5 SPI4_NSS; AF10 SAI2_SD_B; AF12 FMC_D8/FMC_DA8; AF14 LCD_G3 |
| PE12 | AF1 TIM1_CH3N; AF3 DFSDM1_DATIN5; AF5 SPI4_SCK; AF10 SAI2_SCK_B; AF12 FMC_D9/FMC_DA9; AF13 COMP1_OUT; AF14 LCD_B4 |
| PE13 | AF1 TIM1_CH3; AF3 DFSDM1_CKIN5; AF5 SPI4_MISO; AF10 SAI2_FS_B; AF12 FMC_D10/FMC_DA10; AF13 COMP2_OUT; AF14 LCD_DE |
| PE14 | AF1 TIM1_CH4; AF5 SPI4_MOSI; AF10 SAI2_MCLK_B; AF12 FMC_D11/FMC_DA11; AF14 LCD_CLK |
| PE15 | AF1 TIM1_BKIN; AF12 FMC_D12/FMC_DA12; AF13 TIM1_BKIN_COMP12/COMP_TIM1_BKIN; AF14 LCD_R7 |
| PF6 | AF1 TIM16_CH1; AF5 SPI5_NSS; AF6 SAI1_SD_B; AF7 UART7_RX; AF8 SAI4_SD_B; AF9 QUADSPI_BK1_IO3 |
| PF7 | AF1 TIM17_CH1; AF5 SPI5_SCK; AF6 SAI1_MCLK_B; AF7 UART7_TX; AF8 SAI4_MCLK_B; AF9 QUADSPI_BK1_IO2 |
| PF8 | AF1 TIM16_CH1N; AF5 SPI5_MISO; AF6 SAI1_SCK_B; AF7 UART7_RTS/UART7_DE; AF8 SAI4_SCK_B; AF9 TIM13_CH1; AF10 QUADSPI_BK1_IO0 |
| PF9 | AF1 TIM17_CH1N; AF5 SPI5_MOSI; AF6 SAI1_FS_B; AF7 UART7_CTS; AF8 SAI4_FS_B; AF9 TIM14_CH1; AF10 QUADSPI_BK1_IO1 |
| PF10 | AF1 TIM16_BKIN; AF2 SAI1_D3; AF9 QUADSPI_CLK; AF10 SAI4_D3; AF13 DCMI_D11; AF14 LCD_DE |
| PF11 | AF5 SPI5_MOSI; AF10 SAI2_SD_B; AF12 FMC_SDNRAS; AF13 DCMI_D12 |
| PF14 | AF3 DFSDM1_CKIN6; AF4 I2C4_SCL; AF12 FMC_A8 |
| PF15 | AF4 I2C4_SDA; AF12 FMC_A9 |
| PG6 | AF1 TIM17_BKIN; AF2 HRTIM_CHE1; AF10 QUADSPI_BK1_NCS; AF12 FMC_NE3; AF13 DCMI_D12; AF14 LCD_R7 |
| PG7 | AF2 HRTIM_CHE2; AF6 SAI1_MCLK_A; AF7 USART6_CK; AF12 FMC_INT; AF13 DCMI_D13; AF14 LCD_CLK |
| PG8 | AF3 TIM8_ETR; AF5 SPI6_NSS; AF7 USART6_RTS/USART6_DE; AF8 SPDIFRX1_IN3; AF11 ETH_PPS_OUT; AF12 FMC_SDCLK; AF14 LCD_G7 |
| PG9 | AF5 SPI1_MISO/I2S1_SDI; AF7 USART6_RX; AF8 SPDIFRX1_IN4; AF9 QUADSPI_BK2_IO2; AF10 SAI2_FS_B; AF12 FMC_NE2/FMC_NCE; AF13 DCMI_VSYNC |
| PG10 | AF2 HRTIM_FLT5; AF5 SPI1_NSS/I2S1_WS; AF9 LCD_G3; AF10 SAI2_SD_B; AF12 FMC_NE3; AF13 DCMI_D2; AF14 LCD_B2 |
| PG11 | AF1 LPTIM1_IN2; AF2 HRTIM_EEV4; AF5 SPI1_SCK/I2S1_CK; AF8 SPDIFRX1_IN1; AF10 SDMMC2_D2; AF11 ETH_MII_TX_EN/ETH_RMII_TX_EN; AF13 DCMI_D3; AF14 LCD_B3 |
| PG12 | AF1 LPTIM1_IN1; AF2 HRTIM_EEV5; AF5 SPI6_MISO; AF7 USART6_RTS/USART6_DE; AF8 SPDIFRX1_IN2; AF9 LCD_B4; AF11 ETH_MII_TXD1/ETH_RMII_TXD1; AF12 FMC_NE4; AF14 LCD_B1 |
| PG13 | AF0 TRACED0; AF1 LPTIM1_OUT; AF2 HRTIM_EEV10; AF5 SPI6_SCK; AF7 USART6_CTS/USART6_NSS; AF11 ETH_MII_TXD0/ETH_RMII_TXD0; AF12 FMC_A24; AF14 LCD_R0 |
| PG14 | AF0 TRACED1; AF1 LPTIM1_ETR; AF5 SPI6_MOSI; AF7 USART6_TX; AF9 QUADSPI_BK2_IO3; AF11 ETH_MII_TXD1/ETH_RMII_TXD1; AF12 FMC_A25; AF14 LCD_B0 |
| PH0 |  |
| PH1 |  |

AF15 = EVENTOUT on every pin (omitted above). Pins PH0/PH1 (HSE), PC14/PC15 (LSE) have no digital AFs.

---

## 08. STM32H755ZIT6 — Digital communication peripherals and timers

Source: DS12919 Rev 3 sections 3.29-3.43 (and 3.15/3.16 external memory). Pin assignment: §07b. Which pins the Nucleo board already uses: §02–§04. Instance counts listed are for the whole device; availability on LQFP144 pins is limited by the AF matrix.

#### Serial / bus peripherals
| Peripheral | Instances | Key specs |
|---|---|---|
| **USART** | USART1, 2, 3, 6 | async, sync/SPI master-slave, Smartcard (ISO7816), IrDA SIR, LIN, RS485 driver-enable, hardware flow control (RTS/CTS), Modbus, auto-baud, receiver timeout, 7/8/9-bit, **16-byte Tx/Rx FIFOs** (FIFO off by default), up to **12.5 Mbit/s**, own kernel clock, wake from Stop (start bit / any frame / specific frame / FIFO status), DMA |
| **UART** | UART4, 5, 7, 8 | same as USART minus synchronous SPI mode and Smartcard |
| **LPUART** | LPUART1 | half-duplex single-wire, CTS/RTS, wake from Stop; **only a 32.768 kHz LSE clock needed for <= 9600 baud**; Tx/Rx FIFO; DMA. Arduino D0/D1 on the Nucleo are LPUART1 (PB7/PB6) |
| **I2C** | I2C1-4 | I2C rev 5 spec; Standard 100 kbit/s, Fast 400 kbit/s, **Fast-mode+ 1 Mbit/s (20 mA drive pins, FT_f)**; 7/10-bit addressing; controller/target, multi-controller; SMBus 2.0 (PEC, ARP, alert) and PMBus 1.1; programmable analog/digital filters; independent clock, wake from Stop on address match; 1-byte buffer + DMA |
| **SPI/I2S** | SPI1-6 (SPI1/2/3 muxed with I2S1/2/3) | up to **150 Mbit/s** master/slave, half/full duplex/simplex, 3-bit prescaler (8 master rates), 4-16 bit frames, NSS pulse mode, TI mode, HW CRC, 8x8-bit Tx/Rx FIFO (16x8-bit for I2S); I2S: master/slave, simplex/full duplex, 16/32-bit, **8-192 kHz**, MCLK = 256 x Fs. One SPI/I2S instance sits in the low-power (D3) domain. Extra SPIs: QUADSPI, and the 4 USARTs in sync mode |
| **SAI** | SAI1-4 (two sub-blocks each) | I2S, LSB/MSB-justified, PCM/DSP, TDM, AC'97, SPDIF-out; up to 192 kHz; PDM interface for up to 6 mics; master/slave; sync/async blocks |
| **SPDIFRX** | 1 (up to 4 inputs) | IEC-60958/61937, symbol rate up to 12.288 MHz, stereo 32-192 kHz, DMA |
| **SWPMI** | 1 | single-wire protocol master (SIM/UICC interface) |
| **MDIOS** | 1 | MDIO *slave* (PHY-like register file), wake from Stop |
| **SDMMC** | SDMMC1, SDMMC2 | MMC 4.51 (1/4/8-bit), SD 4.1 and SDIO 4.0 (1/4-bit), one card at a time, up to 125 MHz, internal DMA. Nucleo Zio D43-D48 = SDMMC1 (PC8-PC12, PD2) |
| **FDCAN** | FDCAN1, FDCAN2 | CAN 2.0A/B + **CAN FD**, FDCAN1 also TT-CAN (ISO 11898-4); **10 KB shared message RAM**; optional clock-calibration unit. No CAN transceiver on the Nucleo — add external. Pins: FDCAN1 PD0/PD1 (Zio D67/D66), also PA11/PA12 |
| **USB OTG** | OTG_HS1 (FS or HS with external ULPI PHY, 480 Mbit/s), OTG_FS2 | USB 2.0 + OTG 2.0, host/device/OTG, SRP/HNP, 9 bidirectional endpoints (incl. EP0), 16 host channels, 4 KB FIFO, LPM, BCD 1.2, internal FS PHY, crystal-less (HSI48), needs 48 MHz clock, external power switch required for host. Nucleo uses FS PHY on PA11/PA12 only |
| **Ethernet MAC** | 1 | IEEE 802.3-2002, **10/100 Mbit/s**, MII (17 sig.) or **RMII (9 sig.)**, external PHY required (Nucleo: LAN8742A, RMII), dedicated DMA, 2 KB Tx + 2 KB Rx FIFOs, VLAN tag, half/full duplex, multicast filtering, **IEEE 1588v2 PTP** hardware timestamp (compare linked to TIM2 input) |
| **HDMI-CEC** | 1 | CEC protocol, wake from Stop |
| **DCMI** | 1 | 8/10/12/14-bit parallel camera, up to 80 MHz pixel clock (105 MB/s @ 60 MHz), RGB565, YCbCr 4:2:2, JPEG, raw Bayer/mono, snapshot or continuous, crop |
| **LCD-TFT (LTDC)** | 1 | 24-bit RGB parallel, up to XGA 1024x768, 2 layers, CLUT 256x24, blending/alpha, color keying, AXI master |
| **JPEG codec** | 1 | HW JPEG encode/decode (ISO 10918-1), 8-bit/channel, single clock per pixel |

#### Timers (all counters freeze in debug)
| Timer | Count | Resolution | Channels | Notes |
|---|---|---|---|---|
| HRTIM1 | 1 | 6 timers (1 master + 5 slave), 480 MHz digital kernel, **2.1 ns resolution**, 10 outputs | 10 PWM outputs, 5 fault inputs, 10 external event inputs | dead-time, burst, push-pull, resonant mode, LLC/buck/boost/LED topologies |
| Advanced TIM1, TIM8 | 2 | 16-bit up/down | 4 (+complementary, 6 PWM outputs), dead time, break | three-phase motor PWM; DMA |
| TIM2, TIM5 | 2 | **32-bit** up/down | 4 | encoder/hall interface, DMA |
| TIM3, TIM4 | 2 | 16-bit up/down | 4 | encoder/hall, DMA |
| TIM12 | 1 | 16-bit up | 2 | |
| TIM13, TIM14 | 2 | 16-bit up | 1 | |
| TIM15 | 1 | 16-bit up | 2 (+1 complementary) | |
| TIM16, TIM17 | 2 | 16-bit up | 1 (+1 compl.) | |
| Basic TIM6, TIM7 | 2 | 16-bit up | 0 | DAC trigger / timebase, DMA |
| LPTIM1-5 | 5 | 16-bit up | pulse/PWM output | run in Stop (LSE/LSI/ext clock), wake-up, encoder mode |
| Watchdogs | 4 | IWDG1/2 (12-bit, 8-bit prescaler, 32 kHz LSI, runs in Stop/Standby, HW/SW enabled via option bytes), WWDG1/2 (7-bit, main clock, early-wake IRQ) | | one pair per domain |
| SysTick | 2 | 24-bit | | one per core |
All timer prescalers 16-bit (1-65536) except LPTIM (1/2/4/.../128). Max timer clock 240 MHz (480 MHz option; 150/300 MHz on extended-temperature suffix-3 parts). TIM1/8/2/3/4/5/15 etc. also usable as ADC triggers (TIM1,2,3,4,6,8,15, HRTIM1, LPTIM1).

#### RTC
Calendar with sub-seconds, 2 alarms, wakeup timer (17-bit), digital calibration (0.95 ppm), 3 tamper pins with filter, timestamp, 1-32767 pulse smooth correction; clock from LSE, LSI (not functional in VBAT then), or HSE/32. Runs in VBAT mode with LSE. 128 B backup registers, 4 KB backup SRAM.

---

## 09. STM32H755ZIT6 — Analog peripherals, crypto/security blocks

Source: DS12919 Rev 3 sections 3.17-3.28 and electrical sections (analog supply rules in §06). Analog pin names (ADCx_INPy etc.) are in §07a "Additional functions". On the Nucleo, VDDA/VREF+ default to VDD (R40 fitted); Zio A0-A5 = PA3, PC0, PC3_C, PB1, PC2_C/PB9, PF11/PB8; A6-A8 = PF6, PF10, PA2.

#### ADC (ADC1, ADC2, ADC3)
- Resolution selectable **16, 14, 12, 10, 8 bit**; up to 36 channels in the family (each ADC shares up to 20 external channels); up to **3.6 MSPS** (max resolution).
- Single-shot or scan mode; simultaneous and interleaved sample-and-hold between ADCs; DMA support; analog watchdog (interrupt when outside thresholds); triggers from TIM1/2/3/4/6/8/15, HRTIM1, LPTIM1.
- Internal channels: **temperature sensor on ADC3_IN18** (range -40..+140 C, needs VDDA 1.7-3.6 V; factory calibration values stored in read-only system memory), VREFINT (not detailed in datasheet text read). Need VDDA >= 1.62 V; Px_C pins (e.g. PC2_C, PC3_C on Zio A4/A2) are analog-only pads, max 1 mA I/O.
- TT_a pins (PA4, PA5, PA7) are 3.3 V-only; FT_a pins max 4 V when used as analog.
#### DAC
Two 12-bit buffered channels (DAC1_OUT1 = PA4, DAC1_OUT2 = PA5), 8/12-bit monotonic, left/right align, noise and triangle wave generation, dual-channel simultaneous/independent, DMA incl. underrun detection, triggers from timer updates (TIM6/7 etc.), reference VREF+ or internal VREFBUF; 1 MHz update. Note PA5 also drives LD1/Zio D13 (SB65) and SPI1_SCK on the Nucleo.
#### Comparators / op-amps / sigma-delta
- **COMP1, COMP2:** rail-to-rail, programmable reference (external I/O, DAC output, internal VREFINT or 1/4, 1/2, 3/4 of it), hysteresis, speed/power selectable, polarity, wake from Stop, timer break inputs, window mode.
- **OPAMP1, OPAMP2:** rail-to-rail, GBW up to 7.3 MHz, PGA non-inverting gain 2/4/8/16 or inverting -1/-3/-7/-15, follower mode, positive input can come from DAC, output routable to ADC, offset <= 1.5 mV, bias <= 1 nA, need VDDA >= 2.0 V.
- **DFSDM1:** 8 serial input channels (SPI / Manchester, clock up to 20 MHz, 10 MHz Manchester), 4 filters (Sinc order 1-5, oversampling 1-1024, integrator 1-256), up to **24-bit** output, PDM microphone to PCM, internal ADC/memory parallel inputs, analog watchdog, short-circuit detector, extremes detector, 16 triggers, DMA.
#### Crypto / security / misc
- **CRYP:** AES 128/192/256 (ECB, CBC, CTR, GCM, CCM), DES/TDES (ECB, CBC), DMA. **HASH:** MD5, SHA-1, SHA-2, HMAC. **RNG:** true RNG, 32-bit samples.
- Security features: RDP, PC-ROP, secure access mode, active tamper, secure firmware upgrade, 96-bit unique ID, CRC unit. (Memory-security detail in §05.)
- Temperature: also see electrical limits (junction max 125 C for suffix-6 parts such as STM32H755ZIT6).

---

## 10. NUCLEO-H755ZI-Q firmware handoff — default hardware state, pin usage, pitfalls

Audience: an AI/engineer writing firmware (HAL/LL/bare-metal, STM32CubeIDE/IAR/Keil) for **STM32H755ZIT6** on **NUCLEO-H755ZI-Q (MB1363)**. Everything here is from the supplied datasheet (DS12919 Rev 3) and board manual (UM2408 Rev 6); items I could not verify are in §99. Hardware detail is in §01–§09.

#### 1. Mandatory first lines (supply configuration)
Board default = **internal SMPS directly supplies VCORE (supply config 2)**. Firmware must select the same, before clock/PLL setup, in `SystemClock_Config()` (CM7 project):
```c
HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);   // default board
// HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);        // only if board reworked to LDO config 1
```
A mismatch = ST-LINK can no longer connect after reset (recovery procedure: §01, "Recovery from mismatch deadlock").
With SMPS-direct, **VOS0 is unavailable -> choose VOS1** (frequency/bus limits: §06 VOS table). Flash wait states depend on ACLK and VCORE range — use the RM0399 table (not in supplied docs); with HAL, derive from the `FLASH_LATENCY_x` passed to `HAL_RCC_ClockConfig`.

#### 2. Clock defaults
- HSE source = **8 MHz MCO from ST-LINK on PH0 (bypass mode)**, not a crystal (SB72 ON). Use `RCC_HSE_BYPASS` (digital bypass) with HSE_VALUE = 8 MHz. If the board is changed to the 25 MHz crystal X2 (SB3/SB4 ON, SB72/SB71 OFF) use `RCC_HSE_ON` and 25 MHz.
- LSE = 32.768 kHz crystal X3 (on-board). Reset system clock and HSI48 notes: §06. Bus/timer clock limits at VOS1: §06.
- USB needs 48 MHz exact (PLL or HSI48 + CRS). Ethernet RMII 50 MHz ref clock is supplied by the PHY on PA1.

#### 3. Pin/function defaults a firmware author must respect
| Function | Pins | Config notes |
|---|---|---|
| ST-LINK VCP (printf/console) | **USART3 TX=PD8, RX=PD9** (AF7) | default route; 115200 typical; USART3 kernel clock must be configured |
| User LEDs | LD1 PB0 (green), LD2 PE1 (yellow), LD3 PB14 (red) | push-pull, active HIGH |
| User button B1 | PC13 (blue), EXTI line 13; pull/polarity not stated in the manual — check schematic | pins default analog after reset: must enable GPIO clock and set as input |
| Reset button | NRST | |
| SWD | PA13 SWDIO, PA14 SWCLK, PB3 SWO | don't reconfigure; avoid GPIO use |
| USB FS device/host | PA11 DM, PA12 DP (AF10 OTG_FS), PA9 VBUS, PD10 power-enable (host), PG7 overcurrent | power board before plugging CN13 |
| Ethernet RMII | PA1 REF_CLK, PA2 MDIO, PC1 MDC, PA7 CRS_DV, PC4 RXD0, PC5 RXD1, PG11 TX_EN, PG13 TXD0, PB13 TXD1 (all AF11) | PHY LAN8742A, 10/100; PHY clock 50 MHz from PHY; set GPIO speed very high |
| Arduino UART D0/D1 | PB7 RX / PB6 TX = **LPUART1** (AF8) | not the VCP |
| Arduino I2C D14/D15 | PB9 SDA / PB8 SCL = I2C1 (AF4), 5 V-tolerant FT pins | external pull-ups needed (none specified on board) |
| Arduino SPI | PA5 SCK, PA6 MISO, PB5 MOSI (SPI1 AF5), PD14 CS (SB56 OFF/SB57 ON for D11 on H755) | D13 shared with LD1 only if SB65 ON |
| QSPI (Zio D26-D31) | PG6 NCS, PB2 CLK, PD11 IO0, PD12 IO1, PE2 IO2, PD13 IO3 | no flash fitted |
| FDCAN1 | PD0 RX, PD1 TX (Zio D67/D66) | needs external transceiver |
| TIM1 PWM (Zio D3-D6, D37-D42) | PE13 CH3, PE11 CH2, PA8 CH1, PE15 BKIN1, PE8 CH1N, PE10 CH2N, PE12 CH3N, PE7 ETR | |

Pin sharing traps (double-connected Zio signals and board-function conflicts): see §03. After reset GPIOs are **analog mode** (except SWD/JTAG pins) — enable RCC GPIO clocks and configure mode, speed, pull, AF.

#### 4. Dual-core (CM7 + CM4) notes
Core/domain/NVIC/MPU/boot facts and the CM4-cannot-access-ITCM/DTCM rule are in §05. Firmware-relevant additions:
- Peripherals in D1 (FMC/QSPI/LTDC/DMA2D/SDMMC1/JPEG/flash) are reached by CM4 through the D2-to-D1 AHB link (the ART accelerator speeds CM4 instruction fetch from D1 memories).
- A CPU can only use peripherals allocated to a powered domain; each core can enter CSleep/CStop independently; Stop/Standby is entered only when all domains agree.
- Inter-core signalling: 2 EXTI lines; the semaphore/shared-RAM scheme is in RM0399 (not supplied; see §99).

#### 5. Quick-start for hardware check (from manual)
1. JP2 on 1-2 (STLK), JP4 fitted, JP5 on 1-2 (3.3 V). 2. Install ST-LINK driver if Windows < 10. 3. USB cable to **CN1**: LD5 green and LD4 on, LD3 red blinks. 4. Press B1: blinking rate of LD1-LD3 changes. 5. Get STM32CubeH7 examples from st.com.
Toolchains: STM32CubeIDE, IAR EWARM (Windows), Keil MDK-ARM (Windows). Host: Windows 10, Linux 64-bit, macOS; USB Type-A/C to Micro-B cable.

Power-measurement caveats (JP4/IDD, Rev C01 vs D01, PA13/PB3/PD8 pulls): §01.

---

## 99. SUPPLEMENTARY — NOT from the supplied PDFs (unverified background knowledge)

Everything in §01–§10 comes from the three supplied documents (DS12919 Rev 3, UM2408 Rev 6, DB3171 Rev 20). **This file is separate on purpose**: it is general STM32H7 knowledge written from memory, **no web lookup was done**, and it must be verified against **RM0399** (STM32H745/755/747/757 reference manual) and the ST HAL headers before relying on it. If something here conflicts with §01–§10, trust 01-10.

- Base addresses (RM0399 memory map): ITCM 0x0000_0000 (64 KB), flash bank 1 0x0800_0000, bank 2 0x0810_0000, DTCM 0x2000_0000 (128 KB), AXI SRAM 0x2400_0000 (512 KB; datasheet confirms flash and AXI range), SRAM1/2/3 0x3000_0000 / 0x3002_0000 / 0x3004_0000, SRAM4 0x3800_0000, backup SRAM 0x3880_0000. Peripheral base addresses: see RM0399.
- Dual-core boot: option bytes (BCM7 / BCM4, a.k.a. BOOT_CM7 / BOOT_CM4 and BOOT_ADD0/1) control which core boots; CM4 normally waits in CStop until CM7 releases it; HSEM (hardware semaphores) are the standard inter-core lock; the RCC/PWR must be configured by CM7 first (CM7 owns SystemClock/supply setup). Typical Cube projects: `CM7/` and `CM4/` sub-projects with `HAL_HSEM_FastTake/Release` boot synchronisation and `HAL_PWREx_ClearPendingEvent`.
- Flash wait-state table (RM0399 "Number of wait states vs ACLK and VCORE range") needed for `FLASH_LATENCY_x`; for VOS1 at 200 MHz AXI it is typically 2 WS (programming delay 2). Verify.
- Cache/DMA: DMA cannot see CM7 D-cache; use non-cacheable MPU regions or clean/invalidate for DMA buffers; DTCM/ITCM are not reachable by DMA1/2 (MDMA only).
- Ethernet with LwIP: descriptors/buffers in SRAM1-3 (D2), non-cacheable.
- ST-LINK-V3E extras (SWV viewer, VCP bridge baud) are described in UM2448/ST-LINK-V3 documentation, not supplied.
- Software: STM32CubeH7 firmware package (HAL/LL/BSP + `Nucleo_H755ZI_Q` BSP examples), STM32CubeProgrammer, STM32CubeMX.
