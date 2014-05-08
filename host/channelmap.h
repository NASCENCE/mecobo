#ifndef __CHANNELMAP_H__
#define __CHANNELMAP_H__

#include <inttypes.h>
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

    //One pin has only one channel
    std::map<int, FPGA_IO_Pins_TypeDef> pinToChannel;
    //One channel can be on many pins
    std::map<FPGA_IO_Pins_TypeDef, std::vector<int>> channelToPin;

    std::map<FPGA_IO_Pins_TypeDef, int> channelToXbar;

  public:
    //This will attempt to map a pin, but it WILL be annoyed if you try to use more channels than we have. 
    void mapPin(const std::vector<int> pins, FPGA_IO_Pins_TypeDef channel);
    FPGA_IO_Pins_TypeDef getChannelForItem(emSequenceItem item);
    std::vector<uint8_t> getXbarConfigBytes();

    //Since one channel can be on many pins, 
    std::vector<int> getPin(FPGA_IO_Pins_TypeDef channel);
    FPGA_IO_Pins_TypeDef getChannel(int pin);

    channelMap();
    ~channelMap();
};
#endif


