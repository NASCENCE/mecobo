# Firmware

This contains the firmware that runs on the microcontroller (an EFM32GG990F1024
at 48MHz) on the Mecobo motherboard.

## Building

Running build.sh will attempt to: 
- download the dependency Energy Micro / Silicon Labs libraries, and unzip them in the current directory.
- download the Linaro ARM GCC Toolchain and unzip it in the current directory (might want to install this properly)
- build the firmware

If something fails, it shouldn't be too hard to discern what needs to be done by peeking at build.sh.


# NOR Flash
This version of the board contains a 256Mb NOR flash that holds
bitfiles. The first 512 bytes is reserved for a file allocation table
that holds information about the rest of the chip. The region is divided
into 512/16=32 fields, where each field is laid out as follows:

0 -  7: File identifier
8 - 11: File size
12 -15: Byte-offset of file in chip.
