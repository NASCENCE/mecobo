import sys
sys.path.append('/home/lykkebo/lib/NascenseAPI_v01d/')
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

for i in xrange(0,100000):
  cli.reset()
  it = emSequenceItem()
  it.pin = 15
  it.startTime = 0
  it.endTime = 100
  it.amplitude = (i*10%255)
  it.operationType = emSequenceOperationType().ARBITRARY   #implies analogue 
  cli.appendSequenceAction(it)
  cli.reset()
  it = emSequenceItem()
  it.pin = 14
  it.startTime = 0
  it.endTime = 100
  it.amplitude = (i*10%255)
  it.operationType = emSequenceOperationType().ARBITRARY   #implies analogue
  cli.runSequences()
  cli.clearSequences()

transport.close()
