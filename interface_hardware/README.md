# Hardware interface files

These directories contain ALTIUM project files and schematic files for
producing your own version of the NASCENCE interface hardware. 


### mecobo_motherboard

This is the "motherboard", which we call "mecobo". It can connect to the material directly.

### mixed_signal_daughterboard

This is the mixed signal daughterboard, which is a daughterboard that connects to the mecobo motherboard.

### crossbar_NI_PCB

This is a special board created for the project that acts as a break-out board
for a National  Instruments cable.

## Gerber files

The directories also contain the subdirectory generated/. This contains the
gerber and NC drill files you need to make a PCB without going through Altium.
You should be able to zip up these files and send them to your favorite PCB
manufacturer for production.


