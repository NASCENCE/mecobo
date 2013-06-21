//Common defines for host and uc

#define CMD_CONFIG_PIN  0x1
#define CMD_READ_PIN    0x2

#define PINTYPE_OUT     0x0
#define PINTYPE_IN      0x1

#define PINCONFIG_DATA_FPGA_PIN 0
#define PINCONFIG_DATA_TYPE 1
#define PINCONFIG_DATA_CONST 2
//TODO: Fill in the rest.
//
//FPGA port enum
typedef enum {
  FPGA_D14 = 0,
  FPGA_A16 = 1,
  FPGA_C14 = 2,
  FPGA_A15 = 3,
  FPGA_F13 = 4,
  FPGA_A13 = 5,
  FPGA_F12 = 6,
  FPGA_B16 = 7,
  FPGA_D12 = 8,
  FPGA_E12 = 9,
  FPGA_D11 = 10,
  FPGA_C15 = 11,
  FPGA_C11 = 12,
  FPGA_A12 = 13
} FPGA_IO_Pins_TypeDef;


