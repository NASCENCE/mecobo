//#include <sys/time.h>
#include <stdio.h>
#include <libusb.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <vector>
#include <iostream>

#include "ga.h"

#include "mecohost.h"
#include "../mecoprot.h"

std::vector<uint8_t> eps;
std::vector<uint8_t> debug_eps;

struct libusb_device_handle * mecoboHandle;
std::vector<libusb_device *> mecobos;
struct libusb_context * ctx;
int numMecobos = 0;

int startUsb()
{
  int r;
  r = libusb_init(&ctx);

  if(r < 0) {
    printf("Init Error\n"); //there was an error
  }
  libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

  libusb_device ** devs;
  int numDevices = libusb_get_device_list(ctx, &devs);
  for(int i = 0; i < numDevices; i++) {
    libusb_device * dev = devs[i];
    libusb_device_descriptor desc;
    libusb_get_device_descriptor(dev, &desc);
    if(desc.idVendor == 0x2544 && desc.idProduct == 0x3) {
      std::cout << "Found Mecobo Device." << std::endl;
      mecobos.push_back(dev);
    }
  }

  int chosen = 0;
  int addr = 0;
  if(mecobos.size() > 1) {
    std::cout << "We only support 1 mecobo per server. Choose one of the connected boards.: " << std::endl;

    int count = 0;
    for (auto meco : mecobos) {
      std::cout << count++ << " Port:" << (int)libusb_get_port_number(meco) << " Address:" << (int)libusb_get_device_address(meco) << std::endl;
    }
    std::cout << "Enter number, followed by [enter]: ";
    std::cin >> chosen;
   
    addr = (int)libusb_get_device_address(mecobos[chosen]); 
    int port = addr + 9090;
    std::cout << "---------------------------------" << std::endl;
    std::cout << "The server will start at port " << port << std::endl;
    std::cout << "---------------------------------" << std::endl;
  }
  
  int err = libusb_open(mecobos[chosen], &mecoboHandle);

  if(err) {
    printf("Could not open device with vid 2544, pid 0003. I'm dying.\n");
    exit(-1);
  }   
 
  libusb_detach_kernel_driver(mecoboHandle, 0x1);	

  if(libusb_claim_interface(mecoboHandle, 0x1) != 0) {
    printf("Could not claim interface 0x1 of Mecobo\n");
    exit(-1);
  }

  printf("Getting endpoints from USB driver\n");
  getEndpoints(eps, mecobos[chosen], 1);
  getEndpoints(debug_eps, mecobos[chosen], 0);

  return addr;
}

void stopUsb()
{
  libusb_release_interface(mecoboHandle, 0x1);
  libusb_attach_kernel_driver(mecoboHandle, 0x1);	

  libusb_close(mecoboHandle);
  libusb_exit(ctx); //close the session
}

void getEndpoints(std::vector<uint8_t> & endpoints, struct libusb_device * dev, int interfaceNumber)
{
  //We know which interface we want the endpoints for (0x1), so
  //we'll just run through the descriptors and dig them out.

  //get Configuration 0 form devicej
  //printf("Retriveving the USB configuration\n");
  struct libusb_config_descriptor * config;
  libusb_get_active_config_descriptor(dev, &config);
  if(config == NULL) {
    printf("Could not retrieve active configuration for device :(\n");
    exit(-1);
  }

  //printf("We have %u interfaces for this configuration\n", config->bNumInterfaces);
  //std::cout << "Selecting interface " << interfaceNumber << std::endl;
  struct libusb_interface_descriptor interface = config->interface[interfaceNumber].altsetting[0];
  //printf("Interface has %d endpoints\n", interface.bNumEndpoints);
  for(int ep = 0; ep < interface.bNumEndpoints; ++ep) {
    if(interface.endpoint[ep].bEndpointAddress & 0x80) {
      //printf("Found input endpoint with address %x\n", interface.endpoint[ep].bEndpointAddress);
    } else {
      //printf("Found output with address %x\n", interface.endpoint[ep].bEndpointAddress);
    }
    endpoints.push_back(interface.endpoint[ep].bEndpointAddress);
  }
}


int createMecoPack(struct mecoPack * packet, uint8_t * data,  uint32_t dataSize, uint32_t command)
{
  if(dataSize > 0) {
    packet->data = (uint8_t*)malloc(dataSize);
    memcpy(packet->data, data, dataSize);
  } else {
    packet->data = NULL;
  }

  packet->size = dataSize;
  packet->command = command;
  return 0;
}

int setReg(uint32_t data) 
{
    struct mecoPack p;
    createMecoPack(&p, (uint8_t *)(&data), 4, USB_CMD_CONFIG_REG);
    sendPacket(&p, eps[2]);
	return 0;
}

