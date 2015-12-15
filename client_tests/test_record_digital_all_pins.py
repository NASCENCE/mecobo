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

recChannels = range(1,8)
cli.reset()
cli.clearSequences()
 
it = emSequenceItem()
it.pin = [0]
it.startTime = 5
it.endTime = 9
it.frequency = 1000000
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)
 
for r in recChannels:
    it = emSequenceItem()
    it.pin = [r]
    it.startTime = 2
    it.endTime = 12
    it.frequency = 1000000
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
