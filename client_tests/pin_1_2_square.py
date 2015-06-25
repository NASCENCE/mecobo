import sys
from time import *
import numpy
import matplotlib.pyplot as plt
sys.path.append('../Thrift interface/gen-py/NascenseAPI_v01e/')
import emEvolvableMotherboard
from ttypes import *
import numpy.fft

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


it = emSequenceItem()
it.pin = [0]
it.startTime = 5
it.endTime = 10
it.frequency = 5000
it.cycleTime = 50
it.operationType = emSequenceOperationType().DIGITAL
cli.appendSequenceAction(it)

cli.runSequences()
cli.joinSequences()

transport.close()