int submitItem( FPGA_IO_Pins_TypeDef pin, 
            uint32_t startTime,
            uint32_t endTime,
            uint32_t duty,
            uint32_t antiduty,
            uint32_t cycles,
            uint32_t sampleRate,
            uint32_t type,
            uint32_t constantValue
            )
{

  uint32_t data[USB_PACK_SIZE_BYTES];

  data[PINCONFIG_START_TIME] = startTime;
  data[PINCONFIG_END_TIME] = endTime;
  data[PINCONFIG_DATA_FPGA_PIN] = pin;
  data[PINCONFIG_DATA_DUTY] = duty;
  data[PINCONFIG_DATA_ANTIDUTY] = antiduty;
  data[PINCONFIG_DATA_CYCLES] = cycles;
  data[PINCONFIG_DATA_SAMPLE_RATE] = sampleRate;
  data[PINCONFIG_DATA_TYPE] = type;
  data[PINCONFIG_DATA_CONST] = constantValue;

  data[PINCONFIG_DATA_RUN_INF] = 0x1; //debug!

  //printf("sending pinconfig on pin:%x\n", pin);
  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);

  sendPacket(&p, eps[2]);
  return 0;
}

int evoMoboRunSeq()
{
  struct mecoPack p;
  createMecoPack(&p, NULL, 0, USB_CMD_RUN_SEQ);
  sendPacket(&p, eps[2]);
  return 0;
}
int startOutput (FPGA_IO_Pins_TypeDef pin)
{
  uint32_t data;
  data = pin;
  struct mecoPack p;
  createMecoPack(&p, (uint8_t*)&data, 4, USB_CMD_START_OUTPUT);
  sendPacket(&p, eps[2]);
  return 0;
}

//TODO: ugly as all ghells
//Based on whatever is in either duty
//or anti duty register 
int startConstOutput (FPGA_IO_Pins_TypeDef pin)
{
  uint32_t data;
  data = pin;
  struct mecoPack p;
  createMecoPack(&p, (uint8_t*)&data, 4, USB_CMD_CONST);
  sendPacket(&p, eps[2]);
  return 0;
}

int startInput (FPGA_IO_Pins_TypeDef pin)
{
  uint32_t data;
  data = pin;
  struct mecoPack p;
  createMecoPack(&p, (uint8_t*)&data, 4, USB_CMD_STREAM_INPUT);
  sendPacket(&p, eps[2]);
  std::cout << "Setting up pin "  << pin << std::endl;

  return 0;
}


int getPin(FPGA_IO_Pins_TypeDef pin, uint32_t * val) 
{
  //Construct packet to get a pin
  uint32_t data[1];
  data[PINCONFIG_DATA_FPGA_PIN] = pin;
  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, 4, USB_CMD_READ_PIN);
  sendPacket(&p, eps[2]);

  //Get data back.
  int bytesRemaining = 4*3;
  int transfered = 0;
  uint8_t * rcv = (uint8_t*)malloc(12);
  while(bytesRemaining > 0) {
    libusb_bulk_transfer(mecoboHandle, eps[0], rcv, 12, &transfered, 10);
    bytesRemaining -= transfered;
  }
  memcpy(val, rcv, 12);
  free(rcv);
  return 0;
}

int sendPacket(struct mecoPack * packet, uint8_t endpoint) 
{
  //First, send header (fixed 8 bytes)
  //Create a buffer of data to send.
  uint32_t toSend[2];
  toSend[0] = packet->size;
  toSend[1] = packet->command;

  //printf("Sending header\n");
  int transfered = 0;
  int remaining = 8;
  while(remaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, (uint8_t*)toSend, 8, &transfered, 0);
    remaining -= transfered;
    //printf("Sent bytes of header, %u\n", transfered);
  } 
  //Send data afterwards.
  //printf("Sending data, size %u\n", packet->size);
  transfered = 0;
  remaining = packet->size;
  while(remaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, packet->data, packet->size, &transfered, 0);
    remaining -= transfered;
    //printf("Sent bytes of data, %u\n", transfered);
  }

  return 0;
}

int getPacket(struct mecoPack * packet)
{
  return 0;
}

bool sortValues(sampleValue i, sampleValue j) {
  return i.sampleNum < j.sampleNum;
}

static inline uint32_t get_bit(uint32_t val, uint32_t bit) 
{
    return (val >> bit) & 0x1;
}

