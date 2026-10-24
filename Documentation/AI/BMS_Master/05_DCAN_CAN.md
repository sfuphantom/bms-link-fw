# DCAN (CAN 2.0A/B) — TMS570LS12x/11x
**Context:** 3 identical CAN controllers (DCAN1/2/3). ISO 11898-1, CAN 2.0 A/B (no CAN-FD), up to 1 Mbit/s. Needs an external CAN transceiver on CAN_TX/CAN_RX. 64 message objects per module (numbers 1–64 / 0x01–0x40). Message objects live in Message RAM and are reachable ONLY via interface register sets IF1/IF2 (IF3 = read-only observation) while the module is running.

## Base addresses (32-bit regs)
DCAN1 `0xFFF7DC00` · DCAN2 `0xFFF7DE00` · DCAN3 `0xFFF7E000`

## Register map (offset from base)
| Off | Reg | Purpose |
|---|---|---|
| 00 | CTL | control |
| 04 | ES | error/status (read clears some flags) |
| 08 | ERRC | TX/RX error counters |
| 0C | BTR | bit timing (writable only if Init=1 AND CCE=1) |
| 10 | INT | pending interrupt IDs (Int0ID [15:0], Int1ID [23:16]) |
| 14 | TEST | test modes (silent, loopback) |
| 1C | PERR | parity error code |
| 80 | ABOTR | auto-bus-on timer |
| 84/88–94 | TXRQX / TXRQ12,34,56,78 | transmit-request flags (1 bit/object) |
| 98/9C–A8 | NWDATX / NWDAT12..78 | new-data flags |
| AC/B0–BC | INTPNDX / INTPND12..78 | interrupt-pending flags |
| C0/C4–D0 | MSGVALX / MSGVAL12..78 | message-valid flags |
| D8–E4 | INTMUX12..78 | per-object routing to int line 0 or 1 |
| 100/104/108/10C/110/114 | IF1CMD/MSK/ARB/MCTL/DATA(A)/DATB | interface set 1 |
| 120/124/128/12C/130/134 | IF2CMD/MSK/ARB/MCTL/DATA(A)/DATB | interface set 2 |
| 140–154 | IF3OBS/MSK/ARB/MCTL/DATA/DATB | IF3 (auto-updated observation/DMA path) |
| 160–16C | IF3UPD12..78 | IF3 update enable |
| 1E0 / 1E4 | TIOC / RIOC | CAN_TX / CAN_RX pin as GIO (Func,Dir,Out,In,OD,PU,PD) |

## Clocking
- Two clocks: **VCLK** (module/bus) and **VCLKA** (async, becomes CAN_CLK for bit timing; source picked in system module "Peripheral Asynchronous Clock Source" register).
- If FMPLL spread-spectrum is on, do NOT feed VCLKA from the PLL — use OSCIN-derived clock.
- Must hold: VCLK ≥ CAN_CLK. CAN_CLK ≥ 8 MHz for 1 Mbit/s. Bit time must be 8–25 tq.

## Bit timing (BTR @0x0C)
Fields (all stored as value−1 except BRPE): `BRPE[19:16]` prescaler ext, `TSeg2[14:12]`, `TSeg1[11:8]`, `SJW[7:6]`, `BRP[5:0]`.
- tq = (BRPE:BRP + 1) / CAN_CLK  (prescaler range 1–1024)
- Bit time = Sync(1) + Prop + PhaseSeg1 + PhaseSeg2 = (TSeg1+1)+(TSeg2+1)+1 tq
- Program: `TSeg1 = Prop+Phase1−1`, `TSeg2 = Phase2−1`, `SJW = SJW−1`. TSeg1 valid 1–15, TSeg2 0–7, SJW 0–3. SJW = min(4, Phase1) recommended. Phase2 ≥ node info-processing time (0–2 tq).
- Reset value 0x2301 = 500 kbit/s @ 8 MHz CAN_CLK. (Check: 8 MHz/2 = 4 MHz tq → 8 tq/bit = 500k.)
- If sample point differs across nodes, pick the config with highest oscillator tolerance.

## Init sequence
1. (Optional) hardware-zero Message RAM via system-module MINITGCR + MSINENA (clears RAM + parity bits).
2. Config bit timing: `CTL.Init=1` → `CTL.CCE=1` → wait until Init reads 1 → write BTR → clear CCE, then Init → wait until Init reads 0. (Bad BTR values can prevent leaving init.) After reset module is already in Init=1, bus-off also forces Init=1.
3. Pins: ensure muxing (IOMM) sets CAN_TX/RX functional; TIOC/RIOC.Func are forced to 1 (functional) when Init=0. External pull-up on CAN_TX recommended so bus isn't disturbed during reset.
4. Configure message objects through IF1/IF2 (below). All objects deactivated (MsgVal=0) after reset — reconfigure while MsgVal=0 or via IF write.
5. Enable interrupts as needed (`CTL.IE0/IE1`, `SIE`, `EIE`), clear `Init`.

## CTL (0x00) bits
`WUBA`25 wake on bus activity · `PDR`24 local power-down req · `DE3/DE2/DE1`20/19/18 DMA enable per IF · `IE1`17 int line 1 · `InitDbg`16 · `SWR`15 SW reset (set Init first, then SWR) · `PMD`[13:10] parity (0x5=disabled, others=enabled) · `ABO`9 auto bus-on · `IDS`8 debug mode interrupt tx/rx · `Test`7 · `CCE`6 · `DAR`5 disable auto-retransmit · `EIE`3 error ints (PER,BOff,EWarn) · `SIE`2 status-change ints (WakeUpPnd,RxOK,TxOK,LEC) · `IE0`1 int line 0 · `Init`0 (reset=1).
- Bus-off: module sets Init=1. Clearing Init starts recovery: waits for 129×11 recessive bits, then resets counters. Each 11-recessive sequence writes LEC=Bit0 (monitor progress). Auto-Bus-On (ABO=1 + ABOTR 32-bit timer) restarts automatically.

