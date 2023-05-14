import pandas as pd
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
import numpy as np
import sys
import os.path

def objective(x, a, b):
    return (a/(x**2)) +b/x

def meanGraph(fileName, outFileName):
    # Read CSVs
    means = []

    for i in range(13):
        tempDf = pd.read_csv(str(i+1) + fileName[1:])
        means.append(tempDf['fps'].mean())
    

    plt.xlabel('Numero di camere analizzate')
    plt.ylabel('FPS medi')

    plt.xticks(range(1, 14))
    

        # curve fit
    popt, _ = curve_fit(objective, np.asarray(range(1,14)).ravel(), np.asarray(means).ravel())
    # summarize the parameter values
    a, b = popt
    print('y = %.5f/x^2 + %.5f/x' % (a, b))


    x_line = np.asarray(range(1,14)).ravel()
    y_line = objective(x_line, a, b)
    plt.plot(x_line, y_line, '--', color='red', label="Curve fitting")
    plt.plot(range(1,14), means, linewidth=0, marker='o', markersize=8, markerfacecolor='cyan', mew=0, label="Dati reali")
    plt.legend(loc="upper right")
    plt.savefig(outFileName, dpi=100)
    plt.show()

if __name__ == '__main__':
    n = len(sys.argv)
    inputFile = 'fps.csv'
    outFile = 'fpsMeanGraph'
    if n > 3:
        raise ValueError("Too many arguments")
    if n > 1:
        inputFile = sys.argv[1]
    if n > 2:
        outFile = sys.argv[2]
    if not os.path.isfile(inputFile):
        raise FileNotFoundError("Input file not found. Provide it as an argument!\n\nExample: pyhton fpsGraph.py fpsFile.csv")
    
    meanGraph(inputFile, outFile + '.png')

