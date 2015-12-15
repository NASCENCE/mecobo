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
    freqs = np.linspace(10, 50e3, 50).astype(int)
    #freqs = [10, 100, 1000, 10e3, 50e3]
    for freq in freqs:
        yield check_analog_input_digital_freq, freq

@with_setup(setup, teardown)
def check_analog_input_digital_freq(freq):
    # Calculate sample time based on frequency to sample at least 10 periods
    freq = int(freq)
    sample_time = max(10e6 / freq, 1)

    # Calculate the number of periods expected based on sample time
    n_periods = sample_time * freq / 1e6

    # Aim for 100 samples per period, or maximum 500kHz
    sample_freq = real_sample_freq(min(freq * 100, 500e3))
    n_samples = sample_freq * sample_time / 1e6

    print "freq", freq, "sample_time", sample_time, \
          "sample_freq", sample_freq, "n_samples", n_samples

    # Round up sample time to make sure we get enough samples
    sample_time = np.ceil(sample_time)

    # Round down the number of expected samples since we might miss the last period
    n_samples = np.floor(n_samples)

    # Test passes if MSE <= 1.0
    mse_pass = 1.0

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

    check_expected_result(expected, result, "%d Hz" % freq, mse_pass)



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
    sample_freq = real_sample_freq(10e3)
    n_samples = int(sample_freq * sample_time / 1e6)

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

    check_expected_result(expected, result, "signal #%d" % index, mse_pass)



def test_analog_input_multiple():
    pins = []
    for n in range(1, N_REC_ANALOG + 1):
        # Randomly select n ADC pins and 1 output pin
        p = np.random.choice(N_PINS, n + 1, replace=False)
        pins.append(list(p))

    # Generate a nice input signal
    t = np.linspace(0, 2 * np.pi, 100)
    signal = 5 * (np.sin(t) * np.cos(3 * t))

    for p in pins:
        out_pins = p[:1]
        rec_pins = p[1:]
        yield check_analog_input_multiple, signal, out_pins, rec_pins

@with_setup(setup, teardown)
def check_analog_input_multiple(signal, out_pins, rec_pins, mse_pass=0.1):
    # Each input signal sample is held for 1ms
    sample_time = 1000 * len(signal)

    # 10 samples per 1ms
    sample_freq = real_sample_freq(10e3)
    n_samples = int(sample_freq * sample_time / 1e6)

    print "out_pins", out_pins, "rec_pins", rec_pins
    print "sample_time", sample_time, "sample_freq", sample_freq, "n_samples", n_samples

    # Generate signal as analog out on out_pins
    for out_pin in out_pins:
        for t, s in enumerate(signal):
            it = emSequenceItem()
            it.pin = [out_pin]
            it.operationType = emSequenceOperationType().CONSTANT
            it.startTime = t * 1000
            it.endTime = t * 1000 + 1000
            it.amplitude = voltage_to_dac(s)
            cli.appendSequenceAction(it)

    # Set up analog recordings
    for rec_pin in rec_pins:
        it = emSequenceItem()
        it.pin = [rec_pin]
        it.startTime = 0
        it.endTime = sample_time
        it.frequency = sample_freq
        it.operationType = emSequenceOperationType().RECORD   #implies analogue
        cli.appendSequenceAction(it)

    cli.runSequences()
    cli.joinSequences()

    # Generate reference signal
    expected = dac_to_voltage(voltage_to_dac(signal))
    expected = np.repeat(expected, 10)
    #print "expected: %r" % (expected)

    for rec_pin in rec_pins:
        result = np.array(cli.getRecording(rec_pin).Samples, dtype=float)
        result = adc_voltage(result)
        print "Got %d samples on pin %d" % (len(result), rec_pin)

        assert len(result) >= n_samples, "Got fewer samples (%d) than expected (%s)" % \
                (len(result), n_samples)

        check_expected_result(expected, result, "pin %d" % rec_pin, mse_pass)



def test_analog_input_sample_freq():
    sample_freqs = np.linspace(10, 500e3, 25).astype(int)

    for sample_freq in sample_freqs:
        yield check_analog_input_sample_freq, sample_freq

@with_setup(setup, teardown)
def check_analog_input_sample_freq(sample_freq):
    sample_time = min(1e6, max(100e6 / sample_freq, 1))
    n_samples = real_sample_freq(sample_freq) * sample_time / 1e6

    print "sample_freq", sample_freq, "sample_time", sample_time, "n_samples", n_samples
    print "real sample_freq", real_sample_freq(sample_freq)

    # Round up sample time to make sure we get enough samples
    sample_time = np.ceil(sample_time)
    # Round down the number of expected samples since we might miss the last period
    n_samples = np.floor(n_samples)

    print "adjusted sample_freq", sample_freq, "sample_time", sample_time, "n_samples", n_samples

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

    assert len(result) >= n_samples, "Got fewer samples (%d) than expected (%s) when sampling at %d Hz for %d us" % (len(result), n_samples, sample_freq, sample_time)
    assert len(result) <= n_samples + 10, "Got more samples (%d) than expected (%s) when sampling at %d Hz for %d us" % (len(result), n_samples, sample_freq, sample_time)

