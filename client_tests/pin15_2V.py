import sys
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

cli.reset()
cli.clearSequences()

for i in xrange(0,10):
  start = i*100;
  end =   i*100 + 100;
  if i%2 == 0: 
    amp = 1
  else:
    amp = 255
  it = emSequenceItem()
  it.pin = [15]
  it.startTime = start
  it.endTime = end
  it.amplitude = amp
  it.operationType = emSequenceOperationType().CONSTANT   #implies analogue 
  cli.appendSequenceAction(it)


it = emSequenceItem()
it.pin = [0]
it.startTime = 100
it.endTime = 1100
it.frequency = 10000
it.operationType = emSequenceOperationType().RECORD   #implies analogue 
cli.appendSequenceAction(it)

print gmtime()
cli.runSequences()
cli.joinSequences()
print len(cli.getRecording(0).Samples)
quit()
res = []

plt.ylim(-6, 6)
plt.ion()
plt.show()
totalSum = 0

for i in xrange(0,100):
  sleep(0.1)
  #res.extend(cli.getRecording(0).Samples)
  totalSum += len(cli.getRecording(0).Samples)
  #for i in cli.getRecording(0).Samples:
  #  print i * (5.0/4096.0);

  #plt.plot(pr)
  #plt.draw()
print totalSum

#window='hanning'
#window_len = 10
#s=numpy.r_[pr[window_len-1:0:-1],pr,pr[-1:-window_len:-1]]
#w=eval('numpy.'+window+'(window_len)') 
#y=numpy.convolve(w/w.sum(),s,mode='valid')
#print y

cli.joinSequences()

print gmtime()

cli.reset()
transport.close()


