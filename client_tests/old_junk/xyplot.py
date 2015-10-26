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
it.endTime = 100
it.frequency = 180000
it.cycleTime = 50
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [3]
it.startTime = 0
it.endTime = 100
it.frequency = 50000
it.cycleTime = 50
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [1]
it.startTime = 0
it.endTime = 100
it.frequency = 250000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)


it = emSequenceItem()
it.pin = [2]
it.startTime = 0
it.endTime = 100
it.frequency = 250000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

res = []
yres = []
for i in cli.getRecording(1).Samples:
  res.append(i * (5.0/4096.0));

for i in cli.getRecording(2).Samples:
  yres.append(i * (5.0/4096.0));

plt.ylim(-6,6)
plt.plot(yres)
plt.plot(res)

plt.draw()
plt.savefig("lol.pdf")
plt.show()

cli.reset()


transport.close()
