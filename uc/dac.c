#include "dac.h"
#include "mecobo.h"
#include <inttypes.h>
#include <stdio.h>

void setupDAC()
{
  uint16_t * dac = ((uint16_t*)EBI_ADDR_BASE) + (DAC0_POSITION * 0x100);
  //Bit string 
  //control bits 100, gain 00, buf 00 [ we have ref ], vdd 00
  //100 xx xx xx x 00 00
  //dac[DAC_PROGRAM_REGISTER] = 0x8030; //with gain on both blocks of channels
  dac[DAC_PROGRAM_REGISTER] = 0x8000;
  while(dac[11] != 0x8000);
  //while(dac[10]);
  dac[DAC_PROGRAM_REGISTER] = 0xA002; //LDAC single update (update DAC regs once) -- this matches state machine.
  while(dac[11] != 0xA002);
  //while(dac[10]);

//  dac[0] = 0x8000;
  for(unsigned int i = DA_CHANNELS_START; i < DA_CHANNELS_START+8; i++) {
    setVoltage(i, 128);
  }
}

void setVoltage(uint16_t channel, uint16_t voltage)
{
      
  command(0, DAC_CONTROLLER_ADDR, DAC_REG_LOAD_VALUE, 128);

  uint16_t channelAddr = channel - DA_CHANNELS_START;
  //printf("channelAddr: channel: %u, %x\n",channel, channelAddr);
  
  uint16_t * dac = ((uint16_t*)EBI_ADDR_BASE) + (DAC0_POSITION * 0x100);
  uint16_t wrd = 0x7FF0 & ((channelAddr << 12) | (voltage << 4));
  dac[DAC_PROGRAM_REGISTER] = wrd;
  while(dac[11] != wrd);
  //printf("   Setup word sent to DAC: %x\n", wrd);
}

