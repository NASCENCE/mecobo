#include "mecobo.h"
#include <stdio.h>


void testRam()
{

  uint16_t * ram = (uint16_t*)SRAM1_START;

  ram[100] = 0xff42;
  ram[101] = 0xff43;
  ram[102] = 0xff44;
  ram[103] = 0xff45;
  ram[104] = 0xff46;
  ram[105] = 0x5555;
  ram[106] = 0xAAAA;

  infop("ram: %x\n", ram[100]);
  infop("ram: %x\n", ram[101]);
  infop("ram: %x\n", ram[102]);
  infop("ram: %x\n", ram[103]);
  infop("ram: %x\n", ram[104]);
  infop("ram: %x\n", ram[105]);
  infop("ram: %x\n", ram[106]);

  /*
  for(uint16_t i = 0; i < 40000; i++) {
    ram[i] = i;
  }


  for(uint16_t i = 0; i < 40000; i++) {
    if (ram[i] != i) {
      infop("FAIL 1 at %p got value %x wanted %x\n", &ram[i], ram[i], i);
    }
  }

  */
  ram = (uint16_t*)SRAM2_START;
  infop("SRAM 2 TEST\n");

  ram[100] = 0xff42;
  ram[101] = 0xff43;
  ram[102] = 0xff44;
  ram[103] = 0xff45;
  ram[104] = 0xff46;
  ram[105] = 0x5555;
  ram[106] = 0xAAAA;

  infop("ram: %x\n", ram[100]);
  infop("ram: %x\n", ram[101]);
  infop("ram: %x\n", ram[102]);
  infop("ram: %x\n", ram[103]);
  infop("ram: %x\n", ram[104]);
  infop("ram: %x\n", ram[105]);
  infop("ram: %x\n", ram[106]);
    
  /*
  for(uint16_t i = 0; i < 40000; i++) {
    ram[i] = i;
  }


  for(uint16_t i = 0; i < 40000; i++) {
    if (ram[i] != i) {
      infop("FAIL 1 at %p got value %x wanted %x\n", &ram[i], ram[i], i);
    }
  }
    */
}
