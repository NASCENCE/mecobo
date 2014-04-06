mecobo
======

Firmware for the Mecobo IO breakout board, plus the linux driver

Stuff from EFM you need:
EM_CMSIS_3.20.0.zip

todo
======
 - Support programming the microcontroller using the on-board bootloader. The bootloader will set up a CDC USB connection if you tie SWCLK to high and give it a reset. 

hardware
========
 - Daughterboards has 16 pins. Each daughterboard has max 8 analogue inputs and 8 analogue outputs, along with potentially 16 digital channels througout the entire stack.
 
