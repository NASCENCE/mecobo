#!/bin/bash

#Get dependency ZIPs from Energy Micro
if [ ! -f Gecko_SDK.zip ]; then
    wget http://www.silabs.com/Support%20Documents/Software/Gecko_SDK.zip
    mkdir -k efm32
    unzip Gecko_SDK.zip -d efm32/
fi

#Get the arm-gcc toolchain (good tip: move this somewhere else)
if [ ! -f gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 ]; then
    wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
    tar jxvf gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
fi


make
