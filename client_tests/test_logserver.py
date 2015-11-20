import sys
from time import *
import numpy
import random
import matplotlib.pyplot as plt
sys.path.append('../Thrift interface/gen-py/NascenseAPI_v01e/')
import emEvolvableMotherboard
import emLogServer
from ttypes import *

from bitstring import *
from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

transport = TSocket.TSocket('localhost', 9090)
transport = TTransport.TBufferedTransport(transport)

ls = emLogServerSettings()
ls.logServer = "copernicio.idsia.supsi.ch"
ls.experimentName = "ALU_FOIL"


prot = TBinaryProtocol.TBinaryProtocol(transport)
cli = emEvolvableMotherboard.Client(prot);


transport.open();

cli.reset()
cli.ping()
cli.clearSequences()

cli.setLogServer(ls)

recChannels = range(1,10)

for x in xrange(0,10000):
    for r in recChannels:
        cli.reset()
        cli.clearSequences()
        it = emSequenceItem()
        it.pin = [0]
        it.startTime = 0
        it.endTime = 30
        it.frequency = random.random() * 1000000
        it.operationType = emSequenceOperationType().DIGITAL
        cli.appendSequenceAction(it)
      
        it = emSequenceItem()
        it.pin = [r]
        it.startTime = 0
        it.endTime = 35
        it.frequency = 100000
        it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
        it.operationType = emSequenceOperationType().RECORD   
        cli.appendSequenceAction(it)

        cli.runSequences()
        cli.joinSequences()
        
        sams = cli.getRecording(r).Samples;

transport.close()
