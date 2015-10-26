import sys
from time import *
import numpy
import matplotlib.pyplot as plt
sys.path.append('../Thrift interface/gen-py/NascenseAPI_v01e/')
import emEvolvableMotherboard
from ttypes import *

from bitstring import *
from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

transport = TSocket.TSocket('localhost', 9090)
transport = TTransport.TBufferedTransport(transport)

prot = TBinaryProtocol.TBinaryProtocol(transport)
cli = emEvolvableMotherboard.Client(prot);
transport.open();

cli.reset()
cli.clearSequences()

p1 = 0
p2 = 5
rectime = 10
vrange = 256
t = 0
for i in range(0,vrange):
    it = emSequenceItem()
    it.pin = [p1]
    it.startTime = t
    it.endTime = t + rectime
    it.amplitude = i
    it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
    cli.appendSequenceAction(it)
    t += rectime


#Keep one node at 0V as reference point
it = emSequenceItem()
it.pin = [p2]
it.startTime = 0
it.endTime = rectime * vrange
it.frequency = 0
it.cycleTime = 100
it.operationType = emSequenceOperationType().DIGITAL   #implies analogue 
#cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [p2]
it.startTime = 0
it.endTime = rectime * vrange
it.amplitude = 127
it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
#cli.appendSequenceAction(it)

for r in [0,1,2,3,4,5,6]:
    it = emSequenceItem()
    it.pin = [r]
    it.startTime = 0
    it.endTime = rectime * vrange
    it.frequency = 500
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

v1 = []
v2 = []

for rr in range(0,8):
  res = []
  for v in cli.getRecording(rr).Samples:
    res.append((v) * (5.0/4096.0));
  plt.plot(res, label=str(rr))

plt.legend()
plt.ylim(-6,6)
plt.draw()
plt.show()

transport.close()
