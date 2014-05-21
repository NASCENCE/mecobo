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
#include "../mecoprot.h"

#define FPGA_BASE_ADDR 0

//LED defines
#define FRONT_LED_0 0
#define FRONT_LED_1 1
#define FRONT_LED_2 2
#define FRONT_LED_3 3

#define BOARD_LED_U0 4
#define BOARD_LED_U1 5
#define BOARD_LED_U2 6
#define BOARD_LED_U3 7

/* Define USB endpoint addresses */
#define EP_DATA_OUT1       0x01  /* Endpoint for USB data reception.       */
#define EP_DATA_OUT2       0x02  /* Endpoint for USB data reception.       */
#define EP_DATA_IN1        0x81  /* Endpoint for USB data transmission.    */
#define EP_DATA_IN2        0x82  /* Endpoint for USB data transmission.    */
#define EP_NOTIFY         0x82  /* The notification endpoint (not used).  */
#define BULK_EP_SIZE     USB_MAX_EP_SIZE  /* This is the max. ep size.    */


#define EBI_ADDR_BASE 0x80000000

//Address offsets for the config of a pin controller
#define PINCONFIG_GLOBAL_CMD 0
#define PINCONFIG_DUTY_CYCLE 1
#define PINCONFIG_ANTIDUTY_CYCLE 2
#define PINCONFIG_CYCLES 3
#define PINCONFIG_RUN_INF 4
#define PINCONFIG_LOCAL_CMD 5
#define PINCONFIG_SAMPLE_RATE 6
#define PINCONFIG_SAMPLE_REG 7
#define PINCONFIG_SAMPLE_CNT 8
#define PINCONFIG_STATUS_REG 9

//Possible FPGA commands
#define CMD_START_OUTPUT  1
#define CMD_READ_PIN    2
#define CMD_INPUT_STREAM  3
#define CMD_PROGRAM_FPGA  4
#define CMD_RESET  5
#define CMD_CONST  6


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


#ifdef __cplusplus
extern "C" {
#endif
  void UsbStateChange(USBD_State_TypeDef oldState, USBD_State_TypeDef newState);
#ifdef __cplusplus
}
#endif



//Start output on given pin
void eADesigner_Init(void);
void startOutput(FPGA_IO_Pins_TypeDef channel);
void startInput(FPGA_IO_Pins_TypeDef channel, int sampleRate);
void getInput(FPGA_IO_Pins_TypeDef channel);
int fpgaConfigPin(struct pinConfig * p);
uint16_t * getPinAddress(FPGA_IO_Pins_TypeDef pin);
int noDataCmd(int cmd);
void execCurrentPack();
void sendPacket(uint32_t size, uint32_t cmd, uint8_t * data);
void resetAllPins();
void led(int l, int mode);
void programFPGA();


#endif //__MECOBO_H_

