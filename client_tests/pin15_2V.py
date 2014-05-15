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

for i in xrange(0,10):
  start = i*100
  end = i*100 + 100
  it = emSequenceItem()
  it.pin = [15]
  it.startTime = start
  it.endTime = end
  it.amplitude = i*20
  it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
  cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [0]
it.startTime = 0
it.endTime = 1000
it.frequency = 10000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)
cli.runSequences()
cli.joinSequences()


pr = []
res = cli.getRecording(0).Samples
for i in res:
  pr.append(i * (5.0/4096.0))


cli.reset()
transport.close()

plt.ylim(-6, 6)
plt.plot(pr)
plt.show()
