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

it = emSequenceItem()
it.pin = [15,14,13,12,11,10,9,8,7,6,5,4,3,2,1]
it.startTime = 0
it.endTime = 1200
it.amplitude = 255
it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
cli.appendSequenceAction(it)

#it = emSequenceItem()
#it.pin = xrange(0,16)
#it.startTime = 0
#it.endTime = 1200
#it.frequency = 1
#it.cycleTime = 0
#it.operationType = emSequenceOperationType().DIGITAL   #implies analogue 
#cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [0]
it.startTime = 100
it.endTime = 1100
it.frequency = 1000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

res = []
for i in cli.getRecording(0).Samples:
  res.append(i * (5.0/4096.0));

plt.ylim(-6,6)
cli.reset()
transport.close()
plt.plot(res)
plt.draw()
plt.show()
