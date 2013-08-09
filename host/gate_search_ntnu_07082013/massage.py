#!/usr/bin/python

import fileinput
import itertools
from collections import defaultdict
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm

gate2xor = {  "00": "0",
              "10": "1",
              "01": "1",
              "11": "0"}

gate2xnor = { "00": "1",
              "10": "0",
              "01": "0",
              "11": "1"}

gate2nand = { "00": "1",
              "10": "1",
              "01": "1",
              "11": "0"}

def is_gate2(gate, ioTuple):
  for p in ioTuple:
    if gate[p[0]] != p[1]:
      return False
  return True

#every 5th line (sampled 5 times)
lines = defaultdict(list)
#filteredLines = []
for line in fileinput.input():
  if fileinput.lineno()%5 == 0:
      lines[fileinput.filename()].append(line)
      #filteredLines.append(line)


def get2Gates(inLines):
  gates = defaultdict(list) 
  #for each line
  for sample in inLines:
    vals = sample.split()
    consider = vals[0:11] + vals[12:13] #skip 11th element, strip non-interesting
    outputPort = str(consider.pop(11)) #pop outputport
    #check all combinations of possible input ports
    for inIndex in itertools.combinations(range(0,11), 2):
      tmp = list(consider) #copy
      #note: this works because combinations has sorted output, (i,j) such that i < j
      #thus, popping index j is safe since position i won't be affected.
      inputPort = str(tmp.pop(inIndex[1])) + str(tmp.pop(inIndex[0]))
      config = "".join(tmp)
      gates[tuple([config, inIndex])].append([inputPort, outputPort])
  return gates


def get3Gates(inLines):
  gates = defaultdict(list) 
  #for each line
  for sample in inLines:
    vals = sample.split()
    consider = vals[0:11] + vals[12:13] #skip 11th element, strip non-interesting
    outputPort = str(consider.pop(11)) #pop outputport
    #check all combinations of possible input ports
    for inIndex in itertools.combinations(range(0,11), 3):
      tmp = list(consider) #copy
      #note: this works because combinations has sorted output, (i,j) such that i < j
      #thus, popping index j is safe since position i won't be affected.
      inputPort = str(tmp.pop(inIndex[2])) + str(tmp.pop(inIndex[1])) + str(tmp.pop(inIndex[0]))
      config = "".join(tmp)
      gates[tuple([config, inIndex])].append([inputPort, outputPort])
  return gates


gates2PerFile = {}
for fil in lines:
  gates2PerFile[fil] = get2Gates(lines[fil])
  print fil

gates3PerFile = {}
for fil in lines:
  gates3PerFile[fil] = get3Gates(lines[fil])
  print "g3:" + fil


#ARGH BUGS BUGS: gate INPUTS HERE -MUST- BE SORTED so that the sum makes sense
#(they seem to be, but that's just luck)
def gateNumbers(gates, ports):
  (x,y) = ([], [])
  for g in gates:
    (_, port) = g
    if port in ports:
      #gate is list, make a bit vector
      s = ""
      for e in gates[g]:
        s += e[1]
      y.append(int(s, 2))
      x.append(int(g[0],2))
  return (x,y)


def lambdasPerConfig(gates, porta):
  (x, y) = ([], [])
  for g in gates:
    (_, port) = g
    if port == porta:
      lives = 0
      for e in gates[g]:
        lives += int(e[1], 2)
      lamb = float(lives)/8.0
      y.append(lamb)
      x.append(int(g[0], 2))
  return (x,y)


#makes new figure
#plt.figure()
#plt.subplots_adjust(hspace=0.5)
#make the little pluuut
#allports = [itertools.combinations(range(0,11), 2)]
#row, col, num = 3, 2, 0
#for g in gatesPerFile:
#  plt.subplot(row, col, num)
#  (x, y) = gateNumbers(gatesPerFile[g], allports)
#  plt.scatter(x, y, marker='+')
#  plt.title(g.split('.')[0])
#  num += 1

