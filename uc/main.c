//LEDS on front case (left to right):
//GPIO_PinModeSet(gpioPortD, 4, gpioModePushPull, 1);  //Led 1
//GPIO_PinModeSet(gpioPortB, 11, gpioModePushPull, 1);  //Led 2
//GPIO_PinModeSet(gpioPortD, 8, gpioModePushPull, 1);  //Led 3
//GPIO_PinModeSet(gpioPortB, 8, gpioModePushPull, 1);  //Led 4

//LEDS on board "north to south"
//A10 : U0
//A9: U1
//B12: U2
//D0: U3
//GPIO_PinModeSet(gpioPortA, 10, gpioModePushPull, 1);  //Led U0
//GPIO_PinModeSet(gpioPortA, 9, gpioModePushPull, 1);  //Led U1
//GPIO_PinModeSet(gpioPortB, 12, gpioModePushPull, 1);  //Led U2


#include "em_device.h"
#include "em_cmu.h"
#include "em_dma.h"
#include "em_gpio.h"
#include "em_int.h"
#include "dmactrl.h"
#include "em_usb.h"
#include "em_usart.h"
#include "em_ebi.h"
#include "em_timer.h"
#include "bsp.h"
#include "bsp_trace.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "../mecoprot.h"
#include "mecobo.h"
#include "pinItem.h"

#include "descriptors.h"
#include "struct_init.h"

#include "dac.h"
#include "adc.h"

char * BUILD_VERSION = __GIT_COMMIT__;

//override newlib function.
int _write_r(void *reent, int fd, char *ptr, size_t len)
{
  (void) reent;
  (void) fd;
  for(size_t i = 0; i < len; i++) 
    ITM_SendChar(ptr[i]);

  return len;
}

void setupSWOForPrint(void)
{
  /* Enable GPIO clock. */
  //CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

  /* Enable Serial wire output pin */
  GPIO->ROUTE |= GPIO_ROUTE_SWOPEN;

#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_LEOPARD_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  /* Set location 0 */
  GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC0;

  /* Enable output on pin - GPIO Port F, Pin 2 */
  GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK);
  GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL;
#else
  /* Set location 1 */
  GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) |GPIO_ROUTE_SWLOCATION_LOC1;
  /* Enable output on pin */
  GPIO->P[2].MODEH &= ~(_GPIO_P_MODEH_MODE15_MASK);
  GPIO->P[2].MODEH |= GPIO_P_MODEH_MODE15_PUSHPULL;
#endif

  /* Enable debug clock AUXHFRCO */
  CMU->OSCENCMD = CMU_OSCENCMD_AUXHFRCOEN;

  /* Wait until clock is ready */
  while (!(CMU->STATUS & CMU_STATUS_AUXHFRCORDY));

  /* Enable trace in core debug */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  ITM->LAR  = 0xC5ACCE55;
  ITM->TER  = 0x0;
  ITM->TCR  = 0x0;
  TPI->SPPR = 2;
  TPI->ACPR = 0xf;
  ITM->TPR  = 0x0;
  DWT->CTRL = 0x400003FE;
  ITM->TCR  = 0x0001000D;
  TPI->FFCR = 0x00000100;
  ITM->TER  = 0x1;
}



//MAP DEFINING WHERE THE CHANNELS ARE LOCATED IN MEMORY
//static uint32_t channelMap[16] = {};


/*** Typedef's and defines. ***/
static int timeMs = 0;
static int timeTick = 0; //10,000 per second.
static int lastTimeTick = 0;

//USB Variables
int packNum = 0;

static uint32_t inBufferTop;
static uint8_t * inBuffer;

static struct mecoPack currentPack;
static struct mecoPack packToSend;
static int sendPackReady = 0;
static int sendInProgress = 0;

//Keep track of which pins are input pins. 
static int nPins = 250;


//Are we programming the FPGA
int fpgaUnderConfiguration = 0;
int fpgaConfigured = 0;

//Data stuff for items
static struct pinItem * itemsToApply;
int itaPos  = 0;
int numItems = 0;
static struct pinItem ** itemsInFlight;
int iifPos  = 0;
int numItemsInFlight = 0;

static int * lastCollected;
static int runItems = 0;


#define SRAM1_START 0x84000000
#define SRAM1_BYTES 256*1024  //16Mbit = 256KB

#define SRAM2_START 0x88000000
#define SRAM2_BYTES 256*1024 

uint16_t * xbar = ((uint16_t*)EBI_ADDR_BASE) + (200 * 0x100);
int32_t DACreg[8];
int registersUpdated = 0;

//Circular buffer in SRAM1, modulo MAX_SAMPLES
static const int MAX_SAMPLES = 43689; //SRAM1_BYTES/sizeof(struct sampleValue);
#define MAX_INPUT_CHANNELS 10
#define MAX_CHANNELS 150
static const int BUFFERSIZE = 1000;  //We will have 1000 samples per channel.
//Such a waste of space, should use hash map.

struct sampleValue * sampleBuffer = (struct sampleValue*)SRAM1_START;
int numSamples = 0;
int sampleBufferStart = 0;
int sampleBufferEnd = 0;

int inputChannels[MAX_INPUT_CHANNELS];
int numInputChannels = 0;
int nextKillTime = 0;


uint8_t sinus[256];

//ADC sequencer registers
uint16_t adcSequence[4] = {0xE000,0xE000,0xE000,0xE000};


static int blinky = 0;
void TIMER1_IRQHandler(void)
{ 
  /* Clear flag for TIMER2 overflow interrupt */
  TIMER_IntClear(TIMER1, TIMER_IF_OF);

  /* Toggle LED ON/OFF */
  if (blinky<5) {
    led(BOARD_LED_U0, 0);
  }  
  if (blinky == 10) {
    led(BOARD_LED_U0, 1);
    blinky = 0;
  }
  blinky++;
  
    //Retrieve sample value from pin controllers and queue them for sending. 
}

