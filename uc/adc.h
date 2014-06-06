#ifndef __ADC_H__
#define __ADC_H__


#define ADC0_POSITION 100
//TODO: Allow more configuration.
void setupADC();

//The analogue channels are numbered 0 -> 32, 8 channels per daughterboard.
void getAnalogueChannelAddr(int channel);

#endif
