# EMAC + MDIO (10/100 Ethernet MAC) — TMS570LS12x/11x
**Context:** IEEE 802.3 10/100 Mbit MAC with MII or RMII to an external PHY, DMA bus master with 8 TX / 8 RX channels, 8 KB local descriptor RAM (CPPI RAM), statistics counters, MDIO master for PHY management. Not used in a typical BMS. Lives in power domain PD5 (see PMM). Clock: logic on **VCLKA4**; MDIO clock = VCLK3 ÷ divider (≤2.5 MHz, typically 1 MHz). PHY supplies TX/RX clocks (MII: 2.5/25 MHz; **RMII: 50 MHz ref clock for both speeds**).

## Base addresses
EMAC module `0xFCF78000` · EMAC control module `0xFCF78800` · MDIO `0xFCF78900` · CPPI descriptor RAM 8 KB (start = EMAC RAM base in memory map, 2K×32; descriptors are 16 B → up to 512 packets).

## Pin muxing (IOMM, one-hot byte per signal; set the listed value in that PINMMR byte)
**MDIO:** MDIO_CLK = PINMMR7[15:8] = 0x04 · MDIO_D = PINMMR8[15:8] = 0x04.
**MII / RMII** (PINMMR byte; first value MII, second RMII):
TXCLK PINMMR17[7:0]=0x02 (MII only) · TXD3 14[7:0]=0x04 · TXD2 13[31:24]=0x04 · TXD1 13[15:8]=0x04 / **0x08** · TXD0 13[7:0]=0x04 / **0x08** · TXEN 13[23:16]=0x04 / **0x08** · COL 20[23:16]=0x04 · CRS(DV) 17[23:16]=0x02 / **0x04** · RXCLK / RMII 50 MHz clk 14[15:8]=0x02 / **0x04** · RXD3 12[31:24]=0x04 · RXD2 12[23:16]=0x04 · RXD1 12[7:0]=0x02 / **0x04** · RXD0 11[31:24]=0x04 / **0x08** · RXDV 19[15:8]=0x02 · RXER 10[7:0]=0x02 / **0x04**. (Unlock IOMM kick registers first.) RMII needs only 7 signals + ref clock; MII/RMII selection also in IOMM (Ethernet mode select).

## Descriptor format (4 words, 32-bit aligned, in CPPI RAM or other RAM)
| Word | TX | RX |
|---|---|---|
| 0 | Next descriptor pointer (0 = end of queue) | same |
| 1 | Buffer pointer (byte-aligned) | same |
| 2 | `[31:16]` buffer offset · `[15:0]` buffer length | same (length = free space; EMAC writes actual) |
| 3 | `[31:16]` flags · `[15:0]` packet length (on SOP descriptor) | flags + packet length |
TX flags (word 3 bits): `SOP`31, `EOP`30, **`OWNER`29** (1 = owned by EMAC), `EOQ`28, `TDOWNCMPLT`27, `PASSCRC`26 (software supplies CRC).
RX flags: SOP/EOP/OWNER/EOQ/TDOWNCMPLT/PASSCRC plus status: Jabber, Oversize, Fragment, Undersized, Control, Overrun, CodeError, AlignError, CRCError, NoMatch.
C struct: `{pNext, pBuffer, BufOffLen, PktFlgLen}`; `EMAC_DSC_FLAG_SOP 0x80000000, EOP 0x40000000, OWNER 0x20000000, EOQ 0x10000000, TDOWNCMPLT 0x08000000, PASSCRC 0x04000000`.
- Buffer offset only checked on SOP descriptor; legal 0…(length−1). Packet length on SOP descriptor = whole packet length; fragments chained via next pointer.
- **Queues:** software chains descriptors; EMAC clears OWNER on completion; sets **EOQ** on last descriptor when it hits NULL next → if software appended a packet after EMAC already read NULL next, the channel halts and software must restart it by writing HDP (detect EOQ on returned descriptor).
- **TX start:** fill descriptor(s) (OWNER|SOP|EOP, lengths) → write head pointer to `TXnHDP` (n = channel). Completion: ISR reads `TXnCP` / TXINTSTATMASKED, walks list until OWNER=1/EOQ, then acknowledges by writing the completed descriptor address to `TXnCP`. Interrupts are level; clear by writing CP (and EOI via `MACEOIVECTOR`). **RX:** prepare empty buffer descriptors (OWNER=1) and write head to `RXnHDP`; on completion walk list, process, ack via `RXnCP`, re-queue buffers.
- Descriptor memory in EMAC CPPI RAM gives best throughput.

