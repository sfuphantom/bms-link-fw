# Handoff — Generated Project Config (BMS_Master, HalCoGen output)

**What this is:** Condensed, AI-oriented summary of the HalCoGen project for the BMS master controller (TMS570LS1227, Cortex-R4F, big-endian, TI compiler). Source files: `BMS_Master.dil` (HalCoGen project, ~10k key=value lines) + `include/*.h` (generated driver headers, TI HAL 04.07.01) + two hand-written headers (`vcu_can.h`, `charger_can.h`). **No `.c` files were provided**, so application logic, CAN signal layouts and pin-to-function wiring are NOT known.
**Companion doc:** `BMS_Master/` handoff set (TMS570 TRM register-level reference). Use it for register/bit details; this file only says *how this project is configured*.
**Reading rule:** values below are what the project configures. "Default" = HalCoGen default / untouched. Anything marked ⚠ is a mismatch or risk to check.

---

## 1. Clocks, memory, core (from .dil / system.h)
| Item | Value |
|---|---|
| Crystal / OSC | 16 MHz |
| PLL1 | ref div 6, mul 120, output div 2 → **160 MHz** (spread-spectrum off). PLL2 disabled |
| GCLK = HCLK | 160 MHz |
| VCLK1/2/3/4 | **80 MHz** (all peripheral buses; divider 1) |
| RTI1 clock | 80 MHz (PLL1 source) |
| Flash | 0x0000_0000, 0x140000 (1.25 MB), linked from 0x20; pipeline mode; wait states: address 1, data 3 (HCLK > 150 MHz). Banks 0,1,7 active, others sleep |
| Flash ECC | **disabled** in project (`FLASH_ECC_ENABLE=0`) ⚠ |
| RAM | 0x0800_0000, 0x30000 (192 KB), RAM ECC **enabled**. Stacks occupy first 0x1500 bytes (USER 0x1000; SVC/FIQ/IRQ/ABORT/UNDEF 0x100 each); link base 0x0800_1500 |
| Core options on | MPU (+background region), VFP, PMU, IRQ via VIC, VIM parity |
| FreeRTOS/OS | **disabled** (`OS_ENABLE=0`) → bare-metal super-loop/ISR design |
| Toolchain | TI compiler only |

Drivers **enabled** in HalCoGen: CAN, SCI, MibSPI, SPI, I2C, GIO, ADC, RTI, ESM, VIM, HET, ePWM, eCAP, eQEP, DMA, CRC, DCC, FEE, EMAC, PMM, PINMUX, AJSM.
**Disabled:** LIN, SCI-LIN (config exists, off), EMIF, DMM, RTP, POM, OS.

## 2. Interrupts (VIM)
Enabled channels (name → type): ESM high (0, **FIQ**), phantom (1, FIQ; 30, IRQ), RTI compare0–3 (2–5, IRQ), RTI overflow0 (6), GIO high (9), GIO low (23), **CAN1 high (16)**, **CAN1 low (29)**, N2HET1 low (24), **eCAP6 (109)**. All others (incl. SPI, SCI, I2C, ADC, CAN2/3) are **not** enabled at the VIM level. Phantom ISR = `phantomInterrupt`.
⚠ SCI RX interrupt and SPI1 RX interrupt are enabled inside their modules, but no VIM channel for them is enabled → those interrupts will never reach the CPU unless enabled in code.

## 3. RTI (system tick / scheduling)
| Compare | Counter | Period | VIM ch |
|---|---|---|---|
| 0 | Counter0 (40 MHz) | **1 ms** (40000) | 2 |
| 1 | Counter0 | **5 ms** (200000) | 3 |
| 2 | Counter1 (10 MHz) | **8 ms** (80000) | 4 |
| 3 | Counter1 | **10 ms** (100000) | 5 |

Callbacks are weak `rtiNotification(uint32 notification)` (notification = compare index via `rtiNOTIFICATION_COMPAREn`). Call `rtiEnableNotification()` / `rtiStartCounter(rtiCOUNTER_BLOCK0/1)` to run them. Watchdog (DWD/DWWD) API exists (`dwdInit`, `dwwdInit`, `dwdReset`) — not evidenced as configured.

## 4. CAN (the communication core of this project)
All 3 DCAN nodes: **500 kbit/s**, CAN 2.0 (no FD). Bit timing identical: BRP=19 (tq = 250 ns @ 80 MHz), 8 tq/bit = Sync 1 + Prop 3 + Phase 2 (+ phase1 split as configured), SJW 2, **sample point 75 %**. RAM parity on. Auto-bus-on OFF (bus-off needs manual recovery). Pins in functional (CAN) mode.

