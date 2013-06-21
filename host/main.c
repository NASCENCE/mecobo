#include <sys/time.h>
#include <stdio.h>
#include <libusb.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include "mecohost.h"
#include "../uc/mecoprot.h"

#define NUM_ENDPOINTS 4

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

  uint32_t pinVal = 0;
  //Command line arguments
  if (argc > 1) {
    pinVal = atoi(argv[1]); 
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

  char eps[NUM_ENDPOINTS];
  getEndpoints(eps, mecobo, 0x1);

  const int bytes = 4;
  int bytesRemaining = bytes;
  struct timespec t0;
  struct timespec t1;

  //Now we have the enpoint addresses and we can start pushing data.
  uint8_t * data = malloc(bytes);
  uint8_t * rcv = malloc(bytes);
  int transfered = 0;
  int packets = 0;

  uint8_t header[8];
  header[0] = 0xa;
  header[1] = 255;
  header[2] = 2;
  header[3] = 0x4;
  uint32_t * h32 = (uint32_t *)header;
  h32[1] = bytes;

  uint32_t * d32 = (uint32_t*)data;
  *d32 = pinVal;

  //send header
  double start = omp_get_wtime();
  
  setPin(FPGA_D14, 1);

  struct mecoPack pack;
  createMecoPack(&pack, data, bytes, 23);
  sendPacket(&pack, eps[2]);

  //Create a getPin command (cmd == 3)
  createMecoPack(&pack, data, 4, 3);
  sendPacket(&pack, eps[2]);
  //Get some data back (just the data part)
  bytesRemaining = 4;
  while(bytesRemaining > 0) {
    libusb_bulk_transfer(mecoboHandle, eps[0], rcv, 4, &transfered, 0);
    bytesRemaining -= transfered;
    uint32_t * r32 = (uint32_t*)rcv;
    printf("received:%d bytes.\n", transfered);
    printf("PinVal: %u\n", *rcv);
  }

  double end = omp_get_wtime();
  /*	
      for(int q = 0; q < transfered; q++) {
      printf("%x,", rcv[q]);
      }
      printf("\n");
      */
  printf("Rate: %f KB/s\n", ((bytes)/(double)(end-start))/(double)1024);

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

int setPin(FPGA_IO_Pins_Typedef pin, uint32_t value) 
{
  uint32_t data[3];
  data[PINCONFIG_DATA_FPGA_PIN] = pin;
  data[PINCONFIG_DATA_TYPE] = PINTYPE_OUT;
  data[PINCONFIG_DATA_CONST] = value;
  struct mecoPack p;
  createMecoPack(&p, data, 3, CMD_CONFIG_PIN);
  sendPacket(&p, eps[2]);
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
    printf("Sent bytes of header, %u\n", transfered);
  } 
  //Send data afterwards.
  transfered = 0;
  remaining = packet->dataSize;
  while(remaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, packet->data, packet->dataSize, &transfered, 0);
    remaining -= transfered;
    printf("Sent bytes of data, %u\n", transfered);
  }

  return 0;
}

int getPacket(struct mecoPack * packet)
{
  return 0;
}
