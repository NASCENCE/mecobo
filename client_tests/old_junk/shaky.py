import pypeaks as pp
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

from numpy import NaN, Inf, arange, isscalar, asarray, array

def peakdet(v, delta, x = None):
    """
    Converted from MATLAB script at http://billauer.co.il/peakdet.html
    
    Returns two arrays
    
    function [maxtab, mintab]=peakdet(v, delta, x)
    %PEAKDET Detect peaks in a vector
    %        [MAXTAB, MINTAB] = PEAKDET(V, DELTA) finds the local
    %        maxima and minima ("peaks") in the vector V.
    %        MAXTAB and MINTAB consists of two columns. Column 1
    %        contains indices in V, and column 2 the found values.
    %      
    %        With [MAXTAB, MINTAB] = PEAKDET(V, DELTA, X) the indices
    %        in MAXTAB and MINTAB are replaced with the corresponding
    %        X-values.
    %
    %        A point is considered a maximum peak if it has the maximal
    %        value, and was preceded (to the left) by a value lower by
    %        DELTA.
    
    % Eli Billauer, 3.4.05 (Explicitly not copyrighted).
    % This function is released to the public domain; Any use is allowed.
    
    """
    maxtab = []
    mintab = []
       
    if x is None:
        x = arange(len(v))
    
    v = asarray(v)
    
    if len(v) != len(x):
        sys.exit('Input vectors v and x must have same length')
    
    if not isscalar(delta):
        sys.exit('Input argument delta must be a scalar')
    
    if delta <= 0:
        sys.exit('Input argument delta must be positive')
    
    mn, mx = Inf, -Inf
    mnpos, mxpos = NaN, NaN
    
    lookformax = True
    
    for i in arange(len(v)):
        this = v[i]
        if this > mx:
            mx = this
            mxpos = x[i]
        if this < mn:
            mn = this
            mnpos = x[i]
        
        if lookformax:
            if this < mx-delta:
                maxtab.append((mxpos, mx))
                mn = this
                mnpos = x[i]
                lookformax = False
        else:
            if this > mn+delta:
                mintab.append((mnpos, mn))
                mx = this
                mxpos = x[i]
                lookformax = True
 
    return array(maxtab), array(mintab)






transport = TSocket.TSocket('localhost', 9090)
transport = TTransport.TBufferedTransport(transport)

prot = TBinaryProtocol.TBinaryProtocol(transport)
cli = emEvolvableMotherboard.Client(prot);
transport.open();

for rep in xrange(0,1000):
    cli.reset()
    cli.clearSequences()
     
    for pulse in xrange(0,10):
        it = emSequenceItem()
        it.pin = [0]
        it.startTime = (pulse*100)
        it.endTime = (pulse*100)+100
        if pulse%2==0:
            it.amplitude = 254
        else:
            it.amplitude = 1

        it.operationType = emSequenceOperationType().CONSTANT
        cli.appendSequenceAction(it)

    it = emSequenceItem()
    it.pin = [2]
    it.startTime = 0
    it.endTime = 100
    it.frequency = 10000
    it.cycleTime = 50
    it.operationType = emSequenceOperationType().DIGITAL   #implies analogue 
    cli.appendSequenceAction(it)

    it = emSequenceItem()
    it.pin = [4]
    it.startTime = 0
    it.endTime = 100
    it.frequency = 500000
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

res = []
for i in cli.getRecording(4).Samples:
  res.append(i * (5.0/4096.0));

pikmin, pikmax = peakdet(res, .005)

plt.ylim(-6,6)
#plt.plot(res)
plt.scatter(array(pikmin)[:,0], array(pikmin)[:,1], color='blue')
plt.scatter(array(pikmax)[:,0], array(pikmax)[:,1], color='red')

plt.draw()
plt.show()

transport.close()
