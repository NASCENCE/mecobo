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

    /*
    channelToXbar[FPGA_ADC_0_A] = 8;
    channelToXbar[FPGA_ADC_0_B] = 9;
    channelToXbar[FPGA_ADC_0_C] = 10;   
    channelToXbar[FPGA_ADC_0_D] = 11;
    channelToXbar[FPGA_ADC_0_E] = 12;
    channelToXbar[FPGA_ADC_0_F] = 13;
    channelToXbar[FPGA_ADC_0_G] = 14;
    channelToXbar[FPGA_ADC_0_H] = 15;
    */
    channelToXbar[FPGA_ADC_0_A] = 8;
    channelToXbar[FPGA_ADC_0_B] = 9;
    channelToXbar[FPGA_ADC_0_C] = 15;      //channel 'C' (2) is attached to pin X15
    channelToXbar[FPGA_ADC_0_D] = 14;
    channelToXbar[FPGA_ADC_0_E] = 10;
    channelToXbar[FPGA_ADC_0_F] = 11;
    channelToXbar[FPGA_ADC_0_G] = 13;
    channelToXbar[FPGA_ADC_0_H] = 12;
 
    //Second crossbar. the channel to XBAR maps a channel to a X-index on the
    //crossbar. 
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
        pinToADChannel[i] = FPGA_IO_Pins_TypeDef::INVALID;
        pinToDAChannel[i] = FPGA_IO_Pins_TypeDef::INVALID;
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

    pinToADChannel.clear();
    pinToDAChannel.clear();
    channelToPin.clear();

    for(int i = 0; i < 16*numCards; i++) {
        pinToADChannel[i] = FPGA_IO_Pins_TypeDef::INVALID;
        pinToDAChannel[i] = FPGA_IO_Pins_TypeDef::INVALID;
    }
}

bool channelMap::isADChannel(FPGA_IO_Pins_TypeDef chan) 
{
    return ((AD_CHANNELS_START <= chan) && (chan <= AD_CHANNELS_END));
}

void channelMap::mapDAPin(int pin, FPGA_IO_Pins_TypeDef channel) 
{
    std::cout << "Mapped DA/Digital channel " << channel << " to pin " << pin << std::endl;
    pinToDAChannel[pin] = channel;
    channelToPin[channel].push_back(pin);
    return;
}

