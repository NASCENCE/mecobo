#include "adc.h"
#include "mecobo.h"
#include <inttypes.h>

void setupADC()
{
  //uint16_t * ad = ((uint16_t*)EBI_ADDR_BASE) + (ADC0_POSITION * 0x100);
  uint16_t * ad = getPinAddress(AD_CHANNELS_START);  //((uint16_t*)EBI_ADDR_BASE) + (ADC0_POSITION * 0x100);
  //uint16_t CTRL = 0x8000;   //100 .
  ad[0x01] = 0; //overflow
  ad[0x02] = 1; //divide

  //Set up mode for all channels (we use 8 single ended inputs.)
  /*
  for(uint16_t i = 0; i < 8; i++) {
 //   //                ad[2:0]
    uint16_t ctrlWord = 0x8014 | (i << 10);
    printf("MODE CTRL: %x\n", ctrlWord);
    ad[0x04] = ctrlWord;
    while(ad[0x0A]);
  }
  */

  //Range register 1
  uint16_t data = 0;
  uint16_t lol = 0xAAA0;
  ad[0x04] = lol; //range register written to +-5V on all channels for chans 0 to 4
  while((data = ad[0x0B]) != lol) {printf("a:%x\n", data);}  //check if busy, if not, go.
  printf("setup finally: %x\n", data);
  //Range register 2
  ad[0x04] = 0xCAA0; //range register written to +-5V on all channels for chans 4 to 7
  while((data = ad[0x0B]) != 0xCAA0) {printf("c:%x\n", data);}  //check if busy, if not, go.
  printf("setup finally: %x\n", data);
}

