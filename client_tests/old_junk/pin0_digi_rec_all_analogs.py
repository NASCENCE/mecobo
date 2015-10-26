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
it.frequency = 1
it.cycleTime = 100
it.operationType = emSequenceOperationType().DIGITAL   #implies analogue 
cli.appendSequenceAction(it)

for r in xrange(1, 8):
    it = emSequenceItem()
    it.pin = [r]
    it.startTime = 0
    it.endTime = 10
    it.frequency = 65000
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

for rr in xrange(1,8):
    res = []
    for i in cli.getRecording(rr).Samples:
      res.append(i * (5.0/4096.0));
    plt.plot(res, label=str(rr))


plt.ylim(-6,6)
plt.legend()
plt.draw()
plt.show()

transport.close()
