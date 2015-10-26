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

#recChannels = [3, 10, 25, 40, 45]
#recChannels = [3, 10]
recChannels = range(1,8)
it = emSequenceItem()
it.pin = [0]
it.startTime = 5
it.endTime = 100
it.frequency = 10000
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)

for i in recChannels:
    it = emSequenceItem()
    it.pin = [i]
    it.startTime = 0
    it.endTime = 150
    it.frequency = 20000
    it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
    it.operationType = emSequenceOperationType().RECORD   
    cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

heigth = 0.0
for r in recChannels:
    res = []
    sams = cli.getRecording(r).Samples;
    print "len", len(sams)
    for i in sams:
        res.append((i&1) + heigth)
    heigth += 1.1

    plt.plot(res, label=str(r))

plt.ylim(-1,20)
plt.legend()
plt.draw()
plt.show()

transport.close()
