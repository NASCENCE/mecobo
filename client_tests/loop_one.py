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


it = emSequenceItem()
it.pin = 10
it.startTime = 0
it.endtime = 250
it.amplitude = 200
it.operationType = emSequenceOperationType().ARBITRARY
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = 15
it.startTime = 0
it.endTime = 100
it.sampleRate = 1000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = 14
it.startTime = 0
it.endTime = 100
it.sampleRate = 1000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

it = emSequenceItem()
it.pin = 13
it.startTime = 0
it.endTime = 100
it.sampleRate = 1000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)



cli.runSequences();

print cli.getRecording(15).Samples
print cli.getRecording(14).Samples
print cli.getRecording(13).Samples

#for i in cli.getRecording(15).Samples:
#  if i > 0x1000:
#    v = i & 0x0FFF;
#  else:
#    v = -(4096.0 - (i & 0x0FFF));

#  print v*(5.0/4096)
#print "-------------------------------------"



transport.close()
