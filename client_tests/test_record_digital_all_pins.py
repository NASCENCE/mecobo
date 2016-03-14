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

recChannels = range(0,50)
cli.reset()
cli.clearSequences()
recChannels.remove(20)
 
it = emSequenceItem()
it.pin = [20]
it.startTime = 0
it.endTime = 1000
it.frequency = 1000000
it.cycleTime = 50
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)
 
for r in recChannels:
    it = emSequenceItem()
    it.pin = [r]
    it.startTime = 0
    it.endTime = 1002
    it.frequency = 200000
    it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
    it.operationType = emSequenceOperationType().RECORD   
    cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

for c in recChannels:
    res = []
    sams = cli.getRecording(c).Samples;
    print "len", len(sams)
    for i in sams:
        res.append(i&1)

    plt.plot(res, label=str(c))

plt.ylim(-1,2)
plt.legend()
plt.draw()
plt.show()

transport.close()
