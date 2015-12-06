#ifndef __MECOPROT_H__
#define __MECOPROT_H__

//This defines the fields and values for the mecobo protocol

#include <stdint.h>

#define MECOBO_RESET_WORD_DB_PRESENT_BIT 0
#define MECOBO_RESET_WORD_RESET_XBAR_BIT 1

#define USB_BUFFER_SIZE 32

#define MECOBO_STATUS_READY 0
#define MECOBO_STATUS_BUSY 1
#define MECOBO_STATUS_RUNNING 2
#define MECOBO_STATUS_SETUP_LOADED 3
#define MECOBO_STATUS_FIFO_PRELOADED 4
#define MECOBO_STATUS_XBAR_CONFIGURED 5
#define MECOBO_STATUS_RESET_COMPLETE 5

#define USB_CMD_CONFIG_PIN    0x1
#define USB_CMD_CONFIG_REG    0x3
#define USB_CMD_PROGRAM_FPGA  0x4
#define USB_CMD_START_OUTPUT  0x5 
#define USB_CMD_GET_INPUT_BUFFER 0x7
#define USB_CMD_STATUS 0x8
#define USB_CMD_GET_INPUT_BUFFER_SIZE 0x9
#define USB_CMD_CONST 0xA
#define USB_CMD_RESET_ALL 0xB
#define USB_CMD_LED 0xC
#define USB_CMD_RUN_SEQ 0xD
#define USB_CMD_PROGRAM_XBAR 0xE
#define USB_CMD_UPDATE_REGISTER 0xF
#define USB_CMD_LOAD_BITFILE 0x10
#define USB_CMD_SETUP_RECORDING 0x11
#define USB_CMD_LOAD_SETUP 0x12
#define USB_CMD_PRELOAD_FIFO 0x13

#define PINTYPE_OUT     0x0
#define PINTYPE_IN      0x1

//USB package data offsets for the pinconfig package
#define USB_PACK_SIZE_BYTES 44 //11 * 4
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
#define PINCONFIG_DATA_NOC_COUNTER 10

#define LED_MODE 0
#define LED_SELECT 1

#define PINCONFIG_DATA_TYPE_DIGITAL_OUT 10
#define PINCONFIG_DATA_TYPE_PREDEFINED_PWM 11
#define PINCONFIG_DATA_TYPE_DAC_CONST 12
#define PINCONFIG_DATA_TYPE_RECORD 13
#define PINCONFIG_DATA_TYPE_RECORD_ANALOGUE 14
#define PINCONFIG_DATA_TYPE_PREDEFINED_SINE 15
#define PINCONFIG_DATA_TYPE_CONSTANT_FROM_REGISTER 16
#define PINCONFIG_DATA_TYPE_DIGITAL_CONST 17

#define STATUS_BYTES 8
#define STATUS_FPGA_CONFIGURED 0
#define STATUS_USB_BUFFER_ELEMENTS 1

#define NOR_FLASH_NODB_POS 0
#define NOR_FLASH_DB_POS 1024*1024

#define AD_CHANNELS_START 64
#define AD_CHANNELS_END 127

#define DA_CHANNELS_START 128
#define DA_CHANNELS_END 239

#define IO_CHANNELS_START 0
#define IO_CHANNELS_END 49
//TODO: Fill in the rest.
//
//FPGA port enum
//These are the offsets for the pin controllers in the FPGA. 
//address. 


typedef enum {
  INVALID = -1,
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
  FPGA_DIGI_14 = 14,
  FPGA_DIGI_15 = 15,
  //DAC channels
  FPGA_DAC_0_A = 128,
  FPGA_DAC_0_B = 129,
  FPGA_DAC_0_C = 130,
  FPGA_DAC_0_D = 131,
  FPGA_DAC_0_E = 132,
  FPGA_DAC_0_F = 133,
  FPGA_DAC_0_G = 134,
  FPGA_DAC_0_H = 135,

  //ADC channels.
  FPGA_ADC_0_A = 64,
  FPGA_ADC_0_B = 65,
  FPGA_ADC_0_C = 66,
  FPGA_ADC_0_D = 67,
  FPGA_ADC_0_E = 68,
  FPGA_ADC_0_F = 69,
  FPGA_ADC_0_G = 70,
  FPGA_ADC_0_H = 71,
  FPGA_DUMMY_0 = 72  //To remove range compiler warning.
  //Etc...
} FPGA_IO_Pins_TypeDef;


struct mecoPack {
    uint32_t size;
    uint32_t command;
    uint8_t * data;
};

struct sampleValue {
  uint16_t sampleNum; 
  uint8_t channel;
  int16_t value;
};

struct mecoboStatus {
  uint8_t state;
  uint8_t foo;
  uint16_t samplesInBuffer;
  uint16_t roomInCmdFifo;
};

struct NORFileTableEntry {
  char name[8]; //must be null-terminated
  uint32_t size; //in bytes
  uint32_t offset; //in bytes
};


#endif //__MECOPROT_H__