## ES (0x04) status
`PDA`10 · `WakeUpPnd`9 · `PER`8 · `BOff`7 · `EWarn`6 (counter ≥96) · `EPass`5 (error-passive; no interrupt) · `RxOK`4 · `TxOK`3 · `LEC`[2:0]: 0 none, 1 stuff, 2 form, 3 ack, 4 bit1, 5 bit0, 6 CRC, 7 no-event (set after ES read).
- **Reading ES clears WakeUpPnd, PER, RxOK, TxOK and sets LEC=7**, and clears the status interrupt (Int0ID=0x8000). Debug/suspend disables this auto-clear.

## Message object (accessed via IFx)
IFxARB: `MsgVal`31 · `Xtd`30 (1=29-bit) · `Dir`29 (**1=transmit, 0=receive**) · `ID[28:0]`. **11-bit IDs go in ID[28:18]** (i.e. write `id<<18`); ID[17:0] ignored/zero.
IFxMSK: `MXtd`31 · `MDir`30 · `Msk[28:0]` (1 = must match, 0 = don't care; reset all 1s). Mask used only if `UMask`=1 in MCTL.
IFxMCTL: `NewDat`15 · `MsgLst`14 · `IntPnd`13 · `UMask`12 · `TxIE`11 · `RxIE`10 · `RmtEn`9 · `TxRqst`8 · `EoB`7 (1=single object or last of FIFO) · `DLC`[3:0].
IFxDATA = data bytes 0–3, IFxDATB = bytes 4–7.

### IFxCMD (0x100 / 0x120) — transfer trigger
Bits: `WR_RD`23 (1=IFx→RAM write, 0=RAM→IFx read) · `Mask`22 · `Arb`21 · `Control`20 · `ClrIntPnd`19 · `TxRqst/NewDat`18 · `DataA`17 · `DataB`16 · `Busy`15 (RO) · `DMAActive`14 · `MsgNum`[7:0] (1–0x40; out-of-range wraps modulo — don't rely on it).
- **Writing MsgNum starts the transfer** (so write all other bits first, or in the same 32-bit write). Busy=1 for 4–14 VBUS cycles; IFx regs are write-protected while Busy. Poll Busy=0 before touching IFx again.
- Read with ClrIntPnd=1 / TxRqst-NewDat=1 clears IntPnd / NewDat atomically (MCTL returned shows state before clearing).
- If CMD.TxRqst/NewDat=1 on a write, the object's TxRqst is set regardless of MCTL.

### Configure TX object (data frames)
MsgVal=1, Dir=1, EoB=1, ID/Xtd per app, DLC+data per app, TxIE per app, RmtEn=1 only if it should auto-answer remote frames, UMask=0 if no remote frames allowed. Set TxRqst only after data valid (can do in same write via CMD.TxRqst bit). Write cmd with WR_RD=1, Arb|Control|DataA|DataB (+Mask if used).
Update a TX object: write new data with Control=0 or with TxRqst; priority between TX objects = lower message number wins.

### Configure RX object (data frames)
MsgVal=1, Dir=0, EoB=1, ID/Xtd (acceptance ID), Mask+UMask=1 for group filtering, DLC, RxIE per app, NewDat=0, TxRqst=0 (setting TxRqst on an RX object sends a **remote frame** with that ID).
- With a "don't care" mask, ARB ID bits get overwritten by the received frame's ID.
- Read RX: write CMD (read, Control|Arb|DataA|DataB, ClrIntPnd=1, TxRqst/NewDat=1) with MsgNum → check `MCTL.NewDat`, `MsgLst` (overrun), then take DLC/data.
- **FIFO**: give several RX objects identical ID/mask; set EoB=0 on all except the highest-numbered one (EoB=1). Messages fill lowest number first.
- Acceptance filtering scans objects in ascending number; first match wins.

## Interrupts
- Two lines: DCAN0INT (IE0) and DCAN1INT (IE1). Message-object ints routable to either via INTMUX (1 bit per object). **Status-change (SIE) and error (EIE) ints go only to line 0.**
- INT register: Int0ID = 0x8000 means status/error pending (highest priority); 1–0x40 = object number (lower number = higher priority). Cleared by clearing object IntPnd (use CMD.ClrIntPnd) or reading ES.
- Object ints require IntPnd plus TxIE (after successful TX) or RxIE (after successful RX).

## Misc features
- **Auto retransmission** on by default (DAR=0). **Auto-bus-on** via ABOTR.
- **Power-down:** local (PDR→PDA ack; WUBA wakes on dominant bit, but the waking frame is lost) and global. 
- **Test modes (TEST reg, Test=1 in CTL):** Silent, Loopback, External loopback, Loopback+Silent, direct TX pin control, direct Message RAM access.
- **Parity** on Message RAM (PMD field; PERR reg shows location; PER in ES).
- **DMA:** DE1/2/3 + IFxCMD.DMAActive for IF1/IF2/IF3 transfers.
- CAN_TX/CAN_RX usable as GPIO via TIOC/RIOC when Func=0 (only writable while Init=1).

## Gotchas
- BTR and TIOC/RIOC writes only work while Init=1 (BTR also needs CCE=1).
- Don't clear Init to "restart" quickly from bus-off — recovery sequence cannot be shortened.
- ES read has side effects (don't poll it casually in debugger or ISR twice).
- Message RAM is not CPU-accessible during operation except through IFx.
