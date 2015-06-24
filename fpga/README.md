# FPGA design

What follows are a few brief comments about how the FPGA design is done.

The system is composed of the modules "digital pincontroller", "adc controller", "dac controller",
and "memory controller".

The EBI bus connects these modules to the microcontroller interface.

## EBI bus
SRAM interface bus.

## Digital pin controller
Numerically Controlloed Oscillator with a 32 bit phase accumulator-based design.

## ADC controller
Forwards commands from the EBI bus and shifts them out serially to the ADC via the SPI interface.
Also continuously shifts in sampling data from the ADC. User sets sampling rate which effectively discards the unused samples by setting an overflow register that counts the number of cycles between each wanted sample that's ready for harvest.

## DAC controller
Shifts out data to the DAC serially.

## Memory interface
Programmed by appending the channels to a list. The list is iterated round-robin, and if a unit (ADC/Digital pin controller) has a new sample ready (based on the user's sample rate selection of the ADC/PC) a new sample is stored in Block RAM.



#Mecobo v4.0 brain dump

There is a FIFO that gets loaded with Commands from the EBI interface. Each command is formatted like this:

Timeslot: 32 bits
Command:  64 bits

the start-time specifies when to schedule the item. the item will run a specific time, given by the command itself and then die. the scheduler does not need to do anything about killing items.



