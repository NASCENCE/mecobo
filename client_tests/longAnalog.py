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
#transport = TSocket.TSocket('localhost', 9090)
transport = TTransport.TBufferedTransport(transport)

prot = TBinaryProtocol.TBinaryProtocol(transport)
cli = emEvolvableMotherboard.Client(prot);
transport.open();

cli.reset()
cli.clearSequences()

#10 seconds of toggling.
for pin in [4,5,6,8,9,10,11,12]:
  for i in xrange(0,8):
    start = i*4000;
    end =   i*4000 + 4000;
    if i%2 == 0:
      amp = 1
    else:
      amp = 255
    it = emSequenceItem()
    it.pin = [pin]
    it.startTime = start
    it.endTime = end
    it.amplitude = amp
    it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
    cli.appendSequenceAction(it)


it = emSequenceItem()
it.pin = [7]
it.startTime = 100
it.endTime = 10100
it.frequency = 10000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

cli.runSequences()

totalSum = 0
#Get sample buffers every 0.5 seconds. Should 
#in total become 100,000 samples.
res = []
for i in xrange(0,20):
  sleep(0.5)
  r = cli.getRecording(7).Samples
  totalSum += len(r)
  res.extend(r)


print totalSum

cli.joinSequences()
cli.reset()
transport.close()

data = []
for i in res:
  data.append(i * (5.0/4096.0))
plt.ylim(-6,6)
plt.plot(data)
plt.show()

