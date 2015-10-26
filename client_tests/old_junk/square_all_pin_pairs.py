import sys
import numpy
import matplotlib.pyplot as plt
sys.path.append('../Thrift interface/gen-py/NascenseAPI_v01e/')
import emEvolvableMotherboard
from ttypes import *

import itertools

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

pins = list(itertools.combinations(range(0, 7), 2))
for (o,r) in pins:
    for fr in numpy.linspace(0,10000,5):
        print "Running", o, r, fr
        cli.reset()
        cli.clearSequences()

        it = emSequenceItem()
        it.pin = [o]
        it.startTime = 0
        it.endTime = 60
        it.frequency = fr
        it.cycleTime = 50
        it.operationType = emSequenceOperationType().DIGITAL
        cli.appendSequenceAction(it)

        it = emSequenceItem()
        it.pin = [r]
        it.startTime = 5
        it.endTime = 55
        it.frequency = 200000
        it.operationType = emSequenceOperationType().RECORD   #implies analogue 
        cli.appendSequenceAction(it)

        cli.runSequences()
        cli.joinSequences()

        res = []
        for i in cli.getRecording(r).Samples:
          res.append(i * (5.0/4096.0));

        plt.ylim(-6,6)
        plt.subplot(len(pins)/2,2,o+r)
        plt.plot(res)
        plt.draw()

plt.show()
cli.reset()

transport.close()
