import pandas as pd
import numpy as np
from scipy.stats import norm
import matplotlib.pyplot as plt
import math
import sys
import os.path

def histAndPDF(fileName, outFileName):
        
    # Read CSV
    df = pd.read_csv(fileName)

    plt.xlabel('Frame rate [fps]')
    plt.ylabel('Densità')

    # plot the histogram 
    plt.hist(x = df["fps"], 
                bins = math.floor(1 + 3.322*math.log(len(df['fps']))),
                color = "lightblue", ec="grey", lw=1, density=True)


    # Plot the PDF.
    mu, std = norm.fit(df['fps']) 
    xmin, xmax = plt.xlim()
    x = np.linspace(xmin, xmax, 100)
    p = norm.pdf(x, mu, std)

    lbl = "μ = {:.2f}\nσ = {:.2f}".format(mu, std)
    plt.plot(x, p, 'k', label=lbl)
    plt.legend(loc="upper right")
    plt.savefig(outFileName, dpi=100)
    plt.show()

def FPSAndTime(fileName, outFileName):
    df = pd.read_csv(fileName)
    plt.xlabel('Tempo [s]')
    plt.ylabel('Frame rate [fps]')
    print([i/25 for i in range(0, len(df['fps']))])
    plt.plot([i/25 for i in range(0, len(df['fps']))], df['fps'], linewidth=0, marker='o', markersize=1)
    plt.savefig(outFileName, dpi=100)
    plt.show()

if __name__ == '__main__':
    n = len(sys.argv)
    inputFile = 'fps.csv'
    outFile = 'fpsGraph'
    if n > 3:
        raise ValueError("Too many arguments")
    if n > 1:
        inputFile = sys.argv[1]
    if n > 2:
        outFile = sys.argv[2]
    if not os.path.isfile(inputFile):
        raise FileNotFoundError("Input file not found. Provide it as an argument!\n\nExample: pyhton fpsGraph.py fpsFile.csv")

    histAndPDF(inputFile, outFile + 'histAndPDF.png')
    FPSAndTime(inputFile, outFile + '.png')