| Node | Base | ID mode (HalCoGen) | Message boxes configured |
|---|---|---|---|
| **CAN1** (`canREG1`) | 0xFFF7DC00 | **11-bit**, mask 0x7FF | boxes 1,2,3,4,5,6,10 (below) |
| CAN2 (`canREG2`) | 0xFFF7DE00 | **29-bit** (Xtd), mask 0x1FFFFFFF | **none** (all `ENA=0`) |
| CAN3 (`canREG3`) | 0xFFF7E000 | 29-bit | none |

**CAN1 message boxes** (all DLC 8, RTR 0, EOB 0): IDs are HalCoGen placeholders (= box number) — real IDs must be set in code via `canUpdateID()`.
| Box | Dir | Arb ID (placeholder) | RX interrupt |
|---|---|---|---|
| 1 | **TX** | 1 | – |
| 2 | RX | 2 | yes, int line 0 |
| 3 | **TX** | 3 | – |
| 4 | RX | 4 | yes, line 0 |
| 5 | RX | 5 | yes, line 0 |
| 6 | **TX** | 6 | – |
| 10 | RX | 10 | yes, line 0 |

Line 0 → VIM ch 16 (`can1HighLevelInterrupt`). Receive callback: `canMessageNotification(canBASE_t *node, uint32 messageBox)`. Error/status callbacks: `canErrorNotification`, `canStatusChangeNotification`.

### Hand-written VCU interface — `vcu_can.h` (ACTIVE)
```c
#define VCU_CAN_NODE canREG2          // VCU link is on CAN2
#define BMS_FAULT canMESSAGE_BOX1
#define BMS_IMD   canMESSAGE_BOX2
#define BMS_HV    canMESSAGE_BOX3
typedef struct { uint8_t bms_fault; uint8_t imd_fault; uint8_t hv_active; } BMSStatusFlags_t;
void VCU_TransmitStatus(const BMSStatusFlags_t *f);   // BMS -> VCU status broadcast
```
Meaning: BMS sends three status flags (BMS fault, insulation-monitor/IMD fault, HV-active) to the vehicle control unit, one message box each (1/2/3) on **CAN2**. Frame IDs/bytes are not in the headers (implemented in the missing `.c`).

### `charger_can.h` — fully COMMENTED OUT (inactive; design intent only)
Planned charger link on CAN: TX box 1 (command), RX box 2 (status). Command: max voltage and max current in **0.1 V / 0.1 A units** (`dV`, `dA`, uint16) + `charge_enable`. Status: output voltage/current (same units) + flags bitmask: bit0 HW fault, bit1 over-temp, bit2 input fault, bit3 no battery, bit4 comm timeout. API sketched: `Charger_SendCmd`, `Charger_GetStatus`, `Charger_Update`. Treat as not implemented. File dated Feb 2026 vs vcu_can Apr 2026.

### ⚠ CAN gotchas specific to this project
1. **VCU uses CAN2 but HalCoGen only generated CAN1.** Only `can1GetConfigValue` exists and no CAN2 boxes are enabled, so `canInit()` will not set up CAN2 message objects (IDs/direction/TX validity). CAN2 boxes 1–3 must be configured in code or enabled in HalCoGen first. CAN2 is also set to 29-bit IDs while CAN1 is 11-bit.
2. For 11-bit IDs the DCAN arbitration field holds the ID in bits [28:18] (`id << 18`); HalCoGen handles this for configured boxes, manual `canUpdateID()` calls must do it themselves (see TRM `05_DCAN_CAN.md`).
3. `canTransmit(node, box, data8)` sends 8 bytes from the box's configured ID; `canGetData()` reads a received frame; `canIsTxMessagePending()` / `canIsRxMessageArrived()` for polling.

### CAN API summary
`canInit()`, `canTransmit(node,box,uint8*)`, `canGetData(node,box,uint8*)`, `canIsTxMessagePending`, `canIsRxMessageArrived`, `canIsMessageBoxValid`, `canGetLastError`, `canGetErrorLevel` (returns `canLEVEL_ACTIVE 0x00 / PASSIVE 0x20 / WARNING 0x40 / BUS_OFF 0x80 / PARITY_ERR 0x100`), `canEnable/DisableErrorNotification`, `canEnable/DisableStatusChangeNotification`, `canEnableloopback(node, type)`, `canUpdateID(node,box,arbVal)`, `canGetID`, `canSendRemoteFrame`, `canFillMessageObjectData`, `canIoSetDirection/SetPort`. Box constants `canMESSAGE_BOX1..64`.

