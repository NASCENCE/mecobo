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
it.endTime = 50
it.amplitude = 255
it.operationType = emSequenceOperationType().CONSTANT
cli.appendSequenceAction(it)
it.startTime = 50
it.endTime = 100
it.amplitude = 0
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [15]
it.startTime = 0
it.endTime = 100
it.frequency = 50000
it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

res = []
for i in cli.getRecording(15).Samples:
  res.append(i&0x1)
  print i
  #res.append(i * (5.0/4096.0));

plt.ylim(-6,6)
plt.plot(res)

plt.draw()
plt.show()

transport.close()
