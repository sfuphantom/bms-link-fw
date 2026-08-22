"""
uart_receiver.py

Interrupt-style UART receiver for pyserial (PC / Raspberry Pi / Linux).

Framing:
    [SOH][msg_ID][payload bytes ...][EOT]

    SOH (0x01) - Start of Header. Lets the parser resynchronize after any
                 corruption/desync instead of hunting for "\n" inside binary
                 data (which can false-trigger, since a payload byte can
                 legitimately equal 0x0A).
    msg_ID     - one byte. Looked up in MSG_TYPES to know:
                   * how many payload bytes follow
                   * how to unpack them (struct format)
    payload    - exact number of bytes for that msg_ID (no delimiter search --
                 we just read N bytes).
    EOT (0x04) - End of Transmission. Validates that we read the frame
                 correctly (if this byte isn't EOT, the frame is discarded).

"Interrupt-driven" here means: a dedicated thread blocks on serial.read(),
which sleeps efficiently (no busy-wait / no manual polling loop checking
in_waiting) until bytes actually arrive, and dispatches a callback per
completed frame. This is the standard pyserial equivalent of an ISR, since
plain CPython has no access to a real UART hardware interrupt.

Per-frame timeout: once SOH is seen, a clock starts. If the full frame
(msg_ID + payload + EOT) doesn't arrive within FRAME_TIMEOUT seconds, the
partial data is discarded and the parser goes back to hunting for SOH.
"""

import serial
import serial.tools.list_ports
import struct
import threading
import time
import logging
# import pandas as pd
import csv
import os
from dataclasses import dataclass, InitVar
from enum import Enum, auto
from typing import Callable


logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("uart_receiver")

# ---------------------------------------------------------------------------
# Message type table: msg_ID -> (name, payload length in bytes, struct format)
# Add your real message types here.
# ---------------------------------------------------------------------------


def get_element_count(fmt):
    if fmt == "s":
        return 0, 1
    byte_size = struct.calcsize(fmt)
    eleCount = len(struct.unpack(fmt, b'\x00' * byte_size))
    return byte_size, eleCount

def search_string_list(list, target : str) -> int:
    tar_low : str = target.lower()
    for (i, ele) in enumerate(list):
        if ele.lower() == tar_low:
            return i
        
    return -1


def appendRow2CSV_helper(file, timestamp, values, col_names=None):

    with open(file, mode='a') as f:
        writer = csv.writer(f)
        row = [timestamp, *values]
        writer.writerow(row)
        # for (t, v) in zip(timestamp, values):
        #     row = [t, *v]
        #     writer.writerow(row)



def CellVoltagesData_handler(file, data, timeData):
    data_ftm = [float(d) / 10000 for d in data]
    appendRow2CSV_helper(file, timeData, data_ftm)
    pass

def IMDData_handler(file, data, timeData):
    data_ftm = [data[0]/2, data[1]/4]
    appendRow2CSV_helper(file, timeData, data_ftm)
    pass


# def Debug_DataFormatHandler(data: str) -> list[str]:
#     data_ftm = data.split("\\")
#     return data_ftm
# def Log_DataFormatHandler(data: str) -> list[str]:
#     data_ftm = data.split("\\")
#     return data_ftm
# def CellVoltages_DataFormatHandler(data: list[int]) -> list[str]:
#     data_ftm = [float(d) / 10000 for d in data]
#     return data_ftm

# def IMD_DataFormatHandler(data: list[int]) -> list[str]:
#     data_ftm = [data[0]/2, data[1]/4]
#     return data_ftm

# def CellVoltages_DataFormatHandler(data: list[int]) -> list[str]:
#     data_ftm = [float(d) / 10000 for d in data]
#     return data_ftm
# def IMD_DataFormatHandler(data) -> list[str]:
#     data_ftm = [data[0]/2, data[1]/4]
#     return data_ftm


# DataFormatHandlers = {
#     # "Debug" : 
#     # "Log
#     "Cell_Voltages" :   CellVoltagesData_handler,
#     "IMD"           :   IMDData_handler,
#     # "Charger"       :
#     # "Cell_Temp"     :
#     # "Fans"          :
#     # "FullBattery"   :
#     # "SlaveState"    :
#     # "DCC"           :

