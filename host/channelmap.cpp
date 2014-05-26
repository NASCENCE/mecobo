#include "channelmap.h"
#include <iostream>
#include <string.h>
#include "emEvolvableMotherboard.h"

channelMap::~channelMap()
{
  //Do something.
}
channelMap::channelMap()
{
  numCards = 1;

  numADchannels = 0;
  maxADchannels = 8;
  
  numDAchannels = 0;
  maxDAchannels = 8;
 
  numIOchannels = 0;
  maxIOchannels = 16;

  //X indices on xbar 1.
  channelToXbar[FPGA_DAC_0_A] = 0;
  channelToXbar[FPGA_DAC_0_B] = 1;
  channelToXbar[FPGA_DAC_0_C] = 2;
  channelToXbar[FPGA_DAC_0_D] = 3;
  channelToXbar[FPGA_DAC_0_E] = 4;
  channelToXbar[FPGA_DAC_0_F] = 5;
  channelToXbar[FPGA_DAC_0_G] = 6;
  channelToXbar[FPGA_DAC_0_H] = 7;
  
  channelToXbar[FPGA_ADC_0_A] = 8;
  channelToXbar[FPGA_ADC_0_B] = 9;
  channelToXbar[FPGA_ADC_0_C] = 10;
  channelToXbar[FPGA_ADC_0_D] = 11;
  channelToXbar[FPGA_ADC_0_E] = 12;
  channelToXbar[FPGA_ADC_0_F] = 13;
  channelToXbar[FPGA_ADC_0_G] = 14;
  channelToXbar[FPGA_ADC_0_H] = 15;

  //Second crossbar.
  channelToXbar[FPGA_DIGI_0] = 0;
  channelToXbar[FPGA_DIGI_1] = 1;
  channelToXbar[FPGA_DIGI_2] = 2;
  channelToXbar[FPGA_DIGI_3] = 3;
  channelToXbar[FPGA_DIGI_4] = 4;
  channelToXbar[FPGA_DIGI_5] = 5;
  channelToXbar[FPGA_DIGI_6] = 6;
  channelToXbar[FPGA_DIGI_7] = 7;
  channelToXbar[FPGA_DIGI_8] = 8;
  channelToXbar[FPGA_DIGI_9] = 9;
  channelToXbar[FPGA_DIGI_10] = 10;
  channelToXbar[FPGA_DIGI_11] = 11;
  channelToXbar[FPGA_DIGI_12] = 12;
  channelToXbar[FPGA_DIGI_13] = 13;
  channelToXbar[FPGA_DIGI_14] = 14;
  channelToXbar[FPGA_DIGI_15] = 15;

  for(int i = 0; i < 16*numCards; i++) {
    pinToChannel[i].push_back(FPGA_IO_Pins_TypeDef::INVALID);
  }
}

void channelMap::reset()
{
  numADchannels = 0;
  maxADchannels = 8 * numCards;
  numDAchannels = 0;
  maxDAchannels = 8 * numCards;
  numIOchannels = 0;
  maxIOchannels = 16 * numCards;

  pinToChannel.clear();
  channelToPin.clear();

  for(int i = 0; i < 16*numCards; i++) {
    pinToChannel[i].push_back(FPGA_IO_Pins_TypeDef::INVALID);
  }

}


void channelMap::mapPin(int pin, FPGA_IO_Pins_TypeDef channel) 
{
  /*
  if(pinToChannel.count(pin)) {
    emException e;
    e.Reason = "Pin already assigned in this sequence";
    throw e;
    return;
  }*/
  
  std::cout << "Mapped channel " << channel << " to pin " << pin << std::endl;
  pinToChannel[pin].push_back(channel);
  channelToPin[channel].push_back(pin);
  return;
}


