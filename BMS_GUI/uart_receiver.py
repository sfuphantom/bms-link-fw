"""
uart_receiver.py

Reads the BMS serial stream (SendDataSerial.c) and writes it to CSV.

Frame, big endian:
    [SOH][id][len hi][len lo][payload (len bytes)][crc16 hi][crc16 lo][EOT]
    payload = [tick u64][body]
    The CRC is CRC-16/CCITT (poly 0x1021, init 0xFFFF) over id .. end of payload.
    Whether the CRC is there is not sent, USE_CRC has to match SERIAL_USE_CRC in SendDataSerial.h.

Bodies:
    LOG      [level][text_len][text]
    LOG_RAW  [level][text_len][text][raw bytes]
    RAW      [raw bytes]
    FAULT    [fault id u16][text_len][text]
    CAN      [msg box][data bytes]
    DATA     ([section id][len u16][section data]) ...

The time of every row is the tick of the sender (its "now" when it built the frame).

Files in the output folder:
    data.csv : one row per DATA frame, one column per value (empty if that section was not sent)
    log.csv  : logs, faults, CAN, raw frames, and events of this program (kind PC: connected,
               connection lost, device restarted, ...)

What it survives: the port missing at start, the cable pulled (it reconnects), the device being
silent or restarting, garbage on the line, and a CSV that is open in another program (the rows wait
in memory and are written when the file is free).
"""

import binascii
import csv
import logging
import os
import struct
import threading
import time
from dataclasses import dataclass
from enum import IntEnum
from typing import Callable, Optional

import serial
import serial.tools.list_ports


logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("uart_receiver")


# ---------------------------------------------------------------------------
# Must match SendDataSerial.h / the BMS headers
# ---------------------------------------------------------------------------
USE_CRC = True              # SERIAL_USE_CRC

SOH = 0x01
EOT = 0x04
HEADER_LEN = 4              # SOH, id, len (2)
TICK_LEN = 8
CRC_LEN = 2
SECTION_HEADER_LEN = 3      # id, len (2)
MAX_PAYLOAD = 4096         # sanity limit, a bigger length is treated as corruption
TICKS_PER_SEC = 10_000_000  # RTI_CLOCK_MEG_HZ * 1e6 in PhantomTimers.h

NUM_CELLS = 6               # NUMBER_OF_CELLS_SERIES
NUM_THERMISTORS = 8         # NUMBER_OF_THERMISTORS_TOTAL
NUM_SLAVES = 1              # TODO: NUMBER_OF_SLAVE_BOARDS_TOTAL
NUM_FANS = 2                # TODO: NUMBER_OF_FANS_TOTAL

SCALE_CELL_VOLT = 10000.0
SCALE_TEMP = 100.0
SCALE_RES = 10000.0
SCALE_SOC = 100.0
SCALE_PACK_VOLT = 100.0
SCALE_CURRENT = 10.0
SLAVE_ADC2VOLTS_SCALE = 10_000


class MsgId(IntEnum):
    LOG = 1
    LOG_RAW = 2
    RAW = 3
    FAULT = 4
    CAN = 5
    DATA = 6


class LogLevel(IntEnum):
    TRACE = 0
    DEBUG = 1
    INFO = 2
    WARNING = 3
    ERROR = 4
    CRITICAL = 5


PY_LOG_LEVEL = {
    LogLevel.TRACE: logging.DEBUG,
    LogLevel.DEBUG: logging.DEBUG,
    LogLevel.INFO: logging.INFO,
    LogLevel.WARNING: logging.WARNING,
    LogLevel.ERROR: logging.ERROR,
    LogLevel.CRITICAL: logging.CRITICAL,
}


class SecId(IntEnum):
    CELL_VOLT = 1
    CELL_TEMP = 2
    CELL_RES = 3
    CELL_SOC = 4
    PACK = 5
    IMD = 6
    CHARGER = 7
    SLAVE_STATE = 8
    FANS = 9
    PWM = 10
    BMS_STATE = 11


