import sys
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
it.pin = [15]
it.startTime = 0
it.endTime = 100
it.amplitude = 100
it.operationType = emSequenceOperationType().ARBITRARY   #implies analogue 
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [0]
it.startTime = 0
it.endTime = 100
it.frequency = 10
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)
cli.runSequences()

res = cli.getRecording(0).Samples

cli.reset()
transport.close()

plt.plot(res)
plt.show()
