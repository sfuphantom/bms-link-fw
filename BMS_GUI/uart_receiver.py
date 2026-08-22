"""
uart_receiver.py

Interrupt-style UART receiver for pyserial (PC / Raspberry Pi / Linux).

Framing:
    [SOH][msg_ID][payload bytes ...][EOT]
    (variable-length messages additionally carry a 4-byte big-endian
     length field right after msg_ID, before the payload -- see
     DATA_LEN_FMT / WAIT_DATA_LEN)

    SOH (0x01) - Start of Header. Lets the parser resynchronize after any
                 corruption/desync instead of hunting for "\n" inside binary
                 data (which can false-trigger, since a payload byte can
                 legitimately equal 0x0A).
    msg_ID     - one byte. Looked up in self.msg_types (built from the CSV
                 config file) to know:
                   * how many payload bytes follow (fixed size), or that a
                     4-byte length field precedes the payload (variable size,
                     fmt == 's')
                   * how to unpack them (struct format)
                   * how to convert the unpacked values into "formatted"
                     data (MSG_Config.data_handler)
                   * what to do once formatted data is ready
                     (MSG_Config.on_message_handler)
    payload    - exact number of bytes for that msg_ID.
    EOT (0x04) - End of Transmission. Validates that we read the frame
                 correctly (if this byte isn't EOT, the frame is discarded).

"Interrupt-driven" here means: a dedicated thread blocks on serial.read(),
which sleeps efficiently (no busy-wait / no manual polling loop checking
in_waiting) until bytes actually arrive, and dispatches a callback per
completed frame. This is the standard pyserial equivalent of an ISR, since
plain CPython has no access to a real UART hardware interrupt.

Per-frame timeout: once SOH is seen, a clock starts and is refreshed on
*every* byte received while mid-frame. If more than frame_timeout seconds
pass since the *last* byte (not since SOH), the partial data is discarded
and the parser goes back to hunting for SOH. This means a slow-but-steady
trickle of bytes won't be falsely timed out, but a stall mid-frame will be.
"""

import serial
import serial.tools.list_ports
import struct
import threading
import time
import logging
import csv
import os
from dataclasses import dataclass
from typing import Callable, Any, Sequence


logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("uart_receiver")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

SLAVE_ADC2VOLTS_SCALE = 10_000
Slave_ADC2VOLTS = lambda ADC_vals: [float(val)/SLAVE_ADC2VOLTS_SCALE for val in ADC_vals]

def get_element_count(fmt):
    """
    Returns (byte_size, element_count) for a struct format string.
    fmt == "s" is our own sentinel for "variable length string/bytes",
    which struct doesn't natively describe a fixed size for.
    """
    if fmt == "s":
        return 0, 1

    byte_size = struct.calcsize(fmt)
    eleCount = len(struct.unpack(fmt, b'\x00' * byte_size))
    return byte_size, eleCount


def search_string_list(lst, target: str) -> int:
    tar_low: str = target.lower()
    for (i, ele) in enumerate(lst):
        if ele.lower() == tar_low:
            return i
    return -1


def appendRow2CSV_helper(file, timestamp, values):
    # newline='' is required (not '\n') so that on Windows the csv module's
    # own \r\n line terminator isn't doubled up by text-mode translation,
    # which otherwise leaves a blank line between every row.
    with open(file, mode='a', newline='') as f:
        writer = csv.writer(f)
        row = [timestamp, *values]
        writer.writerow(row)


# ---------------------------------------------------------------------------
# Data handlers: payload tuple (from struct.unpack) -> formatted values
#
# These ONLY convert data -- they must not touch files, logging, or I/O.
# Real scaling/units are only filled in for the messages whose firmware
# implementation we could actually verify against SendDataSerial.c
# (Cell_Voltages, IMD). Everything else is a placeholder pass-through
# until the firmware side actually sends real data for it -- see the
# comment above each handler.
# ---------------------------------------------------------------------------

