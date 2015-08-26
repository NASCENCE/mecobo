/*
 * USB.h
 *
 *  Created on: May 7, 2014
 *      Author: oddrune
 */

#ifndef USB_H_
#define USB_H_

#include <stdio.h>
#include <libusb.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>

#include "../mecoprot.h"

class USB {
private:

	std::vector<uint8_t> endpoints;
	std::vector<uint8_t> debugEndpoints;

	libusb_device ** devs;
	struct libusb_device_handle * mecoboHandle;
	std::vector<libusb_device *> mecoboBoards;
	struct libusb_context * usbContext;

	int numMecobo;
	int usbAddress;

	void getEndpoints(std::vector<uint8_t> & endpoints, struct libusb_device * dev, int interfaceNumber);

  std::mutex endpointMutex;

public:
	USB();
	virtual ~USB();

	void sendBytes(uint8_t endpoint, uint8_t * bytes, int numBytes);
	void getBytes(uint8_t endpoint, uint8_t * bytes, int numBytes);
	void sendBytesDefaultEndpoint(uint8_t * bytes, int numBytes);
	void getBytesDefaultEndpoint(uint8_t * bytes, int numBytes);

	int getUsbAddress() const {
		return usbAddress;
	}
};

#endif /* USB_H_ */
