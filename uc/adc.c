#include "adc.h"
#include "mecobo.h"
#include <inttypes.h>

void setupADC()
{
  uint16_t * ad = ((uint16_t*)EBI_ADDR_BASE) + (ADC0_POSITION * 0x100);
  //uint16_t CTRL = 0x8000;   //100 .
  ad[0x01] = 0; //overflow
  ad[0x02] = 1; //divide
  //Set up mode for all channels (we use 8 single ended inputs.)
//  for(uint8_t i = 0; i < 8; i++) {
 //   //                ADDR[2:0]
  //  ad[0x04] = CTRL | (i << 9);
   // while(ad[0x0A]);
  //}

  //Range register
  ad[0x04] = 0xAAA0; //range register written to +-5V on all channels for chans 0 to 4
  while(ad[0x0A]);
  ad[0x04] = 0xCAA0; //range register written to +-5V on all channels for chans 4 to 7
  while(ad[0x0A]);
 
  //control write.
  ad[0x04] = 0x8034; //two's complement, setup sequence
  while(ad[0x0A]);

  //ad[0x04] = 0x803C;//AD control command. 8 single ended inputs, straight binary coding, no sequencer.
  //while(ad[0x0A] == 1);
}

