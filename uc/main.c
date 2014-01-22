#define __GIT_COMMIT__ "5e1dca595ed4c443e5f223f3a2b7126c25f54fb6"

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
#include "ll.h"
#include "pinItemQueue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "../mecoprot.h"
#include "mecobo.h"
#include "pinItem.h"

#include "descriptors.h"
#include "struct_init.h"

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

/*** Typedef's and defines. ***/

static int timeMs;

//USB Variables
int packNum = 0;

static uint32_t inBufferTop;
static uint8_t * inBuffer;

static struct mecoPack currentPack;
static struct mecoPack packToSend;
static int sendPackReady = 0;

//Keep track of which pins are input pins. 
static int nPins = 50;


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

static int lastCollected[50];
static int runItems = 0;

#define MAX_SAMPLES 15*1024
struct sampleValue * sampleBuffer;
int numSamples = 0;

int inputPins[10];
int numInputPins = 0;
int nextKillTime = 0;


void TIMER1_IRQHandler(void)
{ 
  /* Clear flag for TIMER2 overflow interrupt */
  TIMER_IntClear(TIMER1, TIMER_IF_OF);

  /* Toggle LED ON/OFF */
  GPIO_PinOutToggle(gpioPortB, 12);
  //Retrieve sample value from pin controllers and queue them for sending. 
}

void TIMER2_IRQHandler(void)
{ 
  TIMER_IntClear(TIMER2, TIMER_IF_OF);
  timeMs += 1;
}

