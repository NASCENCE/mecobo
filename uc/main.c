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



/*
 * Listen to the interrupt! It will be raised if any of the
 * FIFOs are full, empty or any other stuff stuff happens.
 */

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
#include "norflash.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "efm32.h"
#include "../mecoprot.h"
#include "mecobo.h"
#include "pinItem.h"

#include "descriptors.h"

#include "dac.h"
#include "adc.h"


#include "fifo.h"

struct fifo cmdFifo;

#define MAGIC_FPGA_BITFILE_CONSTANT 0x4242

char * BUILD_VERSION = __GIT_COMMIT__;

int NORPollData(uint16_t writtenWord, uint32_t addr);


#define DEBUG_PRINTING 1
//override newlib function.
int _write_r(void *reent, int fd, char *ptr, size_t len)
{
  (void) reent;
  (void) fd;
  for(size_t i = 0; i < len; i++) 
    ITM_SendChar(ptr[i]);

  return len;
}

int timeStarted = 0; 
static uint8_t mecoboStatus = MECOBO_STATUS_READY;

/*** Typedef's and defines. ***/
static int timeMs = 0;
static int timeTick = 0; //10,000 per second.
static int lastTimeTick = 0;

static int has_daughterboard = 0;

//USB Variables

static uint32_t inBufferTop;
static uint8_t * inBuffer;

static struct mecoPack currentPack;
static struct mecoPack packToSend;
static int sendPackReady = 0;
static int sendInProgress = 0;
static int executeInProgress = 0;

static int feedCmdFifo = 0;
static int sampleFifoEmpty = 0;

//Are we programming the FPGA
int fpgaConfigured = 0;
uint32_t bitfileOffset = 0;
uint32_t lastPackSize = 0;



//Data stuff for items
static struct pinItem * itemsToApply;
int itaFifoSize = 50;
int itaPos  = 0;
int itaHead = 0;
int itaTail = 0;


uint16_t numItemsLeftToExecute = 0;
static struct pinItem ** itemsInFlight;
uint16_t iifPos  = 0;
uint16_t numItemsInFlight = 0;

static int runItems = 0;

static int fpgaTableIndex = 0;
static uint8_t fpgaTableToChannel[8];
static uint8_t numSamplesPerFPGATableIndex[8];
static int fpgaNumSamples = 0;
static int samplingStarted = 0;

uint16_t * xbar = ((uint16_t*)EBI_ADDR_BASE) + (200 * 0x100);
#define NUM_DAC_REGS 4
uint16_t DACreg[NUM_DAC_REGS];
uint8_t registersUpdated[NUM_DAC_REGS];

//Circular buffer in SRAM1, modulo MAX_SAMPLES
static const int MAX_SAMPLES = 43689; //SRAM1_BYTES/sizeof(struct sampleValue);
#define MAX_INPUT_CHANNELS 10
#define MAX_CHANNELS 150
static const int BUFFERSIZE = 1000;  //We will have 1000 samples per channel.
//Such a waste of space, should use hash map.

