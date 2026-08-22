import matplotlib
import matplotlib.pyplot as plt
import pandas as pd
import os
import time
import uart_receiver as UR

# def updatePlot(file, )
# def updateVoltsPlot():

fig, ax = plt.subplots(figsize=(10, 6))

df = pd.read_csv(r"data_0\Cell_Voltages.csv")

for col in df.columns[0:5]:
    ax.plot(df[col], label=col)


fig.show()
a=1