int main(void)
{
  eADesigner_Init();
  setupSWOForPrint();
  printf("Printing online.\n");

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

  /* Set TIMER Top value */
  TIMER_TopSet(TIMER1, 10000);
  TIMER_TopSet(TIMER2, 47);

  printf("Initalizing timers\n");
  TIMER_Init(TIMER1, &timerInit);
  TIMER_Init(TIMER2, &timerInit);

  printf("Initializing EBI\n");
  EBI_Init(&ebiConfig);

  GPIO_PinModeSet(gpioPortB,  12, gpioModePushPull, 0);  //LED U1
  GPIO_PinModeSet(gpioPortA, 10, gpioModePushPull, 1);  //Led U2
  GPIO_PinModeSet(gpioPortD,  0, gpioModePushPull, 0);  //LED U3
  //Turn off all LEDS
  for(int l = 1; l < 6; l++) {
    led(l, 1);
  }
  sampleBuffer = malloc(sizeof(struct sampleValue)*MAX_SAMPLES);
  inBuffer = (uint8_t*)malloc(128*8);
  inBufferTop = 0;

  printf("Initializing USB\n");
  USBD_Init(&initstruct);
  printf("USB Initialized.\n");

  //Release fpga reset
  GPIO_PinOutClear(gpioPortB, 4);

  /*
   * When using a debugger it is practical to uncomment the following three
   * lines to force host to re-enumerate the device.
   */
  USBD_Disconnect();
  USBTIMER_DelayMs(100);
  USBD_Connect();
  printf("USB CONNECTED\n");

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


  nextKillTime = 0;

  printf("It's just turtles all the way down.\n");
  printf("I'm the mecobo firmware running on the evolutionary motherboard.\n");
  printf("I was built %s, git commit %s\n", __DATE__, __GIT_COMMIT__);
  printf("Entering main loop.\n");
  for (;;) {
    //check if DONE has gone high, then we are ... uh, done
    if(fpgaUnderConfiguration) {
      if(GPIO_PinInGet(gpioPortD, 12)) {
        printf("FPGA config over, DONE went high. All is well witht he world.\n");
        fpgaUnderConfiguration = 0;
        //Release FPGA reset signal from uC 
        //Send a few 1's to make sure the device boots.
        GPIO_PinOutSet(gpioPortA, 7); 
        for(int j = 0; j < 64; j++) {
          GPIO_PinOutClear(gpioPortA, 8); //clk low
          GPIO_PinOutSet(gpioPortA, 8); //clk high
        }
        GPIO_PinOutSet(gpioPortD, 0); //turn on led (we're configured)
      }
    }

    //We can start the EBI interface if the FPGA is configured.
    //Whatever time is left should be spent collecting data from input pins.

    //For all the input pins, collect samples into the big buff!
    //TODO: Make pinControllers notify uC when it has new data, interrupt?
    if(runItems) {
      for(int ip = 0; ip < numInputPins; ip++) {
        struct sampleValue val;
        getInput(&val, (FPGA_IO_Pins_TypeDef)inputPins[ip]);
        //printf("num: %d\n", val.sampleNum);
        if(val.sampleNum != (uint16_t)lastCollected[inputPins[ip]]) {
          lastCollected[inputPins[ip]] = val.sampleNum; 
          if(numSamples < MAX_SAMPLES) {
            sampleBuffer[numSamples++] = val;
          }
        }
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
      //Kill items that are in flight and whose time has come.
      //We cannot guarantee the same as we do for scheduling;
      if(nextKillTime <= timeMs) {
        if (numItemsInFlight > 0)  {
          //Iterate array of items in flight, kill whatever needs killing,
          //mark an empty entry as NULL.
          for(int i = 0; i < iifPos; i++) {
            struct pinItem * it = itemsInFlight[i];
            if(it != NULL) {
              if(it->endTime <= timeMs) {
                killItem(it);
                itemsInFlight[i] = NULL;
                numItemsInFlight--;
              }
            }
          }
        }
      }

    }
  } //for loop ends

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
      printf("Expected header of size 8, got ...something not 8.\n");
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

//The purpose of this function is to configure the FPGA
//with the data found in the pin config structure. 
int fpgaConfigPin(struct pinConfig * p)
{
  uint16_t * pin = getPinAddress((FPGA_IO_Pins_TypeDef)p->fpgaPin);

  *(pin + PINCONFIG_DUTY_CYCLE)     = p->duty;
  *(pin + PINCONFIG_ANTIDUTY_CYCLE) = p->antiduty;
  *(pin + PINCONFIG_CYCLES)         = p->cycles;
  *(pin + PINCONFIG_RUN_INF)        = p->runInf;
  *(pin + PINCONFIG_SAMPLE_RATE)    = p->sampleRate;

  //TODO: This doesn't really work,
  //what is a pin is removed.
  //TODO: support everything :-)
  return 0;
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

  if ((status == USB_STATUS_OK) && (xf > 0)) 
  {
    //we probably sent some data :-)
    //don't free the sample buffer
    if((packToSend.command != USB_CMD_GET_INPUT_BUFFER) &&
        (packToSend.command != USB_CMD_GET_INPUT_BUFFER_SIZE)) 
    {
      free(packToSend.data);
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
  printf("E: %d, Tstrt: %d CurT: %d\n", item->pin, item->startTime, timeMs);
  uint16_t * addr = getPinAddress(item->pin);
  switch(item->type) {
    case PINCONFIG_DATA_TYPE_DIRECT_CONST:
      printf("  CONST: %d\n", item->constantValue);
      addr[PINCONFIG_DUTY_CYCLE] = item->constantValue;  //TODO: FPGA will be updated with a constVal register.
      addr[PINCONFIG_LOCAL_CMD] = CMD_CONST;
      break;
    case PINCONFIG_DATA_TYPE_RECORD:
      printf("  RECORD: %d at rate %d\n", item->pin, item->sampleRate);
      startInput(item->pin, item->sampleRate);
      break;
    case PINCONFIG_DATA_TYPE_PREDEFINED_PWM:
      printf("  PWM: duty %d, aduty: %d\n", item->duty, item->antiDuty);
      addr[PINCONFIG_DUTY_CYCLE]     = item->duty;
      addr[PINCONFIG_ANTIDUTY_CYCLE] = item->antiDuty;
      //addr[PINCONFIG_CYCLES]       = item->cycles;
      //addr[PINCONFIG_RUN_INF]        = 1;
      //addr[PINCONFIG_SAMPLE_RATE]    = item->sampleRate;
      addr[PINCONFIG_LOCAL_CMD] = CMD_START_OUTPUT;
      break;

    default:
      break;
  }
}

void killItem(struct pinItem * item)
{
  printf("K: %d, Tend: %d CurT:%d\n", item->pin, item->endTime, timeMs);
  uint16_t * addr = getPinAddress(item->pin);
  switch(item->type) {
    case PINCONFIG_DATA_TYPE_DIRECT_CONST:
      addr[PINCONFIG_DUTY_CYCLE] = 0;  //TODO: FPGA will be updated with a constVal register.
      addr[PINCONFIG_LOCAL_CMD] = CMD_RESET;
    case PINCONFIG_DATA_TYPE_RECORD:
      for(int i = 0; i < numInputPins; i++) {
        if (inputPins[i] == item->pin) {
          printf("Rec stop: %d\n", item->pin);
          numInputPins--;
          for(int j = i; j < numInputPins; j++){
            inputPins[i] = inputPins[i+1];
          }
          break;
        }
      }
      break;
    default:
      break;
  }
}

void startConstOutput(FPGA_IO_Pins_TypeDef pin)
{
  uint16_t * addr = getPinAddress(pin);
  addr[PINCONFIG_LOCAL_CMD] = CMD_RESET;
  addr[PINCONFIG_LOCAL_CMD] = CMD_CONST;
  printf("CONST pin %d\n", pin);
}

inline void startInput(FPGA_IO_Pins_TypeDef pin, int sampleRate)
{
  uint16_t * addr = getPinAddress(pin);
  addr[PINCONFIG_LOCAL_CMD] = CMD_RESET;
  addr[PINCONFIG_SAMPLE_RATE] = sampleRate;
  addr[PINCONFIG_LOCAL_CMD] = CMD_INPUT_STREAM;
  inputPins[numInputPins++] = pin;
  printf("input: %d\n", pin);
}

inline void getInput(struct sampleValue * val, FPGA_IO_Pins_TypeDef pin)
{
  uint16_t * addr = getPinAddress(pin);
  val->sampleNum = addr[PINCONFIG_SAMPLE_CNT];
  val->pin = pin;
  val->value = addr[PINCONFIG_SAMPLE_REG];
}

uint16_t * getPinAddress(FPGA_IO_Pins_TypeDef pin)
{
  //TODO: Ignoring enum type... probably 32 bit int, but..
  uint16_t offset = pin << 8; //8 MSB bits is pinConfig module addr
  return (uint16_t*)EBI_ADDR_BASE + offset;
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
      numItems++;
      printf("Item added to pin %d, starting at %d, samplerate %d\n", item.pin, item.startTime, item.sampleRate);
    } else {
      printf("Curr data NULL\n");
    }
  }

  if(currentPack.command == USB_CMD_RUN_SEQ)
  {
    printf("Starting sequence run\n");
    runItems = 1;
    timeMs = 0;
    itaPos = 0;
  }

  if(currentPack.command == USB_CMD_CONST) {
    /* Start output from pin controllers */
    uint32_t * d = (uint32_t *)(currentPack.data);
    startConstOutput((d[PINCONFIG_DATA_FPGA_PIN]));
  }

  if(currentPack.command == USB_CMD_LED) {
    /* Start output from pin controllers */
    uint32_t * d = (uint32_t *)(currentPack.data);
    led(d[LED_SELECT], d[LED_MODE]);
  }

  if(currentPack.command == USB_CMD_STREAM_INPUT) {
    /* Start input in pin controller */
    //uint32_t * d = (uint32_t *)(currentPack.data);
    //startInput((d[PINCONFIG_DATA_FPGA_PIN]));
  }


  if(currentPack.command == USB_CMD_READ_PIN) {
    struct mecoPack pack;
    struct sampleValue value;
    pack.size = sizeof(struct sampleValue);
    pack.command = currentPack.command;
    pack.data = (malloc(sizeof(struct sampleValue)));
    getInput(&value, (*currentPack.data));
    memcpy(pack.data, &value, sizeof(struct sampleValue));
    //ship it!
    packToSend = pack; 
    sendPackReady = 1; 
  }

  if(currentPack.command == USB_CMD_RESET_ALL) {

    //Reset state
    itaPos = 0;
    numItems = 0;
    iifPos = 0;
    numItemsInFlight = 0;
    runItems = 0;
    numSamples = 0;
    numInputPins = 0;
    nextKillTime = 0;

    resetAllPins();
  }

  if(currentPack.command == USB_CMD_GET_INPUT_BUFFER_SIZE) {
    printf("SHIPPING numSamples: %d\n", numSamples);
    sendPacket(4, USB_CMD_GET_INPUT_BUFFER_SIZE, (uint8_t*)&numSamples);
  }

  if(currentPack.command == USB_CMD_GET_INPUT_BUFFER) {
    //Send back the whole sending buffer.
    printf("Sending back the input buffers, of sample size %d\n", numSamples);
    uint32_t * d = (uint32_t *)(currentPack.data);
    sendPacket(sizeof(struct sampleValue) * *d, USB_CMD_GET_INPUT_BUFFER, (uint8_t*)sampleBuffer);
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
  USBD_Write(EP_DATA_IN1, packToSend.data, packToSend.size, UsbDataSent);
}

void resetAllPins()
{
  for (int i = 0; i < nPins; i++) {
    uint16_t * addr = (getPinAddress((i)));
    addr[PINCONFIG_LOCAL_CMD] = CMD_RESET;
    lastCollected[i] = -1;
  }
}

void led(int l, int mode) 
{
  printf("led: %d, m: %d\n", l, mode);
  switch(l) {
    case 1:
  GPIO_PinModeSet(gpioPortF,  7, gpioModePushPull, mode);
  break;
    case 2:
  GPIO_PinModeSet(gpioPortC,  11, gpioModePushPull, mode);
  break;
    case 3:
  GPIO_PinModeSet(gpioPortB,  8, gpioModePushPull, mode);
  break;
    case 4:
  GPIO_PinModeSet(gpioPortD,  8, gpioModePushPull, mode);
  break;
    case 5:
  GPIO_PinModeSet(gpioPortF,  6, gpioModePushPull, mode);
  break;
    default:
  break;
  }
}