def CellVoltagesData_handler(data: Sequence) -> list:
    # SendCellVoltage_Serial() sends raw uint16 ADC counts per cell.
    # 10000 scale factor kept from the original implementation.
    data_fmt: list[float] = Slave_ADC2VOLTS(data)
    return data_fmt


def IMDData_handler(data: Sequence) -> list:
    # SendIMDData_Serial() sends ecapIMDData_t as raw freq/duty bytes.
    return [data[0] / 2, data[1] / 4]


def ChargerData_handler(data: Sequence) -> list:
    # PLACEHOLDER: SendChargerData_Serial() in SendDataSerial.c is an empty
    # stub -- firmware doesn't send this yet, so no real scaling is defined.
    # fmt from CSV is ">HH?" (Volt, Current, Status).
    return list(data)


def CellTempData_handler(data: Sequence) -> list:
    # PLACEHOLDER: SendCellTemp_Serial() in SendDataSerial.c is an empty
    # stub -- no real scaling defined yet.
    return list(data)


def FansData_handler(data: Sequence) -> list:
    # PLACEHOLDER: SendFanData_Serial() in SendDataSerial.c is an empty stub.
    return list(data)


def FullBatteryData_handler(data: Sequence) -> list:
    # PLACEHOLDER: no corresponding Send*_Serial() function exists in
    # SendDataSerial.c for a combined "FullBattery" (Current, HV) message.
    return list(data)


def SlaveStateData_handler(data: Sequence) -> list:
    # PLACEHOLDER / KNOWN MISMATCH: SendSlaveState_Serial() streams
    # RefVolt2nd/ITMP/SC for NUMBER_OF_SLAVE_BOARDS boards back-to-back
    # (3 * NUMBER_OF_SLAVE_BOARDS halfwords total), but the CSV fmt ">3H"
    # only describes a single board's worth. Until NUMBER_OF_SLAVE_BOARDS
    # is known here and the fmt is updated to match, this only decodes
    # one board correctly.
    data_fmt :list[float] = Slave_ADC2VOLTS(data)
    NUMBER_OF_UNIQUE_ELEMENTS_IN_MESSAGE = 3
    for i in range(0, len(data), NUMBER_OF_UNIQUE_ELEMENTS_IN_MESSAGE):
        # data_fmt[i]
        data_fmt[i+1]  = (data_fmt[i+1] * 0.007_5) - 273.0
        data_fmt[i+2] *= 20

    return list(data)


def DCCData_handler(data: Sequence) -> list:
    # PLACEHOLDER: SendCellDCC_Serial() packs DCC values into compressed
    # nibbles across 3-byte groups (bit-packed, not a plain array), so the
    # raw unpacked bytes here are NOT the true DCC values yet -- decoding
    # that packing needs NUMBER_OF_SLAVE_BOARDS and the exact packing
    # order reproduced from the firmware.
    DCC: list[int] = []
    for d in data:
        for i in range(8):
            DCC.append(1 if bool((d>>i) | 0x1) else 0)
    return list(DCC)


def DebugData_handler(data: Sequence) -> list:
    # data is a 1-tuple containing raw bytes (variable-length 's' format).
    return [data[0].decode("utf-8", errors="replace")]


def LogData_handler(data: Sequence) -> list:
    return [data[0].decode("utf-8", errors="replace")]


DATA_HANDLERS: dict[str, Callable[[Sequence], list]] = {
    "Debug": DebugData_handler,
    "Log": LogData_handler,
    "Cell_Voltages": CellVoltagesData_handler,
    "IMD": IMDData_handler,
    "Charger": ChargerData_handler,
    "Cell_Temp": CellTempData_handler,
    "Fans": FansData_handler,
    "FullBattery": FullBatteryData_handler,
    "SlaveState": SlaveStateData_handler,
    "DCC": DCCData_handler,
}