void TIMER2_IRQHandler(void)
{ 
  TIMER_IntClear(TIMER2, TIMER_IF_OF);
  timeTick++;
}

//"Update DAC tic" == fast clock that ticks. The frequency field of the 
//in-flight items decide if they are to be updated? 

int main(void)
{

  //A10 
  eADesigner_Init();

  //release fpga reset (active high)
  //GPIO_PinOutClear(gpioPortB, 3);

  /*
   * When using a debugger it is practical to uncomment the following three
   * lines to force host to re-enumerate the device.
   */
  setupSWOForPrint();
  printf("Printing online.\n");
  
//Cycle leds.
  for(int i = 0; i < 8; i++) {
    led(i, 1);
    USBTIMER_DelayMs(60);
    led(i, 0);
  }

    printf("USB CONNECTED\n");

  printf("Starting timer clocks\n");
  //Turn on timers
  CMU_ClockEnable(cmuClock_TIMER1, true);
  CMU_ClockEnable(cmuClock_TIMER2, true);


  /* Enable overflow interrupt */
  TIMER_IntEnable(TIMER1, TIMER_IF_OF);
  TIMER_IntEnable(TIMER2, TIMER_IF_OF);

  /* Enable interrupt vector in NVIC */
  NVIC_EnableIRQ(TIMER1_IRQn);
  NVIC_EnableIRQ(TIMER2_IRQn);

  /* Set TIMER Top values */
  TIMER_TopSet(TIMER1, 65535);
  TIMER_TopSet(TIMER2, 75);

  printf("Initalizing timers\n");
  TIMER_Init(TIMER1, &timerInit);
  TIMER_Init(TIMER2, &timerInit);

  printf("Initializing EBI\n");
  EBI_Init(&ebiConfig);
  EBI_Init(&ebiConfigSRAM1);
  EBI_Init(&ebiConfigSRAM2);
  
  //Generate sine table for a half period (0 to 1)
  printf("Generating sine table\n");
  float incr = 6.2830/(float)256.0;
  float j = 0.0;
  for(int i = 0; i < 256; i++, j += incr) {
    sinus[i] = (uint8_t)((sin(j)*(float)128.0)+(float)128.0);
    printf("%u\n", sinus[i]);
  }




  printf("Address of samplebuffer: %p\n", sampleBuffer);
  printf("Have room for %d samples\n", MAX_SAMPLES);
 
  int skip_boot_tests= 0;
  int fpga_alive = 1;
  /*
  for(int j = 0; j < 100000; j++) {
    printf("%x: %x\n", ((uint8_t*)(EBI_ADDR_BASE + j)), *((uint8_t*)(EBI_ADDR_BASE + j)));
  }
  */
  if(!skip_boot_tests) {
  int i = 0;
  printf("Check if FPGA is alive.\n");
  uint16_t * a = getPinAddress(2) + PINCONFIG_STATUS_REG;
  uint16_t foo = *a;
  if (foo != 2) {
    fpga_alive = 0;
    printf("Got unexpected %x from FPGA at %x, addr %p\n", foo, i, a);
  } else {
    fpga_alive = 1;
    printf("FPGA responding as expected\n");
  }

  if(fpga_alive) {
    printf("Setting up DAC and ADCs\n");
    //led(BOARD_LED_U2, 1);
    setupDAC();
    setupADC();
  }


    //Check DAC controllers.
    uint16_t * dac = (uint16_t*)(EBI_ADDR_BASE) + (0x100*DAC0_POSITION);
    printf("DAC: %x\n", dac[PINCONFIG_STATUS_REG]);
    uint16_t * adc = (uint16_t*)(EBI_ADDR_BASE) + (0x100*ADC0_POSITION);
    printf("ADC: %x\n", adc[PINCONFIG_STATUS_REG]);
    printf("XBAR: %x\n", xbar[PINCONFIG_STATUS_REG]);

    printf("FPGA check complete\n");
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

  inBuffer = (uint8_t*)malloc(128*8);
  inBufferTop = 0;

  lastCollected = malloc(sizeof(int)*nPins);
  for(int i = 0; i < nPins; i++) {
    lastCollected[i] = -1;
  }
    //Put FPGA out of reset
  GPIO_PinModeSet(gpioPortB, 5, gpioModePushPull, 1);  
  GPIO_PinOutSet(gpioPortB, 5); //Reset
  GPIO_PinOutClear(gpioPortB, 5); //Reset clear


  sendPackReady = 0;

  //Check if DONE pin is high, which means that the FPGA is configured.
  if(!GPIO_PinInGet(gpioPortD, 15) && GPIO_PinInGet(gpioPortD, 12)) {
    fpgaConfigured = 1;
    fpgaUnderConfiguration = 0;
    GPIO_PinOutSet(gpioPortD, 0); //turn on led (we're configured)
  }


  //itemsToApply will be ordered by start time, hurrah!
  itemsToApply = malloc(sizeof(struct pinItem) * 50);
  itemsInFlight = malloc(sizeof(struct pinItem *) * 100);
  printf("Malloced memory: %p, size %u\n", itemsToApply, sizeof(struct pinItem)*50);
  printf("Malloced memory: %p, size %u\n", itemsInFlight, sizeof(struct pinItem)*100);

  //Default all regs to output 0V
  for(int i = 0; i < 8; i++) {
    DACreg[i] = 128;
  }

  //Note: Do this late, things have to have had a chance to settle.
  printf("Initializing USB\n");
  USBD_Init(&initstruct);
  printf("USB Initialized.\n");
  USBD_Disconnect();
  USBTIMER_DelayMs(100);
  USBD_Connect();
  led(BOARD_LED_U3, 1);


  printf("It's just turtles all the way down.\n");
  printf("I'm the mecobo firmware running on the evolutionary motherboard 3.5new.\n");
  printf("I was built %s, git commit %s\n", __DATE__, BUILD_VERSION);
  printf("Entering main loop.\n");
  for (;;) {
    //check if DONE has gone high, then we are ... uh, done
    if(fpgaUnderConfiguration) {
      if(GPIO_PinInGet(gpioPortD, 12)) {
        printf("FPGA configured.\n");
        fpgaUnderConfiguration = 0;
        //Release FPGA reset signal from uC 
        //Send a few 1's to make sure the device boots.
        GPIO_PinOutSet(gpioPortA, 7); 
        for(int j = 0; j < 64; j++) {
          GPIO_PinOutClear(gpioPortA, 8); //clk low
          GPIO_PinOutSet(gpioPortA, 8); //clk high
        }
        led(BOARD_LED_U2, 1);
      }
    }

    //We can start the EBI interface if the FPGA is configured.
    //Whatever time is left should be spent collecting data from input pins.

    //For all the input pins, collect samples into the big buff!
    //TODO: Make pinControllers notify uC when it has new data, interrupt?
    if(runItems) {
      for(int ch = 0; ch < numInputChannels; ch++) {
        //struct sampleValue val;
        getInput((FPGA_IO_Pins_TypeDef)inputChannels[ch]);
      }


      if(numItems > 0) {
        struct pinItem * currentItem = &(itemsToApply[itaPos]);
        if (currentItem->startTime <= timeMs) {
          execute(currentItem);
          itemsInFlight[iifPos++] = currentItem;

          numItems--;
          numItemsInFlight++;
          itaPos++;
        }

        if (currentItem->endTime < nextKillTime) {
          nextKillTime = currentItem->endTime;
        }
      }


      //Certain items in flight needs updating: specially REGISTERS
      if (lastTimeTick != timeTick) {
        for(int flight = 0; flight < numItemsInFlight; flight++) {
          if(itemsInFlight[flight]->type == PINCONFIG_DATA_TYPE_PREDEFINED_SINE) {
            execute(itemsInFlight[flight]);
          }
          if(registersUpdated && (itemsInFlight[flight]->type == PINCONFIG_DATA_TYPE_CONSTANT_FROM_REGISTER)) {
            execute(itemsInFlight[flight]);
            registersUpdated--;
          }
        }
        //update counters.
        lastTimeTick = timeTick;
        timeMs = timeTick/10;
      }

      //Kill items that are in flight and whose time has come.
      //We cannot guarantee the same as we do for starting so we need to check.
      if(nextKillTime <= timeMs) {
        if (numItemsInFlight > 0)  {
          //Iterate array of items in flight, kill whatever needs killing,
          //mark an empty entry as NULL.
          for(int i = 0; i < iifPos; i++) {
            struct pinItem * it = itemsInFlight[i];
            if(it != NULL) {
              //-1 is special case: run until reset.
              if((it->endTime != -1) && it->endTime <= timeMs) {
                killItem(it);
                itemsInFlight[i] = NULL;
                numItemsInFlight--;
              }
            }
          }
        }
      }

    }
  } //main for loop ends
}



int UsbHeaderReceived(USB_Status_TypeDef status,
    uint32_t xf,
    uint32_t remaining) 
{
  (void) remaining;
  int gotHeader = 0;
  if ((status == USB_STATUS_OK) && (xf > 0)) {
    //Got some data, check if we have header.
    if (xf == 8) {
      uint32_t * inBuffer32 = (uint32_t *)inBuffer;
      currentPack.size = inBuffer32[0]; 
      currentPack.command = inBuffer32[1];
      currentPack.data = NULL;
      gotHeader = 1;
    } else {
      printf("Expected header of size 8, got %u.\n", (unsigned int)xf);
    }
  }

  //Check that we're still good, and get some data for this pack.
  if (USBD_GetUsbState() == USBD_STATE_CONFIGURED) {
    if(gotHeader) {
      if(currentPack.size > 0) {
        //Go to data collection.
        currentPack.data = (uint8_t*)malloc(currentPack.size);
        if(currentPack.data == NULL) {
          printf("cP.data mal failed\n");
        } 

        USBD_Read(EP_DATA_OUT1, currentPack.data, currentPack.size, UsbDataReceived);
      } else {
        //Only header command, so we can parse it, and will not wait for data.
        execCurrentPack();
        //And wait for new header.
        if (USBD_GetUsbState() == USBD_STATE_CONFIGURED) {
          USBD_Read(EP_DATA_OUT1, inBuffer, 8, UsbHeaderReceived); //get new header.
        }
      }
    } else {
      USBD_Read(EP_DATA_OUT1, inBuffer, 8, UsbHeaderReceived);
    }
  }

  return USB_STATUS_OK;
}

int UsbDataReceived(USB_Status_TypeDef status,
    uint32_t xf,
    uint32_t remaining) 
{
  (void) remaining;
  if ((status == USB_STATUS_OK) && (xf > 0)) {
    execCurrentPack();
  }

  //Check that we're still good, and wait for a new header.
  if (USBD_GetUsbState() == USBD_STATE_CONFIGURED) {
    USBD_Read(EP_DATA_OUT1, inBuffer, 8, UsbHeaderReceived); //get new header.
  }
  return USB_STATUS_OK;
}

int UsbDataSent(USB_Status_TypeDef status,
    uint32_t xf,
    uint32_t remaining)
{
  (void) remaining;

  printf("USB DATA SENT HURRA\n");
  sendInProgress = 0;
  if ((status == USB_STATUS_OK) && (xf > 0)) 
  {
    //we probably sent some data :-)
    //don't free the sample buffer
    if((packToSend.command != USB_CMD_GET_INPUT_BUFFER) &&
        (packToSend.command != USB_CMD_GET_INPUT_BUFFER_SIZE)) 
    {
      free(packToSend.data);
    }

    //Reset sample counter now.
    if(packToSend.command == USB_CMD_GET_INPUT_BUFFER){
      numSamples = 0;
    }
  }

  return USB_STATUS_OK;
}

void DmaUsbTxDone(unsigned int channel, int primary, void *user)
{
  (void) channel;
  (void) primary;
  (void) user;

  INT_Disable();
  INT_Enable();
}

void UsbStateChange(USBD_State_TypeDef oldState, USBD_State_TypeDef newState)
{
  (void) oldState;
  if (newState == USBD_STATE_CONFIGURED) {
    USBD_Read(EP_DATA_OUT1, inBuffer, 8, UsbHeaderReceived);
  }

}

void eADesigner_Init(void)
{
  /* HFXO setup */
  /* Note: This configuration is potentially unsafe. */
  /* Examine your crystal settings. */

  /* Enable HFXO as high frequency clock, HFCLK (depending on external oscillator this will probably be 32MHz) */
  CMU->OSCENCMD = CMU_OSCENCMD_HFXOEN;
  while (!(CMU->STATUS & CMU_STATUS_HFXORDY)) ;
  CMU->CMD = CMU_CMD_HFCLKSEL_HFXO;

  /* No LE clock source selected */

  /* Enable GPIO clock */
  CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

  
   /* Pin PB11 is configured to Push-pull */
    GPIO->P[1].MODEH = (GPIO->P[1].MODEH & ~_GPIO_P_MODEH_MODE11_MASK) | GPIO_P_MODEH_MODE11_PUSHPULL;

  /* Pin PB8 is configured to Push-pull */
  GPIO->P[1].MODEH = (GPIO->P[1].MODEH & ~_GPIO_P_MODEH_MODE8_MASK) | GPIO_P_MODEH_MODE8_PUSHPULL;
    /* Pin PD8 is configured to Push-pull */
    GPIO->P[3].MODEH = (GPIO->P[3].MODEH & ~_GPIO_P_MODEH_MODE8_MASK) | GPIO_P_MODEH_MODE8_PUSHPULL;
  /* Pin PA0 is configured to Push-pull */
  GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE0_MASK) | GPIO_P_MODEL_MODE0_PUSHPULL;
  /* Pin PA1 is configured to Push-pull */
  GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE1_MASK) | GPIO_P_MODEL_MODE1_PUSHPULL;
  /* Pin PA2 is configured to Push-pull */
  GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE2_MASK) | GPIO_P_MODEL_MODE2_PUSHPULL;
  /* Pin PA3 is configured to Push-pull */
  GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE3_MASK) | GPIO_P_MODEL_MODE3_PUSHPULL;
  /* Pin PA4 is configured to Push-pull */
  GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE4_MASK) | GPIO_P_MODEL_MODE4_PUSHPULL;
  /* Pin PA5 is configured to Push-pull */
  GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE5_MASK) | GPIO_P_MODEL_MODE5_PUSHPULL;
  /* Pin PA6 is configured to Push-pull */
  GPIO->P[0].MODEL = (GPIO->P[0].MODEL & ~_GPIO_P_MODEL_MODE6_MASK) | GPIO_P_MODEL_MODE6_PUSHPULL;
  /* Pin PA12 is configured to Push-pull */
  GPIO->P[0].MODEH = (GPIO->P[0].MODEH & ~_GPIO_P_MODEH_MODE12_MASK) | GPIO_P_MODEH_MODE12_PUSHPULL;
  /* Pin PA13 is configured to Push-pull */
  GPIO->P[0].MODEH = (GPIO->P[0].MODEH & ~_GPIO_P_MODEH_MODE13_MASK) | GPIO_P_MODEH_MODE13_PUSHPULL;
  /* Pin PA14 is configured to Push-pull */
  GPIO->P[0].MODEH = (GPIO->P[0].MODEH & ~_GPIO_P_MODEH_MODE14_MASK) | GPIO_P_MODEH_MODE14_PUSHPULL;
  /* Pin PA15 is configured to Push-pull */
  GPIO->P[0].MODEH = (GPIO->P[0].MODEH & ~_GPIO_P_MODEH_MODE15_MASK) | GPIO_P_MODEH_MODE15_PUSHPULL;
  /* Pin PB0 is configured to Push-pull */
  GPIO->P[1].MODEL = (GPIO->P[1].MODEL & ~_GPIO_P_MODEL_MODE0_MASK) | GPIO_P_MODEL_MODE0_PUSHPULL;
  /* Pin PB1 is configured to Push-pull */
  GPIO->P[1].MODEL = (GPIO->P[1].MODEL & ~_GPIO_P_MODEL_MODE1_MASK) | GPIO_P_MODEL_MODE1_PUSHPULL;
  /* Pin PB2 is configured to Push-pull */
  GPIO->P[1].MODEL = (GPIO->P[1].MODEL & ~_GPIO_P_MODEL_MODE2_MASK) | GPIO_P_MODEL_MODE2_PUSHPULL;
  /* Pin PB3 is configured to Push-pull */
  GPIO->P[1].MODEL = (GPIO->P[1].MODEL & ~_GPIO_P_MODEL_MODE3_MASK) | GPIO_P_MODEL_MODE3_PUSHPULL;
  /* Pin PB4 is configured to Push-pull */
  GPIO->P[1].MODEL = (GPIO->P[1].MODEL & ~_GPIO_P_MODEL_MODE4_MASK) | GPIO_P_MODEL_MODE4_PUSHPULL;
  /* Pin PB9 is configured to Push-pull */
  GPIO->P[1].MODEH = (GPIO->P[1].MODEH & ~_GPIO_P_MODEH_MODE9_MASK) | GPIO_P_MODEH_MODE9_PUSHPULL;
  /* Pin PB10 is configured to Push-pull */
  GPIO->P[1].MODEH = (GPIO->P[1].MODEH & ~_GPIO_P_MODEH_MODE10_MASK) | GPIO_P_MODEH_MODE10_PUSHPULL;
  /* Pin PC6 is configured to Push-pull */
  GPIO->P[2].MODEL = (GPIO->P[2].MODEL & ~_GPIO_P_MODEL_MODE6_MASK) | GPIO_P_MODEL_MODE6_PUSHPULL;
  /* Pin PC7 is configured to Push-pull */
  GPIO->P[2].MODEL = (GPIO->P[2].MODEL & ~_GPIO_P_MODEL_MODE7_MASK) | GPIO_P_MODEL_MODE7_PUSHPULL;
  /* Pin PC8 is configured to Push-pull */
  GPIO->P[2].MODEH = (GPIO->P[2].MODEH & ~_GPIO_P_MODEH_MODE8_MASK) | GPIO_P_MODEH_MODE8_PUSHPULL;
  /* Pin PC9 is configured to Push-pull */
  GPIO->P[2].MODEH = (GPIO->P[2].MODEH & ~_GPIO_P_MODEH_MODE9_MASK) | GPIO_P_MODEH_MODE9_PUSHPULL;
  /* Pin PC10 is configured to Push-pull */
  GPIO->P[2].MODEH = (GPIO->P[2].MODEH & ~_GPIO_P_MODEH_MODE10_MASK) | GPIO_P_MODEH_MODE10_PUSHPULL;
  /* Pin PD9 is configured to Push-pull */
  GPIO->P[3].MODEH = (GPIO->P[3].MODEH & ~_GPIO_P_MODEH_MODE9_MASK) | GPIO_P_MODEH_MODE9_PUSHPULL;
  GPIO->P[3].MODEH = (GPIO->P[3].MODEH & ~_GPIO_P_MODEH_MODE10_MASK) | GPIO_P_MODEH_MODE10_PUSHPULL;
  GPIO->P[3].MODEH = (GPIO->P[3].MODEH & ~_GPIO_P_MODEH_MODE11_MASK) | GPIO_P_MODEH_MODE11_PUSHPULL;
  /* Pin PE0 is configured to Push-pull */
  GPIO->P[4].MODEL = (GPIO->P[4].MODEL & ~_GPIO_P_MODEL_MODE0_MASK) | GPIO_P_MODEL_MODE0_PUSHPULL;
  /* Pin PE1 is configured to Push-pull */
  GPIO->P[4].MODEL = (GPIO->P[4].MODEL & ~_GPIO_P_MODEL_MODE1_MASK) | GPIO_P_MODEL_MODE1_PUSHPULL;
  /* Pin PE4 is configured to Push-pull */
  GPIO->P[4].MODEL = (GPIO->P[4].MODEL & ~_GPIO_P_MODEL_MODE4_MASK) | GPIO_P_MODEL_MODE4_PUSHPULL;
  /* Pin PE5 is configured to Push-pull */
  GPIO->P[4].MODEL = (GPIO->P[4].MODEL & ~_GPIO_P_MODEL_MODE5_MASK) | GPIO_P_MODEL_MODE5_PUSHPULL;
  /* Pin PE6 is configured to Push-pull */
  GPIO->P[4].MODEL = (GPIO->P[4].MODEL & ~_GPIO_P_MODEL_MODE6_MASK) | GPIO_P_MODEL_MODE6_PUSHPULL;
  /* Pin PE7 is configured to Push-pull */
  GPIO->P[4].MODEL = (GPIO->P[4].MODEL & ~_GPIO_P_MODEL_MODE7_MASK) | GPIO_P_MODEL_MODE7_PUSHPULL;
  /* Pin PE8 is configured to Push-pull */
  GPIO->P[4].MODEH = (GPIO->P[4].MODEH & ~_GPIO_P_MODEH_MODE8_MASK) | GPIO_P_MODEH_MODE8_PUSHPULL;
  /* Pin PE9 is configured to Push-pull */
  GPIO->P[4].MODEH = (GPIO->P[4].MODEH & ~_GPIO_P_MODEH_MODE9_MASK) | GPIO_P_MODEH_MODE9_PUSHPULL;
  /* Pin PE10 is configured to Push-pull */
  GPIO->P[4].MODEH = (GPIO->P[4].MODEH & ~_GPIO_P_MODEH_MODE10_MASK) | GPIO_P_MODEH_MODE10_PUSHPULL;
  /* Pin PE11 is configured to Push-pull */
  GPIO->P[4].MODEH = (GPIO->P[4].MODEH & ~_GPIO_P_MODEH_MODE11_MASK) | GPIO_P_MODEH_MODE11_PUSHPULL;
  /* Pin PE12 is configured to Push-pull */
  GPIO->P[4].MODEH = (GPIO->P[4].MODEH & ~_GPIO_P_MODEH_MODE12_MASK) | GPIO_P_MODEH_MODE12_PUSHPULL;
  /* Pin PE13 is configured to Push-pull */
  GPIO->P[4].MODEH = (GPIO->P[4].MODEH & ~_GPIO_P_MODEH_MODE13_MASK) | GPIO_P_MODEH_MODE13_PUSHPULL;
  /* Pin PE14 is configured to Push-pull */
  GPIO->P[4].MODEH = (GPIO->P[4].MODEH & ~_GPIO_P_MODEH_MODE14_MASK) | GPIO_P_MODEH_MODE14_PUSHPULL;
  /* Pin PE15 is configured to Push-pull */
  GPIO->P[4].MODEH = (GPIO->P[4].MODEH & ~_GPIO_P_MODEH_MODE15_MASK) | GPIO_P_MODEH_MODE15_PUSHPULL;
  /* Pin PF8 is configured to Push-pull */
  GPIO->P[5].MODEH = (GPIO->P[5].MODEH & ~_GPIO_P_MODEH_MODE8_MASK) | GPIO_P_MODEH_MODE8_PUSHPULL;
  /* Pin PF9 is configured to Push-pull */
  GPIO->P[5].MODEH = (GPIO->P[5].MODEH & ~_GPIO_P_MODEH_MODE9_MASK) | GPIO_P_MODEH_MODE9_PUSHPULL;

  /* Enable clock for EBI */
  CMU_ClockEnable(cmuClock_EBI, true);
  /* Module EBI is configured to location 1 */
  EBI->ROUTE = (EBI->ROUTE & ~_EBI_ROUTE_LOCATION_MASK) | EBI_ROUTE_LOCATION_LOC1;
  /* EBI I/O routing */
  EBI->ROUTE |= EBI_ROUTE_APEN_A20 | EBI_ROUTE_CS0PEN | EBI_ROUTE_EBIPEN;

  /* Enable signals VBUSEN, DMPU */
  USB->ROUTE |= USB_ROUTE_VBUSENPEN | USB_ROUTE_DMPUPEN;

}

