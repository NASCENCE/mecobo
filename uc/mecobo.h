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
#include "pinItem.h"


#define INFO_PRINT 1
#define CMD_TRACE 0
#define DEBUG_PRINT 0

#define trace(...) do { if (CMD_TRACE)   fprintf(stdout, ##__VA_ARGS__); } while (0)
#define infop(...) do { if (INFO_PRINT)  fprintf(stdout, ##__VA_ARGS__); } while (0)
#define debug(...) do { if (DEBUG_PRINT) fprintf(stdout, ##__VA_ARGS__); } while (0)


#define min(a,b) ({ a < b ? a : b; })



#define EBI_ADDR_BASE 0x80000000
#define SRAM1_START   0x84000000
#define SRAM1_BYTES 256*1024  //16Mbit = 256KB

#define SRAM2_START 0x88000000
#define SRAM2_BYTES 256*1024 

#define NOR_START 0x8C000000
#define NOR_BYTES 256*1024 

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
//#define EP_DATA_OUT1       0x01  /* Endpoint for USB data reception.       */
//#define EP_DATA_OUT2       0x02  /* Endpoint for USB data reception.       */
//#define EP_DATA_IN1        0x81  /* Endpoint for USB data transmission.    */
//#define EP_DATA_IN2        0x82  /* Endpoint for USB data transmission.    */
//#define EP_NOTIFY         0x82  /* The notification endpoint (not used).  */
//#define BULK_EP_SIZE   USB_FS_BULK_EP_MAXSIZE /* This is the max. ep size.    */



//Address offsets for the config of a pin controller
#define PINCONFIG_GLOBAL_CMD 0
#define PINCONFIG_NCO_COUNTER_LOW 2
#define PINCONFIG_NCO_COUNTER_HIGH 3
#define PINCONFIG_CYCLES 3
#define PINCONFIG_RUN_INF 4
#define PINCONFIG_SAMPLE_RATE 6
#define PINCONFIG_SAMPLE_REG 7
#define PINCONFIG_SAMPLE_CNT 8
#define PINCONFIG_STATUS_REG 9


#define PINCONTROL_REG_SAMPLE_RATE 4
#define PINCONTROL_REG_NCO_COUNTER 1
#define PINCONTROL_REG_END_TIME 2
#define PINCONTROL_REG_LOCAL_CMD 3
#define PINCONTROL_REG_REC_START_TIME 6

//Possible FPGA commands
#define CMD_START_OUTPUT  1
#define CMD_CONST_LOW  2
#define CMD_CONST_HIGH   4
#define CMD_RESET  5
//#define CMD_CONST  6

#define PINCONTROL_CMD_START_OUTPUT  3
#define PINCONTROL_CMD_CONST  2
#define PINCONTROL_CMD_CONST_NULL  6
#define PINCONTROL_CMD_INPUT_STREAM  4
#define PINCONTROL_CMD_RESET 5

#define SAMPLE_COLLECTOR_ADDR 242
#define SAMPLE_COLLECTOR_REG_NEW_UNIT 4
#define SAMPLE_COLLECTOR_REG_LOCAL_CMD 5
#define SAMPLE_COLLECTOR_REG_NUM_UNITS 6

#define SAMPLE_COLLECTOR_CMD_START_SAMPLING 1
#define SAMPLE_COLLECTOR_CMD_STOP_SAMPLING 2
#define SAMPLE_COLLECTOR_CMD_RES_SAMPLE_FIFO 3
#define SAMPLE_COLLECTOR_CMD_RESET 5

#define XBAR_CONTROLLER_ADDR 240
#define XBAR_REG_LOCAL_CMD 0x20

#define DAC_CONTROLLER_ADDR 128
#define DAC_REG_LOAD_VALUE 0x0

#define AD_REG_PROGRAM 0x4
#define AD_REG_OVERFLOW 0xD
#define AD_REG_SAMPLE 0x7
#define AD_REG_DIVIDE 0x2
#define AD_REG_ENDTIME 0x3
#define AD_REG_REC_START_TIME 0xC

#define EBI_SAMPLE_FIFO_DATA_COUNT 11
#define EBI_READBACK_ADDR 12
#define EBI_CMD_FIFO_DATA_COUNT 13

//FPGA status register
#define STATUS_REG_CMD_FIFO_ALMOST_FULL_BIT 0x8000
#define STATUS_REG_CMD_FIFO_FULL_BIT 		    0x4000
#define STATUS_REG_CMD_FIFO_ALMOST_EMPTY	  0x2000
#define STATUS_REG_CMD_FIFO_EMPTY           0x1000
#define STATUS_REG_SAMPLE_FIFO_ALMOST_FULL  0x0800
#define STATUS_REG_SAMPLE_FIFO_FULL         0x0400
#define STATUS_REG_SAMPLE_FIFO_EMPTY        0x0200
#define STATUS_REG_SAMPLE_FIFO_ALMOST_EMPTY 0x0100
#define STATUS_REG_XBAR_BUSY 0X0080


struct pinConfig {
  uint32_t fpgaPin;
  uint32_t pinType;
  uint32_t constantVal;
  uint32_t waveform;
  uint32_t freq;
  uint32_t phase;
  uint32_t antiduty;
  uint32_t duty;
  uint16_t noc;
  uint32_t cycles;
  uint32_t runInf;
  uint32_t sampleRate;

  //If pin is to be a DAC pin
  uint32_t nSamples;
  uint32_t * samples; //points to allocated data of samples. (12 bit samples)
};



//ordered like this to avoid alignment issues
struct fifoCmd {
  uint32_t startTime;
  uint16_t data[2];
  uint8_t addr;
  uint8_t controller;
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
void startInput();
void setupInput(FPGA_IO_Pins_TypeDef channel, int sampleRate, uint32_t startTime, uint32_t endtime);
void getInput(FPGA_IO_Pins_TypeDef channel);
int fpgaConfigPin(struct pinConfig * p);
uint16_t * getChannelAddress(FPGA_IO_Pins_TypeDef pin);
int noDataCmd(int cmd);
void execCurrentPack();
void sendPacket(uint32_t size, uint32_t cmd, uint8_t * data);
void resetAllPins();
void led(int l, int mode);
void programFPGA();

void testRam();
void testNOR();
void NORBusy();
void programFPGA();
void eraseNorChip();
void autoSelectNor();
void enterEnhancedMode();
void exitEnhancedMode();
void write256Buffer(uint16_t * data, uint32_t offset);
int NORToggling();

//fifo related stuff
void putInFifo(struct fifoCmd * cmd);
void pushToCmdFifo(struct pinItem * item);
//void parseNORFileTable(int * numEntries, struct NORFileTableEntry ** entries);
struct fifoCmd makeCommand(uint32_t startTime, uint8_t controller, uint8_t reg, uint32_t data);
void command(uint32_t startTime, uint8_t controller, uint8_t reg, uint32_t data);
void command2x16(uint32_t startTime, uint8_t controller, uint8_t reg, uint16_t data1, uint16_t data2);
void resetXbar();
void runTime();
void resetTime();
void checkStatusReg();
int sampleFifoDataCount();
uint16_t cmdFifoDataCount();

void ebiw(uint32_t addr, uint16_t data);
uint16_t ebir(uint32_t addr);

#endif //__MECOBO_H_