# ---------------------------------------------------------------------------
# on_message handlers: called once per completed message, AFTER the row has
# been written to CSV, with the fully formatted data. Placeholders for now --
# each msg type gets its own hook so you can wire up e.g. dashboards, alarms,
# or triggering other logic per message type without touching _dispatch.
# ---------------------------------------------------------------------------

def on_message_Debug(name, msg_id, formatted_data, timestamp):
    pass  # TODO: e.g. print/log debug string live


def on_message_Log(name, msg_id, formatted_data, timestamp):
    pass  # TODO


def on_message_CellVoltages(name, msg_id, formatted_data, timestamp):
    maxCellVolt = max(formatted_data)
    maxCellIdx = formatted_data.index(maxCellVolt)
    minCellVolt = min([d for d in formatted_data if d !=0 and d != 0xFF])
    minCellIdx = formatted_data.index(minCellVolt)
    SumVolt = sum(formatted_data)
    log_str: str= f"{maxCellVolt=}, {maxCellIdx=}, {minCellVolt=}, {minCellIdx=}, {SumVolt=}"
    log.info(log_str)
    pass  # TODO: e.g. check for over/under-voltage and raise an alert


def on_message_IMD(name, msg_id, formatted_data, timestamp):
    pass  # TODO


def on_message_Charger(name, msg_id, formatted_data, timestamp):
    pass  # TODO


def on_message_CellTemp(name, msg_id, formatted_data, timestamp):
    pass  # TODO: e.g. check for over-temp


def on_message_Fans(name, msg_id, formatted_data, timestamp):
    pass  # TODO


def on_message_FullBattery(name, msg_id, formatted_data, timestamp):
    pass  # TODO


def on_message_SlaveState(name, msg_id, formatted_data, timestamp):
    log_str: str= ""

    for i in range(int(len(formatted_data)/3)):
        idx = 3*i
        log_str += f"2ndRef_{i}={formatted_data[idx]}, "
        log_str += f"Temp_{i}={formatted_data[idx+1]}, "
        log_str += f"SC_{i}={formatted_data[idx+2]}, "

    log.info(log_str)
    pass  # TODO


def on_message_DCC(name, msg_id, formatted_data, timestamp):
    log_str: str= f"DCC = {formatted_data}"

    log.info(log_str)

    pass  # TODO


ON_MESSAGE_HANDLERS: dict[str, Callable[[str, int, list, float], None]] = {
    "Debug": on_message_Debug,
    "Log": on_message_Log,
    "Cell_Voltages": on_message_CellVoltages,
    "IMD": on_message_IMD,
    "Charger": on_message_Charger,
    "Cell_Temp": on_message_CellTemp,
    "Fans": on_message_Fans,
    "FullBattery": on_message_FullBattery,
    "SlaveState": on_message_SlaveState,
    "DCC": on_message_DCC,
}


@dataclass(frozen=True, kw_only=True)
class MSG_Config:
    name: str
    file: str
    fmt: str
    msgSize: int
    NumberOfElement: int
    data_handler: Callable[[Sequence], list]
    on_message_handler: Callable[[str, int, list, float], None]