static inline uint32_t get_bit(uint32_t val, uint32_t bit) 
{
  return (val >> bit) & 0x1;
}


inline void execute(struct pinItem * item)
{
  uint16_t * addr = NULL;// = getPinAddress(item->pin);
  //printf("Ex C: %d, Tstrt: %d CurT: %d\n", item->pin, item->startTime, timeMs);
  int index = 0;
  switch(item->type) {
    case PINCONFIG_DATA_TYPE_DIGITAL_OUT:
      addr = getPinAddress(item->pin);
      printf("  DIGITAL: Digital C:%d, duty: %d, anti: %d ad: %p\n", item->pin, item->duty, item->antiDuty, addr);
      addr[PINCONFIG_DUTY_CYCLE] = item->duty;
      addr[PINCONFIG_ANTIDUTY_CYCLE] = item->antiDuty;
      addr[PINCONFIG_LOCAL_CMD] = CMD_START_OUTPUT;
      break;

    case PINCONFIG_DATA_TYPE_RECORD:
      printf("  RECORD DIGITAL: %d at rate %d\n", item->pin, item->sampleRate);
      startInput(item->pin, item->sampleRate);
      break;

    case PINCONFIG_DATA_TYPE_RECORD_ANALOGUE:
      printf("  RECORD ANAL: %d at rate %d\n", item->pin, item->sampleRate);
      startInput(item->pin, item->sampleRate);
      break;

    case PINCONFIG_DATA_TYPE_PREDEFINED_PWM:
      printf("  PWM: duty %d, aduty: %d", item->duty, item->antiDuty);
      printf("  NOT IMPLEMENTED :(\n)");
      //addr[PINCONFIG_DUTY_CYCLE]     = (uint16_t)item->duty;
      //addr[PINCONFIG_ANTIDUTY_CYCLE] = (uint16_t)item->antiDuty;
      //addr[PINCONFIG_SAMPLE_RATE]    = (uint16_t)item->sampleRate;
      //addr[PINCONFIG_RUN_INF]        = 1;
      //addr[PINCONFIG_LOCAL_CMD] = CMD_START_OUTPUT;
      break;

    case PINCONFIG_DATA_TYPE_DAC_CONST:
      printf("  CONST DAC VOLTAGE,channel %u, %u\n", item->pin, item->constantValue);
      setVoltage(item->pin, item->constantValue);
      break;

    case PINCONFIG_DATA_TYPE_PREDEFINED_SINE:
      //This reeeeally doesn't work all that well. execute happens too slow?
      index = (item->sampleRate*timeTick)%255;
      setVoltage(item->pin, sinus[index]);
      break;

    case PINCONFIG_DATA_TYPE_CONSTANT_FROM_REGISTER:
      //Note: Index is coded in constantValue
      //printf("  CONST REGISTER DAC VOLTAGE,channel %d, %d\n", item->pin, DACreg[item->constantValue]);
      setVoltage(item->pin, DACreg[item->constantValue]);
      break;

    default:
      break;
  }
}

