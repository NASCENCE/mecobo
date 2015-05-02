#include "mecobo.h"

void testNOR()
{
  printf("NOR test\n");
  uint8_t * ram = (uint8_t*)NOR_START;
  int err = 0;
  for(int i = 0; i < 4; i++) {
    if (err) {
      break;
    }
    for(int j = 0; j < SRAM1_BYTES; j++) {
      ram[i*(16*1024)+j] = j%255;
    }
    for(int j = 0; j < 16*1024; j++) {
      uint8_t rb = ram[i*(16*1024) + j];
      if(rb != j%255) {
        printf("FAIL at %u wanted %u got %u\n", i*(16 * 1024) + j, j%255, rb);
        err = 1;
        break;
      }
    }
    //Null out before use.
    ram[i] = 0;
  }
  if (!err) {
    printf("OK\n");
  } else {
    printf("FAIL\n");
  }
  err = 0;
}

void testRam()
{
  int err = 0;
  uint8_t * ram = (uint8_t*)SRAM1_START;
  /*
  printf("SRAM 1 TEST\n");
  for(int i = 0; i < 4; i++) {
    if (err) {
      break;
    }
    for(int j = 0; j < SRAM1_BYTES; j++) {
      ram[i*(16*1024)+j] = j%255;
    }
    for(int j = 0; j < 16*1024; j++) {
      uint8_t rb = ram[i*(16*1024) + j];
      if(rb != j%255) {
        printf("FAIL at %u wanted %u got %u\n", i*(16 * 1024) + j, j%255, rb);
        err = 1;
        break;
      }
    }
    //Null out before use.
    ram[i] = 0;
  }
  printf("Complete.\n");
  err = 0;

   
  printf("SRAM 1 TEST SAME PATTERN\n");
  uint8_t * pat = (uint8_t*)SRAM1_START;
  for(int j = 0; j < SRAM1_BYTES; j++) {
    if (err) {
      err = 0;
      break;
    }

    pat[j] = 0xAA;
    if(pat[j] != 0xAA) {
      printf("Failed RAM test!\n");
      err = 1;
      break;
    }
  }
  printf("Complete.\n");
  */

  printf("SRAM 2 TEST\n");
  err = 0;
  ram = (uint8_t*)SRAM2_START;
  for(int i = 0; i < 4; i++) {
    if (err) {
      break;
    }

    for(int j = 0; j < SRAM2_BYTES/2; j++) {
      ram[i*(16*1024)+j] = j%255;
    }
    for(int j = 0; j < 16*1024; j++) {
      uint8_t rb = ram[i*(16*1024) + j];
      if(rb != j%255) {
        printf("FAIL at %u wanted %u got %u\n", i*(16 * 1024) + j, j%255, rb);
        err = 1;
        break;
      }
    }
  }
  if (!err) {
    printf("OK\n");
  } else {
    printf("FAIL\n");
  }
}


