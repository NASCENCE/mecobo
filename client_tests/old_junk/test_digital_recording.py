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
it.startTime = 50
it.endTime = 150
it.frequency = 664422
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [1]
it.startTime = 0
it.endTime = 2000
it.frequency = 1000
it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
it.operationType = emSequenceOperationType().RECORD   
cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

res = []
for i in cli.getRecording(1).Samples:
    #print "sam:", i
    res.append(i&1)

plt.ylim(-1,2)
plt.plot(res)

plt.draw()
plt.show()

transport.close()