void killItem(struct pinItem * item)
{
  uint16_t * addr = getPinAddress(item->pin);
  switch(item->type) {
    case PINCONFIG_DATA_TYPE_DIGITAL_OUT:
      addr[PINCONFIG_DUTY_CYCLE] = 0;  //TODO: FPGA will be updated with a constVal register.
      addr[PINCONFIG_ANTIDUTY_CYCLE] = 0;  //TODO: FPGA will be updated with a constVal register.
      addr[PINCONFIG_LOCAL_CMD] = CMD_RESET;
      break;
    case PINCONFIG_DATA_TYPE_PREDEFINED_PWM:
      addr[PINCONFIG_DUTY_CYCLE] = 0;  //TODO: FPGA will be updated with a constVal register.
      addr[PINCONFIG_LOCAL_CMD] = CMD_RESET;
      break;
    case PINCONFIG_DATA_TYPE_RECORD_ANALOGUE:
    case PINCONFIG_DATA_TYPE_RECORD:
      for(int i = 0; i < numInputChannels; i++) {
        if (inputChannels[i] == item->pin) {
          printf("Rec stop: %d\n", item->pin);
          numInputChannels--;
          for(int j = i; j < numInputChannels; j++){
            inputChannels[i] = inputChannels[i+1];
          }
          break;
        }
      }
      break;
    default:
      break;
  }
}

