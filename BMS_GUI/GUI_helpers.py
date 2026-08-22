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
# matplotlib.use("TkAgg")  # remove/change this line if you end up embedding elsewhere
import matplotlib.pyplot as plt
# from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
# import tkinter as tk
import pandas as pd
import time
import numpy as np

log = logging.getLogger("GUI_helpers")


class LineGraphClass:
    """
    Generic multi-line matplotlib plot. Creates its own Figure/Axes but does
    not embed itself anywhere -- that's the widget subclass's job (or embed
    `.figure` yourself if you're not using LineGraphWidgetClass).
    """

    def __init__(self,   file,  ycols = None, 
                 X_WinSize = 0, ylims = None,
                 fig_fmt = None, Graph_fmt = None, Line_fmt = None):
        """
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

        # self.text_fmt = text_fmt or {
        #     "x": 0.02, "y": 0.95, "fontsize": 9,
        #     "bbox": {"facecolor": "white", "alpha": 0.7},
        # }
        
        self.X_WINSIZE = X_WinSize
        self.minTime = 0
        self.maxTime = 0

        self.file = file
        self.fig, self.ax = plt.subplots()
        self.ax.set_xlim([0, max(1, X_WinSize*2)])
        

        self._loadFromFile()

        # if Graph_fmt is not None:
        #     self._init_graph_fmt(Graph_fmt)

        self.default_ylim = ylims or self.ax.get_ylim()
        self.cols = ycols
        self.fig.show()


    # -- formatting helpers ---------------------------------------------------

    def _init_graph_fmt(self, fmt):
        for (key, val) in fmt.items():
            self._change_graph_fmt(self, key, val)

    def _change_graph_fmt(self, attr:str, val = False):
        match attr:
            case "title":
                self.ax.set_title(val)
            case "xlabel":
                self.ax.set_xlabel(val)
            case "ylabel":
                self.ax.set_ylabel(val)
            case "xlim":
                self.ax.set_xlim(val)
            case "ylim":
                self.ax.set_ylim(val)
            case "grid":
                self.ax.grid(val)
            case "legend":
                self.ax.legend()

    def changeLineFormat(self, Line_fmt, lineId):

        # self.lines[lineId].set(**Line_fmt)
        self.ax.get_lines()[lineId].set(**Line_fmt)

        # if self.Graph_fmt.get("legend"):

        self.ax.legend()
        self._redraw()

    # -- data -------------------------------------------------------------

    def getData(self):
        if not os.path.exists(self.file):
            raise FileNotFoundError(self.file)
        if os.path.isdir(self.file):
            raise IsADirectoryError(self.file)
        
        if os.path.getsize(self.file) == 0:
            log.warning("Warning: CSV file is 0 bytes")
            return None, None
        
        def getDF(file, minTime, chunksize):
            if minTime > 0:
                chunks = []
                read_iter = pd.read_csv(file, header=0, index_col=0, chunksize=chunksize)
                for chunk in read_iter:
                    if chunk.index[0] >= self.minTime:
                        chunks.append(chunk)
                        break
                    elif chunk.index[-1] >= self.minTime:
                        chunks.append(chunk[chunk.index >= self.minTime])
                        break

                return pd.concat([*chunks, *read_iter])
            else:
                return pd.read_csv(file, header=0, index_col=0) # fix Read Error

    
        try:
            df = getDF(self.file, self.minTime, chunksize=500)
        except pd.errors.EmptyDataError:
            print("ERROR: CSV has no readable columns")
            return None, None

        except pd.errors.ParserError as e:
            raise e
            # print(f"ERROR: Invalid CSV format: {e}")
            # return None

        except PermissionError:
            raise e
            # print("ERROR: Permission denied")
            # return None

        except UnicodeDecodeError:
            raise e
            # print("ERROR: CSV encoding could not be decoded")
            # return None

        except OSError as e:
            raise e
            # print(f"ERROR: File system error: {e}")
            # return None

        if df.empty:
            log.warning("ERROR: CSV contains no data")
            return None, False
        if df.shape[1] == 0:
            log.warning("ERROR: CSV has no columns")
            return None, None

        # if self.minTime != 0:
        #     df = df[df.index >= self.minTime]

        if df.index[-1] <= self.maxTime:
            return None, False
        else:
            self.maxTime = df.index[-1]


        return df, True

    def _loadFromFile(self):
        df, NewData = self.getData()

        if not NewData:
            return

        # lines = self.ax.get_lines()
        # self.lines.clear()
        for col in df.columns:
            self.ax.plot(df[col], label=col)
            # self.lines.append(line)

    def _reloadFromFile(self):
        df, NewData = self.getData()
        
        if not NewData:
            return

        lines = self.ax.get_lines()
        for line_k, col in zip(lines, df.columns):
            line_k.set_data(df.index, df[col])

        self._resize_lims(self.maxTime, df.min().min(), df.max().max(), 10)

    def clearGraph(self):
        for line in self.lines:
            line.set_data([], [])
        for text in self.texts.values():
            text.set_text("")
        self._redraw()

    def _redraw(self):
        """Redraw the attached canvas if there is one (widget subclass);
        otherwise no-op -- you can still call plt.show() yourself."""
        canvas = getattr(self.fig, "canvas", None)

        if canvas is not None:
            canvas.draw_idle()

    def _resize_lims(self, x_max, y_min, y_max, x_max_jump_factor = 1):
            x_ax_max = self.ax.get_xlim()[1]

            y_ax_max = self.default_ylim[1]
            y_ax_min = self.default_ylim[0]

            x_ax_min = self.minTime 

            changeAxis = False

            if x_ax_max < x_max:
                x_ax_min = 0 if self.X_WINSIZE == 0 else x_max - self.X_WINSIZE
                x_ax_max = x_max + x_max_jump_factor * self.X_WINSIZE
                self.minTime = x_ax_min
                self.ax.set_xlim([x_ax_min, x_ax_max])

                changeAxis = True

            if y_ax_max < y_max:
                y_ax_max = y_max
                changeAxis = True
            if y_ax_min > y_min:
                y_ax_min = y_min
                changeAxis = True

            if changeAxis:
                self.ax.set_ylim([y_ax_min, y_ax_max])
                self._redraw()

                



    # -- text annotations ---------------------------------------------------

    # def updateText(self, text_ID, text_str):
    #     """Create (on first use) or update a text annotation identified by
    #     text_ID. Multiple text_IDs stack vertically."""
    #     if text_ID not in self.texts:
    #         self.texts[text_ID] = self.ax.text(
    #             self.text_fmt.get("x", 0.02),
    #             self.text_fmt.get("y", 0.95) - 0.06 * len(self.texts),
    #             "",
    #             transform=self.ax.transAxes,
    #             fontsize=self.text_fmt.get("fontsize", 9),
    #             bbox=self.text_fmt.get("bbox"),
    #             verticalalignment="top",
    #         )
    #     self.texts[text_ID].set_text(text_str)
    #     self._redraw()

    # -- misc ---------------------------------------------------------------



