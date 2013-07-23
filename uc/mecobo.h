#ifndef __MECOBO_H__
#define __MECOBO_H__

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
#include "em_gpio.h"
#include "mecoprot.h"

#define FPGA_BASE_ADDR 0


//Address offsets for the config of a pin controller
#define PINCONFIG_GLOBAL_CMD 0
#define PINCONFIG_DUTY_CYCLE 1
#define PINCONFIG_ANTIDUTY_CYCLE 2
#define PINCONFIG_CYCLES 3
#define PINCONFIG_RUN_INF 4
#define PINCONFIG_LOCAL_CMD 5
#define PINCONFIG_SAMPLE_RATE 6
#define PINCONFIG_SAMPLE_REG 7


//Possible FPGA commands
#define CMD_CONFIG_PIN  0x1
#define CMD_READ_PIN    0x2
#define CMD_CONFIG_REG  0x3
#define CMD_PROGRAM_FPGA  0x4


struct pinConfig {
  uint32_t fpgaPin;
  uint32_t pinType;
  uint32_t constantVal;
  uint32_t waveform;
  uint32_t freq;
  uint32_t phase;
  uint32_t antiduty;
  uint32_t duty;
  uint32_t cycles;
  uint32_t runInf;
  uint32_t sampleRate;

  //If pin is to be a DAC pin
  uint32_t nSamples;
  uint32_t * samples; //points to allocated data of samples. (12 bit samples)
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

#endif //__MECOBO_H_

