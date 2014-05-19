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
it = emSequenceItem()
it.pin = 7
it.startTime = 0
it.endTime = 10000
it.ValueSourceRegister = 3 #use register 3
it.operationType = emSequenceOperationType().CONSTANT_FROM_REGISTER   #implies analogue 
cli.appendSequenceAction(it)
cli.runSequences()

res = []
for i in xrange(0,10):
  sleep(1)
  cli.setConfigRegister(3, (i*50)%255)

cli.joinSequences()
cli.reset()
transport.close()

data = []
for i in res:
  data.append(i * (5.0/4096.0))
plt.ylim(-6,6)
plt.plot(data)
plt.show()

