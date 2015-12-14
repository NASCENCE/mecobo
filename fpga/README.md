# FPGA design

What follows are a few brief comments about how the FPGA design is done.

The system is composed of the modules "digital pincontroller", "adc controller", "dac controller" and some FIFOs that receieve commands from the microcontroller and pushes collected samples to the microcontroller.

The EBI bus connects these modules to the microcontroller interface.

## EBI bus and ebi interface
20 bit address, 16 bit data word Interface bus. The EBI interface module takes
care of all requests to the physical interface. The microcontroller can read the
status register from address 0 and write 80-bit commands on addresses 1 through
5. After address 5 has been written, the command is pushed to the 'command
   FIFO', whose outputs are read by the 'scheduler' unit. This unit looks at the
   timestamp (the topmost 32 bits of the command) to decide if the command is to
   be scheduled. This process is controlled by a 1MHz clock, effectively giving
   a 1us scheduling granularity.

## Digital pin controller
Numerically Controlled Oscillator with a 32 bit phase accumulator-based design.

## ADC controller
Forwards commands from the EBI bus and shifts them out serially to the ADC via the SPI interface.
Also continuously shifts in sampling data from the ADC. User sets sampling rate which effectively discards the unused samples by setting an overflow register that counts the number of cycles between each wanted sample that's ready for harvest.

## DAC controller
Shifts out data to the DAC serially.

## Sample collector 
Programmed by appending the channels to a list. The list is iterated round-robin, and if a unit (ADC/Digital pin controller) has a new sample ready (based on the user's sample rate selection of the ADC/PC) a new sample is pushed to a FIFO that can be read from the EBI interface.
