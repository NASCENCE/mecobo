#ifndef __MECOPROT_H__
#define __MECOPROT_H__

//This defines the fields and values for the mecobo protocol

#include <stdint.h>
#define USB_BUFFER_SIZE 32


#define USB_CMD_CONFIG_PIN    0x1
#define USB_CMD_READ_PIN      0x2
#define USB_CMD_CONFIG_REG    0x3
#define USB_CMD_PROGRAM_FPGA  0x4
#define USB_CMD_START_OUTPUT  0x5 
#define USB_CMD_STREAM_INPUT  0x6
#define USB_CMD_GET_INPUT_BUFFER 0x7
#define USB_CMD_STATUS 0x8
#define USB_CMD_GET_INPUT_BUFFER_SIZE 0x9
#define USB_CMD_CONST 0xA
#define USB_CMD_RESET_ALL 0xB
#define USB_CMD_LED 0xC
#define USB_CMD_RUN_SEQ 0xD

#define PINTYPE_OUT     0x0
#define PINTYPE_IN      0x1

//USB package data offsets for the pinconfig package
#define USB_PACK_SIZE_BYTES   40 //10 * 4
#define PINCONFIG_DATA_FPGA_PIN 0
#define PINCONFIG_DATA_TYPE 1
#define PINCONFIG_DATA_CONST 2
#define PINCONFIG_DATA_DUTY 3
#define PINCONFIG_DATA_ANTIDUTY 4
#define PINCONFIG_DATA_CYCLES 5
#define PINCONFIG_DATA_SAMPLE_RATE 6
#define PINCONFIG_DATA_RUN_INF 7  
#define PINCONFIG_START_TIME 8  
#define PINCONFIG_END_TIME 9  
#define LED_MODE 0
#define LED_SELECT 1

#define PINCONFIG_DATA_TYPE_DIRECT_CONST 10
#define PINCONFIG_DATA_TYPE_PREDEFINED_PWM 11
#define PINCONFIG_DATA_TYPE_DAC_CONST 12
#define PINCONFIG_DATA_TYPE_RECORD 13

#define STATUS_BYTES 8
#define STATUS_FPGA_CONFIGURED 0
#define STATUS_USB_BUFFER_ELEMENTS 1


//TODO: Fill in the rest.
//
//FPGA port enum
//This MUST MUST correspond to the generate in toplevel.v; because that gives them their real
//address. I guess we'll stop using these now, honestly...
//so TODO: generate this from that. 
typedef enum {
  FPGA_DIGI_0 = 0,
  FPGA_DIGI_1 = 1,
  FPGA_DIGI_2 = 2,
  FPGA_DIGI_3 = 3,
  FPGA_DIGI_4 = 4,
  FPGA_DIGI_5 = 5,
  FPGA_DIGI_6 = 6,
  FPGA_DIGI_7 = 7,
  FPGA_DIGI_8 = 8,
  FPGA_DIGI_9 = 9,
  FPGA_DIGI_10 = 10,
  FPGA_DIGI_11 = 11,
  FPGA_DIGI_12 = 12,
  FPGA_DIGI_13 = 13,
  FPGA_DAC_0 = 61,
} FPGA_IO_Pins_TypeDef;


struct mecoPack {
    uint32_t size;
    uint32_t command;
    uint8_t * data;
};

struct sampleValue {
  uint16_t sampleNum; 
  uint8_t pin;
  uint8_t value;
};


#endif //__MECOPROT_H__