class UARTReceiver:
    DEFAULT_ROOT_FILE = "data"
    DEFAULT_TIME_COL_NAME = "time"
    CONFIG_FILE_COLS = ("msg_ID", "Name", "file_rel", "fmt", "header")

    SOH = 0x01  # Start of Header
    EOT = 0x04  # End of Transmission
    # States for the frame parser state machine
    WAIT_SOH, WAIT_ID, WAIT_DATA_LEN, WAIT_PAYLOAD, WAIT_EOT = range(5)

    # Used when the sender tells us the payload length up front (fmt == 's')
    DATA_LEN_FMT = ">H"
    DATA_LEN_LEN = struct.calcsize(DATA_LEN_FMT)
    # Sanity ceiling on a wire-supplied variable length, to avoid trying to
    # allocate/accumulate a huge buffer if that length field gets corrupted
    # or the parser desyncs mid-stream.
    MAX_VARIABLE_PAYLOAD_LEN = 4096

    DEFAULT_UART_KW_ARGS = {"port": "COM6",
                             "baudrate": 115200,
                             "timeout": 0.1}

    def __init__(self, *, uart_kw_args,
                 MsgTypeConfigFile, RootDesFolder=None, clearIfExist=True, TIME_COL_NAME=None,
                 on_message=None,
                 frame_timeout=0.5):
        """
        uart_kw_args      : kwargs forwarded to serial.Serial(...), e.g.
                             {"port": "COM4", "baudrate": 115200}
        MsgTypeConfigFile : path to the CSV describing msg_ID -> name/fmt/file/header
        RootDesFolder     : output folder for per-message CSV logs. Auto-
                             increments (data_0, data_1, ...) if not given.
        clearIfExist      : if True, always (re)write CSV headers, overwriting
                             any existing per-message CSV files
        on_message        : optional global callback(name, msg_id, formatted_data,
                             timestamp) called after the per-type on_message
                             handler, for anything you want applied to every
                             message type without editing ON_MESSAGE_HANDLERS
        frame_timeout     : max seconds allowed between consecutive bytes of
                             an in-progress frame before it's discarded
        """

        self.TIME_COL_NAME = TIME_COL_NAME or __class__.DEFAULT_TIME_COL_NAME
        self.on_message = on_message

        # Get root Folder
        if RootDesFolder is None:
            folderExist = True
            FolderSubscript = 0

            while folderExist:
                RootDesFolder = f"{__class__.DEFAULT_ROOT_FILE}_{FolderSubscript}"
                folderExist = os.path.isdir(RootDesFolder)
                FolderSubscript += 1

        os.makedirs(RootDesFolder, exist_ok=True)

        self.msg_types: dict[int, MSG_Config] = {}

        with open(MsgTypeConfigFile, 'r', newline='') as MTCF:
            csv_reader = csv.reader(MTCF, delimiter=',', quotechar='"')
            header = next(csv_reader)
            header = [h.strip() for h in header]

            file_col = {}
            for col in __class__.CONFIG_FILE_COLS:
                index = search_string_list(header, col)
                if index == -1:
                    raise ValueError(f"could not find {col=} in header: {header}")
                file_col[col] = index

            for row in csv_reader:
                if not row:
                    continue

                # msg_ID must be an int to match the bytes read off the wire.
                # base=0 lets the CSV use "0", "4", "0x04", etc. interchangeably.
                MSG_ID = int(row[file_col["msg_ID"]].strip(), 0)
                MSG_TYPE_NAME = row[file_col["Name"]].strip()
                MSG_FMT: str = row[file_col["fmt"]].strip()
                DES_FILE: str = os.path.join(RootDesFolder, row[file_col["file_rel"]].strip())
                MSG_SIZE, NUMBER_OF_COL = get_element_count(MSG_FMT)

                if MSG_TYPE_NAME not in DATA_HANDLERS:
                    raise ValueError(
                        f"No data handler registered for msg type '{MSG_TYPE_NAME}' "
                        f"(msg_ID={MSG_ID}). Add one to DATA_HANDLERS."
                    )
                if MSG_TYPE_NAME not in ON_MESSAGE_HANDLERS:
                    raise ValueError(
                        f"No on_message handler registered for msg type '{MSG_TYPE_NAME}' "
                        f"(msg_ID={MSG_ID}). Add one to ON_MESSAGE_HANDLERS."
                    )

                MSG_ID_CONFIG = MSG_Config(
                    name=MSG_TYPE_NAME,
                    file=DES_FILE,
                    fmt=MSG_FMT,
                    msgSize=MSG_SIZE,
                    NumberOfElement=NUMBER_OF_COL,
                    data_handler=DATA_HANDLERS[MSG_TYPE_NAME],
                    on_message_handler=ON_MESSAGE_HANDLERS[MSG_TYPE_NAME],
                )
                self.msg_types[MSG_ID] = MSG_ID_CONFIG

                if clearIfExist or not os.path.exists(DES_FILE):
                    ALL_COLS_HEADERS = [col.strip() for col in row[file_col["header"]].split(',')]

                    with open(DES_FILE, newline='', mode='w') as DF:
                        writer = csv.writer(DF)
                        writer.writerow(ALL_COLS_HEADERS)

        self.StartTime: float = 0
        self.frame_timeout = frame_timeout

        uart_kw_args = dict(uart_kw_args)  # don't mutate caller's dict
        for key, val in __class__.DEFAULT_UART_KW_ARGS.items():
            uart_kw_args.setdefault(key, val)

        self.__SER = serial.Serial(**uart_kw_args)
        self._stop_event = threading.Event()
        self._thread = None

    # -- public API ---------------------------------------------------------
    def start(self):
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._reader_loop, daemon=True)
        self._thread.start()
        log.info("UART receiver started on %s", self.__SER.port)

    def stop(self):
        self._stop_event.set()
        if self._thread:
            self._thread.join(timeout=2)
        if self.__SER.is_open:
            self.__SER.close()
        log.info("UART receiver stopped")

    # -- internal -------------------------------------------------------------
    def _dispatch(self, msg_id, payload, frame_start_time):
        timeData_formated = frame_start_time - self.StartTime

        MSG_ID_CONFIG = self.msg_types[msg_id]

        # 1) payload -> data
        if MSG_ID_CONFIG.fmt == 's':
            unpack_fmt = f">{__class__.DATA_LEN_FMT}{len(payload) - __class__.DATA_LEN_LEN}s"
        else:
            unpack_fmt = MSG_ID_CONFIG.fmt
        data = struct.unpack(unpack_fmt, bytes(payload))

        # 2) data -> formatted data
        formatted_data = MSG_ID_CONFIG.data_handler(data)

        # 3) time + formatted data -> CSV
        appendRow2CSV_helper(MSG_ID_CONFIG.file, timeData_formated, formatted_data)

        log.debug(
            "Full formatted message received: %s (0x%02X) @ t=%.6f -> %s",
            MSG_ID_CONFIG.name, msg_id, timeData_formated, formatted_data,
        )

        # Per-type hook, called now that formatted data + CSV write are done.
        MSG_ID_CONFIG.on_message_handler(MSG_ID_CONFIG.name, msg_id, formatted_data, timeData_formated)

        # Optional global hook, in addition to the per-type one above.
        if self.on_message is not None:
            self.on_message(MSG_ID_CONFIG.name, msg_id, formatted_data, timeData_formated)

    def _reader_loop(self):
        state = __class__.WAIT_SOH

        msg_id = None
        expected_len = 0
        payload = bytearray()
        sentLength = bytearray()

        frame_start_time = 0.0
        last_byte_time = time.monotonic()
        self.StartTime = time.monotonic()

        def check_timedOut():
            nonlocal state, msg_id, payload, sentLength, last_byte_time
            # Timeout is measured from the last byte received, not from SOH,
            # so a slow-but-steady trickle of bytes doesn't get discarded --
            # only an actual stall mid-frame does.
            if state != __class__.WAIT_SOH and (time.monotonic() - last_byte_time) > self.frame_timeout:
                log.warning("Frame timeout waiting for msg_ID=%s, discarding partial data", msg_id)
                state, msg_id, payload, sentLength = __class__.WAIT_SOH, None, bytearray(), bytearray()

        def wait4SOH(b):
            nonlocal state, frame_start_time
            if b == __class__.SOH:
                state, frame_start_time = __class__.WAIT_ID, time.monotonic()

        def wait4MSG_ID(b):
            nonlocal state, msg_id, expected_len
            if b not in self.msg_types:
                log.warning("Unknown msg_ID 0x%02X, discarding and resyncing", b)
                state = __class__.WAIT_SOH
                return

            msg_id_config = self.msg_types[b]
            msg_id = b

            if msg_id_config.fmt == 's':
                state = __class__.WAIT_DATA_LEN
            else:
                state = __class__.WAIT_PAYLOAD
                expected_len = msg_id_config.msgSize

        def Wait4DataLength(b):
            nonlocal state, expected_len, sentLength

            sentLength.append(b)

            if len(sentLength) < __class__.DATA_LEN_LEN:
                return

            length = struct.unpack(__class__.DATA_LEN_FMT, bytes(sentLength))[0]
            sentLength = bytearray()

            if length > __class__.MAX_VARIABLE_PAYLOAD_LEN:
                log.warning(
                    "Declared payload length %d for msg_ID=0x%02X exceeds max %d, "
                    "discarding and resyncing (likely corrupted length field)",
                    length, msg_id, __class__.MAX_VARIABLE_PAYLOAD_LEN,
                )
                state = __class__.WAIT_SOH
                return

            expected_len = length
            state = __class__.WAIT_PAYLOAD

        def WaitFullPayload(b):
            nonlocal state, payload
            payload.append(b)
            if len(payload) >= expected_len:
                state = __class__.WAIT_EOT

        def WaitEOT(b):
            nonlocal state, msg_id, payload, frame_start_time
            if b == __class__.EOT:
                try:
                    self._dispatch(msg_id, payload, frame_start_time)
                except Exception:
                    # Never let a bad frame (bad fmt, handler bug, etc.) kill
                    # the reader thread -- log it and keep receiving.
                    log.exception("Error dispatching msg_ID=%s, discarding frame", msg_id)
            else:
                log.warning(
                    "Bad end byte (got 0x%02X, expected 0x%02X) for msg_ID=0x%02X -- "
                    "msg_ID was likely wrong or data corrupted, discarding",
                    b, __class__.EOT, msg_id,
                )
                print(payload)
            state, msg_id, payload = __class__.WAIT_SOH, None, bytearray()

        while not self._stop_event.is_set():
            check_timedOut()

            # Blocking read (bounded by read_timeout so we can check stop_event
            # periodically). This is what makes it "interrupt-style" rather
            # than a manual polling loop: the thread sleeps until a byte is
            # available or the short timeout elapses.

            byte = self.__SER.read(1)
            if not byte:  # Nothing arrived this tick. If we're mid-frame, check timeout.
                continue
            b = byte[0]
            # print(b )
            last_byte_time = time.monotonic()

            match(state):
                case __class__.WAIT_SOH     : wait4SOH(b)
                case __class__.WAIT_ID      : wait4MSG_ID(b)
                case __class__.WAIT_DATA_LEN: Wait4DataLength(b)
                case __class__.WAIT_PAYLOAD : WaitFullPayload(b)
                case __class__.WAIT_EOT     : WaitEOT(b)
                case _: raise ValueError(f"self.UartReadingStateis not handled: {state=}")

            # if state == __class__.WAIT_SOH:
            #     wait4SOH(b)
            # elif state == __class__.WAIT_ID:
            #     wait4MSG_ID(b)
            # elif state == __class__.WAIT_DATA_LEN:
            #     Wait4DataLength(b)
            # elif state == __class__.WAIT_PAYLOAD:
            #     WaitFullPayload(b)
            # elif state == __class__.WAIT_EOT:
            #     WaitEOT(b)


# ---------------------------------------------------------------------------
# Example usage
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    for p in serial.tools.list_ports.comports():
        print(f"{p.device} - {p.description} - location: {p.location}")

    UART_KW_ARGS = {"port": "COM6", "baudrate": 115200}
    receiver = UARTReceiver(
        uart_kw_args=UART_KW_ARGS,
        MsgTypeConfigFile=r"UART_Data_Config.csv",
        frame_timeout=0.5,
        RootDesFolder = "data_1",
        # clearIfExist = False
    )

    receiver.start()
    try:
        while True:
            time.sleep(1)  # main thread is free to do other work
    except KeyboardInterrupt:
        pass
    finally:
        receiver.stop()