# ---------------------------------------------------------------------------
# Example usage
# ---------------------------------------------------------------------------
# if __name__ == "__main__":
#     root = tk.Tk()
#     root.title("BMS Live Plot Demo")

#     graph = LineGraphWidgetClass(
#         parent=root,
#         numberOfLines=3,
#         Graph_fmt={"title": "Cell Voltages", "xlabel": "Time (s)",
#                    "ylabel": "Voltage (V)", "grid": True, "legend": True},
#         DataFile="bms_data.csv",
#         updatePeriod=500,  # ms
#         Line_fmt=[
#             {"color": "tab:blue", "label": "Cell 1"},
#             {"color": "tab:orange", "label": "Cell 2"},
#             {"color": "tab:green", "label": "Cell 3"},
#         ],
#         y_cols=["cell1_v", "cell2_v", "cell3_v"],
#         maxPoints=200,
#     )
#     graph.startAnimate()

#     root.protocol("WM_DELETE_WINDOW", lambda: (graph.stopAnimate(), root.destroy()))
#     root.mainloop()



if __name__ == "__main__":
    import csv
    # import random

    rng = np.random.default_rng()

    

    def writeRanData2CSV(currentTimeIndex, mode='a'):
        tic = time.time()
        # NumOfStep = np.random.randint(1,10)

        # for key, val in TheNewData.items():
        #     if key is TIME_COL_NAME:
        #         continue
        #     NewData = rng.random(NumOfStep).tolist()
        #     val += NewData

        # TheNewData[TIME_COL_NAME] += [currentTimeIndex[0] + idx*TIME_STEP_SIZE for idx in range(NumOfStep)]
        # currentTimeIndex[0] += TIME_STEP_SIZE * NumOfStep

        numberOfrows = np.random.randint(0,20)
        sine_t = np.linspace(currentTimeIndex[0], numberOfrows * TIME_STEP_SIZE + currentTimeIndex[0], numberOfrows)
        sine_y = 10*np.sin(sine_t/np.pi)

        ADD_SINE = False

        with open(FILE, newline='\n', mode=mode) as f:
            writer = csv.writer(f, delimiter=',')
            for i in range(numberOfrows):
                data = rng.random(NUM_COL)
                data_list = list(data + (sine_y[i] if ADD_SINE else 0))
                row = currentTimeIndex + data_list
                writer.writerow(row)
                currentTimeIndex[0] += TIME_STEP_SIZE

        toc = time.time()
        return toc - tic


    FILE = "data0/Test.csv"
    NUM_COL = 512
    COL_NAMES = [f"Col{i}" for i in range(NUM_COL)]
    TIME_COL_NAME = "time"
    TIME_STEP_SIZE = 0.1
    currentTimeIndex : float = [0]

    N=500
    time_taken = [0] * (N+1)
    f_time_taken = [0] * (N+1)

    TheNewData = {col:[] for col in ([TIME_COL_NAME] + COL_NAMES)}

    with open(FILE, newline='\n', mode='w') as f:
        writer = csv.writer(f, delimiter=',')
        writer.writerow([TIME_COL_NAME] + COL_NAMES)

    f_t_diff = writeRanData2CSV(currentTimeIndex, mode='a')

    tic = time.time()
    G = LineGraphClass(FILE, X_WinSize=2)
    toc = time.time()

    t_dif = toc - tic
    time_taken[0] = t_dif
    f_time_taken[0] = f_t_diff
    for i in range(1, N+1):
        
        f_t_diff = writeRanData2CSV(currentTimeIndex, mode='a')

        tic = time.time()
        G._reloadFromFile()
        toc = time.time()

        t_dif = toc-tic
        # f_t_diff = f_toc-f_tic

        time_taken[i] = t_dif
        f_time_taken[i] = f_t_diff

        print(f"{t_dif=}, {i=}, {f_t_diff=}")
        plt.pause(0.001)

    print("\n\n"+"results".center(30,'='))

    def printStats(time_arr, Name):
        max = np.max(time_arr)
        mean = np.mean(time_arr)
        mid = np.median(time_arr)

        print(f"\n\n{Name}resaults:")
        print(f"max = {max}")
        print(f"mean = {mean}")
        print(f"mid = {mid}")

    printStats(time_taken[0], "read Plot Time - init")
    printStats(time_taken[1:], "read Plot Time")
    printStats(f_time_taken, "write CSV Time")

    fig, ax = plt.subplots()
    ax.plot(range(1,N+1), time_taken[1:])
    fig.show()

    figf, axf = plt.subplots()
    axf.plot(range(N+1), f_time_taken)
    figf.show()

    # ax.legend(['plot time', 'write time'])

    # fig.show()
    print("done, done")