# TODO: fill in with FaultId_t from SendDataSerial.h
FAULT_NAMES: dict[int, str] = {
    0: "NONE",
    1: "UNKNOWN",
}

# canMESSAGE_BOXn from Phantom_Can.h
CAN_BOX_NAMES: dict[int, str] = {
    1: "BMS2ALL_FAULT",
    3: "BMS2VCU_DATA",
    4: "VCU2BMS_DATA",
    6: "BMS2CHARGER_DATA",
    7: "CHARGER2BMS_DATA",
}


# ---------------------------------------------------------------------------
# Section decoders: tuple from struct.unpack -> list of values (one per CSV column).
# Only convert here, no I/O.
# ---------------------------------------------------------------------------
def scaled(scale: float) -> Callable[[tuple], list]:
    return lambda vals: [v / scale for v in vals]


def decode_pack(vals: tuple) -> list:
    volt, current, soc, res = vals
    return [volt / SCALE_PACK_VOLT, current / SCALE_CURRENT, soc / SCALE_SOC, res / SCALE_RES]


def decode_slave_state(vals: tuple) -> list:
    out = [float(v) / SLAVE_ADC2VOLTS_SCALE for v in vals]
    for i in range(0, len(out), 3):
        out[i + 1] = out[i + 1] * 0.0075 - 273.0
        out[i + 2] *= 20
    return out


def decode_pwm(vals: tuple) -> list:
    # 2 cells per byte, low nibble first
    pct = [100 / 16 * ((byte >> (4 * i)) & 0xF) for byte in vals for i in range(2)]
    return pct[:NUM_CELLS]


# ---------------------------------------------------------------------------
# on_data hooks: called after the row is written, with the decoded values of that section
# ---------------------------------------------------------------------------
def on_cell_volt(values: list, t: float):
    hi, lo = max(values), min(values)
    log.info("cell volt: max=%.4f (cell %d) min=%.4f (cell %d) sum=%.4f",
             hi, values.index(hi), lo, values.index(lo), sum(values))