void channelMap::mapADPin(int pin, FPGA_IO_Pins_TypeDef channel) 
{
    std::cout << "Mapped AD channel " << channel << " to pin " << pin << std::endl;
    pinToADChannel[pin] = channel;
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
    //we could get a list of pins to assign to a single channel here
    //since one channel can be sent to many pins 
    for (auto p: pin) {

        FPGA_IO_Pins_TypeDef chan = FPGA_IO_Pins_TypeDef::INVALID;

        switch(pinconfigDataType) {
            case PINCONFIG_DATA_TYPE_RECORD_ANALOGUE:

                //check if there is a mapping
                chan = pinToADChannel[p];
                if(isADChannel(chan)) {
                    emException e;
                    e.Reason = "Can't record the same pin twice, ignored.";
                    throw e;
                }

                if (numADchannels < maxADchannels) {
                    channel = (FPGA_IO_Pins_TypeDef)(AD_CHANNELS_START + (numADchannels));
                    mapADPin(p, channel);
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
                //check if there is a mapping
                chan = pinToDAChannel[p];
                //Pin has already been mapped
                if(chan != FPGA_IO_Pins_TypeDef::INVALID) {
                    return chan;
                }

                //Try to map to a analogue pin.
                if (numDAchannels < maxDAchannels) {
                    channel = (FPGA_IO_Pins_TypeDef)(DA_CHANNELS_START + (numDAchannels));
                    mapDAPin(p, channel);
                    numDAchannels++;
                    std::cout << "Mapped DA pin " << p << " to channel " << channel << std::endl;
                } else {
                    std::cout << "All out of DA channels" << std::endl;
                    emException e;
                    e.Reason = "All out of DA channels.";
                    e.Source = "channelMap::mapItem()";
                    throw e;
                }
                break;

            default:
                //      case PINCONFIG_DATA_TYPE_DIGITAL_OUT:
                //     case PINCONFIG_DATA_TYPE_RECORD:
                if(pinToDAChannel[pin[0]] != FPGA_IO_Pins_TypeDef::INVALID) {
                    channel = pinToDAChannel[pin[0]];
                } else {
                    std::cout << "IO channel added to DA pin map\n"; 
                    channel = (FPGA_IO_Pins_TypeDef)(IO_CHANNELS_START + (numIOchannels));
                    mapDAPin(p, channel);
                    numIOchannels++;
                }

                break;

                //By default we give a Digital channel -- these can be both recording and analogue,
                //depending on the command given.
        }
    }
    return channel;
}

std::vector<uint8_t> channelMap::getXbarConfigBytes()
{
    //From the DATASHEET:
    /*Data to control the switches is clocked serially into a 256-bit
      shift register and then transferred in parallel to 256 bits of mem-
      ory. The rising edge of SCLK, the serial clock input, loads data
      into the shift register. The first bit loaded via SIN, the serial
      data input, controls the switch at the intersection of row Y15
      and column X15. The next bits control the remaining columns
      (down to X0) of row Y15, and are followed by the bits for row
      Y14, and so on down to the data for the switch at the intersec-
      tion of row Y0 and column X0. The shift register is dynamic, so
      there is a minimum clock rate, specified as 20 kHz.
      */



    //Example: To get dac output on pin 15,
    //we need to open X0,Y31, which is the XBAR2 Y15.
    //
    //Now, the config registers are clocked serially,
    //so to be able to hit THIS bit in particular, we must send it
    //in the FIRST word then.
    //
    //Every 16 bits controls a new Mecobo pin (Y-axis on the XBAR).
    //
    //Y31 is CONTROLLED BY config word 0.
    //X0 is controlled by the LAST bit of this config word, i.e. bit 15.
    //
    //This is a little confusing, but it's how it is.
    //
    //Thus what we are looking for, output wise, is config word 0.
    //
    //There are 32 Y-pins out of the XBARs. 16-bit words 0 to 15 control 
    //the output from xbar 2 (analog stuff), and 16 to 31 control xbar 1 (digital
    //stuff)
    //
    //But take care here: we do NOT want to be able to drive the 
    //same Y out from each crossbar. 

    std::vector<uint16_t> config;
    for(int i = 0; i < 32; i++) {
        uint16_t data = 0;

        config.push_back(data);
    }

    for(auto pc : pinToDAChannel) { 

        //pinToDAchannel is a map, so we get back a touple when iterating
        //the second element is the channel, the first element is the pin it is
        //assigned to. 
        //
        //This essentially means that 'channel' is what goes into the 
        //xbar on the X-indices and Y-indices are the pin.
        FPGA_IO_Pins_TypeDef channel = pc.second;

        //Skip invalid mappings
        if(channel == FPGA_IO_Pins_TypeDef::INVALID) {
            continue;
        }

        int pin = pc.first;
        //int configIndexYpinInverse = -1;

        //configIndexYpinInverse is the index into the config stream.
        //Since we have mapped the PINs to the Y's of the XBARS,
        //we have two choices to drive/source one pin.
        //xbar1, or xbar2. We add 16 to program xbar1.

        /*
           if (channel < IO_CHANNELS_END) { //Digital channels
           configIndexYpinInverse = 15 - pin;
           } else {
           configIndexYpinInverse = 31 - pin;
        //This is not a digital channel.
        }
        std::cout << "DA channel on pin " << pin << "config word " << configIndexYpinInverse << std::endl;

        config[configIndexYpinInverse] |= (1 << (channelToXbar[channel]));
        std::cout << "Config word " << configIndexYpinInverse << " Y-pin:" << pin + 15 <<", X:" << channelToXbar[channel] << " ::" << config[configIndexYpinInverse] << std::endl;
        //}
        */
        //int offset = 16 * (31 - pin) + (15 - channel);
        //    std::cout << "CHANNELMAP: Opening switch X" << channel << ", Y" << pin << " at offset " << offset << std::endl;

        //because i.e. pin 15 is controlled by word 16 and word 31, but we don't
        //want to open both.
        int configWord = 0;
        if (channel < IO_CHANNELS_END) {
            configWord = 15-pin;
        } else {
            configWord = 31-pin;
        }
        //channel to XBAR tells us which X-index is associated with this channel.
        config[configWord] |= (0x1 << channelToXbar[channel]);
        std::cout << "DA list, config word " << configWord << " used for x,y " << channelToXbar[channel] << "," <<  pin << std::endl;

}


for(auto pc : pinToADChannel) { 
    //It's actually possible for 2 channels to be on 1 pin. It's odd, but... allowable. <= NO NO NO
    //for (auto pin : channelToPin) {
    FPGA_IO_Pins_TypeDef channel = pc.second;

    //Skip invalid mappings
    if(channel == FPGA_IO_Pins_TypeDef::INVALID) {
        continue;
    }

    int pin = pc.first;
    /*
       int configIndexYpinInverse = -1;

    //The first 16 words control the digital channels.  word 0 controls Y15, and so on.
    //The next 16 words control the AD/DA-chans.
    //Since we have mapped the PINs to the Y's of the XBARS,
    //we have two choices to drive/source one pin.
    //xbar1, or xbar2. We add 16 to program xbar1.

    if (channel < IO_CHANNELS_END) { //Digital channels
    std::cout << "Digital recording channel setup\n";
    configIndexYpinInverse = 15 - pin;
    } else {
    configIndexYpinInverse = 31 - pin;
    //This is not a digital channel.
    }

    config[configIndexYpinInverse] |= (1 << (channelToXbar[channel]));
    std::cout << "Config word " << configIndexYpinInverse << " Y:" << pin <<", X:" << channelToXbar[channel] << " ::" << config[configIndexYpinInverse] << std::endl;
    */
    int configWord = 0;
    if (channel < IO_CHANNELS_END) {
        configWord = 15-pin;
    } else {
        configWord = 31-pin;
    }

    config[configWord] |= (0x1 << channelToXbar[channel]);
    std::cout << "AD list, config word " << configWord << " used for x,y " << channelToXbar[channel] << "," << pin << std::endl;   //}
}


//Swap all the bytes before transmission because we have worked with
//  //little endian stuff so the bytes are the wrong order for xmission
/*
   std::vector<uint16_t> reordered;
   for (auto w : config) {
#ifdef WIN32
reordered.push_back(_byteswap_ushort(w));
#else 
reordered.push_back(__builtin_bswap16(w));
#endif
}*/

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
FPGA_IO_Pins_TypeDef channelMap::getADChannel(int pin) 
{
    return pinToADChannel[pin];
}

//one pin can only have one channel assigned to it.
FPGA_IO_Pins_TypeDef channelMap::getDAChannel(int pin) 
{
    return pinToDAChannel[pin];
}

