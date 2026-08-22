"""
GUI_helpers.py

LineGraphClass       - framework-agnostic wrapper around a matplotlib Figure/Axes
                        with N lines, optional text annotations, and format
                        control. Knows nothing about Tkinter/PyQt/CSV files --
                        just plotting.

LineGraphWidgetClass - LineGraphClass + Tkinter embedding + periodic reload from
                        a CSV data file (timestamp column + one column per signal).

Requires: matplotlib, pandas
    py -m pip install matplotlib pandas

NOTE ON FRAMEWORK: this assumes Tkinter since no GUI framework has been chosen
yet and Tkinter ships with Python. If you move to PyQt/PySide, LineGraphClass
does not change -- only LineGraphWidgetClass's canvas creation and scheduling
(FigureCanvasTkAgg -> FigureCanvasQTAgg, widget.after() -> QTimer) would need
to change.
"""

import os
import logging

import matplotlib
matplotlib.use("TkAgg")  # remove/change this line if you end up embedding elsewhere
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk
import pandas as pd

log = logging.getLogger("GUI_helpers")


class LineGraphClass:
    """
    Generic multi-line matplotlib plot. Creates its own Figure/Axes but does
    not embed itself anywhere -- that's the widget subclass's job (or embed
    `.figure` yourself if you're not using LineGraphWidgetClass).
    """

    def __init__(self, numberOfLines, Graph_fmt=None, Line_fmt=None, text_fmt=None):
        """
        numberOfLines : how many lines (signals) this plot will hold
        Graph_fmt     : dict of axes-level formatting, e.g.
                         {"title": "Cell Voltages", "xlabel": "Time (s)",
                          "ylabel": "Voltage (V)", "xlim": None, "ylim": None,
                          "grid": True, "legend": True}
        Line_fmt      : a single dict applied to every line, OR a list of
                         per-line dicts (length == numberOfLines), e.g.
                         {"color": "tab:blue", "linestyle": "-", "label": "Cell 1"}
        text_fmt      : dict controlling the optional status-text annotations,
                         e.g. {"x": 0.02, "y": 0.95, "fontsize": 9,
                               "bbox": {"facecolor": "white", "alpha": 0.7}}
        """
        self.numberOfLines = numberOfLines
        self.Graph_fmt = Graph_fmt or {}
        self.text_fmt = text_fmt or {
            "x": 0.02, "y": 0.95, "fontsize": 9,
            "bbox": {"facecolor": "white", "alpha": 0.7},
        }

        self.figure, self.ax = plt.subplots()
        self.lines = []
        self.texts = {}  # text_ID -> Text artist, supports multiple labels

        line_fmts = self._normalize_line_fmt(Line_fmt)
        for i in range(numberOfLines):
            (line,) = self.ax.plot([], [], **line_fmts[i])
            self.lines.append(line)

        self._apply_graph_fmt()

    # -- formatting helpers ---------------------------------------------------

    def _normalize_line_fmt(self, Line_fmt):
        """Turn Line_fmt (None / single dict / list of dicts) into a list of
        exactly numberOfLines dicts, one per line."""
        if Line_fmt is None:
            return [{} for _ in range(self.numberOfLines)]
        if isinstance(Line_fmt, dict):
            return [dict(Line_fmt) for _ in range(self.numberOfLines)]
        if isinstance(Line_fmt, list):
            if len(Line_fmt) != self.numberOfLines:
                raise ValueError(
                    f"Line_fmt list has {len(Line_fmt)} entries, expected {self.numberOfLines}"
                )
            return Line_fmt
        raise TypeError("Line_fmt must be None, a dict, or a list of dicts")

    def _apply_graph_fmt(self):
        fmt = self.Graph_fmt
        if "title" in fmt:
            self.ax.set_title(fmt["title"])
        if "xlabel" in fmt:
            self.ax.set_xlabel(fmt["xlabel"])
        if "ylabel" in fmt:
            self.ax.set_ylabel(fmt["ylabel"])
        if fmt.get("xlim") is not None:
            self.ax.set_xlim(fmt["xlim"])
        if fmt.get("ylim") is not None:
            self.ax.set_ylim(fmt["ylim"])
        if fmt.get("grid"):
            self.ax.grid(True)
        if fmt.get("legend"):
            self.ax.legend()

    def changeLineFormat(self, Line_fmt):
        """Re-apply formatting (color, style, label, ...) to existing lines
        without touching their data."""
        line_fmts = self._normalize_line_fmt(Line_fmt)
        for line, fmt in zip(self.lines, line_fmts):
            line.set(**fmt)
        if self.Graph_fmt.get("legend"):
            self.ax.legend()
        self._redraw()

    # -- data -------------------------------------------------------------

    def updateData(self, X_Data, Y_Data):
        """
        X_Data, Y_Data: either
          - a single 1D array shared by all lines (e.g. one common time axis), or
          - a list of 1D arrays, one per line

        A line is skipped (left unchanged) if its corresponding entry is None,
        so a partial update doesn't wipe other lines.
        """
        X_list = self._broadcast(X_Data)
        Y_list = self._broadcast(Y_Data)

        for line, x, y in zip(self.lines, X_list, Y_list):
            if x is None or y is None:
                continue
            line.set_data(x, y)

        self.ax.relim()
        self.ax.autoscale_view()
        self._redraw()

    def _broadcast(self, data):
        """Turn a single array or a list of per-line arrays into a list of
        length numberOfLines."""
        if data is None:
            return [None] * self.numberOfLines
        if isinstance(data, (list, tuple)) and len(data) == self.numberOfLines \
                and all(hasattr(d, "__len__") for d in data):
            return list(data)
        return [data] * self.numberOfLines  # single shared array

    def clearGraph(self):
        for line in self.lines:
            line.set_data([], [])
        for text in self.texts.values():
            text.set_text("")
        self._redraw()

    # -- text annotations ---------------------------------------------------

    def updateText(self, text_ID, text_str):
        """Create (on first use) or update a text annotation identified by
        text_ID. Multiple text_IDs stack vertically."""
        if text_ID not in self.texts:
            self.texts[text_ID] = self.ax.text(
                self.text_fmt.get("x", 0.02),
                self.text_fmt.get("y", 0.95) - 0.06 * len(self.texts),
                "",
                transform=self.ax.transAxes,
                fontsize=self.text_fmt.get("fontsize", 9),
                bbox=self.text_fmt.get("bbox"),
                verticalalignment="top",
            )
        self.texts[text_ID].set_text(text_str)
        self._redraw()

    # -- misc ---------------------------------------------------------------

    def _redraw(self):
        """Redraw the attached canvas if there is one (widget subclass);
        otherwise no-op -- you can still call plt.show() yourself."""
        canvas = getattr(self.figure, "canvas", None)
        if canvas is not None:
            canvas.draw_idle()


