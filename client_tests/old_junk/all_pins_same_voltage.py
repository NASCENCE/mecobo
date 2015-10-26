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

cli.reset()
cli.clearSequences()
for i in xrange(8,16):
  it = emSequenceItem()
  it.pin = i
  it.startTime = 0
  it.endTime = 30000
  it.amplitude = 215
  it.operationType = emSequenceOperationType().ARBITRARY   #implies analogue 
  cli.appendSequenceAction(it)

cli.runSequences()
cli.reset()
transport.close()
