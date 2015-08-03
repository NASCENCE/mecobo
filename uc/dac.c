#include "dac.h"
#include "mecobo.h"
#include <inttypes.h>
#include <stdio.h>

void setupDAC()
{


//  uint16_t * dac = ((uint16_t*)EBI_ADDR_BASE) + (DAC0_POSITION * 0x100);
  //Bit string 
  //control bits 100, gain 00, buf 00 [ we have ref ], vdd 00
  //100 xx xx xx x 00 00
  //note the 1 at the front --- this is a special little thing to notify the controller that it's a new value
  command(0, DAC_CONTROLLER_ADDR, DAC_REG_LOAD_VALUE, 0x18000);
  command(0, DAC_CONTROLLER_ADDR, DAC_REG_LOAD_VALUE, 0x1A002);
}

void setVoltage(uint16_t channel, uint16_t voltage)
{

  uint16_t channelAddr = channel - DA_CHANNELS_START;
  //printf("channelAddr: channel: %u, %x\n",channel, channelAddr);
  //uint16_t * dac = ((uint16_t*)EBI_ADDR_BASE) + (DAC0_POSITION * 0x100);
  uint32_t wrd = 0x7FF0 & ((channelAddr << 12) | (voltage << 4));
  wrd |= 0x10000;
  command(0, DAC_CONTROLLER_ADDR, DAC_REG_LOAD_VALUE, wrd);
}

