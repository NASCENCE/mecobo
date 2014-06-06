import sys
sys.path.append('../Thrift interface/gen-py/NascenseAPI_v01e/')
from time import *
import numpy
import matplotlib.pyplot as plt
import emEvolvableMotherboard
from ttypes import *

from bitstring import *
from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

#transport = TSocket.TSocket('129.241.102.247', 9090)
transport = TSocket.TSocket('localhost', 9090)
transport = TTransport.TBufferedTransport(transport)

prot = TBinaryProtocol.TBinaryProtocol(transport)
cli = emEvolvableMotherboard.Client(prot);
transport.open();

cli.reset()
cli.clearSequences()

cli.setConfigRegister(0, 1)
recPin = 0
#10 seconds of toggling.
it = emSequenceItem()
it.pin = [15]
it.startTime = 0
it.endTime = 5000
it.ValueSourceRegister = 0 #use register 3
it.operationType = emSequenceOperationType().CONSTANT_FROM_REGISTER   #implies analogue 
cli.appendSequenceAction(it)

#Record
it = emSequenceItem()
it.pin = [0]
it.startTime = 0
it.endTime = 5000
it.frequency = 1000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)


cli.runSequences()

res = []
volt = 1
voltStep = 255/5
for i in xrange(1,6):
  sleep(1)
  cli.setConfigRegister(0, volt)
  volt = (volt + voltStep)%254
  for i in cli.getRecording(recPin).Samples:
    res.append(i * (5.0/4096.0))

cli.joinSequences()
cli.reset()
transport.close()

plt.ylim(-6, 6)
plt.plot(res)
plt.show()


