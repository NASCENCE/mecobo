"""
Tests for analog input via the ADC
"""
from nose.tools import *
from . import *

import numpy as np
import matplotlib.pyplot as plt
import scipy.signal

def setup():
    cli.reset()
    cli.clearSequences()

def teardown():
    cli.reset()
    cli.clearSequences()



def test_analog_input_digital_constant():
    # Analog read of constant digital output
    bits = np.random.randint(2, size=10)
    for bit in bits:
        yield check_analog_input_digital_constant, bit

@with_setup(setup, teardown)
def check_analog_input_digital_constant(bit):
    # Digital out on pin 15
    it = emSequenceItem()
    it.pin = [15]
    it.operationType = emSequenceOperationType().DIGITAL
    it.startTime = 0
    it.endTime = 10000
    it.frequency = bit
    it.cycleTime = 100
    cli.appendSequenceAction(it)

    # Analog recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 5000
    it.endTime = 6000
    it.frequency = 1e3
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=float)
    result = adc_voltage(result)

    assert len(result) >= 1, "No samples!"

    # Should read ~0.0V for 0 and ~3.3V for 1
    expected = np.round(3.3 * bit)
    assert all(expected == np.round(result, 0)), "Expected all %s, but got %s" % (expected, result)




def test_analog_input_analog_constant():
    # Analog read of constant analog output
    voltages = [-5.0, -2.5, 0.0, 2.5, 5.0]

    for voltage in voltages:
        yield check_analog_input_analog_constant, voltage

@with_setup(setup, teardown)
def check_analog_input_analog_constant(voltage):
    # Analog out on pin 15
    it = emSequenceItem()
    it.pin = [15]
    it.operationType = emSequenceOperationType().CONSTANT
    it.startTime = 0
    it.endTime = 10000
    it.amplitude = voltage_to_dac(voltage)
    cli.appendSequenceAction(it)

    # Analog recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 5000
    it.endTime = 6000
    it.frequency = 1e3
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=float)
    result = adc_voltage(result)

    assert len(result) >= 1, "No samples!"

    assert all(voltage == np.round(result, 1)), "Expected all %s but got %s" % (voltage, result)



def test_analog_input_digital_freq():
    # Analog read of digital frequency output
    freqs = np.linspace(10, 100e3, 50).astype(int)
    #freqs = [10, 100, 1000, 10e3, 100e3]
    for freq in freqs:
        yield check_analog_input_digital_freq, freq

@with_setup(setup, teardown)
def check_analog_input_digital_freq(freq):
    # Calculate sample time based on frequency to sample at least 4 periods
    freq = int(freq)
    sample_time = np.ceil(max(4e6 / freq, 1))

    # Calculate the number of periods expected based on (rounded up) sample time
    n_periods = sample_time * freq / 1e6

    # Aim for 10 samples per period, or maximum 500kHz
    sample_freq = min(freq * 10, 500e3)
    n_samples = int(real_sample_freq(sample_freq) * sample_time / 1e6)

    # Test passes if MSE <= 2.2
    mse_pass = 2.2

    # Signal lag cannot be more than 10% of the samples
    lag_limit = 0.1 * n_samples

    print "freq", freq, "sample_time", sample_time, "n_periods", n_periods, \
          "sample_freq", sample_freq, "n_samples", n_samples

    # Digital out on pin 15
    it = emSequenceItem()
    it.pin = [15]
    it.operationType = emSequenceOperationType().DIGITAL
    it.startTime = 0
    it.endTime = sample_time
    it.frequency = freq
    cli.appendSequenceAction(it)

    # Analog recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 0
    it.endTime = sample_time
    it.frequency = sample_freq
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=float)
    result = adc_voltage(result)

    print "Got %d samples: %r" % (len(result), result)

    assert len(result) >= n_samples, "Got fewer samples (%d) than expected (%s)" % \
            (len(result), n_samples)

    # Generate ideal reference signal
    t = np.linspace(0, 2 * n_periods * np.pi, n_samples, endpoint=False)
    expected = scipy.signal.square(t - np.pi)
    expected = (expected + 1) * 3.3 / 2

    # Zero pad expected
    if len(expected) < len(result):
        expected = np.concatenate([expected, np.zeros(len(result) - len(expected))])

    print "expected (%d): %r" % (len(expected), expected)

    # Calculate lag
    lag = signal_lag(expected, result)
    print "Lag:", lag

    # Shift signals
    if abs(lag) <= lag_limit:
        expected, result = signal_shift(expected, result, lag)
    else:
        print "WARNING: Lag unexpectedly high (%d)" % lag

    mse = np.mean((result - expected) ** 2)
    print "MSE:", mse

    if mse > mse_pass:
        plt.title("FAIL: %d Hz, MSE %s" % (freq, mse))
        plt.plot(result, 'r-', label="result")
        plt.plot(expected, 'g-', label="expected")
        plt.legend()
        plt.show()

    assert mse <= mse_pass, "MSE %s too high (>%s) for %d Hz" % (mse, mse_pass, freq)



