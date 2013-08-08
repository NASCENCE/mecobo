#include <sys/time.h>
#include <stdio.h>
#include <libusb.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include "mecohost.h"
#include "../uc/mecoprot.h"

#define NUM_ENDPOINTS 4




char eps[NUM_ENDPOINTS];
int getPin(FPGA_IO_Pins_TypeDef pin, uint32_t * val);

static inline uint32_t get_bit(uint32_t val, uint32_t bit);
int experiment_foo();
int setReg(uint32_t data);
int programFPGA(const char * filename);

struct libusb_device_handle * mecoboHandle;
struct libusb_device * mecobo;
void getEndpoints(char * endpoints, struct libusb_device * dev, int interfaceNumber)
{
  //We know which interface we want the endpoints for (0x1), so
  //we'll just run through the descriptors and dig them out.

  //get Configuration 0 form device
  struct libusb_config_descriptor * config;
  libusb_get_active_config_descriptor(dev, &config);

  //Get the interface 0x1 descp
  struct libusb_interface_descriptor interface = config->interface[1].altsetting[0];
  for(int ep = 0; ep < interface.bNumEndpoints; ++ep) {
    if(interface.endpoint[ep].bEndpointAddress & 0x80) {
      printf("Found input endpoint with address %x\n", interface.endpoint[ep].bEndpointAddress);
    } else {
      printf("Found output with address %x\n", interface.endpoint[ep].bEndpointAddress);
    }
    endpoints[ep] = (char)interface.endpoint[ep].bEndpointAddress;
  }
}



int main(int argc, char ** argv) {

  uint32_t progFpga = 0;
  //Command line arguments
  if (argc > 1) {
    for(int i = 0; i < argc; i++) {
      if(strcmp(argv[i], "-f") == 0) {
          progFpga = 1;
      }
    }
 }


  libusb_context * ctx = NULL;

  int r;
  ssize_t cnt;

  r = libusb_init(&ctx);
  if(r < 0) {
    printf("Init Error\n"); //there was an error
    return 1;
  }
  libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

  mecoboHandle = libusb_open_device_with_vid_pid(ctx, 0x2544, 0x3);
  mecobo = libusb_get_device(mecoboHandle);	

  libusb_detach_kernel_driver(mecoboHandle, 0x1);	
  if(libusb_claim_interface(mecoboHandle, 0x1) != 0) {
    printf("Could not claim interface 0 of Mecobo\n");
  }

  getEndpoints(eps, mecobo, 0x1);

  double start = omp_get_wtime();
 
  //setReg(42);

  if(progFpga) 
    programFPGA("mecobo.bin");

  experiment_foo();

  double end = omp_get_wtime();
  /*	
      for(int q = 0; q < transfered; q++) {
      printf("%x,", rcv[q]);
      }
      printf("\n");
      */
  //printf("Rate: %f KB/s\n", ((bytes)/(double)(end-start))/(double)1024);

  libusb_release_interface(mecoboHandle, 0x1);
  libusb_attach_kernel_driver(mecoboHandle, 0x1);	

  libusb_close(mecoboHandle);

  libusb_exit(ctx); //close the session

  return 0;
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
}

int setPin( FPGA_IO_Pins_TypeDef pin, 
            uint32_t duty,
            uint32_t antiduty,
            uint32_t cycles,
            uint32_t sampleRate)
{

  uint32_t data[USB_PACK_SIZE_BYTES];

  data[PINCONFIG_DATA_FPGA_PIN] = pin;
  data[PINCONFIG_DATA_DUTY] = duty;
  data[PINCONFIG_DATA_ANTIDUTY] = antiduty;
  data[PINCONFIG_DATA_CYCLES] = cycles;
  data[PINCONFIG_DATA_SAMPLE_RATE] = sampleRate;

  data[PINCONFIG_DATA_RUN_INF] = 0x1; //debug!

  //printf("sending pinconfig on pin:%x\n", pin);
  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);

  sendPacket(&p, eps[2]);
}

int startOutput (FPGA_IO_Pins_TypeDef pin)
{
  uint32_t data;
  data = pin;
  struct mecoPack p;
  createMecoPack(&p, (uint8_t*)&data, 4, USB_CMD_START_OUTPUT);
  sendPacket(&p, eps[2]);
}