int programFPGA(const char * filename)
{
  FILE* bitfile;

 
#ifdef WIN32
  int openResult = fopen_s(&bitfile, filename, "rb");
  perror ("programFPGA");
#else
  bitfile = fopen(filename, "rb");
#endif

  printf("Programming FPGA\n");
  fseek(bitfile, 0L, SEEK_END);
  long nBytes = ftell(bitfile);
  rewind(bitfile);

  int packsize = 32*1024;
  int nPackets = nBytes / packsize;
  int rest = nBytes % (packsize);
  printf("supposed to have ballpark 6,440,432 bits. have %ld\n", nBytes * 8);
  printf("file is %ld bytes, sending %d packets of %d bytes and one pack of %d bytes\n",
        nBytes, nPackets, packsize, rest);
  uint8_t * bytes;
  bytes = (uint8_t *)malloc(nBytes);

  fread(bytes, 1, nBytes, bitfile);

  struct mecoPack send;
  int i;
  for(i =0; i < nPackets; i++) {
    printf("Sending pack %d of %d, %d bytes of %ld for fpga programming\n", i + 1, nPackets, packsize, nBytes);
    printf("position %u in array\n", (i * packsize));
    createMecoPack(&send, bytes + (i*packsize), packsize, USB_CMD_PROGRAM_FPGA);
    sendPacket(&send, eps[2]);
  }
  //Send the rest if there is any.
  if(rest > 0) {
    printf("Sending the rest pack, position %u, size %d\n", (i*packsize), rest);
    struct mecoPack lol;
    createMecoPack(&lol, bytes + (i*packsize), rest, USB_CMD_PROGRAM_FPGA);
    sendPacket(&lol, eps[2]);
  }

  free(bytes);
  printf("\n");
  fclose(bitfile);
  return 0;
}


int getSampleBuffer(std::vector<sampleValue> & samples)
{
  //Send request for buffer size
  struct mecoPack pack;
  createMecoPack(&pack, 0, 0, USB_CMD_GET_INPUT_BUFFER_SIZE);
  sendPacket(&pack, eps[2]);
  uint32_t nSamples = 0;
  getBytesFromUSB(eps[0], (uint8_t *)&nSamples, 4);
 
  std::cout << "SampleBuffer size collected: " << nSamples << std::endl;
  if(nSamples == 0) {
    return 0;
  } else {
    sampleValue* collectedSamples = new sampleValue[nSamples];
    std::vector<sampleValue> collected;
    //So ask for the samples back. Not more at least. 
    createMecoPack(&pack, (uint8_t*)&nSamples, 4, USB_CMD_GET_INPUT_BUFFER);
    sendPacket(&pack, eps[2]);
    std::cout << "Receiving bytes:" << sizeof(sampleValue) * nSamples << std::endl;
    getBytesFromUSB(eps[0], (uint8_t*)collectedSamples, sizeof(sampleValue) * nSamples);
    std::cout << "Collected from USB" << std::endl;
    sampleValue * casted = (sampleValue *)collectedSamples;
    for(int i = 0; i < (int)nSamples; i++) {
      samples.push_back(casted[i]);
    }
    delete[] collectedSamples;

  }
   return 0;
}

int getBytesFromUSB(int endpoint, uint8_t * bytes, int nBytes)
{
  //Get data back.
  //printf("Waiting for %d bytes from meco\n", nBytes);
  //There is a max size to the end point...
  int bytesRemaining = nBytes;
  int transfered = 0;
  int ret = 0;
  while(bytesRemaining > 0) {
    ret = libusb_bulk_transfer(mecoboHandle, endpoint, bytes, nBytes, &transfered, 100);
    if(ret != 0) {
      printf("LIBUSB ERROR: %s\n", libusb_error_name(ret));
    }
    bytesRemaining -= transfered;
    printf("we have %d bytes remaining\n", bytesRemaining);
  }

  return 0;
}


int getMecoboStatus(struct mecoboDev * dev)
{
  struct mecoPack p;
  uint8_t dat[STATUS_BYTES];
  createMecoPack(&p, 0, 0, USB_CMD_STATUS);
  sendPacket(&p, eps[2]);
  getBytesFromUSB(eps[0], dat, STATUS_BYTES);
  uint32_t * d = (uint32_t *)dat;
  dev->fpgaConfigured = d[0];
  dev->bufElements = d[1];

  return 0;
}

bool UsbIsFpgaConfigured()
{
  struct mecoPack p;
  uint8_t dat[STATUS_BYTES];
  createMecoPack(&p, 0, 0, USB_CMD_STATUS);
  sendPacket(&p, eps[2]);
  getBytesFromUSB(eps[0], dat, STATUS_BYTES);
  printf("getting!\n");
  uint32_t * d = (uint32_t *)dat;
  bool fpgaConfigured = (d[STATUS_FPGA_CONFIGURED] ? true : false);

  if(fpgaConfigured == 1) {
    return true;
  } else {
    return false;
  }
}

void resetAllPins()
{
  struct mecoPack p;
  createMecoPack(&p, 0, 0, USB_CMD_RESET_ALL);
  sendPacket(&p, eps[2]);
}

void moboSetLed(int led, int mode) {
  struct mecoPack p;
  uint32_t dat[2];
  dat[LED_SELECT] = (uint32_t)led;
  dat[LED_MODE] = (uint32_t)mode; 
  createMecoPack(&p, (uint8_t*)dat, 8, USB_CMD_LED);
  sendPacket(&p, eps[2]);
}