## Register map highlights
**Control module (0xFCF78800):** 00 REVID · 04 SOFTRESET · 0C INTCONTROL (pacing) · 10 C0RXTHRESHEN · 14 C0RXEN · 18 C0TXEN · 1C C0MISCEN · 40 C0RXTHRESHSTAT · 44 C0RXSTAT · 48 C0TXSTAT · 4C C0MISCSTAT · 70 C0RXIMAX · 74 C0TXIMAX (interrupts per ms pacing).
**MDIO (0xFCF78900):** 00 REVID · **04 CONTROL** (`IDLE`31, **`ENABLE`30**, `HIGHEST_USER_CHANNEL`[28:24], `PREAMBLE`20 (1 = disable preamble), `FAULT`19, `FAULTENB`18, **`CLKDIV`[15:0]** → MDIO_clk = VCLK3/(CLKDIV+1): e.g. 50 MHz → CLKDIV 49–50 for ~1 MHz) · 08 ALIVE · 0C LINK · 10/14 LINKINT RAW/MASKED · 20–2C USERINT RAW/MASKED/MASKSET/MASKCLR · **80 USERACCESS0** (`GO`31, `WRITE`30, `ACK`29, `REGADR`[25:21], `PHYADR`[20:16], `DATA`[15:0]) · 84 USERPHYSEL0 (`LINKSEL`7, `LINKINTENB`6, `PHYADRMON`[4:0]) · 88/8C USERACCESS1/USERPHYSEL1.
**EMAC (0xFCF78000):** 00 TXREVID · 04 TXCONTROL (`TXEN`[0]) · 08 TXTEARDOWN · 10 RXREVID · 14 RXCONTROL (`RXEN`[0]) · 18 RXTEARDOWN · 80–8C TX int stat raw/masked/mask set/clear · 90 MACINVECTOR · 94 MACEOIVECTOR · A0–AC RX int regs · B0–BC MAC int regs (HOSTMASK, STATMASK) · **100 RXMBPENABLE** (`RXPASSCRC`30, `RXQOSEN`29, `RXNOCHAIN`28, `RXCMFEN`24, `RXCSFEN`23, `RXCEFEN`22, `RXCAFEN`21 promiscuous; broadcast/multicast enables + channel select fields below) · 104/108 RXUNICASTSET/CLEAR · 10C RXMAXLEN · 110 RXBUFFEROFFSET · 114 RXFILTERLOWTHRESH · 120–13C RXnFLOWTHRESH · 140–15C RXnFREEBUFFER · **160 MACCONTROL** · 164 MACSTATUS · 168 EMCONTROL · 16C FIFOCONTROL · 170 MACCONFIG · 174 SOFTRESET · 1D0/1D4 MACSRCADDRLO/HI · 1D8/1DC MACHASH1/2 · 1E8 RXPAUSE · 1EC TXPAUSE · 200–2xx statistics (RXGOODFRAMES 200, RXBCASTFRAMES 204, …) · **500/504/508 MACADDRLO/HI/INDEX** · **600–61C TXnHDP · 620–63C RXnHDP · 640–65C TXnCP · 660–67C RXnCP**.
**MACCONTROL (160h) bits:** `RMIISPEED`15 (1 = 100 Mbit RMII) · `RXOFFLENBLOCK`14 · `RXOWNERSHIP`13 · `CMDIDLE`11 · `TXSHORTGAPEN`10 · `TXPTYPE`9 (0 round-robin, 1 fixed priority) · `TXPACE`6 · **`GMIIEN`5 (must be 1 for any TX/RX)** · `TXFLOWEN`4 · lower bits: full-duplex, loopback, RX buffer flow control, per datasheet table 32-xx.

## Initialization order (EMAC module)
1. Disable control-module interrupts (C0RXTHRESHEN/C0RXEN/C0TXEN/C0MISCEN = 0). 2. Clear MACCONTROL, RXCONTROL, TXCONTROL. 3. Zero all 16 HDP regs. 4. Zero 36 statistics regs. 5. Program MAC address: for each of 8 channels write `MACINDEX`, `MACADDRHI`, `MACADDRLO` (duplicate same address into unused channels; start at ch0). 6. (Flow control: RXnFREEBUFFER/FLOWTHRESH/FILTERLOWTHRESH.) 7. Clear MACHASH1/2. 8. `RXBUFFEROFFSET` (0). 9. `RXUNICASTCLEAR = 0xFF`, then `RXUNICASTSET` for wanted channel(s). 10. `RXMBPENABLE` (broadcast/multicast/promiscuous). 11. `MACCONTROL` config **without GMIIEN**. 12. Clear unused channel int masks (RXINTMASKCLEAR/TXINTMASKCLEAR). 13. Set used channel masks (RXINTMASKSET/TXINTMASKSET) + `MACINTMASKSET` (HOSTMASK, STATMASK). 14. Build descriptor queues. 15. Write RX head to `RXnHDP`. 16. `RXCONTROL.RXEN=1`, `TXCONTROL.TXEN=1`, then `MACCONTROL.GMIIEN=1`. 17. Enable control-module interrupts. Map interrupts in VIM.

## MDIO bring-up
Set `CLKDIV` (1 MHz) and `PREAMBLE` → `ENABLE=1` → poll `ALIVE`/`LINK` (state machine polls all 32 PHY addresses; each register read can take ~50 µs; **PHY autonegotiation can take up to 3 s** → use timers) → `USERPHYSEL` PHY address (+ link-change interrupt) → to read/write PHY registers: write `USERACCESS` with GO=1, WRITE, REGADR, PHYADR, DATA; wait GO=0 (read data valid when ACK=1). Use PHY result (speed/duplex) to set MACCONTROL.FULLDUPLEX and RMIISPEED.

## Interrupts
26 events: TXPEND0–7, RXPEND0–7, RXTHRESHPEND0–7 (flow control), STATPEND (stats rollover), HOSTPEND (host/descriptor error). Level-type; clear TX/RX by writing the completion pointers; ack with MACEOIVECTOR. Pacing by INTCONTROL/C0RXIMAX/C0TXIMAX.

## Gotchas
- `GMIIEN` last; programming MAC address for all 8 channels is mandatory.
- Descriptors must be word-aligned; EMAC needs a dedicated, init'd CPPI RAM (RAM auto-init).
- IOMM unlock + RMII/MII pin mux.
- Short MDIO clock divider or missing PREAMBLE disable can break PHYs.
- DMA masters for EMAC bus matter for performance; keep descriptors in CPPI RAM.