int startInput (FPGA_IO_Pins_TypeDef pin)
{
  uint32_t data;
  data = pin;
  struct mecoPack p;
  createMecoPack(&p, (uint8_t*)&data, 4, USB_CMD_STREAM_INPUT);
  sendPacket(&p, eps[2]);
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
    libusb_bulk_transfer(mecoboHandle, eps[0], rcv, 12, &transfered, 0);
    bytesRemaining -= transfered;
  }
  memcpy(val, rcv, 12);
  free(rcv);
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

int experiment_foo()
{
    //Repeat of experiment 
    std::vector<FPGA_IO_Pins_TypeDef> outPins = {
      FPGA_F16,
      FPGA_F17,
      FPGA_G14,
      FPGA_G16,
      FPGA_H16,
      FPGA_H17,
      FPGA_H15,
      FPGA_L12,
      FPGA_H14,
      FPGA_K14,
      FPGA_K12
    };
    
    std::vector<FPGA_IO_Pins_TypeDef> inPins = {
      FPGA_J16
    };

    //Setup input pins
    for(FPGA_IO_Pins_TypeDef inPin : inPins) {
      setPin(inPin, 0x1, 0x1, 0x1, 0xFFF);
    }

    for(uint16_t gene = 0; gene < 2048; gene++) {
      int bit = 0;
      std::string geneString;
      for(FPGA_IO_Pins_TypeDef pin : outPins) {
        if(get_bit(gene, bit++)) {
          setPin(pin, 0xFFFF, 0x0, 0x1, 0x0);
          geneString += "1 ";
        } else {
          setPin(pin, 0x0, 0xFFFF, 0x1, 0x0);
          geneString += "0 ";
        }
        startOutput(pin);
      }

      //start input
      for(FPGA_IO_Pins_TypeDef inPin : inPins) {
        startInput(inPin); //flips buffers. The next buffer should now be clean.
      }

      //Collect at least 5 samples 
      std::vector<sampleValue> samples;
      struct mecoboDev dev;
      bool done = false;
      while(!done) {
        getMecoboStatus(&dev);
        if(dev.bufElements >= 5) {
          done = true;
          getSampleBuffer(samples);
        }
      }
      for (int s = 0; s < 5; s++) {
        std::cout << geneString << " r: " << samples[s].value << " " << samples[s].sampleNum << " " << samples[s].pin << " " << std::endl;
      }
      samples.clear();
    }
}


static inline uint32_t get_bit(uint32_t val, uint32_t bit) 
{
    return (val >> bit) & 0x1;
}

int programFPGA(const char * filename)
{
  FILE * bitfile;


  bitfile = fopen(filename, "r");
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
}


int getSampleBuffer(std::vector<sampleValue> & samples)
{
  //Send request for buffer size
  struct mecoPack pack;
  createMecoPack(&pack, 0, 0, USB_CMD_GET_INPUT_BUFFER_SIZE);
  sendPacket(&pack, eps[2]);
  uint32_t size;
  getBytesFromUSB(eps[0], (uint8_t *)&size, 4);
  //Now get data back.
  //uint8_t * data = malloc(sizeof(struct sampleValue)*USB_BUFFER_SIZE);
  
  sampleValue collectedSamples[size];
  createMecoPack(&pack, 0, 0, USB_CMD_GET_INPUT_BUFFER);
  sendPacket(&pack, eps[2]);
  getBytesFromUSB(eps[0], (uint8_t*)collectedSamples, sizeof(sampleValue) * size);
  //std::cout << "Got " << size  << "samples" << std::endl;
  for(int i = 0; i < size; i++) {
    samples.push_back(collectedSamples[i]);
  }
}

int getBytesFromUSB(int endpoint, uint8_t * bytes, int nBytes)
{
  //Get data back.
  //printf("Waiting for %d bytes from meco\n", nBytes);
  int bytesRemaining = nBytes;
  int transfered = 0;
  while(bytesRemaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, bytes, nBytes, &transfered, 0);
    bytesRemaining -= transfered;
  }
  //printf("    ... got them!\n\n");

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
