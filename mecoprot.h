#ifndef __MECOPROT_H__
#define __MECOPROT_H__

//This defines the fields and values for the mecobo protocol

#include <stdint.h>
#define USB_BUFFER_SIZE 32


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

#define PINCONFIG_DATA_TYPE_DIGITAL_OUT 10
#define PINCONFIG_DATA_TYPE_PREDEFINED_PWM 11
#define PINCONFIG_DATA_TYPE_DAC_CONST 12
#define PINCONFIG_DATA_TYPE_RECORD 13
#define PINCONFIG_DATA_TYPE_RECORD_ANALOGUE 14
#define PINCONFIG_DATA_TYPE_PREDEFINED_SINE 15
#define PINCONFIG_DATA_TYPE_CONSTANT_FROM_REGISTER 16

#define STATUS_BYTES 8
#define STATUS_FPGA_CONFIGURED 0
#define STATUS_USB_BUFFER_ELEMENTS 1


#define AD_CHANNELS_START 100
#define AD_CHANNELS_END 199

#define DA_CHANNELS_START 50
#define DA_CHANNELS_END 99

#define IO_CHANNELS_START 0
#define IO_CHANNELS_END 49
//TODO: Fill in the rest.
//
//FPGA port enum
//This MUST MUST correspond to the generate in toplevel.v; because that gives them their real
//address. 
//This is ... actually offsets. Isn't it. YES INDEED.
typedef enum {
  INVALID = -1,
  FPGA_DIGI_0 = 0,
  FPGA_DIGI_1 = 2,
  FPGA_DIGI_2 = 4,
  FPGA_DIGI_3 = 6,
  FPGA_DIGI_4 = 1,   //note : this is due to the missing fpga pins, reroute on daughterboard.
  FPGA_DIGI_5 = 3,   //same.
  FPGA_DIGI_6 = 12,
  FPGA_DIGI_7 = 14,
  FPGA_DIGI_8 = 5,
  FPGA_DIGI_9 = 7,
  FPGA_DIGI_10 = 9,
  FPGA_DIGI_11 = 13,
  FPGA_DIGI_12 = 24,
  FPGA_DIGI_13 = 26,
  FPGA_DIGI_14 = 15,
  FPGA_DIGI_15 = 30,
  //DAC channels
  FPGA_DAC_0_A = 50,
  FPGA_DAC_0_B = 51,
  FPGA_DAC_0_C = 52,
  FPGA_DAC_0_D = 53,
  FPGA_DAC_0_E = 54,
  FPGA_DAC_0_F = 55,
  FPGA_DAC_0_G = 56,
  FPGA_DAC_0_H = 57,
  
  FPGA_DAC_1_A = 58,
  FPGA_DAC_1_B = 59,
  FPGA_DAC_1_C = 60,
  FPGA_DAC_1_D = 61,
  FPGA_DAC_1_E = 62,
  FPGA_DAC_1_F = 63,
  FPGA_DAC_1_G = 64,
  FPGA_DAC_1_H = 65,

  //ADC channels.
  FPGA_ADC_0_A = 100,
  FPGA_ADC_0_B = 101,
  FPGA_ADC_0_C = 102,
  FPGA_ADC_0_D = 103,
  FPGA_ADC_0_E = 104,
  FPGA_ADC_0_F = 105,
  FPGA_ADC_0_G = 106,
  FPGA_ADC_0_H = 107,
  FPGA_DUMMY_0 = 200  //To remove range compiler warning.
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


#endif //__MECOPROT_H__
