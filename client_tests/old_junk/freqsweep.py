import sys
from time import *
import numpy
import matplotlib.pyplot as plt
sys.path.append('../Thrift interface/gen-py/NascenseAPI_v01e/')
import emEvolvableMotherboard
from ttypes import *
import scipy

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


plt.figure()
plt.ylim(-4.0,4)

freqs = numpy.arange(0, 1000000, 1000)

maxfreq = []
minfreq = []
for fre in freqs:

    cli.reset()
    cli.clearSequences()

    print fre
    it = emSequenceItem()
    it.pin = [2]
    it.startTime = 0
    it.endTime = 50
    it.frequency = fre
    it.cycleTime = 50
    it.operationType = emSequenceOperationType().DIGITAL
    cli.appendSequenceAction(it)

    it = emSequenceItem()
    it.pin = [3]
    it.startTime = 5
    it.endTime = 45
    it.frequency = 200000
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    res = []
    for i in cli.getRecording(3).Samples:
      res.append(i * (5.0/4096.0));
    maxfreq.append(max(res))
    minfreq.append(min(res))

plt.plot(freqs, maxfreq, linewidth=0.5)
#plt.plot(freqs, minfreq, linewidth=0.3)

plt.ylim(0, 5)
plt.xlabel("Frequency [Hz]")
plt.ylabel("Voltage [V]")
plt.savefig("freqsweep.pdf")
plt.show()

cli.reset()

transport.close()
