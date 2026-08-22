import csv
from dataclasses import dataclass

# ---------------------------------------------------------------------------
# Change this in one place to scale every slave-board-dependent message
# format (SlaveState, DCC) up or down.
# ---------------------------------------------------------------------------
NUMBER_OF_SLAVES = 1
NUMBER_OF_CELLS  = NUMBER_OF_SLAVES * 12
NUMBER_OF_BATTERY_MODULES = int((NUMBER_OF_SLAVES+1)/2)
NUMBER_OF_FANS   = 8
NUMBER_OF_THEREMISTORS = 256
NUMBER_OF_DCC_BYTES = 3*NUMBER_OF_BATTERY_MODULES

HEADER_ROW = ["Enable", "msg_ID", "Name", "file_rel", "fmt", "header"]
CONFIG_FILE = r"UART_Data_Config.csv"

def make_Header(UNIQUE_COLS:list[str], NUMBER_OF_COL:int, fillerZeros:int = 0) -> list[str]:
    NUM_OF_UNIQUE_COLS = len(UNIQUE_COLS)

    assert NUMBER_OF_COL % NUM_OF_UNIQUE_COLS == 0, \
        ValueError(
            f"{NUM_OF_UNIQUE_COLS=}, {NUMBER_OF_COL=}, "
            f"{NUMBER_OF_COL % NUM_OF_UNIQUE_COLS =} (should be zero),\n"
            f"{UNIQUE_COLS=}"
        )
    i=1
    All_Cols = []
    while len(All_Cols) < NUMBER_OF_COL:
        for Col in UNIQUE_COLS:
            All_Cols.append(f"{Col}_{i}")
        i+=1
    return All_Cols

@dataclass(frozen=True, kw_only=True)
class MSG_Config:
    Enable: bool = True
    msgID: int
    name: str
    file_rel: str
    fmt: str
    header: list[str]

    def as_row(self):
        return [
            "TRUE" if self.Enable else "FALSE",
            self.msgID,
            self.name,
            self.file_rel,
            self.fmt,
            ",".join(self.header),
        ]


CELL_VOLTAGE_FMT = f">{NUMBER_OF_CELLS}H"


msgTypeRows = {
    "Debug":         MSG_Config(msgID=0,  name="Debug",         file_rel="Debug.csv",         fmt="s",                          header=["Mes"]),
    "Log":           MSG_Config(msgID=1,  name="Log",           file_rel="Log.csv",           fmt="s",                          header=["Mes"]),
    "Cell_Voltages": MSG_Config(msgID=4,  name="Cell_Voltages", file_rel="Cell_Voltages.csv", fmt=f">{NUMBER_OF_CELLS}H",       header=make_Header(["Cell"], NUMBER_OF_CELLS)),
    "IMD":           MSG_Config(msgID=5,  name="IMD",           file_rel="IMD.csv",           fmt=">BB",                         header=["freq", "duty", "Resistance"]),
    "Charger":       MSG_Config(msgID=6,  name="Charger",       file_rel="Charger.csv",       fmt=">HH?",                       header=["Volt", "Current", "Status"]),
    "Cell_Temp":     MSG_Config(msgID=7,  name="Cell_Temp",     file_rel="Cell_Temp.csv",     fmt=f">{NUMBER_OF_THEREMISTORS}H",header=make_Header(["Thermistor"], NUMBER_OF_THEREMISTORS)),
    "Fans":          MSG_Config(msgID=8,  name="Fans",          file_rel="Fans.csv",          fmt=f">{NUMBER_OF_FANS}B",        header=make_Header(["Fan"], NUMBER_OF_FANS)),
    "FullBattery":   MSG_Config(msgID=9,  name="FullBattery",   file_rel="FullBattery.csv",   fmt=">HH",                        header=["Current", "HV"]),
    # SlaveState / DCC scale with NUMBER_OF_SLAVES above.
    "SlaveState":    MSG_Config(msgID=11, name="SlaveState",    file_rel="SlaveState.csv",    fmt=f">{3 * NUMBER_OF_SLAVES}H",  header=make_Header(["2ndRef", "Temp", "SC"], 3*NUMBER_OF_SLAVES)),
    "DCC":           MSG_Config(msgID=13, name="DCC",           file_rel="DCC.csv",           fmt=f">{NUMBER_OF_DCC_BYTES}B",   header=make_Header(["Cell"], NUMBER_OF_CELLS)),
}

with open(CONFIG_FILE, mode='w', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(HEADER_ROW)
    for cfg in msgTypeRows.values():
        writer.writerow(cfg.as_row())