//inline void startInput(struct pinItem * item)
inline void startInput(FPGA_IO_Pins_TypeDef channel, int sampleRate)
{
  //If it's a ADC channel we set up the ADC here in stead of all this other faff.
  uint16_t * addr = getPinAddress(channel);
  if ((AD_CHANNELS_START <= channel) && (channel <= (AD_CHANNELS_END))) {
    
    uint16_t boardChan = channel - AD_CHANNELS_START;
    //board 0
    if(boardChan < 16) {
      //Program sequence register with channel.
      adcSequence[0] |= 0xE000 | (1 << (12 - boardChan));
      printf("ADCregister write channel %x, %x\n", boardChan, adcSequence[0]);

      //Range register 1
      addr[0x04] = 0xAAA0; //range register written to +-5V on all channels for chans 0 to 4
      for(int i = 0; i < 100000; i++);
      while(addr[0x0A]);

      //Range register 2
      addr[0x04] = 0xCAA0; //range register written to +-5V on all channels for chans 4 to 7
      for(int i = 0; i < 100000; i++);
      while(addr[0x0A]);

      //power bits.
      addr[0x04] = 0x8014;

      //setup FPGA AD controller
      addr[0x01] = sampleRate; //overflow
      addr[0x02] = 1; //divide
      //Sequence register write.
      
      addr[0x04] = adcSequence[0];
      for(int i = 0; i < 100000; i++);
      while(addr[0x0A]);
 
      //Program the ADC to use this channel as well now. Result in two comp, internal ref.
      //sequencer on.
      //Control register
      addr[0x04] = 0x8014;
      for(int i = 0; i < 100000; i++);
      while(addr[0x0A]);

      printf("ADC programmed to new sequences\n");
    }
  }
  
  //Digital channel.
  else {
    addr[PINCONFIG_LOCAL_CMD] = CMD_RESET;
    addr[PINCONFIG_SAMPLE_RATE] = (uint16_t)sampleRate;
    addr[PINCONFIG_LOCAL_CMD] = CMD_INPUT_STREAM;
  }

  inputChannels[numInputChannels++] = channel;
}

