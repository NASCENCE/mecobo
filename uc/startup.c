#include "mecobo.h"

void testRam()
{
  printf("SRAM 1 TEST\n");
    uint8_t * ram = (uint8_t*)sampleBuffer;
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < SRAM1_BYTES; j++) {
        ram[i*(16*1024)+j] = j%255;
      }
      for(int j = 0; j < 16*1024; j++) {
        uint8_t rb = ram[i*(16*1024) + j];
        if(rb != j%255) {
          printf("FAIL at %u wanted %u got %u\n", i*(16 * 1024) + j, j%255, rb);
        }
      }
      //Null out before use.
      ram[i] = 0;
    }
    printf("Complete.\n");


    printf("SRAM 1 TEST SAME PATTERN\n");
    uint8_t * pat = (uint8_t*)SRAM1_START;
    for(int j = 0; j < SRAM1_BYTES; j++) {
      pat[j] = 0xAA;
      if(pat[j] != 0xAA) {
        printf("Failed RAM test!\n");
      }
    }
    printf("Complete.\n");

    printf("SRAM 2 TEST\n");
    ram = (uint8_t*)SRAM2_START;
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < SRAM2_BYTES; j++) {
        ram[i*(16*1024)+j] = j%255;
      }
      for(int j = 0; j < 16*1024; j++) {
        uint8_t rb = ram[i*(16*1024) + j];
        if(rb != j%255) {
          printf("FAIL at %u wanted %u got %u\n", i*(16 * 1024) + j, j%255, rb);
        }
      }
    }
  }
  printf("Complete.\n");
}
