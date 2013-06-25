//Common defines for host and uc

#define CMD_CONFIG_PIN  0x1
#define CMD_READ_PIN    0x2
#define CMD_CONFIG_REG  0x3

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
  FPGA_A14 = 5,
  FPGA_E13 = 6,
  FPGA_A13 = 7,
  FPGA_F12 = 8,
  FPGA_B16 = 9,
  FPGA_D12 = 10,
  FPGA_E12 = 11,
  FPGA_D11 = 12,
  FPGA_C15 = 13,
  FPGA_C11 = 14,
  FPGA_A12 = 15,
  FPGA_F11 = 16,
  FPGA_B14 = 17,
  FPGA_E11 = 18,
  FPGA_C12 = 19,
  FPGA_G11 = 20,
  FPGA_F10 = 22,
  FPGA_D9  = 24,
  FPGA_F9  = 26,
  FPGA_C9  = 28,
  FPGA_G9  = 30,
  FPGA_D8  = 32,
  FPGA_C8  = 34,
  FPGA_C6  = 36

} FPGA_IO_Pins_TypeDef;