//TODO: We can get away with only 1 read here. In time. 
inline void getInput(FPGA_IO_Pins_TypeDef channel)
{
  
  uint16_t * addr = getPinAddress(channel);
  
  struct sampleValue val;
 
  val.sampleNum = addr[PINCONFIG_SAMPLE_CNT];
  val.channel = (uint8_t)channel;
  val.value = addr[PINCONFIG_SAMPLE_REG];
 
  //Copy to sample buffer.
  if(!sendInProgress && (val.sampleNum != (uint16_t)lastCollected[channel])) {
    lastCollected[channel] = val.sampleNum; 
    //NOTE: This wraps around the per-channel buffer, so we'll drop samples for sure.
    sampleBuffer[numSamples++] = val;
  }

  //printf("G: n:%u ch:%u hex:0x%x twodec:%d unsigned:%u\n", sample->sampleNum, sample->channel, got, got&0x1FFF, got&0x1FFF);
}

//TODO: MAKE LOOKUPTABLE
inline uint16_t * getPinAddress(FPGA_IO_Pins_TypeDef channel)
{

  //Digital channels.
  if (channel < IO_CHANNELS_END) {
    return (uint16_t*)(EBI_ADDR_BASE) + (channel * 0x100);
  }

  //ADC channels
  if ((AD_CHANNELS_START <= channel) && (channel <= AD_CHANNELS_END)) {
    uint16_t boardChan = channel - AD_CHANNELS_START;
    uint16_t controllerNr = channel - boardChan;
    //Each board has 1 controller, each channel is at 16 uint16_t's offset.
    return (uint16_t*)(EBI_ADDR_BASE) + (controllerNr * 0x100) + (0x10*boardChan);
  }
  
  if ((DA_CHANNELS_START <= channel) && (channel <= DA_CHANNELS_END)) {
    return (uint16_t*)(EBI_ADDR_BASE) + (channel * 0x100);
  }
  //Each channel controller has 2^8 = 0x100 16-bit words, or
  //2^9 bytes.
  return (uint16_t*)(EBI_ADDR_BASE) + channel*0x100;
}

