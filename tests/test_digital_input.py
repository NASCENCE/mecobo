"""
Tests for digital input
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



def test_digital_input_digital_constant():
    # Digital read of constant digital output
    bits = np.random.randint(2, size=10)
    for bit in bits:
        yield check_digital_input_digital_constant, bit

@with_setup(setup, teardown)
def check_digital_input_digital_constant(bit):
    # Digital out on pin 15
    it = emSequenceItem()
    it.pin = [15]
    it.operationType = emSequenceOperationType().DIGITAL
    it.startTime = 0
    it.endTime = 10000
    it.frequency = bit
    it.cycleTime = 100
    cli.appendSequenceAction(it)

    # Digital recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 1000
    it.endTime = 10000
    it.frequency = 1e3
    it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
    it.operationType = emSequenceOperationType().RECORD
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=int)
    result = digital_samples(result)

    assert len(result) >= 1, "No samples!"

    expected = bit
    assert all(expected == result), "Expected all %s, but got %s" % (expected, result)



def test_digital_input_analog_constant():
    # Digital read of constant analog output
    voltages = np.linspace(-5.0, 5.0, 21)

    for voltage in voltages:
        yield check_digital_input_analog_constant, voltage

@with_setup(setup, teardown)
def check_digital_input_analog_constant(voltage):
    # Analog out on pin 15
    it = emSequenceItem()
    it.pin = [15]
    it.operationType = emSequenceOperationType().CONSTANT
    it.startTime = 0
    it.endTime = 10000
    it.amplitude = voltage_to_dac(voltage)
    cli.appendSequenceAction(it)

    # Digital recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 1000
    it.endTime = 10000
    it.frequency = 1e3
    it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
    it.operationType = emSequenceOperationType().RECORD
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=int)
    result = digital_samples(result)

    assert len(result) >= 1, "No samples!"

    expected = digital_threshold(voltage)

    assert all(expected == result), "Expected all %s for %s V, but got %s" % (expected, voltage, result)



def test_digital_input_digital_freq():
    # Digital read of digital frequency output
    # Max sample rate is SAMPLE_CLK/4 and we want 10 samples per period,
    # so max frequency we can reliably test is SAMPLE_CLK/40
    freqs = np.linspace(1e3, SAMPLE_CLK/40, 50).astype(int)
    #freqs = [459938]
    #freqs = [1645530]

    for freq in freqs:
        yield check_digital_input_digital_freq, freq

@with_setup(setup, teardown)
def check_digital_input_digital_freq(freq):
    # Calculate sample time based on frequency to sample at least 10 periods
    freq = int(freq)
    sample_time = max(10e6 / freq, 1)

    # Calculate the number of periods expected based on sample time
    n_periods = sample_time * freq / 1e6

    # Aim for 10 samples per period, or maximum SAMPLE_CLK/4
    sample_freq = real_sample_freq(min(freq * 10, SAMPLE_CLK/4))
    n_samples = sample_freq * sample_time / 1e6

    print "freq", freq, "sample_time", sample_time, \
          "sample_freq", sample_freq, "n_samples", n_samples

    # Round up sample time to make sure we get enough samples
    sample_time = np.ceil(sample_time)

    # Round down the number of expected samples since we might miss the last period
    n_samples = np.floor(n_samples)

    # Test passes if MSE <= 0.1
    mse_pass = 0.1

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

    # Digital recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 0
    it.endTime = sample_time
    it.frequency = sample_freq
    it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
    it.operationType = emSequenceOperationType().RECORD
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=int)
    result = digital_samples(result)

    print "Got %d samples: %r" % (len(result), result)

    assert len(result) >= n_samples, "Got fewer samples (%d) than expected (%s)" % \
            (len(result), n_samples)

    # Generate ideal reference signal
    t = np.linspace(0, 2 * n_periods * np.pi, n_samples, endpoint=False)
    expected = scipy.signal.square(t - np.pi)
    expected = (expected + 1) / 2

    check_expected_result(expected, result, "%d Hz" % freq, mse_pass)



def test_digital_input_analog_linear():
    # Digital read of linear analog signal
    signals = [ np.linspace(-5, 5, 11),
                np.linspace(0, 5, 100),
                np.linspace(0, 3.3, 100),
                np.linspace(0.0, 3.3, 1000),
                np.linspace(0.0, 1.7, 100),
                ]
    for signal in signals:
        yield check_digital_input_analog_signal, signal

def test_digital_input_analog_sin():
    # Digital read of sine wave analog signal
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
    signals = np.array(signals)
    signals *= 3.3
    for signal in signals:
        yield check_digital_input_analog_signal, signal

def test_digital_input_analog_random():
    # Digital read of random analog signal
    signals = [-5 + 10 * np.random.random(10),
               -5 + 10 * np.random.random(50),
               -5 + 10 * np.random.random(100),
               -5 + 10 * np.random.random(250),
               -5 + 10 * np.random.random(333),
               ]
    for signal in signals:
        yield check_digital_input_analog_signal, signal

@with_setup(setup, teardown)
def check_digital_input_analog_signal(signal):
    # Each input signal sample is held for 1ms
    sample_time = 1000 * len(signal)

    # 10 samples per 1ms
    sample_freq = real_sample_freq(10e3)
    n_samples = int(sample_freq * sample_time / 1e6)

    # Test passes if MSE <= 0.1
    mse_pass = 0.1

    print "signal (%d): %s" % (len(signal), signal)
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

    # Digital recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 0
    it.endTime = sample_time
    it.frequency = sample_freq
    it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
    it.operationType = emSequenceOperationType().RECORD
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=int)
    print "Got %d samples" % len(result)
    result = digital_samples(result)

    assert len(result) >= 1, "No samples!"

    # Generate reference signal
    signal = np.repeat(signal, 10)
    expected = digital_threshold(dac_to_voltage(voltage_to_dac(signal)))

    check_expected_result(expected, result, "signal", mse_pass, signal)



def test_digital_input_sample_freq():
    # Max sample rate is SAMPLE_CLK/(4*n) where n is number of pins
    sample_freqs = list(np.linspace(10, 500e3, 25).astype(int))
    sample_freqs += list(np.linspace(10, SAMPLE_CLK/4.0, 25).astype(int))
    sample_freqs += [2343758]
    sample_freqs += [2419354]
    sample_freqs += [6250006]

    for sample_freq in sample_freqs:
        yield check_digital_input_sample_freq, sample_freq

@with_setup(setup, teardown)
def check_digital_input_sample_freq(sample_freq):
    sample_time = min(1e6, max(100e6 / sample_freq, 1))
    n_samples = real_sample_freq(sample_freq) * sample_time / 1e6

    print "sample_freq", sample_freq, "sample_time", sample_time, "n_samples", n_samples
    print "real sample_freq", real_sample_freq(sample_freq)

    # Round up sample time to make sure we get enough samples
    sample_time = np.ceil(sample_time)
    # Round down the number of expected samples since we might miss the last period
    n_samples = np.floor(n_samples)

    print "adjusted sample_freq", sample_freq, "sample_time", sample_time, "n_samples", n_samples

    # Digital recording at pin 0
    it = emSequenceItem()
    it.pin = [0]
    it.startTime = 2   # BUG? Fudge this in UC?
    it.endTime = sample_time + 2
    it.frequency = sample_freq
    it.waveFormType = emWaveFormType().PWM  #makes it into a digital recording
    it.operationType = emSequenceOperationType().RECORD
    cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    result = np.array(cli.getRecording(0).Samples, dtype=int)
    print "%d Hz: got %d samples" % (sample_freq, len(result))

    assert len(result) >= n_samples, "Got fewer samples (%d) than expected (%s) when sampling at %d Hz for %d us" % (len(result), n_samples, sample_freq, sample_time)
    assert len(result) <= n_samples + 20, "Got more samples (%d) than expected (%s) when sampling at %d Hz for %d us" % (len(result), n_samples, sample_freq, sample_time)

