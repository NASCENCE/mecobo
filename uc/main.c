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

#include <stdlib.h>
#include <string.h>
#include "mecobo.h"
#include "queue.h"
#include "mecoprot.h"


void ebi_gpio_setup(void);
/*** Typedef's and defines. ***/

/* Define USB endpoint addresses */
#define EP_DATA_OUT1       0x01  /* Endpoint for USB data reception.       */
#define EP_DATA_OUT2       0x02  /* Endpoint for USB data reception.       */
#define EP_DATA_IN1        0x81  /* Endpoint for USB data transmission.    */
#define EP_DATA_IN2        0x82  /* Endpoint for USB data transmission.    */
#define EP_NOTIFY         0x82  /* The notification endpoint (not used).  */

#define BULK_EP_SIZE     USB_MAX_EP_SIZE  /* This is the max. ep size.    */

#include "descriptors.h"

struct queue dataIn;

static DMA_CB_TypeDef DmaUsbRxCB;
static int usbTxActive, usbRxActive;

//Receiving buffer. 
struct queue dataInBuffer;

//Try again.
static uint32_t inBufferTop;
static uint8_t * inBuffer;

//sending buffer
static uint32_t outBufferTop;
static uint8_t * outBuffer;

static struct mecoPack currentPack;
static struct mecoPack packToSend;
static int sendPackReady;

//Store some pin configs
//static struct pinConfig pinConfigs[12];

//Temporary map for routing some pins for a early experiment.
struct ucPin routeThroughMap[40];




/* Configure TFT direct drive from EBI BANK2 */
static const EBI_Init_TypeDef fpgaEBI =
{
};



/**************************************************************************//**
 * @brief main - the entrypoint after reset.
 *****************************************************************************/




