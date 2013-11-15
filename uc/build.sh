#!/bin/bash

#Get dependency ZIPs from Energy Micro

#wget http://cdn.energymicro.com/dl/packages/EM_CMSIS_3.20.0.zip
#wget http://cdn.energymicro.com/dl/packages/EM_BSP_COMMON_3.20.0.zip

#Get the arm-gcc toolchain (good tip: move this somewhere else)
wget https://launchpad.net/gcc-arm-embedded/4.7/4.7-2013-q3-update/+download/gcc-arm-none-eabi-4_7-2013q3-20130916-linux.tar.bz2
#unzip them in the local directory
#unzip EM_CMSIS_3.20.0.zip
#unzip EM_BSP_COMMON_3.20.0.zip
tar jxvf gcc-arm-none-eabi-4_7-2013q3-20130916-linux.tar.bz2
make
