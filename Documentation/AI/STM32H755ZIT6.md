# STM32H755ZIT6 / NUCLEO-H755ZI-Q — Combined Handoff

Context for another AI or a new conversation. Scope: the **STM32H755ZIT6** (LQFP144) and the **NUCLEO-H755ZI-Q** (MB1363) board.

**Sources:** STM32H755xI datasheet (DS12919 Rev 3), NUCLEO-H755ZI-Q user manual (UM2408 Rev 6).

**Intentionally removed:** package mechanical drawings, ordering codes for other packages, BGA pinout tables, full timing diagrams (only key specs kept), marketing text, and anything outside the STM32H755ZIT6 / NUCLEO-H755ZI-Q scope.

## Contents
- [01. Core Architecture](#01-core-architecture)
- [02. Memory and Boot](#02-memory-and-boot)
- [03. Power and Low-Power](#03-power-and-low-power)
- [04. Clock and Reset](#04-clock-and-reset)
- [05. Peripherals: Timers, Communication, Analog](#05-peripherals-timers-communication-analog)
- [06. Pinout and Alternate Functions](#06-pinout-and-alternate-functions)
- [07. Electrical Characteristics](#07-electrical-characteristics)
- [08. Package and Ordering](#08-package-and-ordering)
- [09. Board-Specific: NUCLEO-H755ZI-Q](#09-board-specific-nucleo-h755zi-q)

---

## 01. Core Architecture

**Target Device:** STM32H755ZIT6 (LQFP144)
**Board:** NUCLEO-H755ZI-Q (MB1363)

#### Dual-Core Subsystem

| Feature | Cortex-M7 (CPU1) | Cortex-M4 (CPU2) |
| :--- | :--- | :--- |
| Core | Arm Cortex-M7 with double-precision FPU | Arm Cortex-M4 with FPU |
| Max Frequency | 480 MHz (VOS0) / 400 MHz (VOS1) / 300 MHz (VOS2) / 200 MHz (VOS3) | 240 MHz (VOS0) / 200 MHz (VOS1) / 150 MHz (VOS2) / 100 MHz (VOS3) |
| Performance | 1027 DMIPS @ 480 MHz (2.14 DMIPS/MHz) | 300 DMIPS @ 240 MHz (1.25 DMIPS/MHz) |
| Cache | L1: 16 KB I-cache + 16 KB D-cache | ART Accelerator (instruction cache) |
| TCM | 64 KB ITCM + 128 KB DTCM (0 wait state) | — |
| Pipeline | 6-stage dual-issue | 3-stage with branch prediction |
| DSP Instructions | Yes | Yes |
| MPU | Yes (up to 16 regions) | Yes (up to 16 regions) |

**MPU:** Each core has its own MPU. Regions can be 32 bytes to 4 GB, with up to 8 subregions per region. Unauthorized access triggers a memory management exception.

**Power Domains:** D1 (Cortex-M7 + high-perf peripherals), D2 (Cortex-M4 + most peripherals), D3 (reset/clock/power control). Each can be independently clock-gated or switched off.

---

## 02. Memory and Boot

#### Embedded Flash
- Size: 2 MB, dual-bank (2 × 1 MB)
- Organization per bank: 8 sectors × 128 KB, 128 KB system flash, 2 KB option bytes
- Word size: 266-bit (256-bit data + 10 ECC bits)
- Read-while-write support
- Endurance: 10k cycles (Tj = -40 to +125 °C)
- Data retention: 30 years @ 85 °C (1k cycles), 20 years @ 55 °C (10k cycles)

#### SRAM (Total ~1 MB)
| Region | Size | Domain | Notes |
| :--- | :--- | :--- | :--- |
| AXI SRAM | 512 KB | D1 | Mapped on AXI bus |
| SRAM1 | 128 KB | D2 | |
| SRAM2 | 128 KB | D2 | |
| SRAM3 | 32 KB | D2 | |
| SRAM4 | 64 KB | D3 | |
| Backup SRAM | 4 KB | VBAT | Retained in Standby / VBAT |
| ITCM RAM | 64 KB | — | 0 wait state, Cortex-M7 only |
| DTCM RAM | 128 KB | — | 2 × 64 KB, 0 wait state |

**ECC:** SECDED algorithm. 7 ECC bits per 32-bit word; 8 ECC bits per 64-bit word (AXI-SRAM and ITCM).

#### External Memory Interfaces (LQFP144)
- **FMC:** Supports SRAM, PSRAM, NOR/NAND, SDRAM/LPSDR. Up to 32-bit data bus. Max synchronous clock = FMC kernel clock / 2.
- **QUADSPI:** Up to 133 MHz (SDR), 100 MHz (DDR). Single/dual/quad SPI flash. Memory-mapped mode up to 256 MB. XIP supported.

#### Boot Modes
Both cores boot simultaneously by default. Can boot one core while clock-gating the other via option bytes. Boot memory space selected by BOOT pin and `BOOT_ADDx` option bytes (0x0000_0000 to 0x3FFF_FFFF).

Can boot from flash, system memory, or SRAM. ITCM/DTCM not accessible by Cortex-M4.

---

## 03. Power and Low-Power

#### Power Supply Scheme (from Figure 13)
| Rail | Voltage Range | Purpose |
| :--- | :--- | :--- |
| VDD | 1.62–3.6 V | I/Os, main supply |
| VDDLDO | 1.62–3.6 V | Internal LDO input (tied to VDD if not available on package) |
| VDDSMPS | 1.62–3.6 V | SMPS step-down input (must equal VDD) |
| VDDA | 1.62–3.6 V | Analog (ADC, DAC, COMP, OPAMP) |
| VDD33USB | 3.0–3.6 V (USB used) / 0–3.6 V (not used) | USB domain |
| VBAT | 1.2–3.6 V | RTC, backup registers, backup SRAM |
| VCAP | 1.0 / 1.1 / 1.2 / 1.35 V (VOS dependent) | Core domain (D1/D2/D3) |

#### Voltage Regulator (SMPS + LDO)
- **SMPS step-down converter** (~85–90% typical efficiency) can directly supply VCORE or feed the LDO. Requires: 2.2 µH inductor, 4.7 µF input cap, 10 µF output cap.
- **LDO** supplies VCORE when SMPS is not used or as intermediate stage.
- **Run-mode voltage scaling:** VOS0–VOS3 (per-VOS core frequencies: see §01 core table). VOS0 only with LDO.
- **Stop-mode scaling:** SVOS3 (peripherals with wakeup active), SVOS4/SVOS5 (limited wakeup).

#### Low-Power Modes
| Mode | CPU | System Clock | Wakeup Sources | Notes |
| :--- | :--- | :--- | :--- | :--- |
| CSleep | Stopped | Running | Any interrupt | — |
| CStop | Stopped | Stopped | EXTI, peripheral | — |
| DStop | Stopped | Stopped | Domain-specific | — |
| Stop | Stopped | Stopped | EXTI, LPUART, LPTIM, RTC, etc. | ~2.95 µA (Standby, Backup SRAM off, RTC/LSE on) |
| Standby | Powered down | Powered down | WKUP pins, RTC, tamper | See datasheet tables |
| VBAT | — | — | RTC, tamper | ~1.2 µA (RTC on, no backup SRAM) |

**Wakeup times (Stop mode):** 4.4 µs (VOS3, HSI, flash normal) to 68 µs (VOS5, CSI, flash low-power). Standby wakeup: ~390 µs.

#### Reset and Power Control
- **POR/PDR:** Fixed thresholds (~1.62 V rising, ~1.58 V falling).
- **BOR:** 4 levels (BOR0 ~1.67 V, BOR1 ~2.10 V, BOR2 ~2.41 V, BOR3 ~2.70 V), configurable via option bytes.
- **PVD:** 7 levels (1.96 V to 2.86 V).
- **NRST pin:** Internal weak pull-up (30–50 kΩ). External 0.1 µF cap recommended.

---

## 04. Clock and Reset

#### Clock Sources
| Source | Frequency | Type | Notes |
| :--- | :--- | :--- | :--- |
| HSI | 64 MHz | Internal RC | Default system clock after reset |
| HSI48 | 48 MHz | Internal RC | USB, RNG, SDMMC |
| CSI | 4 MHz | Internal RC | Low-power, fast startup |
| LSI | 32 kHz | Internal RC | RTC, IWDG, LPTIM |
| HSE | 4–50 MHz (ext) / 4–48 MHz (crystal) | External | Main high-speed clock |
| LSE | 32.768 kHz | External crystal | RTC, low-power modes |

**PLLs:** 3 × PLL (1 system, 2 kernel). Fractional mode supported. VCO range: 192–960 MHz (wide), 150–420 MHz (medium). Lock time ~50 µs (normal).

#### Clock Tree (simplified)
- SYSCLK â† PLL1 / HSE / HSI / CSI
- CPU1 (M7) â† SYSCLK (max 480 MHz)
- CPU2 (M4) â† SYSCLK (max 240 MHz)
- AXI/AHB â† SYSCLK (max 240 MHz)
- APB1/APB2 â† max 120 MHz; APB3/APB4 â† max 240 MHz
- Peripheral kernel clocks â† PLL2/PLL3, HSI48, LSE, etc.

#### Reset Sources
- Power-on reset (POR)
- Brownout reset (BOR)
- NRST pin (external)
- Independent watchdogs (IWDG1, IWDG2)
- Window watchdogs (WWDG1, WWDG2)
- Software reset
- Low-power mode security reset
- Exit from Standby

---

## 05. Peripherals: Timers, Communication, Analog

#### Timers (22 total)
| Type | Instances | Key Features |
| :--- | :--- | :--- |
| HRTIM1 | 1 | 16-bit, 480 MHz, 10 outputs, 5 fault inputs, 6 timers (1 master + 5 slaves). Digital power / LLC / buck-boost. |
| Advanced-control | TIM1, TIM8 | 16-bit, 240 MHz, 4 ch, complementary PWM with dead-time, 3-phase generator. |
| General-purpose | TIM2, TIM3, TIM4, TIM5 | 32-bit (TIM2/5) or 16-bit (TIM3/4), 4 ch each. Encoder interface. |
| General-purpose | TIM12–TIM17 | 16-bit, 1–2 ch. |
| Basic | TIM6, TIM7 | 16-bit, DAC trigger, DMA. |
| Low-power | LPTIM1–LPTIM5 | 16-bit, runs in Stop mode (LSE/LSI), wakeup capability. |
| Watchdogs | IWDG1/2, WWDG1/2 | Independent (32 kHz RC) and window (main clock). |
| SysTick | 2 (one per core) | 24-bit downcounter. |

#### Communication Peripherals
| Interface | Instances | Max Speed / Notes |
| :--- | :--- | :--- |
| I2C | 4 (I2C1–I2C4) | 1 Mbit/s (Fm+), SMBus/PMBus, wakeup from Stop |
| USART / UART | 4 USART + 4 UART + 1 LPUART | Up to 12.5 Mbit/s; USARTs support ISO7816, LIN, IrDA, SPI mode |
| SPI / I2S | 6 SPI (SPI1–SPI6) + 3 I2S | SPI up to 150 Mbit/s (master); I2S up to 192 kHz |
| SAI | 4 (SAI1–SAI4) | I2S, LSB/MSB, PCM/DSP, TDM, AC'97; up to 192 kHz |
| SPDIFRX | 1 | 4 inputs, up to 12.288 MHz symbol rate |
| SWPMI | 1 | Single-wire protocol master, up to 2 Mbit/s |
| MDIO | 1 slave | 32 registers, wakeup from Stop |
| SDMMC | 2 (SDMMC1, SDMMC2) | SD 4.1, MMC 4.51, up to 125 MHz |
| FDCAN | FDCAN1 (TT-FDCAN), FDCAN2 | CAN FD, ISO 11898-1. 10 KB shared message RAM |
| USB OTG | OTG_HS1 (480 Mbit/s), OTG_HS2 (12 Mbit/s) | OTG_HS1 supports ULPI (external PHY) |
| Ethernet | 1 MAC | 10/100 Mbit/s, MII/RMII, IEEE 1588 PTP |
| HDMI-CEC | 1 | Consumer Electronics Control |

#### Analog Peripherals
| Peripheral | Instances | Key Specs |
| :--- | :--- | :--- |
| ADC | 3 (ADC1, ADC2, ADC3) | 16-bit, up to 3.6 MSPS, up to 36 ch; direct/fast/slow channel types |
| DAC | 2 ch (12-bit) | 1 MHz, buffered/unbuffered, noise/triangle generation |
| COMP | 2 (COMP1, COMP2) | Rail-to-rail, programmable hysteresis, wakeup from Stop |
| OPAMP | 2 (OPAMP1, OPAMP2) | PGA gain 2/4/8/16 (non-inv) or -1/-3/-7/-15 (inv); 7.3 MHz GBW |
| Temperature Sensor | 1 | Connected to ADC3_IN18; -40 to +140 °C |
| VREFBUF | 1 | 1.5 V / 1.8 V / 2.048 V / 2.5 V output |
| DFSDM | 1 (4 filters, 8 ch) | Sigma-delta modulator interface, up to 24-bit output |

#### Graphics
- **LTDC:** Up to XGA (1024×768), 2 layers, 24-bit RGB.
- **Chrom-ART (DMA2D):** Bit-blitting, rectangle fill/copy, pixel format conversion.
- **JPEG Codec:** Hardware encode/decode, up to 4 quantization tables.

#### Security and Crypto
- AES (128/192/256), TDES, HASH (MD5, SHA-1, SHA-2), HMAC, RNG (true random).
- Secure Access Mode: user-defined secure services, executed after reset. Cortex-M7 only.
- ROP/PCROP, active tamper, secure firmware upgrade.

---

## 06. Pinout and Alternate Functions

**Package:** LQFP144 (20 × 20 mm, 0.5 mm pitch)
**I/Os:** Up to 97 (LQFP144; family up to 168 on larger packages)

#### Key Pin Groups (LQFP144)
| Pin(s) | Function | Notes |
| :--- | :--- | :--- |
| PA13/PA14 | SWDIO/SWCLK | Debug (also JTMS/JTCK) |
| PA15, PB3, PB4 | JTDI, JTDO/TRACESWO, NJTRST | JTAG |
| BOOT0 | Boot mode select | |
| NRST | Reset | Active low |
| PDR_ON | Power-down reset enable | Tie to VSS for internal reset off |
| VBAT | Backup supply | 1.2–3.6 V |
| VCAP | Core voltage | 2.2 µF external cap (×2) |
| VLXSMPS, VFBSMPS, VDDSMPS | SMPS step-down converter | Inductor + caps required |
| VDDLDO | LDO input | Tied to VDD if not present |
| VDD33USB, VDD50USB | USB power | |
| VREF+, VREF- | ADC reference | VREF- only on BGA; on LQFP144 VREF- tied to VSSA |
| PA0–PA15, PB0–PB15, PC0–PC15, PD0–PD15, PE0–PE15, PF0–PF15, PG0–PG15, PH0–PH15, PI0–PI11 | GPIO / AF | See datasheet Table 9 for full mapping |

#### Important Alternate Functions (selected)
| Pin | AF0 | AF1 | AF2 | AF3 | AF4 | AF5 | AF6 | AF7 | AF8 | AF9 | AF10 | AF11 | AF12 | AF13 | AF14 | AF15 |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| PA0 | — | TIM2_CH1 | TIM5_CH1 | TIM8_ETR | TIM15_BKIN | USART2_CTS | UART4_TX | SDMMC2_CMD | SAI2_SD_B | ETH_MII_CRS | — | — | — | — | — | EVENTOUT |
| PA1 | — | TIM2_CH2 | TIM5_CH2 | LPTIM3_OUT | TIM15_CH1N | USART2_RTS | UART4_RX | QUADSPI_BK1_IO3 | SAI2_MCLK_B | ETH_RMII_REF_CLK | — | — | — | LCD_R2 | — | EVENTOUT |
| PA2 | — | TIM2_CH3 | TIM5_CH3 | LPTIM4_OUT | TIM15_CH1 | USART2_TX | SAI2_SCK_B | ETH_MDIO | MDIOS_MDIO | — | — | — | — | LCD_R1 | — | EVENTOUT |
| PA8 | MCO1 | TIM1_CH1 | HRTIM_CHB2 | TIM8_BKIN2 | I2C3_SCL | USART1_CK | OTG_FS_SOF | UART7_RX | — | — | — | — | — | LCD_B3 | — | EVENTOUT |
| PA9 | — | TIM1_CH2 | HRTIM_CHC1 | LPUART1_TX | I2C3_SMBA | SPI2_SCK/I2S2_CK | USART1_TX | ETH_TX_ER | DCMI_D0 | LCD_R5 | — | — | — | — | — | EVENTOUT |
| PA10 | — | TIM1_CH3 | HRTIM_CHC2 | LPUART1_RX | USART1_RX | OTG_FS_ID | MDIOS_MDIO | LCD_B4 | DCMI_D1 | LCD_B1 | — | — | — | — | — | EVENTOUT |
| PA11 | — | TIM1_CH4 | HRTIM_CHD1 | LPUART1_CTS | SPI2_NSS/I2S2_WS | UART4_RX | USART1_CTS | FDCAN1_RX | OTG_FS_DM | LCD_R4 | — | — | — | — | — | EVENTOUT |
| PA12 | — | TIM1_ETR | HRTIM_CHD2 | LPUART1_RTS | SPI2_SCK/I2S2_CK | UART4_TX | USART1_RTS | SAI2_FS_B | FDCAN1_TX | OTG_FS_DP | LCD_R5 | — | — | — | — | EVENTOUT |
| PB6 | — | TIM16_CH1N | TIM4_CH1 | HRTIM_EEV8 | I2C1_SCL | I2C4_SCL | USART1_TX | LPUART1_TX | FDCAN2_TX | QUADSPI_BK1_NCS | DFSDM1_DATIN5 | FMC_SDNE1 | DCMI_D5 | UART5_TX | — | EVENTOUT |
| PB7 | — | TIM17_CH1N | TIM4_CH2 | HRTIM_EEV9 | I2C1_SDA | I2C4_SDA | USART1_RX | LPUART1_RX | DFSDM1_CKIN5 | FMC_NL | DCMI_VSYNC | — | — | — | — | EVENTOUT |
| PB10 | — | TIM2_CH3 | HRTIM_SCOUT | LPTIM2_IN1 | I2C2_SCL | SPI2_SCK/I2S2_CK | DFSDM1_DATIN7 | USART3_TX | QUADSPI_BK1_NCS | OTG_HS_ULPI_D3 | ETH_MII_RX_ER | LCD_G4 | — | — | — | EVENTOUT |
| PB11 | — | TIM2_CH4 | HRTIM_SCIN | LPTIM2_ETR | I2C2_SDA | DFSDM1_CKIN7 | USART3_RX | OTG_HS_ULPI_D4 | ETH_MII_TX_EN | LCD_G5 | — | — | — | — | — | EVENTOUT |

#### NUCLEO-H755ZI-Q Board Pin Mapping (UM2408)
##### Arduino / ST Zio (CN9, CN10)
| Arduino Pin | STM32 Pin | Function |
| :--- | :--- | :--- |
| D0 (RX) | PB7 | LPUART1_RX |
| D1 (TX) | PB6 | LPUART1_TX |
| D2 | PG14 | GPIO |
| D3 (PWM) | PE13 | TIM1_CH3 |
| D4 | PE14 | GPIO |
| D5 (PWM) | PE11 | TIM1_CH2 |
| D6 (PWM) | PA8 | TIM1_CH1 |
| D7 | PG12 | GPIO |
| D8 | PG9 | SMPS_EN |
| D9 (PWM) | PC6 | TIM8_CH1 |
| D10 (PWM) | PA15 | TIM2_CH1 |
| D11 | PB5 | — |
| D12 | PA6 | — |
| D13 | PA5 | — |
| D14 (SDA) | PB9 | I2C1_SDA |
| D15 (SCL) | PB8 | I2C1_SCL |
| A0 | PA3 | ADC12_INP15 |
| A1 | PC0 | ADC123_INP10 |
| A2 | PC3 | ADC12_INP13 |
| A3 | PF3 | ADC3_INP5 |
| A4 | PF5 | ADC3_INP4 |
| A5 | PF10 | ADC3_INP6 |

##### ST Morpho Connector (CN11, CN12) — Key Pins
| Morpho Pin | STM32 Pin | Notes |
| :--- | :--- | :--- |
| CN11-1 | PC10 | |
| CN11-3 | PC12 | |
| CN11-5 | 3V3_VDD | Power |
| CN11-7 | BOOT0 | Boot select |
| CN11-9 | PF6 | |
| CN11-11 | PF7 | |
| CN11-13 | PA13 | SWDIO |
| CN11-15 | PA14 | SWCLK |
| CN11-17 | PA15 | |
| CN11-21 | PB7 | LPUART1_RX |
| CN11-23 | PC13 | USER_BUTTON |
| CN12-1 | PC9 | |
| CN12-3 | PB8 | |
| CN12-5 | PB9 | |
| CN12-7 | VREFP | Analog reference |
| CN12-9 | GND | Ground |
| CN12-11 | PA5 | |
| CN12-13 | PA6 | |
| CN12-15 | PA7 | |
| CN12-17 | PB6 | LPUART1_TX |
| CN12-21 | PA9 | |
| CN12-23 | PA8 | |

(LEDs, buttons, USB OTG FS and Ethernet RMII pin assignments: see §09 On-Board Peripherals.)

---

## 07. Electrical Characteristics

#### Absolute Maximum Ratings
| Symbol | Parameter | Min | Max | Unit |
| :--- | :--- | :--- | :--- | :--- |
| VDDX | External main supply (VDD, VDDLDO, VDDSMPS, VDDA, VDD33USB, VBAT) | -0.3 | 4.0 | V |
| VIN (FT) | Input voltage on FT_xxx pins | VSS-0.3 | Min(VDD, VDDA, VDD33USB, VBAT) + 4.0, 6 V | V |
| VIN (TT) | Input voltage on TT_xx pins | VSS-0.3 | 4.0 | V |
| VIN (BOOT0) | Input voltage on BOOT0 | VSS-0.3 | 9.0 | V |
| Σ IVDD | Total current into VDD lines | — | 620 | mA |
| Σ IVSS | Total current out of VSS lines | — | 620 | mA |
| I10 | Output current sunk by any I/O | — | 20 | mA |
| Σ I(PIN) | Total output current (all I/Os) | — | 140 | mA |
| TJ | Max junction temperature | — | 125 (industrial) / 140 (extended) | °C |
| TSTG | Storage temperature | -65 | +150 | °C |

#### Operating Conditions
(VDD range 1.62–3.6 V: see §03 supply table.)

| Symbol | Parameter | Conditions | Min | Typ | Max | Unit |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| VDDA | Analog operating voltage | ADC/COMP used | 1.62 | — | 3.6 | V |
| VDDA | Analog operating voltage | DAC used | 1.8 | — | 3.6 | V |
| VDDA | Analog operating voltage | OPAMP used | 2.0 | — | 3.6 | V |
| fCPU1 (M7) | Max frequency | VOS0 | — | — | 480 | MHz |
| fCPU2 (M4) | Max frequency | VOS0 | — | — | 240 | MHz |
| fHCLK | AHB clock | VOS0 | — | — | 240 | MHz |
| fACLK | AXI clock | VOS0 | — | — | 240 | MHz |

#### I/O Characteristics
| Symbol | Parameter | Conditions | Min | Typ | Max | Unit |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| VIL | Input low (except BOOT0) | 1.62 < VDD < 3.6 V | — | — | 0.3VDD | V |
| VIH | Input high (except BOOT0) | 1.62 < VDD < 3.6 V | 0.7VDD | — | — | V |
| VHYS | Input hysteresis | 1.62 < VDD < 3.6 V | — | 250 | — | mV |
| RPU | Weak pull-up | VIN = VSS | 30 | 40 | 50 | kΩ |
| RPD | Weak pull-down | VIN = VDD | 30 | 40 | 50 | kΩ |
| C_IO | I/O pin capacitance | — | — | 5 | — | pF |
| VOL | Output low (I/O = 8 mA) | 2.7 < VDD < 3.6 V | — | — | 0.4 | V |
| VOH | Output high (I/O = 8 mA) | 2.7 < VDD < 3.6 V | VDD-0.4 | — | — | V |

#### ADC Characteristics (16-bit)
| Symbol | Parameter | Conditions | Min | Typ | Max | Unit |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| VDDA | Analog supply | — | 1.62 | — | 3.6 | V |
| VREF+ | Positive reference | — | 1.62 | — | VDD | V |
| fADC | ADC clock | BOOST=11 | 0.12 | — | 50 | MHz |
| fS | Sampling rate (16-bit) | Direct channel | — | — | 3.6 | MSPS |
| ET | Total unadjusted error | Direct ch, single-ended | — | +10/-20 | — | LSB |
| ENOB | Effective number of bits | Single-ended | — | 12.2 | — | bits |
| SINAD | Signal-to-noise + distortion | Single-ended | — | 75.2 | — | dB |

#### DAC Characteristics (12-bit)
| Symbol | Parameter | Conditions | Min | Typ | Max | Unit |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| VDDA | Analog supply | — | 1.8 | 3.3 | 3.6 | V |
| VREF+ | Positive reference | — | 1.8 | — | VDDA | V |
| DNL | Differential non-linearity | Buffer ON | -2 | — | 2 | LSB |
| INL | Integral non-linearity | Buffer ON, RL ≥ 5 kΩ | -4 | — | 4 | LSB |
| Offset | Offset at code 0x800 | Buffer ON, VREF+=3.6 V | — | — | ±15 | LSB |
| SNR | Signal-to-noise ratio | Buffer ON, 1 kHz | — | 67.8 | — | dB |
| THD | Total harmonic distortion | Buffer ON, 1 kHz | — | -78.6 | — | dB |

#### SMPS Step-Down Converter
| Parameter | Conditions | Min | Typ | Max | Unit |
| :--- | :--- | :--- | :--- | :--- | :--- |
| VDDSMPS | VOUT = 1.8 V | 2.3 | — | 3.6 | V |
| VOUT | Iout = 600 mA (2.5 V setting) | 2.25 | 2.5 | 2.75 | V |
| VOUT | Iout = 600 mA (1.8 V setting) | 1.62 | 1.8 | 1.98 | V |
| IOUT | Internal + external | — | — | 600 | mA |
| Efficiency | Typical, Run mode | — | ~85–90 | — | % |
| Switching freq | — | — | 2.4 ±10% | — | MHz |

---

## 08. Package and Ordering

#### Package Information (LQFP144)
| Parameter | Value |
| :--- | :--- |
| Package | LQFP144 (Low-profile Quad Flat Package) |
| Body Size | 20 × 20 mm |
| Pitch | 0.5 mm |
| Lead Count | 144 |
| Height (A) | 1.60 mm max |
| Lead Width (b) | 0.17–0.27 mm |
| Lead Length (L) | 0.45–0.75 mm |
| Footprint | 22.6 × 22.6 mm (including leads) |
| Thermal ΘJA | 43.7 °C/W |
| Thermal ΘJC | 11.3 °C/W |
| Thermal ΘJB | 38.3 °C/W |

#### Ordering Information
**Part Number: STM32H755ZIT6**
- STM32 = Arm-based 32-bit MCU
- H = High performance
- 755 = STM32H7x5 (dual-core, crypto)
- Z = 144 pins
- I = 2 MB flash
- T = LQFP ECOPACK2
- 6 = Temperature -40 to +85 °C (industrial)
- A = Advanced mission profile (VOS1, 480 MHz) — if present

**NUCLEO-H755ZI-Q**
- Order Code: NUCLEO-H755ZI-Q
- Board Reference: MB1363
- Target MCU: STM32H755ZIT6 (LQFP144)
- User Manual: UM2408

#### Device Marking
- Refer to TN1433 for marking location and orientation.
- Parts marked "ES" or "E" are engineering samples — not for production.

---

## 09. Board-Specific: NUCLEO-H755ZI-Q

#### Board Overview
- Board Reference: MB1363
- Target MCU: STM32H755ZIT6 (LQFP144)
- Debugger: STLINK-V3E (on-board)
- Power: Flexible (ST-LINK USB VBus, external VIN 7–11 V, 5V_EXT, USB charger, 3V3_EXT)
- SMPS: External SMPS for VCORE (1.8 V / 3.3 V selectable via JP5); internal SMPS also available
- Clocks: 32.768 kHz LSE crystal on board; HSE can come from STLINK MCO (8 MHz) or external crystal
- Expandability: ST Zio (Arduino Uno V3 compatible) + ST Morpho headers

#### Power Configuration
| Jumper / Bridge | Function | Default |
| :--- | :--- | :--- |
| JP5 | VCORE voltage selection (1.8 V / 3.3 V) | 3.3 V |
| JP4 (IDD) | Current measurement | ON |
| JP8 | USB FS PWR EN | ON |
| JP7 | PA2 (Ethernet MDIO) | OFF (disconnect for GPIO) |
| SB72 (MCO) | STLINK MCO to PF0/PH0 | ON |
| SB80, SB84, SB63, SB48, SB46, SB36 | Ethernet PHY disconnect | OFF |

#### On-Board Peripherals
| Component | Connection | Notes |
| :--- | :--- | :--- |
| LED_GREEN | PB0 | Active high |
| LED_BLUE | PB7 | Active high |
| LED_RED | PB14 | Active high |
| USER_BUTTON | PC13 | Active low, internal pull-up |
| RESET_BUTTON | NRST | — |
| USB OTG FS | PA11 (DM), PA12 (DP), PD10 (PWR_EN) | Micro-AB (CN13) |
| Ethernet (RMII) | PA1 (REF_CLK), PA2 (MDIO), PA7 (CRS_DV), PB13 (TXD1), PC1 (MDC), PC4 (RXD0), PC5 (RXD1), PG11 (TX_EN), PG13 (TXD0) | RJ45 (only on H755ZI-Q) |
| Virtual COM Port | PD8 (USART3_TX), PD9 (USART3_RX) | Via STLINK-V3E |
| SWD Debug | PA13 (SWDIO), PA14 (SWCLK) | STLINK-V3E |

#### Board Limitations
- IDD measurement in Standby mode is not possible (per UM2408 product history).
- Some Ethernet pins (PA1, PA2, PC1, PC4, PC5, PG11, PG13) are used as GPIOs on ST Morpho unless solder bridges are reconfigured.

#### Quick Reference — Arduino Header Mapping
See §06 (NUCLEO-H755ZI-Q Board Pin Mapping → Arduino / ST Zio).
