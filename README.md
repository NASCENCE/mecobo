# mecobo
======

Firmware for the Mecobo IO breakout board, plus the linux driver

Stuff from EFM you need:
EM_CMSIS_3.20.0.zip

## todo
======
 - Support programming the microcontroller using the on-board bootloader. The bootloader will set up a CDC USB connection if you tie SWCLK to high and give it a reset. 

 - "Registers": We add a feature to the API in which you can insert a special
   update-from-register sequence item. This sequence item will in stead of
   carrying a value to be updated on a pin, carry the register number from
   which it will update the selected channel with a value. The registers are
   stored on the microcontroller, and we also add a special "update register"
   API call that updates these on-board registers. API is updated, but
   the implementation is not in place yet.

- Multiple outputs to a single material pin should be allowed.

## hardware
 - Daughterboards has 16 pins. Each daughterboard has max 8 analogue inputs and 8 analogue outputs, along with potentially 16 digital channels througout the entire stack.

### specifications
With 1 daughterboard connected, there is a maximum samplerate of about 200K
samples/second output for each of the digital pins. For analogue input, a
maxium rate of 75K samples/second. These rates are divided by 8 for each
stacked daughterboard. Note that these are absolute maximum ratings that can be
achieved, and that these numbers depend highly on the usage pattern of the stack.

With no daughterboard connected, each IO pin controlled by the fpga can toggle
at rates > 10MHz. 
