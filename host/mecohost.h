#ifndef __MECOHOST_H__
#define __MECOHOST_H__

#include <stdint.h>
#include <vector>
#include "../mecoprot.h"


void startUsb();
void stopUsb();

int getPin(FPGA_IO_Pins_TypeDef pin, uint32_t * val);
void getEndpoints(std::vector<uint8_t> & endpoints, struct libusb_device * dev, int interfaceNumber);

static inline uint32_t get_bit(uint32_t val, uint32_t bit);
int experiment_foo();
int experiment_ca();
int setReg(uint32_t data);
int programFPGA(const char * filename);
int startOutput (FPGA_IO_Pins_TypeDef pin);



struct mecoboDev {
  int fpgaConfigured;
  uint32_t bufElements;
};

int getMecoboStatus(struct mecoboDev * dev);

struct usbContext {
    
};
//This will take the data and copy it into a newly allocated 
//field in the data packet.
int createMecoPack(struct mecoPack * packet, uint8_t * data,  uint32_t dataSize, uint32_t command);

//This will create two USB calls; one for the header and one for the data.
//This will block until the data is delivered.
int sendPacket(struct mecoPack * packet, uint8_t endpoint);

//This will wait for the amount of data specified in the packet, 
//and update the internal fields to point to ALLOCATED data. (Please free this when done!).
int getPacket(struct mecoPack * packet);


int startInput (FPGA_IO_Pins_TypeDef pin);
int getBytesFromUSB(int endpoint, uint8_t * bytes, int nBytes);
int getSampleBuffer(std::vector<sampleValue> & values);

int setPin( FPGA_IO_Pins_TypeDef pin, 
            uint32_t duty,
            uint32_t antiduty,
            uint32_t cycles,
            uint32_t sampleRate);
#endif
