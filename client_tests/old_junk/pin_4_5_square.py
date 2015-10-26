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
it.pin = [4]
it.startTime = 100
it.endTime = 200
it.frequency = 5000
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [6]
it.startTime = 100
it.endTime = 200
it.frequency = 10000
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [5]
it.startTime = 0
it.endTime = 400
it.frequency = 200000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)


it = emSequenceItem()
it.pin = [3]
it.startTime = 0
it.endTime = 400
it.frequency = 200000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)


cli.runSequences()
cli.joinSequences()

res = []
for i,j in zip(cli.getRecording(5).Samples, cli.getRecording(3).Samples):
  res.append((i-j) * (5.0/4096.0));

plt.ylim(-6,6)
plt.plot(res)

plt.draw()
plt.show()

transport.close()
