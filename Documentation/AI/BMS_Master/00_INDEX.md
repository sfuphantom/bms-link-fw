# Handoff Index — TMS570LS12x/11x Technical Reference (BMS_Master.pdf, 2030 pp.)
**What this is:** Condensed, AI-oriented extracts of a 16/32-bit Cortex-R4F safety MCU reference manual (Hercules TMS570LS12x/11x). Manufacturer/package/marketing content removed; register addresses, bit fields, formulas, init sequences and gotchas kept. All base addresses are 32-bit, **big-endian** MCU. Peripheral clocks must be enabled (`SYS.CLKCNTL.PENA=1`) and pins muxed (IOMM) before any peripheral works.

## Read order for a typical embedded task
| File | Read when you need… |
|---|---|
| `01_System_Memory_Clocks.md` | memory map, base addresses, PLL/clock domains, RAM init, reset causes |
| `11_IOMM_PinMux.md` | pin function selection, kick unlock, SPI/SCI/I2C pin muxing |
| `02_RTI_Watchdog.md` | periodic tick, watchdog |
| `03_VIM_ESM_Interrupts_Errors.md` | interrupts, ISR vectoring, error pin/ESM |
| `10_GIO.md` | GPIO |
| `05_DCAN_CAN.md` | CAN bus (3 controllers) |
| `06_MibSPI_SPI.md` | SPI (5 modules) incl. multi-buffer mode |
| `07_SCI_LIN_UART.md` | UART (SCI), LIN |
| `04_I2C.md` | I2C |
| `08_ADC.md` | analog inputs (2 × 12-bit) |
| `12_DMA.md` | DMA channels / requests |
| `09_N2HET_HTU.md` | N2HET timer, PWM, capture (HTU only mentioned) |
| `13_Flash_and_Safety_Overview.md` | flash wait states/ECC; short overview of safety modules |
| `14_ePWM.md` | PWM generation, dead-band, trip-zone, ADC triggering |
| `15_eCAP.md` | input capture / period+duty measurement / single APWM |
| `16_eQEP.md` | quadrature encoder position/speed |
| `17_HTU.md` | N2HET→RAM transfer unit |
| `18_Safety_Modules_Detail.md` | register-level CCM, STC (LBIST), PBIST, DCC, CRC |
| `19_TCRAM_PMM.md` | RAM ECC wrapper and power domains (which peripherals live in PD3/PD5) |
| `20_POM_Overlay.md` | flash→RAM calibration overlay |
| `21_EMIF.md` | external SDRAM/async memory |
| `22_EMAC_MDIO.md` | Ethernet MAC/MDIO, descriptors, init |
| `23_FlexRay.md` | FlexRay controller + FTU (structural) |

## Depth levels
- **Full register-level:** 01–08, 10–12, 14–22.
- **Structural/medium:** 09 (N2HET: instruction opcodes not in manual), 23 (FlexRay: states, access flow, register map; protocol config field values not expanded), 13 (flash moderate, rest overview).
- **Whole manual now covered**; a few sub-topics inside files are summarized (noted in each file).

## Known caveats
- Peripheral-to-VIM-channel mapping, DMA request-line mapping, ESM channel list, pin/ball names, flash wait-state-vs-HCLK table and max clocks are in the **device datasheet**, not this TRM.
- Figures were interpreted from the surrounding text; verify timing diagrams on hardware.
