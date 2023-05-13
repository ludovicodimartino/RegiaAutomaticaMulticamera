import pandas as pd
import matplotlib.pyplot as plt
import sys
import os.path

def meanGraph(fileName, outFileName):
    # Read CSV
    df = pd.read_csv(fileName)

    plt.xlabel('Numero di camere analizzate')
    plt.ylabel('FPS medi')

    
    plt.plot(df['camNum'], df['fpsMean'])
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