int numSamples = 0;

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

  eADesigner_Init();

  setupSWOForPrint();
  printf("Print output online\n");
  
  //Small check for endinanness
  uint32_t integer = 0xDEADBABE;
  uint8_t * int8 = (uint8_t*)&integer;
  if(int8[0] == 0xDE) printf("Arch is BIG ENDIAN\n");
  if(int8[0] == 0xBE) printf("Arch is LITTLE ENDIAN\n");

  //Turn on front led 1 to indicate we are setting up the system.
  led(FRONT_LED_0, 1);

  //nor in reset
  GPIO_PinModeSet(gpioPortC, 2, gpioModePushPull, 1);  
  GPIO_PinOutClear(gpioPortC, 2); //active low

  //set byte mode
  GPIO_PinModeSet(gpioPortC, 1, gpioModePushPull, 1);  
  GPIO_PinOutSet(gpioPortC, 1); //active low

  //turn off write protect
  GPIO_PinModeSet(gpioPortB, 7, gpioModePushPull, 1);  
  GPIO_PinOutSet(gpioPortB, 7); //active low

  //nor out of reset
  GPIO_PinOutSet(gpioPortC, 2); //active low


  inBuffer = (uint8_t*)malloc(128*8);
  inBufferTop = 0;

  //Put FPGA out of reset
  GPIO_PinModeSet(gpioPortB, 5, gpioModePushPull, 1);  
  GPIO_PinOutSet(gpioPortB, 5); //Reset
  GPIO_PinOutClear(gpioPortB, 5); //Reset clear


  sendPackReady = 0;

  //resetNor();
  //autoSelectNor();

  //Make sure NOR has come up.
  for(int norwait = 0; norwait < 10000000; norwait++);

  uint16_t * a;
  a = getChannelAddress(2) + PINCONFIG_STATUS_REG;
  uint16_t foo = *a;
  if (foo != 2) {
    printf("Got unexpected %x from FPGA. Reprogramming.\n", foo);
    //programFPGA();
  } else {
    printf("FPGA responding as expected\n");
  }


  int skip_boot_tests= 0;

  if(!skip_boot_tests) {

    //Verify presence of daughterboard bitfile
    uint16_t * dac = (uint16_t*)(EBI_ADDR_BASE) + (0x100*DAC0_POSITION);
    if (dac[PINCONFIG_STATUS_REG] == 0xdac) {
      has_daughterboard = 1;
      printf("Detected daughterboard bitfile (has DAC controller)\n");
    }

    has_daughterboard = 0;
    if (has_daughterboard) {
      //Check DAC controllers.
      printf("DAC: %x\n", dac[PINCONFIG_STATUS_REG]);
      uint16_t * adc = (uint16_t*)(EBI_ADDR_BASE) + (0x100*ADC0_POSITION);
      printf("ADC: %x\n", adc[PINCONFIG_STATUS_REG]);
      printf("XBAR: %x\n", xbar[PINCONFIG_STATUS_REG]);
      printf("Setting up DAC and ADCs\n");
      setupDAC();
      setupADC();
    }

    printf("Response from digital controllers at 0 to 57:\n");
    for(int i = 0; i < 57; i++) {
      uint16_t * a = getChannelAddress(i) + PINCONFIG_STATUS_REG;
      uint16_t foo = *a;
      if (DEBUG_PRINTING) printf("Controller %d says it's position is %d\n", i, foo);
    }

    printf("FPGA check complete\n");

    //testNOR();
    testRam();
  }




  itemsToApply = malloc(sizeof(struct pinItem) * itaFifoSize);
  itemsInFlight = malloc(sizeof(struct pinItem *) * 100);
  printf("Malloced memory: %p, size %u\n", itemsToApply, sizeof(struct pinItem)*50);
  printf("Malloced memory: %p, size %u\n", itemsInFlight, sizeof(struct pinItem)*100);

  //Default all regs to output 0V
  for(int i = 0; i < NUM_DAC_REGS; i++) {
    DACreg[i] = 128;
  }




  USBD_Init(&initstruct);
  printf("USB Initialized.\n");
  USBD_Disconnect();
  USBTIMER_DelayMs(100);
  USBD_Connect();
  led(BOARD_LED_U3, 1);


  printf("Cycling LEDs :-)\n");
  //Cycle leds.
    for(int i = 0; i < 48; i++) {
      led(i%8, 1);
      USBTIMER_DelayMs(60);
      led(i%8, 0);
    }


  printf("It's just turtles all the way down.\n");
  printf("I'm the mecobo firmware running on the evolutionary motherboard 3.5new.\n");
  printf("I was built %s, git commit %s\n", __DATE__, BUILD_VERSION);
  printf("Entering main loop.\n");


  fifoInit(&cmdFifo, 50, sizeof(struct pinItem));
 
  uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;

  for (;;) {
    if (cmdFifo.numElements > 0) {
      void * item = NULL;
      fifoGet(&cmdFifo, &item);
      struct pinItem * it = (struct pinItem *)item;
      printf("pushing to fifo, endtime %u\n", it->endTime);
      pushToCmdFifo(it);
    }

    if(runItems & !timeStarted) {
      cmdInterfaceAddr[7] = 0xDEAD;   //reset and stop time
      cmdInterfaceAddr[8] = 0x1234;
      printf("Clock started, time is %x\n", cmdInterfaceAddr[9]);
      timeStarted = 1;
    }
  }
}
/*

  for (;;) {
    //For all the input pins, collect samples into the big buff!
    //TODO: Make pinControllers notify uC when it has new data, interrupt?
    if(runItems) {

      //This is the execution stage.
      //Special cases are required for items
      //that should stay in-flight forever.
      if(numItemsLeftToExecute > 0) {

        //Is it time to start the item at the head of the queue?
        if (currentItem->startTime <= timeMs) {
          execute(currentItem);
          if(numItemsLeftToExecute == 0) {
            if(DEBUG_PRINTING) printf("WARNING: TRIED TO SUBTRACT FROM 0 OF UNSIGNED NUMITEMS\n");
          } else {
            numItemsLeftToExecute--;
          }

          itemsInFlight[iifPos++] = currentItem;
          numItemsInFlight++;
          itaPos++;
        }

        //See if we should upate the kill time.
        if ((currentItem->endTime != -1) && currentItem->endTime < nextKillTime) {
          nextKillTime = currentItem->endTime;
        }
      }




      //Certain items in flight needs updating and "re-execution"
      if (lastTimeTick != timeTick) {
        for(unsigned int flight = 0; flight < numItemsInFlight; flight++) {
          //if(itemsInFlight[flight]->type == PINCONFIG_DATA_TYPE_PREDEFINED_SINE) {
          //execute(itemsInFlight[flight]);
          //}
          if((itemsInFlight[flight]->type == PINCONFIG_DATA_TYPE_CONSTANT_FROM_REGISTER) && (registersUpdated[itemsInFlight[flight]->constantValue]))  {
            execute(itemsInFlight[flight]);
            registersUpdated[itemsInFlight[flight]->constantValue] = 0;
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
              if((it->endTime != -1) && (it->endTime <= timeMs)) {
                if(DEBUG_PRINTING) printf("numItemsinFlight: %u\n", numItemsInFlight);
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
*/


