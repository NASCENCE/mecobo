#ifndef __CHANNELMAP_H__
#define __CHANNELMAP_H__

#include <map>
#include <vector>

#include "emEvolvableMotherboard.h"
#include "../mecoprot.h"

using namespace  ::emInterfaces;
class channelMap {

  private:

    int numCards;

    int numADchannels;
    int maxADchannels;
    int numDAchannels;
    int maxDAchannels;
    int numIOchannels;
    int maxIOchannels;

    //One pin can only be one DAC and one AD so we have to separate maps
    std::map<int, FPGA_IO_Pins_TypeDef> pinToDAChannel;
    std::map<int, FPGA_IO_Pins_TypeDef> pinToADChannel;
    //One channel can be on many pins
    std::map<FPGA_IO_Pins_TypeDef, std::vector<int>> channelToPin;

    std::map<FPGA_IO_Pins_TypeDef, int> channelToXbar;

    bool isADChannel(FPGA_IO_Pins_TypeDef channel);
  public:
    //This will attempt to map a pin, but it WILL be annoyed if you try to use more channels than we have. 
    void mapDAPin(int pin, FPGA_IO_Pins_TypeDef channel);
    void mapADPin(int pin, FPGA_IO_Pins_TypeDef channel);
    FPGA_IO_Pins_TypeDef getChannelForPins(std::vector<int> pins, int pinType);
    std::vector<uint8_t> getXbarConfigBytes();

    //Since one channel can be on many pins, 
    std::vector<int> getPin(FPGA_IO_Pins_TypeDef channel);
    FPGA_IO_Pins_TypeDef getDAChannel(int pin);
    FPGA_IO_Pins_TypeDef getADChannel(int pin);

    void reset();

    channelMap();
    ~channelMap();

};
#endif


