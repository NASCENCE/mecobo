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

recChannels = range(1,50)
for r in recChannels:
    cli.reset()
    cli.clearSequences()
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 0
    it.endTime = 30
    it.frequency = 10000
    it.operationType = emSequenceOperationType().DIGITAL
    cli.appendSequenceAction(it)
  
    it = emSequenceItem()
    it.pin = [r]
    it.startTime = 10
    it.endTime = 20
    it.frequency = 100000
    it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
    it.operationType = emSequenceOperationType().RECORD   
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    res = []
    sams = cli.getRecording(r).Samples;
    print "len", len(sams)
    for i in sams:
        res.append(i&1)

    plt.plot(res, label=str(r))

plt.ylim(-1,2)
plt.legend()
plt.draw()
plt.show()

transport.close()