def test_analog_input_analog_linear():
    # Analog read of linear analog signal
    signals = [ np.linspace(-5, 5, 11),
                np.linspace(0, 5, 42),
                np.linspace(-3.3, 3.3, 123),
                np.linspace(0.0, 1.0, 100),
                np.linspace(-4.1, 2.9, 1000),
                ]
    for i, signal in enumerate(signals):
        yield check_analog_input_analog_signal, i, signal

def test_analog_input_analog_sin():
    # Analog read of sine wave analog signal
    t10 = np.linspace(0, 2 * np.pi, 10)
    t50 = np.linspace(0, 2 * np.pi, 50)
    t100 = np.linspace(0, 2 * np.pi, 100)
    t250 = np.linspace(0, 2 * np.pi, 250)
    signals = [ np.sin(t10),
                np.sin(t10 - 3),
                np.sin(2 * t10),
                np.sin(t50),
                np.sin(t50 - 20),
                np.sin(3 * t50),
                np.sin(t100),
                np.sin(t100 + 42),
                np.sin(8 * t100),
                np.sin(t250),
                np.sin(t250 + 102),
                np.sin(13 * t250),
                ]
    for i, signal in enumerate(signals):
        yield check_analog_input_analog_signal, i, signal

def test_analog_input_analog_random():
    # Analog read of random analog signal
    signals = [-5 + 10 * np.random.random(10),
               -5 + 10 * np.random.random(50),
               -5 + 10 * np.random.random(100),
               -5 + 10 * np.random.random(250),
               -5 + 10 * np.random.random(333),
               ]
    for i, signal in enumerate(signals):
        yield check_analog_input_analog_signal, i, signal, 0.5

@with_setup(setup, teardown)
def check_analog_input_analog_signal(index, signal, mse_pass=0.1):
    # Each input signal sample is held for 1ms
    sample_time = 1000 * len(signal)

    # 10 samples per 1ms
    sample_freq = 10e3
    n_samples = int(real_sample_freq(sample_freq) * sample_time / 1e6)

    # Signal lag cannot be more than 10% of the samples
    lag_limit = 0.1 * n_samples

    print "signal #%d (%d): %s" % (index, len(signal), signal)
    print "sample_time", sample_time, "sample_freq", sample_freq, "n_samples", n_samples

    # Generate signal as analog out on pin 15
    for t, s in enumerate(signal):
        it = emSequenceItem()
        it.pin = [15]
        it.operationType = emSequenceOperationType().CONSTANT
        it.startTime = t * 1000
        it.endTime = t * 1000 + 1000
        it.amplitude = voltage_to_dac(s)
        cli.appendSequenceAction(it)

    # Analog recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 0
    it.endTime = sample_time
    it.frequency = sample_freq
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=float)
    result = adc_voltage(result)
    print "Got %d samples: %r" % (len(result), result)

    assert len(result) >= n_samples, "Got fewer samples (%d) than expected (%s)" % \
            (len(result), n_samples)

    # Generate reference signal
    expected = dac_to_voltage(voltage_to_dac(signal))
    expected = np.repeat(expected, 10)
    print "expected: %r" % (expected)

    # Zero pad expected
    if len(expected) < len(result):
        expected = np.concatenate([expected, np.zeros(len(result) - len(expected))])

    # Calculate lag
    lag = signal_lag(expected, result)
    print "Lag:", lag

    # Shift signals
    if abs(lag) <= lag_limit:
        expected, result = signal_shift(expected, result, lag)
    else:
        print "WARNING: Lag unexpectedly high (%d)" % lag

    mse = np.mean((result - expected) ** 2)
    print "MSE:", mse

    if mse > mse_pass:
        plt.title("FAIL: MSE %s" % (mse))
        plt.plot(expected, 'g-', label="expected")
        plt.plot(result, 'r-', label="result")
        plt.legend()
        plt.show()

    assert mse <= mse_pass, "MSE %s too high (>%s) for signal #%d" % (mse, mse_pass, index)



def test_analog_input_sample_freq():
    sample_freqs = np.linspace(10, 500e3, 10).astype(int)

    for sample_freq in sample_freqs:
        yield check_analog_input_sample_freq, sample_freq

@with_setup(setup, teardown)
def check_analog_input_sample_freq(sample_freq):
    sample_time = min(1e6, max(10e6 / sample_freq, 1))
    n_samples = int(real_sample_freq(sample_freq) * sample_time / 1e6)

    print "sample_freq", sample_freq, "sample_time", sample_time, "n_samples", n_samples

    # Analog recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 0
    it.endTime = sample_time
    it.frequency = sample_freq
    it.operationType = emSequenceOperationType().RECORD   #implies analogue 
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=float)
    print "Got %d samples" % len(result)

    assert len(result) >= n_samples, "Got fewer samples (%d) than expected (%s) when sampling at %d Hz for %d ms" % (len(result), n_samples, sample_freq, sample_time)
    assert len(result) <= n_samples + 10, "Got more samples (%d) than expected (%s) when sampling at %d Hz for %d ms" % (len(result), n_samples, sample_freq, sample_time)