## 5. SPI / MibSPI
- Generated-code scope: `mibspi5`, `spi1`, `spi3` (only these have `GetConfigValue` functions). Only **MibSPI5** has `ENABLE=1` in the .dil.
- **MibSPI5** (`mibspiREG5`, 0xFFF7FC00): **master**, clock-mod 1, 1 Mbit/s (prescale 79 @ 80 MHz), 16-bit words, SPI mode 0 (polarity 0, phase 0), MSB first (shift dir 0), no parity, no wait-delay, CS default idle 0xFF (all high). Transfer group 0 = **8 buffers**, mode 4, trigger disabled (software trigger via `mibspiTransfer`); TG1–7 length 0. All interrupts off (polling via `mibspiIsTransferComplete`). RAM at 0xFF0A0000.
- **SPI1** (`spiREG1`, 0xFFF7F400): configured as **slave** (MASTER=0), clock-mod 0, 500 kbit/s nominal (prescale 159), fmt0 16-bit, fmt1 8-bit, mode 0, **RX interrupt enabled** (no VIM channel – see §2).
- **SPI3** (`spiREG3`, 0xFFF7F800): **master**, 500 kbit/s fmt0 (prescale 159), **8-bit** words, **polarity 1 / phase 0 (SPI mode 2)** on fmt0; fmt1 1 Mbit/s 16-bit; fmt3 8-bit. Polling mode.
- SPI2/4/5: default (1 Mbit/s, 16-bit, mode 0). Which SPI talks to which chip is not in the files.
- API: `spiTransmitData / spiReceiveData / spiTransmitAndReceiveData(spi, spiDAT1_t*, blocksize, uint16* buf)` (blocking), `spiSendData / spiGetData / spiSendAndGetData` (interrupt/non-blocking). `spiDAT1_t { boolean CS_HOLD; boolean WDEL; SPIDATAFMT_t DFSEL (SPI_FMT_0..3); uint8 CSNR }`; chip-select values `SPI_CS_0=0xFE … SPI_CS_7=0x7F, SPI_CS_NONE=0xFF` (active-low one-hot). MibSPI: `mibspiSetData(m,group,uint16*)`, `mibspiTransfer(m,group)`, `mibspiIsTransferComplete`, `mibspiGetData(m,group,uint16*)`. Note word buffers are `uint16` even for 8-bit formats.

## 6. SCI (UART) / LIN
- **SCI** (`sciREG`, 0xFFF7E500): **115200 baud, 8 data bits, 1 stop, no parity** (actual 116279, ~0.9 % error), async, **RX interrupt enabled** in module (VIM not enabled ⚠), TX interrupt off. Use for debug/console or serial link.
- `scilinREG` (0xFFF7E400) has same settings but driver disabled; LIN disabled.
- API: `sciInit`, `sciSend(sci,len,uint8*)`, `sciSendByte`, `sciReceive(sci,len,uint8*)` (blocking), `sciReceiveByte`, `sciIsRxReady`, `sciIsTxReady`, `sciRxError`, `sciSetBaudrate`, `sciEnableNotification(sci, SCI_RX_INT=0x200 | SCI_TX_INT=0x100 | SCI_FE_INT/OE/PE…)`, callback `sciNotification(sci, flags)`.

## 7. I2C
`i2cREG1` 0xFFF7D400. **Master, transmitter, 7-bit addressing, 100 kHz** (prescale 9, ICCL=ICCH=35), 8-bit data, count 8, STOP condition generated, no interrupts, no DMA. API: `i2cSetSlaveAdd`, `i2cSetCount`, `i2cSetMode`, `i2cSetDirection`, `i2cSetStart/Stop`, `i2cSend`, `i2cReceive`, `i2cIsTxReady/RxReady/StopDetected`, `i2cClearSCD`. Configure direction/mode before each transfer.

## 8. GIO
Ports: `gioPORTA` (0xFFF7BC34), `gioPORTB` (0xFFF7BC54), both enabled. Non-default bits (Port A): **A2** input, interrupt enabled, low-level VIM line (ch 23); **A5** input, interrupt enabled, high-level VIM line (ch 9), pull-up; **A6** input, interrupt enabled, low-level line; **A7** output (pull disabled). All other bits default (input, no interrupt). Likely external fault/enable/interlock signals but the wiring is not in the files. Callback: `gioNotification(gioPORT_t *port, uint32 bit)`. API: `gioSetBit/GetBit/ToggleBit/SetPort/GetPort/SetDirection/EnableNotification`.