#new figure
#plt.figure()
#plt.subplots_adjust(hspace=0.5)
#row, col, num = 3, 2, 0
#for g in gates2PerFile:
#  plt.subplot(row, col, num)
#  (x, y) = gateNumbers(gates2PerFile[g], [(0,1)])
#  (x2, y2) = gateNumbers(gates2PerFile[g], [(4,10)])
#  plt.subplots_adjust(hspace=0.5)
#  plt.scatter(x, y, marker='+')
#  plt.scatter(x2, y2, marker='+', color='k')
#  plt.title(g.split('.')[0])
#  num += 1
#
#plt.figure()
#plt.subplots_adjust(hspace=0.5)
#all3ports = list(itertools.combinations(range(0,11),3))
#row, col, num = 3, 2, 0
#for g in gates3PerFile:
#  plt.subplot(row, col, num)
#  (x, y) = gateNumbers(gates3PerFile[g], all3ports)
#  #(x2, y2) = gateNumbers(gates3PerFile[g], all3ports)
#  plt.subplots_adjust(hspace=0.5)
#  plt.scatter(x, y, marker='+')
#  #plt.scatter(x2, y2, marker='+', color='k')
#  plt.title(g.split('.')[0])
#  num += 1

#plt.figure()
#plt.subplots_adjust(hspace=0.5)
#all3ports = list(itertools.combinations(range(0,11),3))
#row, col, num = 3, 2, 0
#for g in gates3PerFile:
#  plt.subplot(row, col, num)
#  (x, y) = lambdasPerConfig(gates3PerFile[g], [(0,1,2)])
#  (x2, y2) = lambdasPerConfig(gates3PerFile[g], [(3,5,9)])
#  plt.ylabel('Lambda, [0, 1]')
#  plt.xlabel('Configuration bits, [0, 256]')
#  plt.subplots_adjust(hspace=0.5)
#  plt.subplots_adjust(wspace=0.5)
#  plt.scatter(x, y, marker='+')
#  plt.scatter(x2, y2, marker='+', color='red')
#  plt.title(g.split('.')[0])
#  num += 1

def makeLambdaPlot(inputPins):
  plt.figure()
  plt.subplots_adjust(hspace=0.5)
  row, col, num = 3, 2, 0
  for g in gates3PerFile:
    plt.subplot(row, col, num)
    mark = 0
    colors = iter(cm.rainbow(np.linspace(0,1,len(inputPins))))
    plt.ylim([0,1])
    plt.xlim([0,260])
    for i in inputPins:
      (x, y) = lambdasPerConfig(gates3PerFile[g], i)
      plt.scatter(x, y, marker='.', color=next(colors))
      mark += 1
    plt.ylabel('Lambda, [0, 1]')
    plt.xlabel('Configuration bits, [0, 256]')
    plt.subplots_adjust(hspace=0.5)
    plt.title(g.split('.')[0])
    num += 1
  plt.savefig("lambdaplot_" + str(len(inputPins)) + "_pins.pdf")


all3ports = list(itertools.combinations(range(0,11),3))
#print all3ports
makeLambdaPlot([(0,1,2), (3, 5, 9), (2,3,4)])
makeLambdaPlot(all3ports)

#plt.show()

exit()

xData = []
xLabel = []
yData = []
yLabel = []

yData2 = []
xData2 = []
xLabel2 = []


#for g in gates:
#  if is_gate2(gate2xor, gates[g]):
#    print "In " + fileinput.filename() + " XOR: " + str(g)
#    xData.append(int(g[0], 2))
#    xLabel.append(g[0])
#    #linear column-wide mapping from 2d pin to 1d 
#    up1, up2 = g[1]
#    yData.append(up1*11 + up2)
#    yLabel.append(str(g[1]))
#  if is_gate2(gate2xnor, gates[g]):
#    print "In " + fileinput.filename() + " XNOR:" + str(g)
#  if is_gate2(gate2nand, gates[g]):
#    print "In " + fileinput.filename() + " NAND:" + str(g) 


plt.figure(figsize=(20,10))
plt.plot(xData2, yData2, 'ro')
#plt.xticks(xData2, xLabel2, rotation='vertical')
#plt.yticks(yData, yLabel)
plt.subplots_adjust(bottom=0.2)
plt.savefig(fileinput.filename()+".pdf")
#plt.show()
