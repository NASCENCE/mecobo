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
it.endTime = 10000000
it.amplitude = 222
it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
cli.appendSequenceAction(it)

for i in xrange(1,8):
    it = emSequenceItem()
    it.pin = [i]
    it.startTime = 0
    it.endTime = 10000
    it.frequency = 2e5
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

for i in xrange(1,8):
    res = []
    for q in cli.getRecording(i).Samples:
      res.append(q * (5.0/4096.0));

    plt.ylim(-6,6)
    plt.plot(res, label=str(i))

plt.legend()
plt.draw()
plt.show()

cli.reset()
transport.close()
