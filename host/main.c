#include <sys/time.h>
#include <stdio.h>
#include <libusb.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include "mecohost.h"
#include "../uc/mecoprot.h"

#define NUM_ENDPOINTS 4
char eps[NUM_ENDPOINTS];

int setPin( FPGA_IO_Pins_TypeDef pin, 
            uint32_t duty,
            uint32_t antiduty,
            uint32_t cycles) ;
int getPin(FPGA_IO_Pins_TypeDef pin, uint32_t * val);
static inline uint32_t get_bit(uint32_t val, uint32_t bit);
int experiment_foo();
int setReg(uint32_t data);
int programFPGA(char * filename);

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
  packet->data = malloc(dataSize);
  memcpy(packet->data, data, dataSize);

  packet->dataSize = dataSize;
  packet->command = command;
  return 0;
}

int setReg(uint32_t data) 
{
    struct mecoPack p;
    createMecoPack(&p, (uint8_t *)(&data), 4, CMD_CONFIG_REG);
    sendPacket(&p, eps[2]);
}

int setPin( FPGA_IO_Pins_TypeDef pin, 
            uint32_t duty,
            uint32_t antiduty,
            uint32_t cycles) 
{
  uint32_t data[5];

  data[PINCONFIG_DATA_FPGA_PIN] = pin;
  data[PINCONFIG_DATA_DUTY] = duty;
  data[PINCONFIG_DATA_ANTIDUTY] = antiduty;
  data[PINCONFIG_DATA_CYCLES] = cycles;

  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, 30, CMD_CONFIG_PIN);
  sendPacket(&p, eps[2]);
}

int getPin(FPGA_IO_Pins_TypeDef pin, uint32_t * val) 
{
    uint32_t data[1];
    data[PINCONFIG_DATA_FPGA_PIN] = pin;
    struct mecoPack p;
    createMecoPack(&p, (uint8_t *)data, 4, CMD_READ_PIN);
    sendPacket(&p, eps[2]);
    //Get data back.
    int bytesRemaining = 4;
    int transfered = 0;
    uint8_t rcv[4];
    while(bytesRemaining > 0) {
        libusb_bulk_transfer(mecoboHandle, eps[0], rcv, 4, &transfered, 0);
    bytesRemaining -= transfered;
    *val = (uint32_t)*rcv;
    //printf("received:%d bytes.\n", transfered);
  }
}

int sendPacket(struct mecoPack * packet, uint8_t endpoint) 
{
  //First, send header (fixed 8 bytes)
  //Create a buffer of data to send.
  uint32_t toSend[2];
  toSend[1] = packet->dataSize;
  uint8_t * toSend8 = (uint8_t *)toSend;
  toSend8[0] = 0xa;
  toSend8[1] = 0xb;
  toSend8[2] = 0xc;
  toSend8[3] = packet->command;

  int transfered = 0;
  int remaining = 8;
  while(remaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, toSend8, 8, &transfered, 0);
    remaining -= transfered;
    //printf("Sent bytes of header, %u\n", transfered);
  } 
  //Send data afterwards.
  transfered = 0;
  remaining = packet->dataSize;
  while(remaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, packet->data, packet->dataSize, &transfered, 0);
    remaining -= transfered;
    //printf("Sent bytes of data, %u\n", transfered);
  }

  return 0;
}

int getPacket(struct mecoPack * packet)
{
  return 0;
}

int experiment_foo()
{
    setPin(FPGA_F16, 0xFF, 0x67, 0xCC);
    setPin(FPGA_F17, 10, 50, 0x500);
    setPin(FPGA_G14, 0xFFFF, 0xFFFF, 0xFF);
    setPin(FPGA_G16, 0xABCD, 0xABCD, 0xFF);
}

static inline uint32_t get_bit(uint32_t val, uint32_t bit) 
{
    return (val >> bit) & 0x1;
}

int programFPGA(char * filename)
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
  bytes = malloc(nBytes);

  fread(bytes, 1, nBytes, bitfile);

  struct mecoPack send;
  int i;
  for(i =0; i < nPackets; i++) {
    printf("Sending pack %d of %d, %d bytes of %ld for fpga programming\n", i + 1, nPackets, packsize, nBytes);
    printf("position %u in array\n", (i * packsize));
    createMecoPack(&send, bytes + (i*packsize), packsize, CMD_PROGRAM_FPGA);
    sendPacket(&send, eps[2]);
  }
  //Send the rest if there is any.
  if(rest > 0) {
    printf("Sending the rest pack, position %u, size %d\n", (i*packsize), rest);
    struct mecoPack lol;
    createMecoPack(&lol, bytes + (i*packsize), rest, CMD_PROGRAM_FPGA);
    sendPacket(&lol, eps[2]);
  }

  free(bytes);
  printf("\n");
  fclose(bitfile);
}
