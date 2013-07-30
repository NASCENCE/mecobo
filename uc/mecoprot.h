#ifndef __MECOPROT_H__
#define __MECOPROT_H__

//This defines the fields and values for the mecobo protocol

#define USB_BUFFER_SIZE 1024


#define USB_CMD_CONFIG_PIN    0x1
#define USB_CMD_READ_PIN      0x2
#define USB_CMD_CONFIG_REG    0x3
#define USB_CMD_PROGRAM_FPGA  0x4
#define USB_CMD_START_OUTPUT  0x5 
#define USB_CMD_STREAM_INPUT  0x6
#define USB_CMD_GET_INPUT_BUFFER 0x7

#define PINTYPE_OUT     0x0
#define PINTYPE_IN      0x1

//USB package data offsets for the pinconfig package
#define USB_PACK_SIZE_BYTES   32 //8 * 4
#define PINCONFIG_DATA_FPGA_PIN 0
#define PINCONFIG_DATA_TYPE 1
#define PINCONFIG_DATA_CONST 2
#define PINCONFIG_DATA_DUTY 3
#define PINCONFIG_DATA_ANTIDUTY 4
#define PINCONFIG_DATA_CYCLES 5
#define PINCONFIG_DATA_SAMPLE_RATE 6
#define PINCONFIG_DATA_RUN_INF 7  

//TODO: Fill in the rest.
//
//FPGA port enum
typedef enum {
  FPGA_F16 = 0,
  FPGA_F17 = 1,
  FPGA_G14 = 2,
  FPGA_G16 = 3,
  FPGA_H16 = 4,
  FPGA_H17 = 5,
  FPGA_J16 = 6,
  FPGA_H15 = 7
} FPGA_IO_Pins_TypeDef;


struct mecoPack {
    uint32_t size;
    uint32_t command;
    uint8_t * data;
};

struct sampleValue {
  uint32_t sampleNum; 
  uint32_t pin;
  uint32_t value;
};


#endif //__MECOPROT_H__
