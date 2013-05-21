#!/bin/bash

#Get dependency ZIPs from Energy Micro

wget http://cdn.energymicro.com/dl/packages/EM_CMSIS_3.20.0.zip
wget http://cdn.energymicro.com/dl/packages/EM_BSP_COMMON_3.20.0.zip

#unzip them in the local directory
unzip EM_CMSIS_3.20.0.zip
unzip EM_BSP_COMMON_3.20.0.zip

