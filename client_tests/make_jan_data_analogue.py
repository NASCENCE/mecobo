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

allPins = range(0,16)
recordings = range(8,16)
voltPins = [0,3,7]
#0:82,3:168,7:238,8:[0.008544921875
#0:126,3:164,7:110,8
#0:224,3:170,7:194,8

for foo in itertools.product(range(112,128),range(85,128),range(97,128)):
    printStr = ""

    cli.reset()
    cli.clearSequences()
       
    for p,a in zip(voltPins, foo):
        it = emSequenceItem()
        it.pin = [p]
        it.startTime = 1
        it.endTime = 9
        it.amplitude = a*2
        it.operationType = emSequenceOperationType().CONSTANT
        cli.appendSequenceAction(it)
        printStr += str(p) + ":" + str(a*2) + ","

    for r in recordings:
        it = emSequenceItem()
        it.pin = [r]
        it.startTime = 0
        it.endTime = 10
        it.frequency = 22000
        it.operationType = emSequenceOperationType().RECORD   
        cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    for rr in recordings:
        res = []
        for i in cli.getRecording(rr).Samples:
            res.append(i * (5.0/4096.0));
        printStr += str(rr) + ":" + str(res) + ","

    print printStr

transport.close()
