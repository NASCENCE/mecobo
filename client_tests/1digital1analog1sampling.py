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

transport = TSocket.TSocket('129.241.102.247', 9090)
transport = TTransport.TBufferedTransport(transport)

prot = TBinaryProtocol.TBinaryProtocol(transport)
cli = emEvolvableMotherboard.Client(prot);
transport.open();

cli.reset()
cli.clearSequences()

cli.setConfigRegister(1, 1)
recPin = 0
#10 seconds of toggling.
it = emSequenceItem()
it.pin = [4]
it.startTime = 0
it.endTime = 5000
it.ValueSourceRegister = 1 #use register 1
it.operationType = emSequenceOperationType().CONSTANT_FROM_REGISTER   #implies analogue 
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [8]
it.startTime = 0
it.endTime = 5000
it.amplitude = 255
it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
cli.appendSequenceAction(it)

it = emSequenceItem()
#it.pin = xrange(0,16)
it.pin = [4]
it.startTime = 0
it.endTime = 5000
it.frequency = 1
it.cycleTime = 0
it.operationType = emSequenceOperationType().DIGITAL   #implies analogue 
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [7]
it.startTime = 0
it.endTime = 5000
it.frequency = 1000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

cli.runSequences()

sleep(2.5)
cli.setConfigRegister(1,255)
cli.joinSequences()
res = []
for i in cli.getRecording(7).Samples:
  res.append(i * (5.0/4096.0));

plt.ylim(-6,6)
cli.reset()
transport.close()
plt.plot(res)
plt.draw()
plt.show()
