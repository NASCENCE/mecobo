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
  channelToXbar[FPGA_ADC_0_C] = 15;   
  channelToXbar[FPGA_ADC_0_D] = 14;
  channelToXbar[FPGA_ADC_0_E] = 10;
  channelToXbar[FPGA_ADC_0_F] = 11;
  channelToXbar[FPGA_ADC_0_G] = 13;
  channelToXbar[FPGA_ADC_0_H] = 12;

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
    pinToChannel[i] = FPGA_IO_Pins_TypeDef::INVALID;
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
    pinToChannel[i] = FPGA_IO_Pins_TypeDef::INVALID;
  }
}

bool channelMap::isADChannel(FPGA_IO_Pins_TypeDef chan) 
{
  return ((AD_CHANNELS_START <= chan) && (chan <= AD_CHANNELS_END));
}

void channelMap::mapPin(int pin, FPGA_IO_Pins_TypeDef channel) 
{
  /*
     if(pinToChannel.count(pin)) {
     }*/
  //Check if we record from the same pin twice

  //auto chan = pinToChannel[pin];

  std::cout << "Mapped channel " << channel << " to pin " << pin << std::endl;
  pinToChannel[pin] = channel;
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
  for (auto p: pin) {

    auto chan = pinToChannel[p];
    if(isADChannel(chan)) {
      emException e;
      e.Reason = "Can't record the same pin twice, ignored.";
      throw e;
    }
    //Pin has already been mapped
    if(chan != FPGA_IO_Pins_TypeDef::INVALID) {
      return chan;
    }


    switch(pinconfigDataType) {
      case PINCONFIG_DATA_TYPE_RECORD_ANALOGUE:

        if (numADchannels < maxADchannels) {
          channel = (FPGA_IO_Pins_TypeDef)(AD_CHANNELS_START + (numADchannels));
          mapPin(p, channel);
          //or not...
          numADchannels++;
        }
        else {
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
          mapPin(p, channel);
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
        if (numIOchannels < maxIOchannels) {
          //TODO Bug? We whould probably return a list of channels here
          if(pinToChannel[pin[0]] != FPGA_IO_Pins_TypeDef::INVALID) {
            channel = pinToChannel[pin[0]];
          } else {
            channel = (FPGA_IO_Pins_TypeDef)(IO_CHANNELS_START + (numIOchannels));
            mapPin(p, channel);
            numIOchannels++;
          }
        } else {
          std::cout << "All out of IO channels" << std::endl;
          emException e;
          e.Reason = "All out of IO channels.";
          e.Source = "channelMap::mapItem()";
          throw e;
        }
        break;
    }
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

  for(auto pc : pinToChannel) { 
    //It's actually possible for 2 channels to be on 1 pin. It's odd, but... allowable. <= NO NO NO
    //for (auto pin : channelToPin) {
    FPGA_IO_Pins_TypeDef channel = pc.second;

    //Skip invalid mappings
    if(channel == FPGA_IO_Pins_TypeDef::INVALID) {
      continue;
    }

    int pin = pc.first;
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
      //
    }

    config[configIndex] |= (1 << (channelToXbar[channel]));
    std::cout << "Config word " << configIndex << " Y:" << pin <<", X:" << channelToXbar[channel] << " ::" << config[configIndex] << std::endl;
    //}
  }


  //Swap all the bytes before transmission because we have worked with
  //  //little endian stuff so the bytes are the wrong order
  std::vector<uint16_t> reordered;
  for (auto w : config) {
    reordered.push_back(__builtin_bswap16(w));
  }

  std::vector<uint8_t> ret;
  uint8_t * rawData = (uint8_t*)reordered.data();
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
  return pinToChannel[pin];
}

