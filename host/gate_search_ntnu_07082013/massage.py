#!/usr/bin/python

import fileinput
import itertools
from collections import defaultdict
import matplotlib.pyplot as plt

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


def getGates(inLines):
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

gatesPerFile = {}
for fil in lines:
  gatesPerFile[fil] = getGates(lines[fil])
  print fil


def gateNumbers(gates, ports):
  (x,y) = ([], [])
  for g in gates:
    (_, port) = g
    if port in ports:
      #gate is list of 4
      s = ""
      for e in gates[g]:
        s += e[1]
      y.append(int(s, 2))
      x.append(int(g[0],2))

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
plt.figure()
plt.subplots_adjust(hspace=0.5)
row, col, num = 3, 2, 0
for g in gatesPerFile:
  plt.subplot(row, col, num)
  (x, y) = gateNumbers(gatesPerFile[g], [(0,1)])
  (x2, y2) = gateNumbers(gatesPerFile[g], [(4,10)])
  plt.subplots_adjust(hspace=0.5)
  plt.scatter(x, y, marker='+')
  plt.scatter(x2, y2, marker='+', color='k')
  plt.title(g.split('.')[0])
  num += 1


plt.show()

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
