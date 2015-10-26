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


#Output constant voltages between -5 to 5 on 0 to 8, increase by 1 V.
#Measure across 8 to 15.

for p in xrange(0,8):
  it = emSequenceItem()
  it.pin = p
  it.startTime = 0
  it.endtime = 50
  it.amplitude = (255/10) * p
  it.operationType = emSequenceOperationType().ARBITRARY
  cli.appendSequenceAction(it)


#Measure across those pins.
for m in xrange(8, 16):
  it = emSequenceItem()
  it.pin = m
  it.startTime = 0
  it.endTime = 200
  it.sampleRate = 1000
  it.operationType = emSequenceOperationType().RECORD   #implies analogue 
  cli.appendSequenceAction(it)

cli.runSequences();

for r in xrange(8,16):
  print cli.getRecording(r).Samples;

#for i in cli.getRecording(15).Samples:
#  if i > 0x1000:
#    v = i & 0x0FFF;
#  else:
#    v = -(4096.0 - (i & 0x0FFF));

#  print v*(5.0/4096)
#print "-------------------------------------"



transport.close()
