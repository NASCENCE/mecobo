# mecobo

Mecobo is an open source experimental platform made for the EU-funded NASCENCE project.

It consists of a custom produced PCB, communicating with a host computer using
a USB 2.0 interface. The host computer runs a Thrift Server that exposes the features of the board through a Thrift Interface.

### The most recent branch is Mecobo_v4.1.

What follows is a short description of each of the subfolders.

## host
This contains the implementation of the Thrift server that uses the board as
it's backend. Also contains makefile for linux based systems.

## fpga
This contains the HDL of the hardware that runs on the FPGA on the Mecobo
motherboard.

## uc 
This contains the firmware that runs on the microcontroller of the Mecobo
motherboard. This is the unit that exposes the USB interface to the host
computer.

## interface_hardware
This contains the Altium schematics and also preproduced Gerber files that you
can use to produce your own version of the board.

## winbuild
This contains a Visual Studio project which you can use to build the Thrift
server to run on a windows machine.

## EMServer
Contains a 'dummy' implementation of the Thrift server to be used for testing, and various support software to be used on the client side. 

# Misc

### specifications

Daughterboards has 16 pins. Each daughterboard has max 8 analogue inputs and 8
analogue outputs, along with potentially 16 digital channels througout the
entire stack.

With 1 daughterboard connected, there is a maximum samplerate of about 200K
samples/second output for each of the analoge output pins. For analogue input, a
maxium rate of 75K samples/second. These rates are divided by 8 for each
stacked daughterboard. Note that these are absolute maximum ratings that can be
achieved, and that these numbers depend highly on the usage pattern of the stack.

With no daughterboard connected, each IO pin controlled by the fpga can toggle
at rates > 10MHz. 	