class LineGraphWidgetClass(LineGraphClass):
    """
    LineGraphClass embedded in a Tkinter widget, refreshing itself from a CSV
    file every updatePeriod milliseconds.

    Expected CSV format: a timestamp column plus one column per signal, e.g.
        timestamp,cell1_v,cell2_v,cell3_v
        0.10,3.71,3.69,3.70
        0.20,3.71,3.68,3.70
        ...
    """

    def __init__(self, parent, numberOfLines, Graph_fmt, DataFile, updatePeriod,
                 Line_fmt=None, text_fmt=None, x_col="timestamp", y_cols=None,
                 maxPoints=None):
        """
        parent      : Tkinter parent widget/frame to embed the plot canvas into
        DataFile    : path to the CSV file being written by the data source
        updatePeriod: milliseconds between reads of DataFile
        x_col       : CSV column used as the shared X axis (e.g. "timestamp")
        y_cols      : list of CSV column names to plot, one per line (length
                       must equal numberOfLines); if None, defaults to the
                       first numberOfLines columns other than x_col
        maxPoints   : if set, only the most recent maxPoints rows are plotted
                       (keeps rendering fast once the CSV gets long)
        """
        super().__init__(numberOfLines, Graph_fmt, Line_fmt, text_fmt)

        self.parent = parent
        self.DataFile = DataFile
        self.updatePeriod = updatePeriod
        self.x_col = x_col
        self.y_cols = y_cols
        self.maxPoints = maxPoints

        self.canvas = FigureCanvasTkAgg(self.figure, master=parent)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        self._after_id = None
        self._running = False

    # -- animation control ----------------------------------------------------

    def startAnimate(self):
        if self._running:
            return
        self._running = True
        self.updateAnimate()  # fires immediately, then reschedules itself

    def stopAnimate(self):
        self._running = False
        if self._after_id is not None:
            self.parent.after_cancel(self._after_id)
            self._after_id = None

    def updateAnimate(self):
        """Read the latest data from DataFile and refresh the plot, then
        reschedule itself via Tkinter's after(). This polls the FILE on the
        GUI thread -- the UART side stays interrupt-driven and just writes
        the CSV independently; this class never touches the serial port."""
        try:
            self._reloadFromFile()
        except (FileNotFoundError, pd.errors.EmptyDataError):
            # File doesn't exist yet, or was mid-write when we read it --
            # skip this cycle instead of crashing the GUI loop.
            log.debug("DataFile not ready yet: %s", self.DataFile)
        except Exception as e:
            log.warning("Failed to read %s: %s", self.DataFile, e)

        if self._running:
            self._after_id = self.parent.after(self.updatePeriod, self.updateAnimate)

    def _reloadFromFile(self):
        if not os.path.exists(self.DataFile):
            raise FileNotFoundError(self.DataFile)

        df = pd.read_csv(self.DataFile)
        if df.empty:
            return

        y_cols = self.y_cols or [c for c in df.columns if c != self.x_col][: self.numberOfLines]
        if len(y_cols) != self.numberOfLines:
            raise ValueError(
                f"Expected {self.numberOfLines} y_cols, got {len(y_cols)}: {y_cols}"
            )

        if self.maxPoints is not None:
            df = df.tail(self.maxPoints)

        x = df[self.x_col].to_numpy()
        y_list = [df[col].to_numpy() for col in y_cols]

        self.updateData(x, y_list)


# ---------------------------------------------------------------------------
# Example usage
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    root = tk.Tk()
    root.title("BMS Live Plot Demo")

    graph = LineGraphWidgetClass(
        parent=root,
        numberOfLines=3,
        Graph_fmt={"title": "Cell Voltages", "xlabel": "Time (s)",
                   "ylabel": "Voltage (V)", "grid": True, "legend": True},
        DataFile="bms_data.csv",
        updatePeriod=500,  # ms
        Line_fmt=[
            {"color": "tab:blue", "label": "Cell 1"},
            {"color": "tab:orange", "label": "Cell 2"},
            {"color": "tab:green", "label": "Cell 3"},
        ],
        y_cols=["cell1_v", "cell2_v", "cell3_v"],
        maxPoints=200,
    )
    graph.startAnimate()

    root.protocol("WM_DELETE_WINDOW", lambda: (graph.stopAnimate(), root.destroy()))
    root.mainloop()
