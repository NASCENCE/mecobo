import sys
import os
import numpy as np
import scipy.signal
import matplotlib.pyplot as plt

test_dir = os.path.dirname(os.path.abspath(__file__))
api_dir = os.path.join(test_dir, '../Thrift interface/gen-py/NascenseAPI_v01e/')
sys.path.append(api_dir)

import emEvolvableMotherboard
from ttypes import *

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

transport = TSocket.TSocket('localhost', 9090)
transport = TTransport.TBufferedTransport(transport)

prot = TBinaryProtocol.TBinaryProtocol(transport)
cli = emEvolvableMotherboard.Client(prot);

def setup():
    print "package setup()"
    transport.open()
    cli.reset()
    cli.clearSequences()

def teardown():
    print "package teardown()"
    transport.close()

N_PINS = 16
N_REC_ANALOG = 8
N_REC_DIGITAL = 8

ALL_PINS = range(N_PINS)
TEST_SLIDE_PINS_LEFT = ALL_PINS[:9]
TEST_SLIDE_PINS_RIGHT = ALL_PINS[9:]

SAMPLE_CLK = 75e6
def real_sample_freq(f):
    overflow = np.floor(SAMPLE_CLK / f)
    return SAMPLE_CLK / overflow

def adc_voltage(a):
    """ Convert binary ADC sample to voltage """
    return a * 5.0 / 4096.0

def voltage_to_dac(v):
    """ Convert analog voltage to 8-bit DAC """
    # DAC is +-5V
    d = 127 + 256 * v / 10.0
    d = np.clip(d, 0, 255)
    if type(d) is np.ndarray:
        return d.astype(int)
    else:
        return int(d)

def dac_to_voltage(d):
    """ Convert 8-bit DAC to analog voltage """
    # DAC is 0-255
    # 127 is ~0V
    return (-5.0 + 2 * 5.0 * (d + 1) / 256)

def signal_lag(s1, s2):
    corr = scipy.signal.correlate(s1, s2)
    lag = len(s1) - 1 - np.argmax(corr)
    return lag

def signal_shift(s1, s2, lag):
    if lag > 0:
        # s2 lags s1
        s2 = s2[lag:]
        s1 = s1[:-lag]
    elif lag < 0:
        # s1 lags s2
        s1 = s1[-lag:]
        s2 = s2[:lag]

    return (s1, s2)

def digital_samples(d):
    # Discard upper bits
    return d & 1

DIGITAL_THRESHOLD_VOLTAGE = 1.5
def digital_threshold(a):
    return (a > DIGITAL_THRESHOLD_VOLTAGE).astype(int)

def check_expected_result(expected, result, label="result", mse_pass=0.1, signal=None, lag_limit=0.1):
    """ Check result against expected signal """
    # Discard extra samples
    if len(result) > len(expected):
        result = result[:len(expected)]

    # Calculate lag
    lag = signal_lag(expected, result)
    print "Lag:", lag

    # Shift signals
    if abs(lag) <= lag_limit * len(expected):
        expected, result = signal_shift(expected, result, lag)
    else:
        print "WARNING: Lag too high (%d), no signal alignment performed" % lag

    mse = np.mean((result - expected) ** 2)
    print "MSE:", mse

    if mse > mse_pass:
        plt.title("%s failed, MSE %.4f > %.4f" % (label, mse, mse_pass))
        plt.plot(expected, 'g-', label="expected")
        if signal is not None:
            plt.plot(signal, 'b-', label="signal")
        plt.plot(result, 'r-', label="result")
        ymin, ymax = plt.ylim()
        plt.ylim(ymin - .5, ymax + .5)
        plt.legend()
        plt.show()

    assert mse <= mse_pass, "MSE %.4f too high (>%.4f) for %s" % (mse, mse_pass, label)
