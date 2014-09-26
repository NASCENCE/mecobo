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

plt.ylim(-6,6)

pins = range(0,16)
for p in pins:
  #apply same voltage to all pins except measuring pin
  it = emSequenceItem()
  it.pin = [f for f in pins if f != p]
  it.startTime = 0
  it.endTime = 1200
  it.amplitude = 250
  it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
  cli.appendSequenceAction(it)

  it = emSequenceItem()
  it.pin = [p]
  it.startTime = 100
  it.endTime = 1100
  it.frequency = 1000
  it.operationType = emSequenceOperationType().RECORD   #implies analogue 
  cli.appendSequenceAction(it)

  cli.runSequences()
  cli.joinSequences()

  res = []
  for i in cli.getRecording(p).Samples:
    res.append(i * (5.0/4096.0));

  print p, ":", res[5]
  plt.plot(res)
  cli.clearSequences()
  cli.reset()

transport.close()
plt.show()