int main(void)
{
    CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
   
    CMU_ClockEnable(cmuClock_GPIO, true);
    //GPIO_PinModeSet(gpioPortE, 8, gpioModePushPull, 0); 
    //GPIO_PinModeSet(gpioPortE, 9, gpioModeInput, 0); 
   
        /* Setup DMA */
//    setupDma();
    //Initialize a queue to 2K
    //queueInit(&dataInBuffer, 1024*2);
    ebi_gpio_setup(); //enable GPIO bus.
    //Init EBI.
    //EBI_Init();

    //And start writing. Nothing more to it!

    inBufferTop = 0;
    inBuffer = (uint8_t*)malloc(32*1024);
   
    outBufferTop = 0;
    outBuffer = (uint8_t*)malloc(32*1024);

    //Build the pin map
    buildMap(routeThroughMap);

    USBD_Init(&initstruct);

    /*
     * When using a debugger it is practical to uncomment the following three
     * lines to force host to re-enumerate the device.
    */
    USBD_Disconnect();
    USBTIMER_DelayMs(100);
    USBD_Connect();


    sendPackReady = 0;
    //read and write in loop	
    for (;;)
    {
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

//This is a temporary map for this experiment
void buildMap(struct ucPin * map)
{
  //This here is really the FPGA_DATA bus, but I'm just using it.
  map[FPGA_C14] = (struct ucPin){.port = gpioPortE, .pin = 8};  //FPGADATA0
  map[FPGA_F13] = (struct ucPin){.port = gpioPortE, .pin = 9};  //1
  map[FPGA_F12] = (struct ucPin){.port = gpioPortE, .pin = 10};  //2
  map[FPGA_D12] = (struct ucPin){.port = gpioPortE, .pin = 11}; //3
  map[FPGA_C11] = (struct ucPin){.port = gpioPortE, .pin = 12}; //4
  map[FPGA_F11] = (struct ucPin){.port = gpioPortE, .pin = 14}; //5
  map[FPGA_G11] = (struct ucPin){.port = gpioPortE, .pin = 13}; //6
  map[FPGA_D9] = (struct ucPin){.port = gpioPortE, .pin = 15}; //7 
  map[FPGA_F9] = (struct ucPin){.port = gpioPortA, .pin = 15}; //8
  map[FPGA_C9] = (struct ucPin){.port = gpioPortA, .pin = 0}; //9
  map[FPGA_G9] = (struct ucPin){.port = gpioPortA, .pin = 1}; //10
  map[FPGA_C6] = (struct ucPin){.port = gpioPortA, .pin = 2}; //11  (fpga pin n7)

  //hole
}

//The purpose of this function is to configure the FPGA
//with the data found in the pin config structure. 
int fpgaConfigPin(struct pinConfig * p)
{
  //Get the pin we're setting.
  struct ucPin pin = routeThroughMap[p->fpgaPin];

  if (p->pinType == PINTYPE_OUT) {
    GPIO_PinModeSet(pin.port, pin.pin, gpioModePushPull, p->constantVal);
  }

  if (p->pinType == PINTYPE_IN) {
    GPIO_PinModeSet(pin.port, pin.pin, gpioModeInput, 0); 
  }
  
  //TODO: support everything :-)
  return 0;
}


int UsbDataReceived(USB_Status_TypeDef status,
                            uint32_t xf,
                            uint32_t remaining) 
{
    (void) remaining;
    if ((status == USB_STATUS_OK) && (xf > 0)) {

        if(currentPack.command == CMD_CONFIG_PIN) {
          struct pinConfig conf;
          uint32_t * d = (uint32_t *)(currentPack.data);
          conf.fpgaPin = d[PINCONFIG_DATA_FPGA_PIN];
          conf.pinType = d[PINCONFIG_DATA_TYPE];
          conf.constantVal = d[PINCONFIG_DATA_CONST];

          //PinVal is stored in uC-internal per-pin register
          fpgaConfigPin(&conf); 
        }
        
        if(currentPack.command == CMD_READ_PIN) {
            uint32_t pinToRead = (uint32_t)(*currentPack.data);
            struct mecoPack pack;
            pack.size = 4;
            pack.data = malloc(4);
            //read
            struct ucPin pin = routeThroughMap[pinToRead];
            GPIO_PinModeSet(pin.port, pin.pin, gpioModeInput, 0); 
            *pack.data = GPIO_PinInGet(pin.port, pin.pin);

            packToSend = pack;
            sendPackReady = 1;
        }
        //For now, we will just make a mecoPack and queue it for sending.
        /*
        struct mecoPack pack;
        pack.size = currentPack.size; //send back the same amount of data
        pack.command = currentPack.command;
        pack.data = malloc(currentPack.size); //allocate 4 bytes for the data.
        //This loops back back!
        packToSend = pack; 
        sendPackReady = 1; //ship it as soon as we can!
        */

        free(currentPack.data); //we've sent the data back, no need to store it.
    }

    //Check that we're still good, and get a new header.
    if (USBD_GetUsbState() == USBD_STATE_CONFIGURED) {
        USBD_Read(EP_DATA_OUT1, inBuffer, 8, UsbHeaderReceived); //get new header.
    }
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

void DmaUsbRxDone(unsigned int channel, int primary, void *user)
{
    (void) channel;
    (void) primary;
    (void) user;

    INT_Disable();
    INT_Enable();
}

/*
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
    if (newState == USBD_STATE_CONFIGURED) {
        USBD_Read(EP_DATA_OUT1, inBuffer, 8, UsbHeaderReceived);
    }
}

void ebi_gpio_setup(void)
{
  /* Using HFRCO at 14MHz as high frequency clock, HFCLK */
  
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
  /* Pin PC3 is configured to Push-pull */
  GPIO->P[2].MODEL = (GPIO->P[2].MODEL & ~_GPIO_P_MODEL_MODE3_MASK) | GPIO_P_MODEL_MODE3_PUSHPULL;
  /* Pin PC5 is configured to Push-pull */
  GPIO->P[2].MODEL = (GPIO->P[2].MODEL & ~_GPIO_P_MODEL_MODE5_MASK) | GPIO_P_MODEL_MODE5_PUSHPULL;
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
  /* Pin PF6 is configured to Push-pull */
  GPIO->P[5].MODEL = (GPIO->P[5].MODEL & ~_GPIO_P_MODEL_MODE6_MASK) | GPIO_P_MODEL_MODE6_PUSHPULL;
  /* Pin PF7 is configured to Push-pull */
  GPIO->P[5].MODEL = (GPIO->P[5].MODEL & ~_GPIO_P_MODEL_MODE7_MASK) | GPIO_P_MODEL_MODE7_PUSHPULL;
  /* Pin PF8 is configured to Push-pull */
  GPIO->P[5].MODEH = (GPIO->P[5].MODEH & ~_GPIO_P_MODEH_MODE8_MASK) | GPIO_P_MODEH_MODE8_PUSHPULL;
  /* Pin PF9 is configured to Push-pull */
  GPIO->P[5].MODEH = (GPIO->P[5].MODEH & ~_GPIO_P_MODEH_MODE9_MASK) | GPIO_P_MODEH_MODE9_PUSHPULL;
  
  /* Enable clock for EBI */
  CMU_ClockEnable(cmuClock_EBI, true);
  /* Module EBI is configured to location 1 */
  EBI->ROUTE = (EBI->ROUTE & ~_EBI_ROUTE_LOCATION_MASK) | EBI_ROUTE_LOCATION_LOC1;
  /* EBI I/O routing */
  EBI->ROUTE |= EBI_ROUTE_APEN_A21 | EBI_ROUTE_NANDPEN | EBI_ROUTE_BLPEN | EBI_ROUTE_CS0PEN | EBI_ROUTE_EBIPEN;
  
  /* Enable signal VBUSEN */
  USB->ROUTE |= USB_ROUTE_VBUSENPEN;
  
}

