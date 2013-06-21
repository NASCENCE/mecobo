// Microcontroller code
// DMA will take care of interface-comms:
// USB -> CPU-RAM (inputDataQueue)
// CPU-RAM -> USB (outputDataQueue)
//
// The uC will access the FPGA itself via the EBI
// interface, where it will communicate via a 
// internal protocol. 
// 

#include <stdint.h>
#include "em_usb.h"

#define FPGA_BASE_ADDR 0

//Signal generator defines
#define SIGNAL_GEN_OFFSET 0x4
#define NUM_PINS 100

#define COMMAND_REG_OFFSET 0

struct mecoPack {
    uint32_t size;
    uint8_t command;
    uint8_t * data;
};

//Programs the FPGA via the slave serial interface.
int progFPGA(uint8_t * data);

int setupUSB();
int setupEBI();
int healthCheckFPGA();

// This package takes a pointer to the raw queue of 
// bytes retrieved over USB by the DMA controller, 
// and decodes it into a complete mecoPack.
//
// Blocks until there is enough data in the queue.
int getNextPack(void * queue, struct mecoPack * out);

//Decode and execute any commands in a packet.
//Blocks.
int decodeExecPack(struct mecoPack * p);

//DMA will also feed a inputbuffer in CPU RAM.
//Returndata will be packed as mecoPacks,
//and shipped to host over USB.
int sendPack(void * queue, struct mecoPack * in);

//FPGA interface. 

//Program a data generator and get response back if
//programming was OK.
//int progDatagen(uint32_t addr, uint32_t command, fpgaWord data);

//Internal functions for the uC.
int setupDma();
int setupUSB();
void DmaUsbRxDone(unsigned int channel, int primary, void *user);

int UsbDataReceived(USB_Status_TypeDef status,
                            uint32_t xf,
                            uint32_t remaining);
int UsbHeaderReceived(USB_Status_TypeDef status,
                            uint32_t xf,
                            uint32_t remaining);

int UsbDataSent(USB_Status_TypeDef status,
        uint32_t xf,
        uint32_t remaining);

void UsbStateChange(USBD_State_TypeDef oldState, USBD_State_TypeDef newState);
