import sys
from time import *
import itertools
import matplotlib.pyplot as plt
sys.path.append('../Thrift interface/gen-py/NascenseAPI_v01e/')
import emEvolvableMotherboard
import random
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

channels = range(0,15)

for permutation in itertools.permutations(channels):
    for amp in range(0,256):
        frqChans = permutation[:8]
        recChans = permutation[8:]
        printStr = ""
        
        cli.reset()
        cli.clearSequences()
           
        for f in frqChans:
            it = emSequenceItem()
            it.pin = [f]
            it.startTime = 1
            it.endTime = 9
            it.amplitude = amp
            it.operationType = emSequenceOperationType().DIGITAL
            cli.appendSequenceAction(it)
            printStr += str(f) + ":" + str(amp) + ","

        for r in recChans:
            it = emSequenceItem()
            it.pin = [r]
            it.startTime = 0
            it.endTime = 10
            it.frequency = 22000
            it.operationType = emSequenceOperationType().RECORD   
            cli.appendSequenceAction(it)

        cli.runSequences()
        cli.joinSequences()

        for rr in recChans:
            res = []
            for i in cli.getRecording(rr).Samples:
                res.append(i * (5.0/4096.0));
            printStr += str(rr) + ":" + str(res) + ","

        print printStr

transport.close()