int UsbHeaderReceived(USB_Status_TypeDef status,
    uint32_t xf,
    uint32_t remaining) 
{
  (void) remaining;
  int gotHeader = 0;

  //idle while send finishes if we're doing that.
  //This is supposed to fix the race where this routine would be called
  //before we were finished with freeing data from the old 
  //package.
  while(sendInProgress && executeInProgress);

  if ((status == USB_STATUS_OK) && (xf > 0)) {
    //Got some data, check if we have header.
    if (xf == 8) {
      uint32_t * inBuffer32 = (uint32_t *)inBuffer;
      currentPack.size = inBuffer32[0]; 
      currentPack.command = inBuffer32[1];
      currentPack.data = NULL;
      gotHeader = 1;
    } else {
      if(DEBUG_PRINTING) printf("Expected header of size 8, got %u.\n", (unsigned int)xf);
    }
  }

  //Check that we're still good, and get some data for this pack.
  if (USBD_GetUsbState() == USBD_STATE_CONFIGURED) {
    if(gotHeader) {
      if(currentPack.size > 0) {
        //Go to data collection.
        currentPack.data = (uint8_t*)malloc(currentPack.size);
        if(currentPack.data == NULL) {
          if(DEBUG_PRINTING) printf("currentPack.data null\n");
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


  if ((status == USB_STATUS_OK) && (xf > 0)) {
    //we probably sent some data :-)
    //don't free the sample buffer
    if(packToSend.command != USB_CMD_GET_INPUT_BUFFER_SIZE) {
      if(packToSend.data != NULL) {
        free(packToSend.data);
      } else {
        if(DEBUG_PRINTING) printf("Tried to free NULL-pointer\n");
      }
    }

    //Reset sample counter now.
    if(packToSend.command == USB_CMD_GET_INPUT_BUFFER){
      numSamples = 0;
      fpgaNumSamples = 0;
    }
  }
  sendInProgress = 0;
  //printf("USB DATA SENT HURRA\n");

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

inline uint32_t get_bit(uint32_t val, uint32_t bit) 
{
  return (val >> bit) & 0x1;
}


inline void execute(struct pinItem * item)
{
  /*
  uint16_t * addr = NULL;// = getChannelAddress(item->pin);
  //printf("Ex C: %d, Tstrt: %d CurT: %d\n", item->pin, item->startTime, timeMs);
  int index = 0;
  switch(item->type) {
    case PINCONFIG_DATA_TYPE_DIGITAL_OUT:
      addr = getChannelAddress(item->pin);
      uint16_t nocLow = (uint16_t)item->nocCounter;
      uint16_t nocHigh = (uint16_t)(item->nocCounter >> 16);
      if(DEBUG_PRINTING) printf("  %d DIGITAL: Digital C:%d, nocCounter: %u, ad: %p\n", timeMs, item->pin, item->nocCounter, addr);

      addr[PINCONTROL_REG_LOCAL_CMD] = CMD_RESET;
      while(addr[10] != CMD_RESET) addr[PINCONTROL_REG_LOCAL_CMD] = CMD_RESET;
      addr[PINCONFIG_NCO_COUNTER_LOW]   = nocLow;
      while(addr[10] != nocLow) addr[PINCONFIG_NCO_COUNTER_LOW]   = nocLow;
      addr[PINCONFIG_NCO_COUNTER_HIGH]  = nocHigh;
      while(addr[10] != nocHigh) addr[PINCONFIG_NCO_COUNTER_HIGH]  = nocHigh;
      addr[PINCONTROL_CMD_LOCAL_CMD] = CMD_START_OUTPUT;
      while(addr[10] != CMD_START_OUTPUT) addr[PINCONTROL_REG_LOCAL_CMD] = CMD_START_OUTPUT;
      break;

    case PINCONFIG_DATA_TYPE_DIGITAL_CONST:
      addr = getChannelAddress(item->pin);
      if(DEBUG_PRINTING) printf("  %d DIGITAL CONST: Digital C:%d, constant: %u, ad: %p\n", timeMs, item->pin, item->constantValue, addr);
      if(item->constantValue == 1) {
        if(DEBUG_PRINTING) printf( "    const high\n");
        addr[PINCONTROL_REG_LOCAL_CMD] = CMD_CONST_HIGH;
      } else {
        if(DEBUG_PRINTING) printf( "    const low\n");
        addr[PINCONTROL_REG_LOCAL_CMD] = CMD_CONST_LOW;
      }
      break;

    case PINCONFIG_DATA_TYPE_RECORD:
      if(DEBUG_PRINTING) printf("  %d RD: %d at rate %d\n", timeMs, item->pin, item->sampleRate);
      startInput();
      //    startInput(item->pin, item->sampleRate, item->endTime - item->startTime);
      break;

    case PINCONFIG_DATA_TYPE_RECORD_ANALOGUE:
      if(DEBUG_PRINTING) printf("  %d RA: %d at rate %d\n", timeMs, item->pin, item->sampleRate);
      //startInput(item->pin, item->sampleRate, item->endTime - item->startTime);
      startInput();
      break;

    case PINCONFIG_DATA_TYPE_PREDEFINED_PWM:
      if(DEBUG_PRINTING) printf("  PWM: duty %d, aduty: %d", item->duty, item->antiDuty);
      if(DEBUG_PRINTING) printf("  NOT IMPLEMENTED :(\n)");
      //addr[PINCONFIG_DUTY_CYCLE]     = (uint16_t)item->duty;
      //addr[PINCONFIG_ANTIDUTY_CYCLE] = (uint16_t)item->antiDuty;
      //addr[PINCONFIG_SAMPLE_RATE]    = (uint16_t)item->sampleRate;
      //addr[PINCONFIG_RUN_INF]        = 1;
      //addr[PINCONTROL_CMD_LOCAL_CMD] = CMD_START_OUTPUT;
      break;

    case PINCONFIG_DATA_TYPE_DAC_CONST:
      if(DEBUG_PRINTING) printf("  CONST DAC VOLTAGE,channel %u, %u\n", item->pin, item->constantValue);
      setVoltage(item->pin, item->constantValue);
      break;

    case PINCONFIG_DATA_TYPE_PREDEFINED_SINE:
      //This reeeeally doesn't work all that well. execute happens too slow?
      index = (item->sampleRate*timeTick)%255;
      setVoltage(item->pin, sinus[index]);
      break;

    case PINCONFIG_DATA_TYPE_CONSTANT_FROM_REGISTER:
      //Note: Index is coded in constantValue
      if(DEBUG_PRINTING) printf("  %d CONST REGISTER DAC VOLTAGE,channel %d, %u\n", timeMs, item->pin, (unsigned int)DACreg[item->constantValue]);
      setVoltage(item->pin, DACreg[item->constantValue]);
      break;

    default:
      break;
  }
*/
}

void killItem(struct pinItem * item)
{
  /*
  uint16_t * addr = getChannelAddress(item->pin);
  switch(item->type) {
    case PINCONFIG_DATA_TYPE_DIGITAL_OUT:
    case PINCONFIG_DATA_TYPE_DIGITAL_CONST:
      if(DEBUG_PRINTING) printf("  KILL %d : DIGITAL: Digital C:%d, duty: %d, anti: %d ad: %p\n", timeMs, item->pin, item->duty, item->antiDuty, addr);
      //addr[PINCONFIG_DUTY_CYCLE] = 0;  //TODO: FPGA will be updated with a constVal register.
      //addr[PINCONFIG_ANTIDUTY_CYCLE] = 0;  //TODO: FPGA will be updated with a constVal register.
      addr[PINCONTROL_CMD_LOCAL_CMD] = CMD_RESET;
      break;
    case PINCONFIG_DATA_TYPE_PREDEFINED_PWM:
      //addr[PINCONFIG_DUTY_CYCLE] = 0;  //TODO: FPGA will be updated with a constVal register.
      addr[PINCONTROL_CMD_LOCAL_CMD] = CMD_RESET;
      break;
    case PINCONFIG_DATA_TYPE_DAC_CONST:
      setVoltage(item->pin, 128);
      break;

    case PINCONFIG_DATA_TYPE_RECORD_ANALOGUE:
    case PINCONFIG_DATA_TYPE_RECORD:
      if(DEBUG_PRINTING) printf("  %d K R: %d at rate %d\n", timeMs, item->pin, item->sampleRate);
    for(int i = 0; i < numInputChannels; i++) {
        if (inputChannels[i] == item->pin) {
          if(DEBUG_PRINTING) printf("Rec stop: %d\n", item->pin);
          for(int j = i; j < numInputChannels; j++){
            inputChannels[j] = inputChannels[j+1];
          }
          numInputChannels--;
          if(numInputChannels == 0) {
            samplingStarted = 0;
          }
          //break;
        }
      }
      break;
    default:
      break;
  }
  */
}

inline void setupInput(FPGA_IO_Pins_TypeDef channel, int sampleRate, int duration)
{
  command(0, SAMPLE_COLLECTOR_ADDR, SAMPLE_COLLECTOR_REG_NEW_UNIT, channel);  //set up the sample collector to this channel
  //uint16_t * memctrl = (uint16_t*)getChannelAddress(242);
  //memctrl[4] = channel;
  fpgaTableToChannel[fpgaTableIndex] = (uint8_t)channel;
  if(DEBUG_PRINTING) printf("Channel %u added, index %u, table entry %d\n", channel, fpgaTableIndex, (uint8_t)fpgaTableToChannel[fpgaTableIndex]);

  //How many samples?
  //The overflow register is what decides this.
  //We set it to something that sort of achieves the wished sample rate.
  //If we want for instance 44.1KHz sample rate, the counter is
  //roughly 1701, so actual sample rate is about 44.09
  float overflow = (75000000)/((float)sampleRate);
  float numSamples = (sampleRate)*((float)duration/(float)1000);
  numSamplesPerFPGATableIndex[fpgaTableIndex] = (int)numSamples;
  fpgaNumSamples += (int)numSamples;
  fpgaTableIndex++;

  if(DEBUG_PRINTING) printf("Estimated %f samples for rate %d, of %f, channel %d, duration %d\n", numSamples, sampleRate, overflow, channel, duration);

  //If it's a ADC channel we set up the ADC here in stead of all this other faff.
  uint16_t * addr = getChannelAddress(channel);
  if ((AD_CHANNELS_START <= channel) && (channel <= (AD_CHANNELS_END))) {

    uint16_t boardChan = channel - AD_CHANNELS_START;
    //board 0
    if(boardChan < 16) {
      //Program sequence register with channel.
      adcSequence[0] |= 0xE000 | (1 << (12 - boardChan));
      if(DEBUG_PRINTING) printf("ADCregister write channel %x, %x\n", boardChan, adcSequence[0]);

      //Range register 1
      addr[0x04] = 0xAAA0; //range register written to +-5V on all channels for chans 0 to 4
      while(addr[0x0B] != 0xAAA0);
      //1101 0101 0100 0000 = D reg 1, +/- 2.5 : 
      //addr[0x04] = 0xD540; //range register written to +-2.5V on all channels for chans 0 to 4

      //Range register 2
      addr[0x04] = 0xCAA0; //range register written to +-5V on all channels for chans 4 to 7
      while(addr[0xB] != 0xCAA0);
      //1011 0101 0100 0000
      //while(addr[0x0A]);
      //addr[0x04] = 0xB540; //range register written to +-2.5V on all channels for chans 0 to 4

      //power bits.
      //addr[0x04] = 0x8014;

      //setup FPGA AD controller
      //addr[0x01] = (uint16_t)overflow; //overflow register, so this isn't sample rate at all.
      //addr[0x01] = (uint16_t)overflow; //overflow register, so this isn't sample rate at all.
      //addr[0x01] = (uint16_t)overflow; //overflow register, so this isn't sample rate at all.
      addr[0x01] = (uint16_t)overflow;
      addr[0x01] = (uint16_t)overflow;
      for (int i = 0; i < 10; i++);
      addr[0x02] = 1; //divide
      //Sequence register write.

      //while(addr[0x0A]);
      //while(addr[0x0A]);
      addr[0x04] = adcSequence[0];
      while(addr[0x0B] != adcSequence[0]);

      //Program the ADC to use this channel as well now. Result in two comp, internal ref.
      //sequencer on.
      //Control register
      //while(addr[0x0A]);
      addr[0x04] = 0x8014;
      while(addr[0x0B] != 0x8014);

      if(DEBUG_PRINTING) printf("ADC programmed to new sequences\n");
    }
  }

  //Digital channel.
  else {
    if(DEBUG_PRINTING) printf("Recording dig ch: %x\n", channel);
    command(0, channel, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_RESET);  //reset pin controllah
    command(0, channel, PINCONTROL_REG_SAMPLE_RATE, (uint32_t)overflow);
    command(0, channel, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_INPUT_STREAM);

    /*
    addr[PINCONTROL_CMD_LOCAL_CMD] = CMD_RESET;
    addr[PINCONFIG_SAMPLE_RATE] = (uint16_t)overflow;
    addr[PINCONTROL_CMD_LOCAL_CMD] = CMD_INPUT_STREAM;
    */
  }

  inputChannels[numInputChannels++] = channel;

}


inline void startInput()
{
  printf("Starting sampling\n");
  command(0, SAMPLE_COLLECTOR_ADDR, SAMPLE_COLLECTOR_REG_LOCAL_CMD, SAMPLE_COLLECTOR_CMD_START_SAMPLING);  //this sends STAT SAMPLING COMMAND
}

inline uint16_t * getChannelAddress(FPGA_IO_Pins_TypeDef channel)
{

  if(!has_daughterboard) {
    return (uint16_t*)(EBI_ADDR_BASE) + (channel * 0x100);
  }
  return (uint16_t*)(EBI_ADDR_BASE) + channel*0x100;
}



void execCurrentPack()
{
  executeInProgress = 1;
  if(currentPack.command == USB_CMD_STATUS) {
    struct mecoboStatus s;
    s.state = (uint8_t)mecoboStatus;
    s.samplesInBuffer = (uint16_t)numSamples;
    s.itemsInQueue = (uint16_t)numItemsLeftToExecute + numItemsInFlight;
    //printf("QI: %u\n", numItemsInFlight);

    sendPacket(sizeof(struct mecoboStatus), USB_CMD_GET_INPUT_BUFFER_SIZE, (uint8_t*)&s);
  }

  else if(currentPack.command == USB_CMD_CONFIG_PIN) {
    struct pinItem item;
    if(currentPack.data != NULL) {
      uint32_t * d = (uint32_t *)(currentPack.data);
      item.pin = (d[PINCONFIG_DATA_FPGA_PIN]);  //pin is channel, actually
      item.duty = d[PINCONFIG_DATA_DUTY];
      item.antiDuty   = d[PINCONFIG_DATA_ANTIDUTY];
      item.startTime  = 75000*d[PINCONFIG_START_TIME];
      item.endTime    = 75000*d[PINCONFIG_END_TIME];
      item.constantValue = d[PINCONFIG_DATA_CONST];
      item.type = d[PINCONFIG_DATA_TYPE];
      item.sampleRate = d[PINCONFIG_DATA_SAMPLE_RATE];
      item.nocCounter = d[PINCONFIG_DATA_NOC_COUNTER];

      //itemsToApply[itaHead++] = item;
      fifoInsert(&cmdFifo, &item);

      numItemsLeftToExecute++;
      if(DEBUG_PRINTING) printf("Item %d added to pin %d, starting at %x, ending at %x, samplerate %d\n", numItemsLeftToExecute, item.pin, item.startTime, item.endTime, item.sampleRate);
    } else {
      if(DEBUG_PRINTING) printf("Curr data NULL\n");
    }
    //Recording pins need some setup
    if((item.type == PINCONFIG_DATA_TYPE_RECORD_ANALOGUE) || (item.type == PINCONFIG_DATA_TYPE_RECORD)) {
      setupInput(item.pin, item.sampleRate, (item.endTime-item.startTime)/75000);
    }

  }

  if(currentPack.command == USB_CMD_RUN_SEQ)
  {
    runItems = 1;
    timeTick = 0;
    lastTimeTick = 0;
    timeMs = 0;
    itaPos = 0;
  }

  if(currentPack.command == USB_CMD_PROGRAM_XBAR)
  {
    return;
    mecoboStatus = MECOBO_STATUS_BUSY;
    if(DEBUG_PRINTING) printf("Configuring XBAR\n");
    uint16_t * d = (uint16_t*)(currentPack.data);
    for(int i = 0; i < 32; i++) {
      xbar[i] = d[i];
      //printf("XBAR: Word %d: %x\n", i, d[i]); 
    }

    xbar[0x20] = 0x1; //whatever written to this register will be interpreted as a cmd.
    USBTIMER_DelayMs(3);
    int waitCount = 0;
    while(xbar[0x0A]) {
      USBTIMER_DelayMs(1);
      waitCount++;
      if(waitCount == 100) {
        printf("WARNING: Timeout. Continuing.\n");
        xbar[0x20] = 0x1; //whatever written to this register will be interpreted as a cmd.
        continue;
      }
    }


    if(DEBUG_PRINTING) printf("\nXBAR configured\n");
    mecoboStatus = MECOBO_STATUS_READY;
  }


  if(currentPack.command == USB_CMD_UPDATE_REGISTER)
  {
    if(DEBUG_PRINTING) printf("Updating Register\n");
    int * d = (int*)(currentPack.data);
    if(DEBUG_PRINTING) printf("REG %d: %d\n", d[0], d[1]); 
    DACreg[d[0]] = d[1];
    registersUpdated[d[0]] = 1;
  }


  if(currentPack.command == USB_CMD_LED) {
    /* Start output from pin controllers */
    uint32_t * d = (uint32_t *)(currentPack.data);
    led(d[LED_SELECT], d[LED_MODE]);
  }

  if(currentPack.command == USB_CMD_RESET_ALL) {
    mecoboStatus = MECOBO_STATUS_BUSY;

    
       fpgaTableIndex = 0;
    command(0, 242, 5, 5);  //reset sample collector

    runItems = 0;
    fpgaNumSamples = 0;
    if(DEBUG_PRINTING) printf("Reset called! Who answers?\n");
    //Reset state
    itaPos = 0;
    numItemsLeftToExecute = 0;
    iifPos = 0;
    numItemsInFlight = 0;
    numSamples = 0;
    numInputChannels = 0;
    nextKillTime = 0;
    adcSequence[0] = 0;

    timeTick = 0;
    lastTimeTick = 0;
    timeMs = 0;

    for(int q = 0; q < 8; q++) {
      numSamplesPerFPGATableIndex[q] = 0;
      fpgaTableToChannel[q] = 0;
    }



    if(has_daughterboard) {
      for(int r = 0; r < 4; r++) {
        adcSequence[r] = 0xE000;
      }

      for(uint32_t i = 0; i < NUM_DAC_REGS; i++ ) {
        DACreg[i] = 128;
        registersUpdated[i] = 0;
      }

      for(unsigned int i = DA_CHANNELS_START; i < DA_CHANNELS_START+8; i++) {
        setVoltage(i, 128);
      }

      for(int i = 0; i < 32; i++) {
        xbar[i] = 0;
      }

      xbar[0x20] = 0x1; //whatever written to this register will be interpreted as a cmd.
      USBTIMER_DelayMs(3);
      int waitCount = 0;
      while(xbar[0x0A]) { 
        if(DEBUG_PRINTING) printf("Waiting for XBAR\n"); 
        USBTIMER_DelayMs(1);
        waitCount++;
        if(waitCount == 100) {
          printf("WARNING: Timeout. Continuing.\n");
          xbar[0x20] = 0x1; //whatever written to this register will be interpreted as a cmd.
          continue;
        }
      }; //hang around until command completes.
    }

    resetAllPins();

    //make sure clock is running here 
    uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;
    cmdInterfaceAddr[8] = 0xDEAD;
    while (!(cmdInterfaceAddr[0] & STATUS_REG_CMD_FIFO_EMPTY));
    cmdInterfaceAddr[7] = 0xDEAD;
    timeStarted = 0;

    printf("\n\n---------------- MECOBO RESET DONE ------------------\n\n");
    mecoboStatus = MECOBO_STATUS_READY;
  

  }

  if(currentPack.command == USB_CMD_GET_INPUT_BUFFER_SIZE) {
    //printf("SHIPPING numSamples: %d\n", numSamples);
    if(DEBUG_PRINTING) printf("Sending %u samples back\n", fpgaNumSamples);

    sendPacket(4, USB_CMD_GET_INPUT_BUFFER_SIZE, (uint8_t*)&fpgaNumSamples);

  }

  if(currentPack.command == USB_CMD_GET_INPUT_BUFFER) {

    uint16_t * memctrl = (uint16_t*)EBI_ADDR_BASE;// getChannelAddress(0); //this is the EBI interface

    //Send back the whole sending buffer.
    //Data contains the number of samples to xfer.
    uint32_t * txSamples = (uint32_t *)(currentPack.data);
    int bytes = sizeof(struct sampleValue) * *txSamples;
    struct sampleValue * samples = (struct sampleValue *)malloc(bytes);

    for(unsigned int i = 0; i < *txSamples;) {
     // uint16_t data = memctrl[1];
      //Read status register
      //if (!(memctrl[0] & STATUS_REG_SAMPLE_FIFO_EMPTY)) {
      //if (!sampleFifoEmpty) {
        uint16_t data = memctrl[6]; //get next sample from EBI INTERFACE
        //printf("m: %x\n", memctrl[0]);
        uint8_t tableIndex = (data >> 13); //top 3 bits of word is index in fpga controller fetch table
        //if(DEBUG_PRINTING)
        //printf("Got data %x from channel %x\n", data, fpgaTableIndex);

        if(i < 50)  {
          if(DEBUG_PRINTING) printf("fpga-data: %x chan: %d\n", data, fpgaTableToChannel[tableIndex]);
        }

        samples[i].sampleNum = i;
        samples[i].channel = fpgaTableToChannel[tableIndex];
        samples[i].value = data;

        i++;
      //}
    }

    sendPacket(bytes, USB_CMD_GET_INPUT_BUFFER, (uint8_t*)samples);

    if(DEBUG_PRINTING) printf("USB has been instructed to send data. Returning to main loop\n");
  }



  if(currentPack.command == USB_CMD_LOAD_BITFILE) {
    if(bitfileOffset == 0) {
      eraseNorChip();  //we want to chuck in a new file, kill this.
    }

    autoSelectNor();
    //resetNor();
    enterEnhancedMode();
    //We send packs in neat multiple-of 512 byte packages,
    //so we can load 256 words (2bytes) a time into the NOR. 
    uint16_t * p = (uint16_t*)currentPack.data;
    int wordsInPack = currentPack.size/2;

    for(int writes = 0; writes < (wordsInPack/256); writes++) {
      write256Buffer(p + (writes*256), bitfileOffset);
      bitfileOffset += 256;
    }

    exitEnhancedMode();
    //word offsets
    free(currentPack.data);
    printf("Got data, current offset %u\n", (unsigned int)bitfileOffset);
  }


  if(currentPack.command == USB_CMD_PROGRAM_FPGA) {
    programFPGA();
    bitfileOffset = 0;
  }

  //if/else done.
  executeInProgress = 0;
}

void sendPacket(uint32_t size, uint32_t cmd, uint8_t * data)
{
  struct mecoPack pack;
  pack.size = size;
  pack.command = cmd;
  pack.data = data;

  packToSend = pack; //This copies the pack structure.
  //printf("Writing back %u bytes over USB.\n", (unsigned int)pack.size);
  sendInProgress = 1;
  USBD_Write(EP_DATA_IN1, packToSend.data, packToSend.size, UsbDataSent);
}

void resetAllPins()
{
  printf("Reseting all digital pin controllers\n");
  for(int j = 0; j < 10; j++) {
    command(0, j, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_RESET);
    //uint16_t * addr = (getChannelAddress((j)));
    //addr[PINCONTROL_REG_LOCAL_CMD] = PINCONTROL_CMD_RESET;
  }
  printf("OK\n");
}

void led(int l, int mode)
{
  switch(l) {
    case FRONT_LED_0:
      GPIO_PinModeSet(gpioPortD, 8, gpioModePushPull, 1-mode);  //Led 1
      break;
    case FRONT_LED_1:
      GPIO_PinModeSet(gpioPortD, 6, gpioModePushPull, 1-mode);  //Led 2
      break;
    case FRONT_LED_2:
      GPIO_PinModeSet(gpioPortD, 13, gpioModePushPull, 1-mode);  //Led 4
      break;
    case FRONT_LED_3:
      GPIO_PinModeSet(gpioPortC, 5, gpioModePushPull, 1-mode);  //Led 3
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



void programFPGA()
{
  autoSelectNor(); //set NOR in read mode
  printf("Programming FPGA!\n");
  //Start the configuration process.
  //set the input pin modes.
  //DONE, active high
  GPIO_PinModeSet(gpioPortC, 4, gpioModeInput, 0); 
  //INIT_B = PD15, active _low_.
  GPIO_PinModeSet(gpioPortD, 15, gpioModeInput, 0); 
  GPIO_PinModeSet(gpioPortC, 3, gpioModePushPull, 0);
  //Prog pins...
  GPIO_PinModeSet(gpioPortA, 7, gpioModePushPull, 0);   //data
  GPIO_PinModeSet(gpioPortA, 8, gpioModePushPull, 0);   //clk


  //start programming (prog b to low)
  GPIO_PinOutClear(gpioPortC, 3);

  while(GPIO_PinInGet(gpioPortD, 15) ||
      GPIO_PinInGet(gpioPortC, 4)) {
    if(DEBUG_PRINTING) printf("Waiting for initb and done\n");
  }

  while(GPIO_PinInGet(gpioPortD, 15)) {
    printf("Waiting for INIT_B to go high\n");
  }

  //set prog b high again (activate programming)
  GPIO_PinOutSet(gpioPortC, 3);
  //wait until initB goes high again (fpga ready for data)
  while(!GPIO_PinInGet(gpioPortD, 15)) {
    printf("Waiting for initb to go high again\n");
  }
  //Bring front led 2 on to indicate programming.

  GPIO_PinModeSet(gpioPortD, 6, gpioModeInput, 1);

  //uint32_t * n32 = (uint32_t *)(NOR_START + 4); //this is where we store the length
  //uint32_t bitfileLength = *n32; //stored at the 4 bytes
  //int nb = 0;


  struct NORFileTableEntry * entries = (struct NORFileTableEntry *)NOR_START;
  printf("Found a bitfile, size %u\n", (unsigned int)entries[0].size);
  //TODO: Select which bitfile to use here. For now,
  //assume only 1 bitfile present.
  uint8_t * nor = (uint8_t *)(NOR_START + entries[0].offset); //this is where data starts
  //printf("Programming with %u bytes\n", (unsigned int)bitfileLength);
  //  for(int i = 0; i < bitfileLength; i++) {

  uint32_t nb = 0;
  while(nb < entries[0].size) {
    uint8_t bait = nor[nb];

    //printf("wat\n");
    for(int b = 7; b >= 0; b--) {
      GPIO_PinOutClear(gpioPortA, 8); //clk low
      //clock a bit.
      if ((bait >> b) & 0x1) {
        GPIO_PinOutSet(gpioPortA, 7); 
      } else {
        GPIO_PinOutClear(gpioPortA, 7); 
      }
      GPIO_PinOutSet(gpioPortA, 8); //clk high
    }
    nb++;
    if((nb%(10*1024)) == 0) {
      printf("Last byte programmed: %x\n", bait);
      if(!GPIO_PinInGet(gpioPortD, 15)) {
        printf("CRC error during config, INITB went high!\n");
      } else {
        printf("BYTE OK\n");
      }
    }
  }

  /*
     while(!GPIO_PinInGet(gpioPortC, 4)) {
     printf("Waiting for DONE to go high\n");
     }
     */

  printf("FPGA programming done\n");
}

void eraseNorChip()
{
  uint16_t * nor = (uint16_t *)NOR_START;
  printf("Doing chip erase\n");
  nor[0x555] = 0xCA;
  nor[0x2AA] = 0x35;
  nor[0x555] = 0x80;
  nor[0x555] = 0xCA;
  nor[0x2AA] = 0x35;
  nor[0x555] = 0x10;

  //while(NORPollData(1, 0));
  NORBusy();

  printf("Done\n");
}



void NORBusy() {

  uint16_t * nor = (uint16_t *)NOR_START;
#define TOGGLE_BIT 0x20
  uint8_t status = 0;
  uint8_t done = 0;
  int counter = 0;
  while(!done) {
    status = nor[0];
    //read once more
    if((nor[0] & TOGGLE_BIT) == (status & TOGGLE_BIT)) {
      done = 1;
    }
    if (((counter++)%100000) == 0) {
      printf("NOR busy... %x\n", status);
    }
  }
  printf("Final status %x\n", nor[0]);

  return;
}

void resetNor()
{
  printf("NOR RESET\n");
  uint16_t * nor = (uint16_t *)NOR_START;
  nor[0x555] = 0xF0;
}

void returnToRead()
{
  uint8_t * nor = (uint8_t *)NOR_START;
  nor[0x555] = 0xCA;
  nor[0x2AA] = 0x35;
  nor[0x123] = 0xF0;
}

void unlockByPass()
{
  uint8_t * nor = (uint8_t *)NOR_START;
  nor[0x555] = 0xA0;
  nor[0x2AA] = 0xA0;
  nor[0x555] = 0x20;
}

void autoSelectNor()
{
  uint16_t * nugg = (uint16_t*)NOR_START;
  nugg[0x55] = 0x90; //auto select
}


void enterEnhancedMode()
{
  printf("Entering enhanced mode\n");
  uint16_t * nor = (uint16_t *)NOR_START;
  //Enter enhanced mode
  nor[0x555] = 0xCA;
  nor[0x2AA] = 0x35;
  nor[0x555] = 0x58;
}

void exitEnhancedMode()
{
  uint16_t * nor = (uint16_t *)NOR_START;
  /* Exit enhanced program command */
  nor[0] = 0x90;
  nor[0] = 0x00;
  printf("Exit enhanced mode\n");
}


/* Writes 256 16-bit words of data to the NOR */
void write256Buffer(uint16_t * data, uint32_t offset)
{
  uint16_t * nor = (uint16_t *)NOR_START;
  /* Set in auto mode select mode */
  //autoSelectNor();

  uint32_t bad = offset & 0xFFFFFF00;
  printf("Writing 512 bytes from data %p at offset 0x%x, block address %x\n", data, (unsigned int)offset, (unsigned int)bad);

  //Now start command
  nor[bad] = 0x53;

  for (int i = 0; i < 256; i++) {
    nor[offset + i] = data[i];
  }


  nor[bad] = 0x49;

  //Wait until we no longer toggle
  while(NORToggling());

  //NORError();

  printf("Wrote bytes\n");
}


#define TOGGLE_BIT 0x20
int NORToggling() 
{
  uint16_t * nor = (uint16_t *)NOR_START;
  uint16_t a = nor[0];
  uint16_t b = nor[0];

  if ((a & TOGGLE_BIT) != (b & TOGGLE_BIT)) {
    //printf("TOGGLECHECK: %x and %x\n", a, b);
    return 1;
  }
  return 0;
}

#define DATA_BIT 0x80
int NORPollData(uint16_t writtenWord, uint32_t addr)
{
  uint16_t * nor = (uint16_t *)NOR_START;
  if ((nor[addr] & DATA_BIT) != (writtenWord & DATA_BIT)) {
    return 1;
  }
  return 0;
}

#define ERROR_BIT 0x40
int NORError() {
  uint16_t * nor = (uint16_t *)NOR_START;
  uint16_t status = nor[0];
  if(status & ERROR_BIT) {
    printf("Error detected. Status reg: %x\n", status);
    return 1;
  }
  else return 0;
}
/*
   void parseNORFileTable(int * numEntries, struct NORFileTableEntry * entries)
   {
//printf("Parsing nor file table\n");
//struct NORFileTableEntry * nte = (struct NORFileTableEntry *)NOR_START;
//size of 0 indicates invalid entry
//int entry = 0;
// entries[entry] = *nte;
//  printf("Entry size %u, offset %u\n", entries[0].size, entries[0].offset);
}
*/

void fpgaIrqHandler(uint8_t pin) {
  uint16_t statusReg = *getChannelAddress(0);
  printf("Interrupt detected: %x\n", pin);
  printf("Status reg: %x\n", statusReg);
  if (statusReg & STATUS_REG_CMD_FIFO_ALMOST_FULL_BIT) {
    printf("FIFO almost full\n");
    feedCmdFifo = 0;
  }

  if (statusReg & STATUS_REG_CMD_FIFO_EMPTY) {
    printf("CMD FIFO empty\n");
    feedCmdFifo = 1;
  }

  if (statusReg & STATUS_REG_CMD_FIFO_ALMOST_EMPTY) {
    printf("CMD FIFO almost empty\n");
    feedCmdFifo = 1;
  }
  if (statusReg & STATUS_REG_SAMPLE_FIFO_ALMOST_EMPTY) {
    printf("SAMPLE FIFO almost empty\n");
  }

  if (statusReg & STATUS_REG_SAMPLE_FIFO_EMPTY) {
    printf("SAMPLE FIFO empty\n");
    sampleFifoEmpty = 1;
  }

}


inline void putInFifo(struct fifoCmd * cmd) 
{
  
  uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;
  uint16_t * serialCmd = (uint16_t *)cmd;
  //This will be funked up when splitting the uint32 into 16 bits,
  //because the ordering is different.
  //To get the value 0xdeadbeef into the 32 bit reg (BE on meco), 
  //we must first write 0xdead and then 0xbeef. However,
  //serialCmd[0] is beef and serialCmd[1] is dead because of little endians.
  //so. we order manually, like so

  cmdInterfaceAddr[1] = serialCmd[1];
  cmdInterfaceAddr[2] = serialCmd[0];
  cmdInterfaceAddr[3] = serialCmd[3];
  cmdInterfaceAddr[4] = serialCmd[2];
  cmdInterfaceAddr[5] = serialCmd[4];  // I've ordered the struct so that this is OK :-)

  printf("pushing word %u : %x\n", 0, serialCmd[1]);
  printf("pushing word %u : %x\n", 1, serialCmd[0]);
  printf("pushing word %u : %x\n", 2, serialCmd[3]);
  printf("pushing word %u : %x\n", 3, serialCmd[2]);
  printf("pushing word %u : %x\n", 4, serialCmd[4]);
}



void command(uint32_t startTime, uint8_t controller, uint8_t reg, uint32_t data) {
  struct fifoCmd cmd;
  cmd.startTime = (uint32_t)(startTime);
  cmd.controller = controller;
  cmd.addr = reg;
  cmd.data = data;
  putInFifo(&cmd);
}


inline struct fifoCmd makeCommand(uint32_t startTime, uint8_t controller, uint8_t reg, uint32_t data) 
{
  struct fifoCmd command;
  command.startTime = startTime;
  command.controller = controller;
  command.addr = reg;
  command.data = data;
  return command;
}

void pushToCmdFifo(struct pinItem * item)
{
  //struct fifoCmd cmd = makeCommand(item->startTime, (uint8_t)item->pin, 0x0, 0x0);

  printf("s: %u, pin %u\n", (unsigned int)item->startTime, (unsigned int)item->pin);
  switch(item->type) {
    case PINCONFIG_DATA_TYPE_DIGITAL_OUT:

      //why do i do this here...
      //cmd = makeCommand(0, 0xFF, 0xFF, 0x00);
      //command(0, 0xFF, 0xFF, 0);

      command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_NCO_COUNTER, (uint32_t)item->nocCounter);
      command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_END_TIME, (uint32_t)item->endTime);
      command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_START_OUTPUT);

      uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;
      printf("tl: %x\n", cmdInterfaceAddr[9]);
      printf("th: %x\n", cmdInterfaceAddr[10]);
 

      break;

    case PINCONFIG_DATA_TYPE_RECORD_ANALOGUE:
    case PINCONFIG_DATA_TYPE_RECORD:
      startInput();
      printf("rtl: %x\n", cmdInterfaceAddr[9]);
      printf("rth: %x\n", cmdInterfaceAddr[10]);
      break; 

    default:
      break;
  }
}

