import sys 
import random
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


for rr in range(0,1):

    cli.reset()
    cli.clearSequences()

    for s in range(0,5):
        it = emSequenceItem()
        it.pin = [s]
        it.startTime = 20
        it.endTime = 80
        it.amplitude = random.randint(0,255)
        it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
        cli.appendSequenceAction(it)

    it = emSequenceItem()
    it.pin = [5]
    it.startTime = 0
    it.endTime = 100
    it.frequency = 500000
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    res = []
    for i in cli.getRecording(5).Samples:
      res.append(i * (5.0/4096.0));

    print len(res)
    plt.ylim(-6,6)
    plt.plot(res)

plt.draw()
plt.show()

transport.close()
