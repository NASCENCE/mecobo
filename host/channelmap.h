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

    std::map<int, FPGA_IO_Pins_TypeDef> pinToChannel;
    std::map<FPGA_IO_Pins_TypeDef, int> channelToPin;

    std::map<FPGA_IO_Pins_TypeDef, int> channelToXbar;

  public:
    //This will attempt to map a pin, but it WILL be annoyed if you try to use more channels than we have. 
    void mapPin(int pin, FPGA_IO_Pins_TypeDef channel);
    FPGA_IO_Pins_TypeDef getChannelForItem(emSequenceItem item);
    void getXbarConfigBytes(uint8_t * bytes);

    int getPin(FPGA_IO_Pins_TypeDef channel);
    FPGA_IO_Pins_TypeDef getChannel(int pin);

    channelMap();
    ~channelMap();
};
#endif