//We allow the same channel to drive multiple pins.
FPGA_IO_Pins_TypeDef channelMap::getChannelForPins(std::vector<int> pin, int pinconfigDataType) 
{
  if((numADchannels + numIOchannels + numDAchannels) >= (16 * numCards)) {

    //TODO:It's possible that we should be able to reuse a channel here, if it's already assigned to a
    //given channel type and we want to keep using it as such.
    emException e;
    e.Reason = "Maximum channels in one sequence used. 16 for 1 daughterboard. ";
    e.Source = "channelMap::mapItem()";

    throw e;
  }

  FPGA_IO_Pins_TypeDef channel = FPGA_IO_Pins_TypeDef::INVALID;
 
  switch(pinconfigDataType) {
    case PINCONFIG_DATA_TYPE_RECORD_ANALOGUE:

      if (numADchannels < maxADchannels) {
        channel = (FPGA_IO_Pins_TypeDef)(AD_CHANNELS_START + (numADchannels));
        for (auto p: pin) {
          mapPin(p, channel);
        }
        numADchannels++;
      } else {
        std::cout << "All out of AD channels" << std::endl;
        emException e;
        e.Reason = "All out of AD channels.";
        e.Source = "channelMap::mapItem()";
        throw e;
      }
    break;

    case PINCONFIG_DATA_TYPE_DAC_CONST:
    case PINCONFIG_DATA_TYPE_PREDEFINED_PWM:
      //Try to map to a analogue pin.
      if (numDAchannels < maxDAchannels) {
        channel = (FPGA_IO_Pins_TypeDef)(DA_CHANNELS_START + (numDAchannels));
        for(auto p : pin) {
          mapPin(p, channel);
        }
        numDAchannels++;
      } else {
        std::cout << "All out of DA channels" << std::endl;
        emException e;
        e.Reason = "All out of DA channels.";
        e.Source = "channelMap::mapItem()";
        throw e;
      }
    break;

    //By default we give a Digital channel -- these can be both recording and analogue,
    //depending on the command given.
    default:
      if (numIOchannels < maxADchannels) {
        channel = (FPGA_IO_Pins_TypeDef)(IO_CHANNELS_START + (numIOchannels));
        for(auto p: pin) {
          mapPin(p, channel);
        }
        numIOchannels++;
      } else {
        std::cout << "All out of IO channels" << std::endl;
        emException e;
        e.Reason = "All out of IO channels.";
        e.Source = "channelMap::mapItem()";
        throw e;
      }
      break;

  }
  return channel;
}

std::vector<uint8_t> channelMap::getXbarConfigBytes()
{
  //Every 16 bits controls a new pin (Y-axis on the XBAR).
  //There are 32 Y-pins out of the XBARs. 16-bit words 0 to 15 control 
  //the output from xbar 2, and 16 to 31 control xbar 1.
  //
  //But take care here: we do NOT want to be able to drive the 
  //same Y out from each crossbar. 

  std::vector<uint16_t> config;
  for(int i = 0; i < 32; i++) {
    uint16_t data = 0;
   
    config.push_back(data);
  }

  for(auto channels : pinToChannel) { 
  //It's actually possible for 2 channels to be on 1 pin. It's odd, but... allowable.
    for (auto pc : channels.second) {
      FPGA_IO_Pins_TypeDef channel = pc;
    
      //Skip invalid mappings
      if(channel == FPGA_IO_Pins_TypeDef::INVALID) {
        continue;
      }

      int pin = channels.first;
      int configIndex = -1;

      //The first 16 words control the digital channels.  word 0 controls Y15, and so on.
      //The next 16 words control the AD/DA-chans.
      //Since we have mapped the PINs to the Y's of the XBARS,
      //we have two choices to drive/source one pin.
      //xbar1, or xbar2. We add 16 to program xbar1.

      if (channel < IO_CHANNELS_END) { //Digital channels
        configIndex = 15 - pin;
      } else {
        configIndex = 31 - pin;
        //This is not a digital channel.
        
        if ((AD_CHANNELS_START <= channel) && (channel <= AD_CHANNELS_END)) {
          //This is an AD pin, so we'll open up the switch that "exposes"" a high impedance 
          //through the XBAR so as to avoid it ("digital channels xbar")from eating energy.
          config[15 - pin] |= 1 << (15-pin);
        }
      }
      
      config[configIndex] |= (1 << (channelToXbar[channel]));
      std::cout << "Config word " << configIndex << " Y:" << pin <<", X:" << channelToXbar[channel] << " ::" << config[configIndex] << std::endl;
    }
  }
 
  //After all the explicitly defined channels, we'll open the switches connecting
  //FPGA pins to the material (if they are unused, they are HIGH-Z)
  //A pin (the "channels" can have two Y-sources, 16 pins inbetween).
  //if the previous phase left any of them unconnected (i.e. a DAC did not use it, neither did an ADC),
  //then we'll simply open the DIGITAL switch.
  
  for(int i = 0; i < 16; i++){
    if ((config[i] == 0) && (config[i+16] == 0)) {
      if(i < 16) {
        config[i] = 1 << (15-i);
      }
    }
  }

  std::vector<uint8_t> ret;
  uint8_t * rawData = (uint8_t*)config.data();
  for (int i = 0; i < 64; i ++) {
    ret.push_back(rawData[i]);;
  }
  return ret;
}

std::vector<int> channelMap::getPin(FPGA_IO_Pins_TypeDef channel) 
{
  return channelToPin[channel];
}

//one pin can only have one channel assigned to it.
FPGA_IO_Pins_TypeDef channelMap::getChannel(int pin) 
{
  return pinToChannel[pin][0];
}

