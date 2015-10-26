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

it = emSequenceItem()
it.pin = [0]
it.startTime = 0
it.endTime = 5000
it.frequency = 1000
it.operationType = emSequenceOperationType().DIGITAL   #implies analogue 
cli.appendSequenceAction(it)


it = emSequenceItem()
it.pin = [1]
it.startTime = 0
it.endTime = 5000
it.frequency = 44000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [2]
it.startTime = 0
it.endTime = 5000
it.frequency = 44000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [3]
it.startTime = 0
it.endTime = 5000
it.frequency = 44000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)




cli.runSequences()
cli.joinSequences()

res1 = []
for i in cli.getRecording(1).Samples:
  res1.append(i * (5.0/4096.0));
res2 = []
for i in cli.getRecording(2).Samples:
  res2.append(i * (5.0/4096.0));
res3 = []
for i in cli.getRecording(3).Samples:
  res3.append(i * (5.0/4096.0));



plt.ylim(-6,6)
plt.plot(res1)
plt.plot(res2)
plt.plot(res3)

plt.draw()
plt.show()

transport.close()
