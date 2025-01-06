# **************************************************************************************************
# @file Plotter.py
# @brief Plots a graph of all .csv files on a given folder. If no folder is given, then it will look
# for all .csv files on the cwd.
#
# @project   MIDDS
# @version   1.0
# @date      2024-12-07
# @author    @dabecart
#
# @license   This project is licensed under the MIT License - see the LICENSE file for details.
# **************************************************************************************************

import numpy as np
from pandas import read_csv
import matplotlib.pyplot as plt
import os
import random
import sys

clkFreq = 160e6
darkenFactor = 0.7
N_lpf = 150
maxPointsInPlot = 15000
applyFilter = True
N_filter = 4

deltaNanoseconds = 1e9/clkFreq

# Taken from https://gist.github.com/adewes/5884820
def getRandomColor(pastel_factor = 0.5):
    return [(x+pastel_factor)/(1.0+pastel_factor) for x in [random.uniform(0,1.0) for i in [1,2,3]]]

def colorDistance(c1,c2):
    return sum([abs(x[0]-x[1]) for x in zip(c1,c2)])

# Returns an tuple of three float.
def newColor(existingColors, pastelFactor = 0.5) -> tuple[float]:
    maxDist = None
    bestColor = None
    for i in range(0,100):
        color = getRandomColor(pastel_factor = pastelFactor)
        if not existingColors:
            return tuple(color)
        bestDist = min([colorDistance(color,c) for c in existingColors])
        if not maxDist or bestDist > maxDist:
            maxDist = bestDist
            bestColor = color
    return tuple(bestColor)

# Low Pass Filter
def lpf(x, N):
    y = np.copy(x)
    for k in range(1, len(x)):
        sample = x[max(k-N, 0):k]
        y[k] = np.sum(sample) / len(sample)
    return y

def medianFilter(x):
    xFiltered = np.copy(x)
    for n in range(len(x)):
        sample = x[max(0, n-N_filter):min(len(x), n+N_filter+1)]
        xFiltered[n] = np.median(sample)
    return xFiltered

def plotCSV(fileName, ax, graphColor):
    try:
        data = read_csv(fileName).values.astype(np.uint64)

        # Get the LSB. If 1, rising edge; if 0, falling edge. Therefore, for a delta between x1 and 
        # x2, if x2 is a falling edge, this delta will be for a high interval; whilst if x2 is a 
        # rising edge, the delta will be for a low interval.
        highInterval = (data & 1) == 0

        data = (data >> 1) * deltaNanoseconds
        freqs = (data[1:] - data[:-1])

        # Trim arrays for the deltas.
        data = data[1:]
        highInterval = highInterval[1:]

        # If data is too lengthy, reduce it.
        freqs  = freqs[::max(1,len(freqs)//maxPointsInPlot)]
        data    = data[::max(1,len(data)//maxPointsInPlot)]

        if applyFilter:
            freqs = medianFilter(freqs)
            data = medianFilter(data)

        fileName = os.path.basename(fileName)
        print(f'{fileName:<15} Count={len(freqs):<10} Avg={np.average(freqs):<10.4} Std={np.std(freqs):<10.4} Max={np.max(freqs):<10.4}')

        lpDelta = lpf(freqs, N_lpf)
        # X-Axis in seconds.
        data /= 1e9

        # Calculate the alpha of the plot as the inverse of the density: more dense, less alpha.
        dotDensity = len(data)/(max(data)-min(data))

        # for x,y,high in zip(data, deltas, highInterval):
        #     if high:
        #         ax.scatter(x, y, color=graphColor, alpha=min(0.7, 10/dotDensity), marker='^')
        #     else:
        #         ax.scatter(x, y, color=graphColor, alpha=min(0.7, 10/dotDensity), marker='v')

        ax.scatter(data, freqs, color=graphColor, alpha=min(0.7, 10/dotDensity))

        ax.plot(data, lpDelta, color=[x*darkenFactor for x in graphColor])
        ax.set_xlabel("Time (s)")
        ax.set_ylabel("Delta (ns)")
        ax.set_title(fileName)
        ax.grid(alpha=0.5)
    except Exception as e:
        print(e)

if __name__ == '__main__':
    if len(sys.argv) == 1:
        searchFolder = os.getcwd()
    elif len(sys.argv) == 2:
        searchFolder = os.path.abspath(sys.argv[1])
    else:
        print("Too many arguments!")
        exit(-1)

    csvFiles = [os.path.join(searchFolder, file) for file in os.listdir(searchFolder) if file.endswith('.csv')]
    csvFiles.sort()

    if len(csvFiles) == 0:
        print("No files found")
        exit(0)

    print(f'dt = {deltaNanoseconds} ns/step')

    colors = []
    for i in range(len(csvFiles)):
        colors.append(newColor(colors, pastelFactor = 0.5))
        
    figs, axs = plt.subplots(nrows=len(csvFiles))
    if len(csvFiles) > 1:
        for file, ax, color in zip(csvFiles, axs, colors):
            plotCSV(file, ax, color)
    else:
            plotCSV(csvFiles[0], axs, colors[0])

    plt.tight_layout()
    plt.show()


# figs, ax = plt.subplots()
# dataT1 = pd.read_csv('save/InputT1.1.csv').values 
# # X-Axis in seconds.
# dataT2 = pd.read_csv('save/InputT4.1.csv').values 
# dataT1 = dataT1[:min(len(dataT1),len(dataT2))]
# dataT2 = dataT2[:min(len(dataT1),len(dataT2))]

# # X-Axis in seconds.
# ax.set(xlim=(min(dataT1), max(dataT1)))
# deltaT2T1 = dataT2 - dataT1
# ax.scatter(dataT1, deltaT2T1, c='#ff0000')
# ax.set(xlim=(min(dataT1), max(dataT1)))
# print(deltaT2T1)
# plt.show()