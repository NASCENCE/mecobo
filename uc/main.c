#include "em_device.h"
#include "em_cmu.h"
#include "em_dma.h"
#include "em_gpio.h"
#include "em_int.h"
#include "dmactrl.h"
#include "em_usb.h"
#include "em_usart.h"
#include "em_ebi.h"
#include "bsp.h"
#include "bsp_trace.h"



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "queue.h"
#include "mecoprot.h"
#include "mecobo.h"

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

void eADesigner_Init(void);
/*** Typedef's and defines. ***/

/* Define USB endpoint addresses */
#define EP_DATA_OUT1       0x01  /* Endpoint for USB data reception.       */
#define EP_DATA_OUT2       0x02  /* Endpoint for USB data reception.       */
#define EP_DATA_IN1        0x81  /* Endpoint for USB data transmission.    */
#define EP_DATA_IN2        0x82  /* Endpoint for USB data transmission.    */
#define EP_NOTIFY         0x82  /* The notification endpoint (not used).  */

#define BULK_EP_SIZE     USB_MAX_EP_SIZE  /* This is the max. ep size.    */
#define EBI_ADDR_BASE 0x80000000

#include "descriptors.h"

struct queue dataIn;


//Receiving buffer. 
struct queue dataInBuffer;

int packNum = 0;
//Try again.
static uint32_t inBufferTop;
static uint8_t * inBuffer;

//sending buffer
static uint32_t outBufferTop;
static uint8_t * outBuffer;

static struct mecoPack currentPack;
static struct mecoPack packToSend;
static int sendPackReady;

//Are we programming the FPGA
int fpgaUnderConfiguration = 0;

EBI_Init_TypeDef ebiConfig = {   
      
      ebiModeD16,      /* 8 bit address, 8 bit data */  \
      ebiActiveHigh,     /* ARDY polarity */              \
      ebiActiveHigh,     /* ALE polarity */               \
      ebiActiveHigh,     /* WE polarity */                \
      ebiActiveHigh,     /* RE polarity */                \
      ebiActiveHigh,     /* CS polarity */                \
      ebiActiveHigh,     /* BL polarity */                \
      false,            /* enable BL */                  \
      false,            /* enable NOIDLE */              \
      false,            /* enable ARDY */                \
      false,            /* don't disable ARDY timeout */ \
      EBI_BANK0,        /* enable bank 0 */              \
      EBI_CS0,          /* enable chip select 0 */       \
      0,                /* addr setup cycles */          \
      1,                /* addr hold cycles */           \
      false,            /* do not enable half cycle ALE strobe */ \
      0,                /* read setup cycles */          \
      0,                /* read strobe cycles */         \
      0,                /* read hold cycles */           \
      false,            /* disable page mode */          \
      false,            /* disable prefetch */           \
      false,            /* do not enable half cycle REn strobe */ \
      0,                /* write setup cycles */         \
      2,                /* write strobe cycles */        \
      0,                /* write hold cycles */          \
      false,            /* do not disable the write buffer */ \
      false,            /* do not enable halc cycle WEn strobe */ \
      ebiALowA0,        /* ALB - Low bound, address lines */ \
      ebiAHighA18,       /* APEN - High bound, address lines */   \
      ebiLocation1,     /* Use Location 0 */             \
      true,             /* enable EBI */                 \
  };



/**************************************************************************//**
 * @brief main - the entrypoint after reset.
 *****************************************************************************/


