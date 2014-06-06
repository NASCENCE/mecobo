#ifndef __DAC_H__
#define __DAC_H__

#include <inttypes.h>
#include "mecobo.h"

#define DAC0_POSITION 50

#define DAC_PROGRAM_REGISTER 0

//TODO: Allow more configuration.
void setupDAC();

//The analogue outputs are numbered 0 -> 32, 8 channels per daughterboard.
void setVoltage(uint16_t channel, uint16_t voltage);

#endif
