/*
 * USB.cpp
 *
 *  Created on: May 7, 2014
 *      Author: oddrune
 */

#include "USB.h"

USB::USB() {
	  int r;
	  r = libusb_init(&usbContext);

	  if(r < 0) {
	    printf("Init Error\n"); //there was an error
	  }
	  
    libusb_set_debug(usbContext, 3); //set verbosity level to 3, as suggested in the documentation

	  int numDevices = libusb_get_device_list(usbContext, &devs);
	  for(int i = 0; i < numDevices; i++) {
	    libusb_device * dev = devs[i];
	    libusb_device_descriptor desc;
	    libusb_get_device_descriptor(dev, &desc);
	    if(desc.idVendor == 0x2544 && desc.idProduct == 0x3) {
	      std::cout << "Found Mecobo Device." << std::endl;
	      mecoboBoards.push_back(dev);
	    }
	  }

	  int chosen = 0;
	  int addr = 0;
	  if(mecoboBoards.size() > 1) {
	    std::cout << "We only support 1 mecobo per server. Choose one of the connected boards.: " << std::endl;

	    int count = 0;
	    for (auto meco : mecoboBoards) {
	      std::cout << count++ << " Port:" << (int)libusb_get_port_number(meco) << " Address:" << (int)libusb_get_device_address(meco) << std::endl;
	    }
	    std::cout << "Enter number, followed by [enter]: ";
	    std::cin >> chosen;
    
	    addr = (int)libusb_get_device_address(mecoboBoards[chosen]);
	    int port = addr + 9090;
	    std::cout << "---------------------------------" << std::endl;
	    std::cout << "The server will start at port " << port << std::endl;
	    std::cout << "---------------------------------" << std::endl;
	  }

	  int err = libusb_open(mecoboBoards[chosen], &mecoboHandle);

	  if(err < 0) {
	    printf("Could not open device with vid 2544, pid 0003. Exiting.\n");
	    exit(-1);
	  }

    if (libusb_kernel_driver_active(mecoboHandle, 0x1) == 1) {
      
      err = libusb_detach_kernel_driver(mecoboHandle, 0x1);
      if (err) {
        std::cout << "Failed to detach kernel driver for USB. Someone stole the board?" << std::endl;
        exit(-1);
      }
    }

	  if((err = libusb_claim_interface(mecoboHandle, 0x1)) < 0) {

	    printf("Could not claim interface 0x1 of Mecobo, error number %d\n", err);
	    exit(-1);
	  }

    std::cout << "Getting endpoints from USB driver" << std::endl;
	  getEndpoints(endpoints, mecoboBoards[chosen], 1);
	  getEndpoints(debugEndpoints, mecoboBoards[chosen], 0);

	  usbAddress = addr;
    std::cout << "USB init done" << std::endl;
}

USB::~USB() {
	  libusb_release_interface(mecoboHandle, 0x1);
	  libusb_attach_kernel_driver(mecoboHandle, 0x1);
    libusb_free_device_list(devs, 1);

	  libusb_close(mecoboHandle);
	  libusb_exit(usbContext); //close the session
}


void USB::getEndpoints(std::vector<uint8_t> & endpoints, struct libusb_device * dev, int interfaceNumber)
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

/*
 * This function ships bytes down the endpoint.
 */
void USB::sendBytes(uint8_t endpoint, uint8_t * bytes, int numBytes)
{
  int remaining = numBytes;
  int transfered = 0;
  while(remaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, bytes, numBytes, &transfered, 0);
    remaining -= transfered;
  }
}

void USB::sendBytesDefaultEndpoint(uint8_t * bytes, int numBytes)
{
  sendBytes(endpoints[2], bytes, numBytes);
}

void USB::getBytes(uint8_t endpoint, uint8_t * bytes, int numBytes)
{
  //Get data back.
  //printf("Waiting for %d bytes from meco\n", nBytes);
  //TODO: There is a bug. Sometimes hang. If the buffer is larger than 64K it seems.

  int bytesRemaining = numBytes;
  int transfered = 0;
  int ret = 0;
  while(bytesRemaining > 0) {
    ret = libusb_bulk_transfer(mecoboHandle, endpoint, bytes, numBytes, &transfered, 5000);
    if(ret != 0) {
      printf("LIBUSB ERROR: %s\n", libusb_error_name(ret));
      printf("We're continuing, but this is probably not a good state to be in.\n");
      break;
    }
    bytesRemaining -= transfered;
  }
}

void USB::getBytesDefaultEndpoint(uint8_t * bytes, int numBytes)
{
  getBytes(endpoints[0], bytes, numBytes);
}