int main(void)
{
    eADesigner_Init();

    setupSWOForPrint();
    printf("Hello world, I'm alive.\n");
    printf("Initializing EBI\n");

    EBI_Init(&ebiConfig);
 
    GPIO_PinModeSet(gpioPortA, 10, gpioModePushPull, 1);  //Led U2
    GPIO_PinModeSet(gpioPortD,  0, gpioModePushPull, 0);  //LED U3

    /*(
    while(1) {
      *((uint16_t *)EBI_ADDR_BASE + 0) = 0x99; 
      *((uint16_t *)EBI_ADDR_BASE + 1) = 0xAA; 
      *((uint16_t *)EBI_ADDR_BASE + 2) = 0xBB; 
      *((uint16_t *)EBI_ADDR_BASE + 3) = 0xCC; 
      *((uint16_t *)EBI_ADDR_BASE + 4) = 0xDD; 

      *((uint16_t *)EBI_ADDR_BASE + 256) = 0xFF; 
    }*/
    //*(((uint16_t *)EBI_ADDR_BASE) + 256 + 6) = 1; //capture rate (every 10)
    //*(((uint16_t *)EBI_ADDR_BASE) + 256 + 8) = 3; //pin mode
    //*(((uint16_t *)EBI_ADDR_BASE) + 256 + 5) = 3; //local command start capt


/*
    while(1) {
        if ((*((uint16_t *)EBI_ADDR_BASE + 256 + 7)) == 1) {
            GPIO_PinOutSet(gpioPortD, 0);
        } else {
            GPIO_PinOutClear(gpioPortD, 0);
        }
    }
*/
    inBuffer = (uint8_t*)malloc(32*1024);
    inBufferTop = 0;
   

    outBufferTop = 0;
    outBuffer = (uint8_t*)malloc(32*1024);

    //Build the pin map
    //buildMap(routeThroughMap);

    printf("Initializing USB\n");
    USBD_Init(&initstruct);
  
    //Release fpga reset
    GPIO_PinOutClear(gpioPortB, 4);

    /*
     * When using a debugger it is practical to uncomment the following three
     * lines to force host to re-enumerate the device.
    */
    USBD_Disconnect();
    USBTIMER_DelayMs(100);
    USBD_Connect();

    //Put FPGA out of reset
    GPIO_PinModeSet(gpioPortB, 5, gpioModePushPull, 1);  
    GPIO_PinOutSet(gpioPortB, 5); //Reset
    GPIO_PinOutClear(gpioPortB, 5); //Reset clear


    printf("USB Started, entering loop\n");
    sendPackReady = 0;
    //read and write in loop	
    for (;;)
    {
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
      if(sendPackReady) {
        USBD_Write(EP_DATA_IN1, packToSend.data, packToSend.size, UsbDataSent);
      }
    }
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
            if (inBuffer[0] == 0xa) {

                uint32_t * inBuffer32 = (uint32_t *)inBuffer;
                currentPack.size = inBuffer32[1]; //TODO: blaaaah

                currentPack.command = inBuffer[3];
                currentPack.data = malloc(currentPack.size);
            } 
            gotHeader = 1;
        } else {
            //Some kind of error here.
        }
    }

    //Check that we're still good, and get some data for this pack.
    if (USBD_GetUsbState() == USBD_STATE_CONFIGURED) {
        if(gotHeader) {
            USBD_Read(EP_DATA_OUT1, currentPack.data, currentPack.size, UsbDataReceived);
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
  uint16_t offset = p->fpgaPin << 8; //8 MSB bits is pinConfig module addr
  uint16_t * pin = ((uint16_t*)EBI_ADDR_BASE) + offset;

  *(pin + PINCONFIG_DUTY_CYCLE)     = p->duty;
  *(pin + PINCONFIG_ANTIDUTY_CYCLE) = p->antiduty;
  *(pin + PINCONFIG_CYCLES)         = p->cycles;
  *(pin + PINCONFIG_RUN_INF)        = p->runInf;
  *(pin + PINCONFIG_SAMPLE_RATE)    = p->sampleRate;

  //printf("Configured pin %u, address %p, duty %u, antiduty %u, inf: %u\n", (int)p->fpgaPin, (void *)pin, (int)p->duty, (int)p->antiduty, 1);
  //TODO: support everything :-)
  return 0;
}


int UsbDataReceived(USB_Status_TypeDef status,
                            uint32_t xf,
                            uint32_t remaining) 
{
    (void) remaining;
    if ((status == USB_STATUS_OK) && (xf > 0)) {

        if(currentPack.command == USB_CMD_CONFIG_PIN) {
          struct pinConfig conf;
          uint32_t * d = (uint32_t *)(currentPack.data);
          conf.fpgaPin = d[PINCONFIG_DATA_FPGA_PIN];
          conf.duty = d[PINCONFIG_DATA_DUTY];
          conf.antiduty = d[PINCONFIG_DATA_ANTIDUTY];
          conf.cycles = d[PINCONFIG_DATA_CYCLES]; 
          conf.sampleRate = d[PINCONFIG_DATA_SAMPLE_RATE];
          conf.runInf = d[PINCONFIG_DATA_RUN_INF];

          //PinVal is stored in uC-internal per-pin register
          fpgaConfigPin(&conf); 
        }

        if(currentPack.command == USB_CMD_START_OUTPUT) {
          /* Start output from pin controllers */
          uint32_t * d = (uint32_t *)(currentPack.data);
          startOutput(d[PINCONFIG_DATA_FPGA_PIN]);
        }
        
        if(currentPack.command == USB_CMD_READ_PIN) {
          struct mecoPack pack;
          pack.size = 2; //send back the same amount of data
          pack.command = currentPack.command;
          pack.data = malloc(2);

          uint16_t ret = getInput(*currentPack.data);
          pack.data = memcpy(&ret, pack.data, 2);
          //ship it!
          packToSend = pack; 
          sendPackReady = 1; 
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

          printf("Sent %d bytes to FPGA, packet %d\n", nb, packNum);
        }

        currentPack.command = 0;
        currentPack.size = 0;
        free(currentPack.data);
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
        sendPackReady = 0;  //reset this for next packet.
        free(packToSend.data);
    }

    return USB_STATUS_OK;
}

/*
void DmaUsbRxDone(unsigned int channel, int primary, void *user)
{
    (void) channel;
    (void) primary;
    (void) user;

    INT_Disable();
    INT_Enable();
}

int setupDma(void)
{
    DMA_Init_TypeDef dmaInit;
    DMA_CfgChannel_TypeDef chnlCfgTx, chnlCfgRx;
    DMA_CfgDescr_TypeDef   descrCfgTx, descrCfgRx;

    //Init DMA in general.
    dmaInit.hprot = 0;
    dmaInit.controlBlock = dmaControlBlock; //where channel config data is stored
    DMA_Init(&dmaInit);

    //Channel 0 for USB Rx
    //Callback function when Rx occurs
    DmaUsbRxCB.cbFunc = DmaUsbRxDone;
    DmaUsbRxCB.userPtr = NULL; //no user data for function.

    chnlCfgRx.highPri = false;
    chnlCfgRx.enableInt = true;
    chnlCfgRx.select    = USB_GRSTCTL_DMAREQ;
    chnlCfgRx.cb = &DmaUsbRxCB;
    DMA_CfgChannel(0, &chnlCfgRx);

    //Channel descriptor
    descrCfgRx.dstInc = dmaDataInc1;
    descrCfgRx.srcInc = dmaDataIncNone;
    descrCfgRx.size = dmaDataSize1;

    descrCfgRx.arbRate = dmaArbitrate1;
    descrCfgRx.hprot = 0;

    DMA_CfgDescr(1, true, &descrCfgRx);

}
*/


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


void startOutput(FPGA_IO_Pins_TypeDef pin)
{
  //TODO: Fix CMD_CONFIG_PIN_NAME
  uint16_t * addr = getPinAddress(pin);
  addr[PINCONFIG_LOCAL_CMD]= CMD_CONFIG_PIN;
}

uint16_t getInput(FPGA_IO_Pins_TypeDef pin)
{
  uint16_t * addr = getPinAddress(pin);
  addr[PINCONFIG_LOCAL_CMD] = CMD_INPUT_STREAM;

  return addr[PINCONFIG_SAMPLE_REG];
}

uint16_t * getPinAddress(FPGA_IO_Pins_TypeDef pin)
{
  //TODO: Ignoring enum type... probably 32 bit int, but..
  uint16_t offset = pin << 8; //8 MSB bits is pinConfig module addr
  return ((uint16_t*)EBI_ADDR_BASE) + offset;
}