void execCurrentPack()
{

  if(currentPack.command == USB_CMD_STATUS) {
    //sendPacket(STATUS_BYTES, USB_CMD_STATUS, (uint8_t*)dat);
  }

  if(currentPack.command == USB_CMD_CONFIG_PIN) {
    struct pinItem item;
    if(currentPack.data != NULL) {
      uint32_t * d = (uint32_t *)(currentPack.data);
      item.pin = (d[PINCONFIG_DATA_FPGA_PIN]);
      item.duty = d[PINCONFIG_DATA_DUTY];
      item.antiDuty = d[PINCONFIG_DATA_ANTIDUTY];
      item.startTime = d[PINCONFIG_START_TIME];
      item.endTime = d[PINCONFIG_END_TIME];
      item.constantValue = d[PINCONFIG_DATA_CONST];
      item.type = d[PINCONFIG_DATA_TYPE];
      item.sampleRate = d[PINCONFIG_DATA_SAMPLE_RATE];
      itemsToApply[itaPos++] = item;
     
      //find lowest first killtime.
      if(numItems == 0) {
        nextKillTime = item.endTime;
      } else if (item.endTime < nextKillTime) {
        nextKillTime = item.endTime;
      }
      
      numItems++;
      printf("Item added to pin %d, starting at %d, ending at %d, samplerate %d\n", item.pin, item.startTime, item.endTime, item.sampleRate);
    } else {
      printf("Curr data NULL\n");
    }
  }

  if(currentPack.command == USB_CMD_RUN_SEQ)
  {
    printf("Starting sequence run\n");
    runItems = 1;
    timeTick = 0;
    itaPos = 0;
  }

  if(currentPack.command == USB_CMD_PROGRAM_XBAR)
  {
    printf("Configuring XBAR\n");
    uint16_t * d = (uint16_t*)(currentPack.data);
    for(int i = 0; i < 32; i++) {
      xbar[i] = d[i];
      printf("XBAR: Word %d: %x\n", i, d[i]); 
    }
    xbar[0x20] = 0x1; //whatever written to this register will be interpreted as a cmd.
    while(xbar[0x0A]) { printf("."); }
    xbar[0x20] = 0x0; //whatever written to this register will be interpreted as a cmd.

    printf("\nXBAR configured\n");
  }


  if(currentPack.command == USB_CMD_UPDATE_REGISTER)
  {
    printf("Updating Register\n");
    int * d = (int*)(currentPack.data);
    printf("REG %d: %d\n", d[0], d[1]); 
    DACreg[d[0]] = d[1];
    registersUpdated++;
  }


  if(currentPack.command == USB_CMD_CONST) {
    /* Start output from pin controllers */
    //uint32_t * d = (uint32_t *)(currentPack.data);
    //startConstOutput((d[PINCONFIG_DATA_FPGA_PIN]));
  }

  if(currentPack.command == USB_CMD_LED) {
    /* Start output from pin controllers */
    uint32_t * d = (uint32_t *)(currentPack.data);
    led(d[LED_SELECT], d[LED_MODE]);
  }


  if(currentPack.command == USB_CMD_RESET_ALL) {

    //Reset state
    itaPos = 0;
    numItems = 0;
    iifPos = 0;
    numItemsInFlight = 0;
    runItems = 0;
    numSamples = 0;
    numInputChannels = 0;
    nextKillTime = 0;
    adcSequence[0] = 0;

    timeTick = 0;

    //Reset all DAC outputs to known values.

    for(unsigned int i = DA_CHANNELS_START; i < DA_CHANNELS_START+8; i++) {
      setVoltage(i, 128);
    }

    printf("RESET. NumSamples: %u\n", numSamples);

    resetAllPins();
  }

  if(currentPack.command == USB_CMD_GET_INPUT_BUFFER_SIZE) {
    printf("SHIPPING numSamples: %d\n", numSamples);
    sendPacket(4, USB_CMD_GET_INPUT_BUFFER_SIZE, (uint8_t*)&numSamples);
  }

  if(currentPack.command == USB_CMD_GET_INPUT_BUFFER) {
    //Send back the whole sending buffer.
    //Max packet size is 64000 bytes, so we need to split into several xfer's.
    
    //Data contains the number of samples to xfer.
    uint32_t * txSamples = (uint32_t *)(currentPack.data);

    int bytes = sizeof(struct sampleValue) * *txSamples;
    printf("Sending back %d BYTES from sampleBuffer\n", bytes);
    sendPacket(bytes, USB_CMD_GET_INPUT_BUFFER, (uint8_t*)sampleBuffer);
    
    printf("Back to main loop\n");
  }


  if(currentPack.command == USB_CMD_PROGRAM_FPGA) {
    if(!fpgaUnderConfiguration) {
      //Start the configuration process.
      //set the input pin modes.
      //DONE
      GPIO_PinModeSet(gpioPortD, 12, gpioModeInput, 0); 
      //INIT_B = PD15, active _low_.
      GPIO_PinModeSet(gpioPortD, 15, gpioModeInput, 0); 
      GPIO_PinModeSet(gpioPortC, 3, gpioModePushPull, 0);
      //Prog pins...
      GPIO_PinModeSet(gpioPortA, 7, gpioModePushPull, 0);  
      GPIO_PinModeSet(gpioPortA, 8, gpioModePushPull, 0);  


      //start programming (prog b to low)
      GPIO_PinOutClear(gpioPortC, 3);
      //wait until init b and done are low both low.
      while(GPIO_PinInGet(gpioPortD, 15) ||
          GPIO_PinInGet(gpioPortD, 12));

      //set prog b high again (activate programming)
      GPIO_PinOutSet(gpioPortC, 3);

      //wait until initB goes high again (fpga ready for data)
      while(!GPIO_PinInGet(gpioPortD, 15));

      fpgaUnderConfiguration = 1;  //fpga now under configuration
    }

    int nb = 0;
    for(uint32_t i = 0; i < currentPack.size; i++) {
      for(int b = 7; b >= 0; b--) {
        GPIO_PinOutClear(gpioPortA, 8); //clk low
        //clock a bit.
        if ((currentPack.data[i] >> b) & 0x1) {
          GPIO_PinOutSet(gpioPortA, 7); 
        } else {
          GPIO_PinOutClear(gpioPortA, 7); 
        }
        GPIO_PinOutSet(gpioPortA, 8); //clk high
      }
      nb++;
    }
    packNum++;
  }

  if(currentPack.size > 0) {
    free(currentPack.data);
  }
}

