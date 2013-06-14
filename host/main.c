#include <sys/time.h>
#include <stdio.h>
#include <libusb.h>
#include <stdlib.h>
#include <omp.h>

#define NUM_ENDPOINTS 4
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

int main(void) {

	libusb_context * ctx = NULL;

	int r;
	ssize_t cnt;

	r = libusb_init(&ctx);
	if(r < 0) {
		printf("Init Error\n"); //there was an error
		return 1;
	}
	libusb_set_debug(ctx, 4); //set verbosity level to 3, as suggested in the documentation
	

	struct libusb_device_handle * mecoboHandle = libusb_open_device_with_vid_pid(ctx, 0x2544, 0x3);
	struct libusb_device * mecobo = libusb_get_device(mecoboHandle);	

	libusb_detach_kernel_driver(mecoboHandle, 0x1);	
	if(libusb_claim_interface(mecoboHandle, 0x1) != 0) {
		printf("Could not claim interface 0 of Mecobo\n");
	}

	char eps[NUM_ENDPOINTS];
	getEndpoints(eps, mecobo, 0x1);

	const int bytes = 47;
	int bytesRemaining = bytes;
	struct timespec t0;
	struct timespec t1;
		
	//Now we have the enpoint addresses and we can start pushing data.
	uint8_t * data = malloc(bytes);
	uint8_t * rcv = malloc(bytes);
	int transfered = 0;
	int packets = 0;
	//clock_gettime(CLOCK_MONOTONIC, &t0);

    for(int d = 0; d < bytes; d++) {
        data[d] = 42+d;
    }
    
    //send
	double start = omp_get_wtime();
	while(bytesRemaining > 0) {
		libusb_bulk_transfer(mecoboHandle, eps[2], data, bytes, &transfered, 0);
		bytesRemaining -= transfered;
		printf("sent %d bytes\n", transfered);
	}
	//recieve
	bytesRemaining = bytes;

	while(bytesRemaining > 0) {
		libusb_bulk_transfer(mecoboHandle, eps[0], rcv, bytes, &transfered, 0);
        bytesRemaining -= transfered;
        printf("received:%u bytes: ", transfered);
        for(int q = 0; q < transfered; q++) {
            printf("%u,", rcv[q]);
        }
        printf("\n");
	}
	double end = omp_get_wtime();
	
	printf("Rate: %f KB/s\n", ((bytes)/(double)(end-start))/(double)1024);

	libusb_release_interface(mecoboHandle, 0x1);
	libusb_attach_kernel_driver(mecoboHandle, 0x1);	
	
	libusb_close(mecoboHandle);

	libusb_exit(ctx); //close the session

	return 0;
}
