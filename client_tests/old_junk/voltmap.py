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
vrange = 1024
t = 0
for i in range(0,vrange):
    it = emSequenceItem()
    it.pin = [p1]
    it.startTime = t
    it.endTime = t + rectime
    it.amplitude = i%256
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
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [p1]
it.startTime = 0
it.endTime = rectime * vrange
it.amplitude = 128
it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
#cli.appendSequenceAction(it)


it = emSequenceItem()
it.pin = [p1]
it.startTime = 0
it.endTime = rectime * vrange
it.frequency = 30000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [p2]
it.startTime = 0
it.endTime = rectime * vrange
it.frequency = 30000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)


cli.runSequences()
cli.joinSequences()

res = []
v1 = []
v2 = []

for v,g in zip(cli.getRecording(p1).Samples, cli.getRecording(p2).Samples):
  res.append((v-g) * (5.0/4096.0));
  v1.append(v * (5.0/4096.0));
  v2.append(g * (5.0/4096.0));

plt.ylim(-6,6)
plt.plot(res, label='diff')
plt.plot(v1, label='pin 1')
plt.plot(v2, label='pin 2')
plt.legend()

plt.draw()
plt.show()

transport.close()