void sendPacket(uint32_t size, uint32_t cmd, uint8_t * data)
{
  struct mecoPack pack;
  pack.size = size;
  pack.command = cmd;
  pack.data = data;

  packToSend = pack; //This copies the pack structure.
  printf("Writing back %u bytes over USB.\n", (unsigned int)pack.size);
  sendInProgress = 1;
  USBD_Write(EP_DATA_IN1, packToSend.data, packToSend.size, UsbDataSent);
}

void resetAllPins()
{
  for (int i = 0; i < nPins; i++) {
    lastCollected[i] = -1;
  }

  printf("Reseting all digital pin controllers\n");
  for(int j = 0; j < 50; j++) {
    uint16_t * addr = (getPinAddress((j)));
    addr[PINCONFIG_LOCAL_CMD] = CMD_RESET;
  }
  printf("OK\n");
}

void led(int l, int mode) 
{
  switch(l) {
    case FRONT_LED_0:
      GPIO_PinModeSet(gpioPortD, 4, gpioModePushPull, 1-mode);  //Led 1
      break;
    case FRONT_LED_1:
      GPIO_PinModeSet(gpioPortB, 11, gpioModePushPull, 1-mode);  //Led 2
      break; 
    case FRONT_LED_2:
      GPIO_PinModeSet(gpioPortB, 8, gpioModePushPull, 1-mode);  //Led 4
      break;
    case FRONT_LED_3:
      GPIO_PinModeSet(gpioPortD, 8, gpioModePushPull, 1-mode);  //Led 3
      break;

    case BOARD_LED_U0:
      GPIO_PinModeSet(gpioPortA, 10, gpioModePushPull,mode);  //Led U0
      break;
    case BOARD_LED_U1:
      GPIO_PinModeSet(gpioPortA, 9, gpioModePushPull,mode);  //Led U1
      break;
    case BOARD_LED_U2:
      GPIO_PinModeSet(gpioPortB, 12, gpioModePushPull,mode);  //Led U2
      break;
    case BOARD_LED_U3:
      GPIO_PinModeSet(gpioPortD, 0, gpioModePushPull,mode);  //Led U3
      break;
    default:
      break;
  }
}

//void programFPGA() {
//
//}