## 9. Other peripherals
- **ADC1/ADC2** (0xFFF7C000 / 0xFFF7C200): 12-bit, prescale 7, groups 0–2 all with **no channels selected** (`PINS=0`), software/event trigger, polling. Effectively **unused so far**. API: `adcStartConversion(adc,group)`, `adcIsConversionComplete`, `adcGetData(adc,group,adcData_t*)` (`{ id, value }`), `adcCalibration`, `adcMidPointCalibration`.
- **eCAP6**: continuous capture, CAP2 falling edge (others rising), counter reset at CAP3, wrap after event 3, interrupt on capture event 3 (VIM 109) → pulse period/duty measurement input. eCAP1–5 default (PWM-mode 1 kHz period, 50 % compare). Callback: `ecapNotification`.
- **ePWM1–7**: all enabled, default timebase (period reg 79 / 1000 µs nominal, compare ~40–50 %), no ADC SOC, no dead-band wiring evidenced. ePWM1–3 differ from 4–7 only trivially (clock-div 1). Treat as unconfigured.
- **eQEP1/2, N2HET1/2, DMA (32 ch), CRC, DCC1/2, EMAC**: enabled in HalCoGen with defaults. N2HET1 low-level interrupt enabled (VIM 24); N2HET program not in the project files. ESM: all channels default-disabled except the high-level ESM FIQ (VIM 0) and error pin handling via `esmInit()`; callbacks `esmGroup1Notification/esmGroup2Notification`.
- **PMM:** all extra power domains disabled/default.

## 10. Pin multiplexing (raw, unresolved)
10 mux groups deviate from option 0: `PIN_MUX_3, 17, 18, 19, 20, 25, 28, 30, 41` = option **1**; `PIN_MUX_35` = option **4**; `PIN_MUX_105` = 1. Pin/ball names are NOT in the files (headers only give `PINMMRn` bit shifts), so identifying physical pins needs HalCoGen's Pinmux tab or the datasheet. Pin mux is applied by `pinmuxInit()` after IOMM kick unlock (see TRM `11_IOMM_PinMux.md`).

## 11. FEE (flash EEPROM emulation, non-volatile storage)
Uses **flash bank 7** (0xF0200000, 64 KB EEP area), polling mode, device error detect ON, flash ECC handling `TI_Fee_None`, checksum ON, write-counter save ON, virtual page 8 B, block overhead 24 B, sector overhead 16 B, max blocking time 600. **One block configured**: block number 1, size **8 bytes**, offset 16, 1 dataset, immediate data, write cycles 0x8. API: `TI_Fee_Init()`, `TI_Fee_WriteAsync(blockNumber, uint8*)`, `TI_Fee_ReadSync(blockNumber, offset, uint8*, len)`, `TI_Fee_MainFunction()` (must be called periodically), `TI_Fee_GetStatus`.
⚠ Mismatches: `ti_fee_cfg.h` says 2 virtual sectors and `CHECK_BANK7_ACCESS=STD_ON`; the .dil says 4 virtual sectors and `CHECK_BANK7_ACCESS=STD_OFF`. Regenerate code in HalCoGen so they agree. Also FEE_FLASH_ECC_ENABLE=0 in the system tab while FEE ECC option is STD_ON.

## 12. Standard HAL headers (no project-specific content)
`reg_*.h` = register structs/base macros (same data as the TRM handoff), `sys_*.h`/`system.h`/`sys_core.h` = startup, VIM, MPU, PMU, DMA, selftest, PCR helpers, `hal_stdtypes.h`/`Device_*.h`/`sys_common.h` = types (`uint8/16/32`, `boolean`), `std_nhet.h` = N2HET instruction macros, `emac.h/mdio.h/phy_dp83640.h` = Ethernet stack (enabled but unused evidence), `errata_SSWF021_45*.h` = PLL-lock erratum workaround, `crc.h`, `dcc.h`, `esm.h`, `ti_fee*.h`. No custom logic there — skip unless asked.

## 13. Open questions the next AI should ask the user
1. CAN2 setup: is it intended to be initialized by HalCoGen (enable boxes 1–3 as TX, set 11- or 29-bit) or manually in code? What are the frame IDs/byte layout for BMS_FAULT/IMD/HV?
2. CAN1 purpose (box IDs 1–6, 10 are placeholders): which devices/frames (cell monitors? charger?) and real IDs?
3. Which SPI/MibSPI connects to which IC (MibSPI5 master 1 MHz mode 0; SPI3 master mode 2 8-bit)? SPI1 as slave to whom?
4. Is flash ECC intentionally off? Should SCI/SPI RX interrupts be enabled in VIM?
5. Which ADC channels and eCAP/GIO signals are used (none are selected/wired in the files)?