# }

@dataclass(frozen=True, kw_only=True)
class MSG_Config:
    name: str
    file: str
    fmt: str
    msgSize: int
    NumberOfElement : int
    function: Callable



class UartReadingState_Enum(Enum):
    NOT_READING: int = 0
    WAIT_SOH: int = auto()
    WAIT_ID: int = auto()
    WAIT_DATA_LEN: int = auto()
    WAIT_PAYLOAD: int = auto()
    WAIT_EOT: int = auto()

class UARTReceiver:
    DEFAULT_ROOT_FILE = "data"
    DEFAULT_TIME_COL_NAME = "time"
    CONFIG_FILE_COLS = ("Enable", "msg_ID", "Name", "file_rel", "fmt", "header")

    SOH = 0x01  # Start of Header
    EOT = 0x04  # End of Transmission
    # States for the frame parser state machine
    # WAIT_SOH, WAIT_ID, WAIT_DATA_LEN, WAIT_PAYLOAD, WAIT_EOT = range(5) # states

    # if you are expecting the sender to tell you the data length
    DATA_LEN_FMT = ">H"
    DATA_LEN_LEN = struct.calcsize(DATA_LEN_FMT)

    DEFAULT_UART_KW_ARGS = {"port"      : "COM9" , 
                            "baudrate"  : 115200 , 
                            "timeout"   : 0.1    }

    def __init__(self, *, uart_kw_args, #port="COM5", baudrate=115200, read_timeout=0.1,
                 MsgTypeConfigFile, RootDesFolder=None, clearIfExist = True, TIME_COL_NAME = None, 
                 on_message=None, frame_timeout=0.5,):
        
        """
        port          : e.g. "/dev/ttyUSB0" or "COM5"
        baudrate      : UART baud rate
        msg_types     : dict as in MSG_TYPES above
        on_message    : callback(name: str, msg_id: int, value) called from
                         the reader thread whenever a valid frame completes
        frame_timeout : max seconds allowed between SOH and a completed frame
        read_timeout  : pyserial's own per-byte read timeout (keeps the
                         thread responsive to stop() without busy-polling)
        """

        self.UartReadingState: UartReadingState_Enum = UartReadingState_Enum.NOT_READING
        self.TIME_COL_NAME = TIME_COL_NAME or __class__.DEFAULT_TIME_COL_NAME
        # Get root Folder
        if RootDesFolder is None:
            folderExist = True
            FolderSubscript = 0
            
            while(folderExist):
                RootDesFolder = f"{__class__.DEFAULT_ROOT_FILE}_{FolderSubscript}"
                folderExist = os.path.isdir(RootDesFolder)
                FolderSubscript += 1
        
        os.makedirs(RootDesFolder,exist_ok=True)

        self.msg_types = {}

        with open(MsgTypeConfigFile, 'r') as MTCF:
            csv_reader = csv.reader(MTCF, delimiter=',', quotechar='"')
            header = next(csv_reader)
            header = [h.strip() for h in header]

            file_col = {col: search_string_list(header, col) for col in __class__.CONFIG_FILE_COLS}
            assert -1 not in file_col.values(), f"in header: {header} could not find {[key for key, val in file_col.items() if val == -1]} "

            for row in csv_reader:

                row_sorted = {col : row[idx].strip() for col,idx in file_col.item()}

                if row_sorted["Enable"].strip().lower() == "false":
                    continue

                MSG_ID = int(row_sorted["msg_ID"].strip(), 0)
                MSG_TYPE_NAME = row_sorted["Name"].strip()
                MSG_FMT :str = row_sorted["fmt"].strip()
                DES_FILE :str = os.path.join(RootDesFolder, row_sorted["file_rel"].strip())
                MSG_SIZE, NUMBER_OF_COL = get_element_count(MSG_FMT)

                MSG_ID_CONFIG :MSG_Config = MSG_Config(name = MSG_TYPE_NAME,  file = DES_FILE, fmt = MSG_FMT, msgSize = MSG_SIZE, NumberOfElement = NUMBER_OF_COL)
                self.msg_types[MSG_ID] = MSG_ID_CONFIG 

                if(clearIfExist or not os.path.exists(DES_FILE)):          
                    UNIQUE_COLS = row_sorted["header"].split(',')
                    NUM_OF_UNIQUE_COLS = len(UNIQUE_COLS)

                    if NUMBER_OF_COL % NUM_OF_UNIQUE_COLS != 0:
                        raise ValueError(f"{NUM_OF_UNIQUE_COLS=}, {NUMBER_OF_COL=}, {NUMBER_OF_COL % NUM_OF_UNIQUE_COLS =} (should be zero),\n{MSG_ID=}, {MSG_FMT=}")

                    ALL_COLS_HEADERS = [self.TIME_COL_NAME] + [f"{UNIQUE_COLS[i%NUM_OF_UNIQUE_COLS].strip()} {i}" for i in range(NUMBER_OF_COL)]
                    with open(DES_FILE, newline='\n', mode='w') as DF:
                        writer = csv.writer(DF)
                        writer.writerow(ALL_COLS_HEADERS)

        # self.initTime = time.monotonic()
        self.StartTime : int = 0
        self.on_message = on_message
        self.frame_timeout = frame_timeout

        for key, val in __class__.DEFAULT_UART_KW_ARGS.items():
            if key not in uart_kw_args:
                uart_kw_args[key] = val

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

        self.UartReadingState = UartReadingState_Enum.NOT_READING
        log.info("UART receiver stopped")
    # -- internal -------------------------------------------------------------
    def _dispatch(self, msg_id, payload, frame_start_time):
        timeData_Shifted: float = frame_start_time - self.StartTime
        timeData_formated: str = f"{timeData_Shifted:.2f}"

        MSG_ID_CONFIG = self.msg_types[msg_id]

        # data = MSG_ID_CONFIG.func(payload, MSG_ID_CONFIG.fmt)
        # row2append = list(timeData_formated) +  list(data)
        # with open(MSG_ID_CONFIG.file, mode = 'a') as f
            # writer = csv.writer(f)
            # writer.writerow(row2append)

        if MSG_ID_CONFIG.fmt != 's':
            data = struct.unpack(MSG_ID_CONFIG.fmt, bytes(payload))
        else:
            data = struct.unpack(f"{len(payload)}s", bytes(payload))
        # data = list(data_tuple)

        print(f"{msg_id=}, {data=}")
        file = MSG_ID_CONFIG.file

        match MSG_ID_CONFIG.name:
            case "Cell_Voltages":
                CellVoltagesData_handler(file, data, timeData_formated)
            case "IMD":
                IMDData_handler(file, data, timeData_formated)
            case "SlaveState":
                pass
            case "DCC":
                pass
                
            case _:
                raise ValueError(f"unhandled Massage type {MSG_ID_CONFIG}")

    def check_timedOut(self, msg_id: list, payload: bytearray, frame_start_time: list):
            if self.UartReadingState != UartReadingState_Enum.WAIT_SOH and (time.monotonic() - frame_start_time) > self.frame_timeout:
                log.warning("Frame timeout waiting for msg_ID=%s, discarding partial data", msg_id)

                self.UartReadingState, msg_id[0], payload[0]  = UartReadingState_Enum.WAIT_SOH, None,  bytearray()
    def wait4SOH(self, b: bytes, frame_start_time: list):
        if b == __class__.SOH:
            self.UartReadingState, frame_start_time[0] = UartReadingState_Enum.WAIT_ID, time.monotonic()
    def wait4MSG_ID(self, b: bytes, msg_id: list, expected_len: list):
        if b not in self.msg_types:
            log.warning("Unknown msg_ID 0x%02X, discarding and resyncing", b)
            self.UartReadingState = UartReadingState_Enum.WAIT_SOH
            return
        
        msg_id_config = self.msg_types[b]
        msg_len = msg_id_config.msgSize

        msg_id[0] = b
        if msg_id_config.fmt == 's':
            self.UartReadingState = UartReadingState_Enum.WAIT_DATA_LEN
        else:
            self.UartReadingState = UartReadingState_Enum.WAIT_PAYLOAD
            expected_len[0] = msg_len

    def Wait4DataLength(self, b:bytes,  expected_len: list, payload: bytearray):
        payload.append(b)

        if len(payload) < __class__.DATA_LEN_LEN:
            return

        self.UartReadingState = UartReadingState_Enum.WAIT_PAYLOAD
        msg_len = struct.unpack(__class__.DATA_LEN_FMT, bytes(payload))
        expected_len = msg_len[0] + __class__.DATA_LEN_LEN

    def WaitFullPayload(self, b: bytes, payload: bytearray, expected_len):
        payload.append(b)
        if len(payload) >= expected_len[0]:
            self.UartReadingState = UartReadingState_Enum.WAIT_EOT
    def waitCRC():
        pass
    def WaitEOT(self, b, msg_id, payload, frame_start_time):
        if b == __class__.EOT:
            self._dispatch(msg_id, payload, frame_start_time)
        else:
            log.warning(
                "Bad end byte (got 0x%02X, expected 0x%02X) for msg_ID=0x%02X -- "
                "msg_ID was likely wrong or data corrupted, discarding",
                b, __class__.EOT, msg_id,
            )
        self.UartReadingState, msg_id[0], payload[0] = UartReadingState_Enum.WAIT_SOH, None, bytearray()
    # -- Main reading -------------------------------------------------------------
    def _reader_loop(self):
        self.UartReadingState = UartReadingState_Enum.WAIT_SOH
        msg_id = [None]
        expected_len = [0]
        payload = bytearray()

        frame_start_time = [0.0]
        last_byte_time = [0.0]
        self.StartTime = time.monotonic()


        # sentLength = bytearray()



        while not self._stop_event.is_set():
            self.check_timedOut([msg_id], [payload], frame_start_time)

            # Blocking read (bounded by read_timeout so we can check stop_event
            # periodically). This is what makes it "interrupt-style" rather
            # than a manual polling loop: the thread sleeps until a byte is
            # available or the short timeout elapses.

            byte = self.__SER.read(1)
            if not byte: # Nothing arrived this tick. If we're mid-frame, check timeout.
                continue
            
            b = byte[0]
            # print(b, end=" ")
            # match(state):
            #     case UartReadingState_Enum.WAIT_SOH     : wait4SOH(b)
            #     case UartReadingState_Enum.WAIT_ID      : wait4MSG_ID(b)
            #     case UartReadingState_Enum.WAIT_DATA_LEN: Wait4DataLength(b)
            #     case UartReadingState_Enum.WAIT_PAYLOAD : WaitFullPayload(b)
            #     case UartReadingState_Enum.WAIT_EOT     : WaitEOT(b)
            #     case _: raise ValueError(f"self.UartReadingStateis not handled: {state=}")
            
            if self.UartReadingState== UartReadingState_Enum.WAIT_SOH:
                wait4SOH(b)
            elif self.UartReadingState== UartReadingState_Enum.WAIT_ID:
                wait4MSG_ID(b)
            elif self.UartReadingState== UartReadingState_Enum.WAIT_DATA_LEN:
                Wait4DataLength(b)
            elif self.UartReadingState== UartReadingState_Enum.WAIT_PAYLOAD:
                WaitFullPayload(b)
            elif self.UartReadingState== UartReadingState_Enum.WAIT_EOT:
                WaitEOT(b)

# ---------------------------------------------------------------------------
# Example usage
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    def handle_message(name, msg_id, value):
        print(f"Received {name} (0x{msg_id:02X}): {value}")


    for p in serial.tools.list_ports.comports():
        print(f"{p.device} - {p.description} - location: {p.location}")

    UART_KW_ARGS= {"port":"Com6", "baudrate":115200}
    receiver = UARTReceiver(
        uart_kw_args = UART_KW_ARGS,
        # port =  "Com6", #/dev/ttyUSB0",       # change to your port, e.g. "COM5" on Windows
        RootDesFolder = "data_0",
        MsgTypeConfigFile = r"UART_Data_Config.csv",
        # baudrate=115200,
        on_message=handle_message,
        frame_timeout=0.5,
    )

    receiver.start()
    try:
        while True:
            time.sleep(1)  # main thread is free to do other work
    except KeyboardInterrupt:
        pass
    finally:
        receiver.stop()


