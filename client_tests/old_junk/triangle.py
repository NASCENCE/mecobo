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

for i in range(0,1):
    cli.reset()
    cli.clearSequences()

    it = emSequenceItem()
    it.pin = [4]
    it.startTime = 0
    it.endTime = 1000
    it.frequency = 10000
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

    num = 1000
    volt = 0
    for s,e in zip(range(0,num, 2), range(1, num,2)):
        print "analog ", volt%255, "start ", s, " end", e
        it = emSequenceItem()
        it.pin = [3,0,1,2]
        it.startTime = s
        it.endTime = e
        it.amplitude = volt%255
        it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
        cli.appendSequenceAction(it)
        volt += 1



    cli.runSequences()
    cli.joinSequences()

    res = []
    for i in cli.getRecording(4).Samples:
      res.append(i * (5.0/4096.0));

    plt.ylim(-6,6)
    plt.plot(res)

plt.draw()
plt.show()

transport.close()