def on_slave_state(values: list, t: float):
    for i in range(0, len(values), 3):
        log.info("slave %d: 2ndRef=%.4f temp=%.2f SC=%.1f", i // 3, *values[i:i + 3])


def on_pwm(values: list, t: float):
    log.info("PWM drain %% = %s", values)


@dataclass(frozen=True)
class Section:
    id: int
    fmt: str                                    # struct format, same size as the C side sends
    columns: list
    decode: Callable[[tuple], list]
    on_data: Optional[Callable[[list, float], None]] = None


def cols(prefix: str, n: int) -> list:
    return [f"{prefix}_{i}" for i in range(n)]


# Column order in data.csv = order of this list (not the order on the wire)
SECTIONS = [
    Section(SecId.CELL_VOLT, f">{NUM_CELLS}H", cols("cell_volt", NUM_CELLS), scaled(SCALE_CELL_VOLT), on_cell_volt),
    Section(SecId.CELL_TEMP, f">{NUM_THERMISTORS}h", cols("therm_temp", NUM_THERMISTORS), scaled(SCALE_TEMP)),
    Section(SecId.CELL_RES, f">{NUM_CELLS}H", cols("cell_res", NUM_CELLS), scaled(SCALE_RES)),
    Section(SecId.CELL_SOC, f">{NUM_CELLS}H", cols("cell_soc", NUM_CELLS), scaled(SCALE_SOC)),
    Section(SecId.PACK, ">HhHH", ["pack_volt", "pack_current", "pack_soc", "pack_res"], decode_pack),
    Section(SecId.IMD, ">4f", ["imd_duty", "imd_freq", "imd_isolation", "imd_resistance"], list),
    Section(SecId.CHARGER, ">BHhB", ["charger_state", "charger_volt", "charger_amps", "charger_status"], list),
    Section(SecId.SLAVE_STATE, f">{3 * NUM_SLAVES}H",
            [f"slave{i}_{n}" for i in range(NUM_SLAVES) for n in ("ref2nd_V", "temp_C", "sc")],
            decode_slave_state, on_slave_state),
    Section(SecId.FANS, f">{NUM_FANS}B", cols("fan_duty", NUM_FANS), list),
    Section(SecId.PWM, f">{(NUM_CELLS + 1) // 2}B", cols("pwm_drain", NUM_CELLS), decode_pwm, on_pwm),
    Section(SecId.BMS_STATE, ">BBH", ["bms_state", "contactor_state", "fault_flags"], list),
]
SECTIONS_BY_ID = {s.id: s for s in SECTIONS}

DATA_HEADER = ["time_s"] + [c for s in SECTIONS for c in s.columns]
LOG_HEADER = ["time_s", "kind", "level", "id", "text_len", "text", "hex"]


# ---------------------------------------------------------------------------
# CSV that can be locked by another program (Excel on Windows does that)
# ---------------------------------------------------------------------------
class CsvFile:
    MAX_PENDING = 100_000   # rows kept in memory while the file is locked

    def __init__(self, path: str, header: list, clear: bool):
        self.path = path
        self.pending: list = []
        self._blocked = False
        if clear or not os.path.exists(path):
            self.path = self._create(path, header)

    @staticmethod
    def _create(path: str, header: list) -> str:
        """Writes the header. If the file is locked it uses name_1, name_2, ... instead."""
        stem, ext = os.path.splitext(path)
        candidate = path
        for n in range(1, 21):
            try:
                with open(candidate, mode="w", newline="") as f:
                    csv.writer(f).writerow(header)
                if candidate != path:
                    log.warning("%s can not be written (open in another program?), using %s", path, candidate)
                return candidate
            except OSError:
                candidate = f"{stem}_{n}{ext}"
        raise OSError(f"could not create {path} or any of its name_N alternatives")

    def write(self, row: list):
        self.pending.append(row)
        self.flush()

    def flush(self):
        if not self.pending:
            return
        try:
            # newline='' so Windows does not turn the csv module's \r\n into \r\r\n (blank line between rows)
            with open(self.path, mode="a", newline="") as f:
                csv.writer(f).writerows(self.pending)
        except OSError as e:
            if not self._blocked:
                log.warning("Can not write %s (%s). Keeping the rows in memory until it is free.", self.path, e)
                self._blocked = True
            if len(self.pending) > self.MAX_PENDING:
                lost = len(self.pending) - self.MAX_PENDING
                del self.pending[:lost]
                log.error("%s still blocked, dropped the %d oldest rows", self.path, lost)
            return
        if self._blocked:
            log.info("%s is writable again, wrote %d waiting rows", self.path, len(self.pending))
            self._blocked = False
        self.pending.clear()

    def close(self):
        self.flush()
        if not self.pending:
            return
        stem, ext = os.path.splitext(self.path)
        alt = f"{stem}_unsaved{ext}"
        try:
            with open(alt, mode="a", newline="") as f:
                csv.writer(f).writerows(self.pending)
            log.warning("%s was blocked until the end, %d rows saved in %s", self.path, len(self.pending), alt)
        except OSError:
            log.error("%s was blocked until the end, %d rows lost", self.path, len(self.pending))
        self.pending.clear()


def level_name(level: int) -> str:
    try:
        return LogLevel(level).name
    except ValueError:
        return str(level)


def decode_text(raw: bytes) -> str:
    return raw.decode("utf-8", errors="replace")


class UARTReceiver:
    DEFAULT_ROOT_FILE = "data"
    DEFAULT_UART_KW_ARGS = {"port": "COM6", "baudrate": 115200, "timeout": 0.1}

    def __init__(self, *, uart_kw_args, RootDesFolder=None, clearIfExist=True,
                 frame_timeout=0.5, reconnect_delay=1.0, silence_warn=5.0):
        """
        uart_kw_args  : kwargs for serial.Serial(...), e.g. {"port": "COM4", "baudrate": 115200}
        RootDesFolder : output folder, data_0, data_1, ... if not given
        clearIfExist  : True = start the CSV files over, False = keep adding to existing ones
        frame_timeout : seconds without a byte before a half received frame is thrown away
        reconnect_delay: seconds between tries to open the port
        silence_warn  : seconds without any byte before it is reported (and again when data is back)
        The port does not have to exist yet, it is opened (and re-opened) by the reader thread.
        """
        if RootDesFolder is None:
            n = 0
            while os.path.isdir(f"{self.DEFAULT_ROOT_FILE}_{n}"):
                n += 1
            RootDesFolder = f"{self.DEFAULT_ROOT_FILE}_{n}"
        os.makedirs(RootDesFolder, exist_ok=True)

        self._data_csv = CsvFile(os.path.join(RootDesFolder, "data.csv"), DATA_HEADER, clearIfExist)
        self._log_csv = CsvFile(os.path.join(RootDesFolder, "log.csv"), LOG_HEADER, clearIfExist)

        self.frame_timeout = frame_timeout
        self.reconnect_delay = reconnect_delay
        self.silence_warn = silence_warn
        self._handlers = {
            MsgId.LOG: self._on_log,
            MsgId.LOG_RAW: self._on_log_raw,
            MsgId.RAW: self._on_raw,
            MsgId.FAULT: self._on_fault,
            MsgId.CAN: self._on_can,
            MsgId.DATA: self._on_data,
        }

        self._uart_kw = {**self.DEFAULT_UART_KW_ARGS, **uart_kw_args}
        self._ser: Optional[serial.Serial] = None
        self._port_error_shown = False
        self._last_t = 0.0                  # sender time of the last frame
        self._stop_event = threading.Event()
        self._thread = None

    # -- public API ---------------------------------------------------------
    def start(self):
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._reader_loop, daemon=True)
        self._thread.start()
        log.info("UART receiver started (%s)", self._uart_kw["port"])

    def stop(self):
        self._stop_event.set()
        if self._thread:
            self._thread.join(timeout=2)
        self._close_port()
        self._data_csv.close()
        self._log_csv.close()
        log.info("UART receiver stopped")

    # -- connection ---------------------------------------------------------
    def _event(self, text: str):
        """A note in log.csv about this program (not from the BMS). Its time is the last sender time seen."""
        self._log_csv.write([f"{self._last_t:.6f}", "PC", "", "", "", text, ""])

    def _connect(self) -> bool:
        try:
            self._ser = serial.Serial(**self._uart_kw)
        except (serial.SerialException, OSError) as e:
            self._ser = None
            if not self._port_error_shown:         # once, not every retry
                log.warning("Can not open %s (%s). Is it plugged in? Trying again every %.1f s",
                            self._uart_kw["port"], e, self.reconnect_delay)
                self._port_error_shown = True
            return False
        self._port_error_shown = False
        log.info("Connected to %s", self._uart_kw["port"])
        self._event("connected")
        return True

    def _close_port(self):
        if self._ser is not None:
            try:
                self._ser.close()
            except (serial.SerialException, OSError):
                pass
            self._ser = None

    def _disconnect(self, reason):
        log.warning("Connection lost (%s). Reconnecting...", reason)
        self._close_port()
        self._event(f"connection lost: {reason}")
        self._port_error_shown = True       # already told the user, do not repeat on the first failed retry

    # -- reading ------------------------------------------------------------
    def _reader_loop(self):
        buf = bytearray()
        last_rx = time.monotonic()
        silent = False

        while not self._stop_event.is_set():
            if self._ser is None:
                if not self._connect():
                    self._stop_event.wait(self.reconnect_delay)
                    continue
                buf.clear()                         # nothing from before the gap can be trusted
                last_rx = time.monotonic()
                silent = False

            try:
                # blocks until a byte arrives or the serial timeout passes
                chunk = self._ser.read(self._ser.in_waiting or 1)
            except (serial.SerialException, OSError) as e:
                self._drop_stalled(buf)             # keep the complete frames that were already received
                self._disconnect(e)
                self._stop_event.wait(self.reconnect_delay)
                continue

            now = time.monotonic()
            if chunk:
                buf += chunk
                last_rx = now
                if silent:
                    silent = False
                    log.info("Data is back")
                    self._event("data is back")
            else:
                self._data_csv.flush()              # retry rows that waited for a locked file (does nothing if none)
                self._log_csv.flush()
                if buf and now - last_rx > self.frame_timeout:
                    self._drop_stalled(buf)
                if not silent and now - last_rx > self.silence_warn:
                    silent = True
                    log.warning("No data for %.1f s (is the BMS on?)", self.silence_warn)
                    self._event(f"no data for {self.silence_warn:.1f} s")
                continue

            try:
                self._parse(buf)
            except Exception:
                log.exception("Parser error, dropping %d buffered bytes", len(buf))
                buf.clear()

    def _drop_stalled(self, buf: bytearray):
        """Nothing more is coming (timeout or connection lost), so whatever waits at the start of buf will never
        finish (a false SOH or a corrupted length). Skip it, and still take out any complete frame queued behind it."""
        if buf:
            log.warning("Giving up on %d buffered bytes of an unfinished frame", len(buf))
        while buf:
            del buf[:1]
            self._parse(buf)

    def _parse(self, buf: bytearray):
        """Takes every complete frame out of buf. On a bad frame it skips one byte and looks for the next SOH."""
        while True:
            start = buf.find(SOH)
            if start < 0:
                buf.clear()
                return
            del buf[:start]
            if len(buf) < HEADER_LEN:
                return

            msg_id = buf[1]
            length = int.from_bytes(buf[2:4], "big")
            if length < TICK_LEN or length > MAX_PAYLOAD:
                log.warning("Bad length %d (msg_ID=0x%02X), resyncing", length, msg_id)
                del buf[:1]
                continue

            end = HEADER_LEN + length                   # end of payload
            total = end + (CRC_LEN if USE_CRC else 0) + 1
            if len(buf) < total:
                return                                  # wait for the rest

            if buf[total - 1] != EOT:
                log.warning("Bad end byte 0x%02X (msg_ID=0x%02X), resyncing", buf[total - 1], msg_id)
                del buf[:1]
                continue
            if USE_CRC:
                got = int.from_bytes(buf[end:end + CRC_LEN], "big")
                want = binascii.crc_hqx(bytes(buf[1:end]), 0xFFFF)
                if got != want:
                    log.warning("CRC error (msg_ID=0x%02X): got 0x%04X, expected 0x%04X, resyncing", msg_id, got, want)
                    del buf[:1]
                    continue

            payload = bytes(buf[HEADER_LEN:end])
            del buf[:total]
            self._dispatch(msg_id, payload)

    # -- handling -----------------------------------------------------------
    def _dispatch(self, msg_id: int, payload: bytes):
        handler = self._handlers.get(msg_id)
        if handler is None:
            log.warning("Unknown msg_ID 0x%02X, ignored", msg_id)
            return

        t = int.from_bytes(payload[:TICK_LEN], "big") / TICKS_PER_SEC
        if t < self._last_t:
            log.warning("Sender time went back from %.3f to %.3f s (BMS restarted?)", self._last_t, t)
            self._event(f"sender time went back from {self._last_t:.6f} s (restart?)")
        self._last_t = t

        try:
            handler(t, payload[TICK_LEN:])
        except Exception:
            # a bad frame must never kill the reader thread
            log.exception("Error handling msg_ID=0x%02X, frame discarded", msg_id)

    def _write_log(self, t: float, kind: str, level="", ident="", text_len="", text="", raw=b""):
        self._log_csv.write([f"{t:.6f}", kind, level, ident, text_len, text, raw.hex(" ")])

    @staticmethod
    def _split_text(body: bytes, head_len: int):
        """body = [head][text_len][text][rest] -> (text_len, text, rest)"""
        text_len = body[head_len]
        start = head_len + 1
        text_raw = body[start:start + text_len]
        if len(text_raw) != text_len:
            log.warning("Log text is %d bytes, the sender said %d", len(text_raw), text_len)
        return text_len, decode_text(text_raw), body[start + text_len:]

    def _on_log(self, t: float, body: bytes):
        level = body[0]
        text_len, text, rest = self._split_text(body, 1)
        if rest:
            log.warning("Log has %d bytes after its text", len(rest))
        self._write_log(t, "LOG", level=level_name(level), text_len=text_len, text=text)
        log.log(PY_LOG_LEVEL.get(level, logging.INFO), "[%.4f] %s", t, text)

    def _on_log_raw(self, t: float, body: bytes):
        level = body[0]
        text_len, text, raw = self._split_text(body, 1)
        self._write_log(t, "LOG_RAW", level=level_name(level), text_len=text_len, text=text, raw=raw)
        log.log(PY_LOG_LEVEL.get(level, logging.INFO), "[%.4f] %s | %s", t, text, raw.hex(" "))

    def _on_raw(self, t: float, body: bytes):
        self._write_log(t, "RAW", raw=body)
        log.info("[%.4f] raw: %s", t, body.hex(" "))

    def _on_fault(self, t: float, body: bytes):
        fault_id = int.from_bytes(body[:2], "big")
        text_len, text, _ = self._split_text(body, 2)
        name = FAULT_NAMES.get(fault_id, str(fault_id))
        self._write_log(t, "FAULT", ident=name, text_len=text_len, text=text)
        log.error("[%.4f] FAULT %s: %s", t, name, text)

    def _on_can(self, t: float, body: bytes):
        box, data = body[0], body[1:]
        name = CAN_BOX_NAMES.get(box, str(box))
        self._write_log(t, "CAN", ident=name, raw=data)
        log.debug("[%.4f] CAN %s: %s", t, name, data.hex(" "))

    def _on_data(self, t: float, body: bytes):
        values: dict[int, list] = {}

        pos = 0
        while pos + SECTION_HEADER_LEN <= len(body):
            sec_id = body[pos]
            length = int.from_bytes(body[pos + 1:pos + 3], "big")
            chunk = body[pos + SECTION_HEADER_LEN:pos + SECTION_HEADER_LEN + length]
            pos += SECTION_HEADER_LEN + length

            sec = SECTIONS_BY_ID.get(sec_id)
            if len(chunk) != length:
                log.warning("Section 0x%02X is cut short, ignoring the rest of the frame", sec_id)
                break
            if sec is None:
                log.warning("Unknown section 0x%02X, skipped", sec_id)
                continue
            if length != struct.calcsize(sec.fmt):
                log.warning("Section %s is %d bytes, expected %d (NUM_* constants wrong?), skipped",
                            SecId(sec_id).name, length, struct.calcsize(sec.fmt))
                continue
            values[sec_id] = sec.decode(struct.unpack(sec.fmt, chunk))

        row = [f"{t:.6f}"]
        for sec in SECTIONS:
            row += values.get(sec.id, [""] * len(sec.columns))
        self._data_csv.write(row)

        for sec in SECTIONS:
            if sec.id in values and sec.on_data is not None:
                sec.on_data(values[sec.id], t)


# ---------------------------------------------------------------------------
# Example usage
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    for p in serial.tools.list_ports.comports():
        print(f"{p.device} - {p.description} - location: {p.location}")

    UART_KW_ARGS = {"port": "COM8", "baudrate": 115200}
    receiver = UARTReceiver(
        uart_kw_args=UART_KW_ARGS,
        frame_timeout=0.5,
        RootDesFolder="data_1",
        # clearIfExist=False
    )

    receiver.start()
    try:
        while True:
            time.sleep(1)  # main thread is free to do other work
    except KeyboardInterrupt:
        pass
    finally:
        receiver.stop()
