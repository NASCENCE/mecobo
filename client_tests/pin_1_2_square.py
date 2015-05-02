import sys
from time import *
import numpy
import matplotlib.pyplot as plt
sys.path.append('../Thrift interface/gen-py/NascenseAPI_v01e/')
import emEvolvableMotherboard
from ttypes import *
import numpy.fft

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
it.pin = [2]
it.startTime = 5
it.endTime = 100
it.frequency = 50000
it.cycleTime = 50
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = [3]
it.startTime = 0
it.endTime = 110
it.frequency = 500000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)


cli.runSequences()
cli.joinSequences()

res = []
for i in cli.getRecording(2).Samples:
  res.append(i * (5.0/4096.0));


fRes = numpy.fft.fft(res)
freqs = numpy.fft.fftfreq(fRes.size, 0.000005)
freqs = numpy.fft.fftshift(freqs)
yplot = numpy.fft.fftshift(fRes)

plt.ylim(-6,6)
plt.plot(freqs, 1.0/len(res) * numpy.abs(yplot))

plt.draw()
plt.show()

cli.reset()


transport.close()
