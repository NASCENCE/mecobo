////LEDS on front case (left to right):
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

#include <stdio.h>

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

//Yes, yes I know
#define min(a,b) ({ a < b ? a : b; })


/* USB STRUCTS */
static const USBD_Callbacks_TypeDef callbacks =
{
    .usbReset        = NULL,
    .usbStateChange  = UsbStateChange,
    .setupCmd        = NULL,
    .isSelfPowered   = NULL,
    .sofInt          = NULL
};

const USBD_Init_TypeDef initstruct =
{
    .deviceDescriptor    = &USBDESC_deviceDesc,
    .configDescriptor    = USBDESC_configDesc,
    .stringDescriptors   = USBDESC_strings,
    .numberOfStrings     = sizeof(USBDESC_strings)/sizeof(void*),
    .callbacks           = &callbacks,
    .bufferingMultiplier = USBDESC_bufferingMultiplier,
    .reserved            = 0
};



struct fifo ucCmdFifo;
struct fifo ucSampleFifo;

#define MAGIC_FPGA_BITFILE_CONSTANT 0x4242
#define USB_MAX_SAMPLES 64000/sizeof(struct sampleValue)
//#define USB_MAX_SAMPLES 1000

char * BUILD_VERSION = __GIT_COMMIT__;

int NORPollData(uint16_t writtenWord, uint32_t addr);

#define DEBUG_PRINTING 0
//override newlib function.
int _write_r(void *reent, int fd, char *ptr, size_t len)
{
    (void) reent;
    (void) fd;
    for(size_t i = 0; i < len; i++) 
        ITM_SendChar(ptr[i]);

    return len;
}

static const int FPGA_CMD_FIFO_SIZE = 1024;
static const int UC_CMD_FIFO_SIZE = 2048;

uint16_t statusReg = 0;

int timeStarted = 0; 
static uint8_t mecoboStatus = MECOBO_STATUS_READY;

/*** Typedef's and defines. ***/

static int has_daughterboard = 0;
static int xbarProgrammed = 0;
static int setupFinished = 0;

//USB Variables

static uint32_t inBufferTop;
static uint8_t * inBuffer;

static struct mecoPack currentPack;
static struct mecoPack packToSend;
static int sendPackReady = 0;
static int sendInProgress = 0;
static int executeInProgress = 0;

static int feedFpgaCmdFifo = 1;
static int sampleFifoEmpty = 1;
static uint16_t samplesToGet = 0;

//Are we programming the FPGA
int fpgaConfigured = 0;
uint32_t bitfileOffset = 0;
uint32_t lastPackSize = 0;

static int runItems = 0;

#define MAX_INPUT_CHANNELS 50
static int fpgaTableIndex = 0;
static uint8_t fpgaTableToChannel[MAX_INPUT_CHANNELS];

uint16_t * xbar = ((uint16_t*)EBI_ADDR_BASE) + (200 * 0x100);
#define NUM_DAC_REGS 4
uint16_t DACreg[NUM_DAC_REGS];
uint8_t registersUpdated[NUM_DAC_REGS];

//Circular buffer in SRAM1, modulo MAX_SAMPLES
static const int MAX_SAMPLES = 43689; //SRAM1_BYTES/sizeof(struct sampleValue);
#define MAX_CHANNELS 150
static const int BUFFERSIZE = 1000;  //We will have 1000 samples per channel.
//Such a waste of space, should use hash map.

uint32_t numSamples = 0;


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

    //Force reprogram
    //programFPGA();

    //register 14 is 1 if the bitfile is the one with db
    uint16_t * fpga = (uint16_t*)EBI_ADDR_BASE;
    has_daughterboard = fpga[14];

    has_daughterboard = 1;
    int skip_boot_tests= 0;

    if(!skip_boot_tests) {

        //Verify presence of daughterboard bitfile
        uint16_t * dac = (uint16_t*)(EBI_ADDR_BASE) + (0x100*DAC0_POSITION);

        if (has_daughterboard) {
            printf("Daughterboard detected\n");
            //Check DAC controllers.
            printf("DAC: %x\n", dac[PINCONFIG_STATUS_REG]);
            uint16_t * adc = (uint16_t*)(EBI_ADDR_BASE) + (0x100*ADC0_POSITION);
            printf("ADC: %x\n", adc[PINCONFIG_STATUS_REG]);
            printf("XBAR: %x\n", xbar[PINCONFIG_STATUS_REG]);
            printf("Setting up DAC and ADCs\n");

            setupDAC();
            setupADC();
            resetXbar();

        }


        //testNOR();
        testRam();
    }



    printf("Initializing USB\n");
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


    fifoInit(&ucCmdFifo, UC_CMD_FIFO_SIZE, sizeof(struct pinItem), NULL);
    fifoInit(&ucSampleFifo, 10000, sizeof(struct sampleValue), (uint8_t *)SRAM2_START);

    /* MAIN LOOP */
    resetTime();

    samplesToGet = sampleFifoDataCount();
    //elementsInFpgaCmdFifo = cmdFifoDataCount();

    int roomInFifo = FPGA_CMD_FIFO_SIZE - cmdFifoDataCount();

    int tableIndexShift = has_daughterboard ? 13 : 1;

    for (;;) {
        //checkStatusReg();
        //feed if fifo is more than half empty
        /*
           if(timeStarted && runItems && (elementsInFpgaCmdFifo < 512)) {
           elementsInFpgaCmdFifo = cmdFifoDataCount();
           feedFpgaCmdFifo = 1;
           printf("start feed fpga cmd fifo (%x)\n", elementsInFpgaCmdFifo);
           } else {
           printf("stop feed fpga cmd fifo\n");
           feedFpgaCmdFifo = 0;
           }*/
        if(roomInFifo < 200) {
            roomInFifo = FPGA_CMD_FIFO_SIZE - cmdFifoDataCount();
        }
        //We want to be able to feed the fifo even if the sequenc run is not started, so no check for runItems here.
        if (setupFinished && (roomInFifo > 0) && (ucCmdFifo.numElements > 0)) {
            //mecoboStatus = MECOBO_STATUS_BUSY;
            // while(ucCmdFifo.numElements > 0) {
            void * item = NULL;
            fifoGet(&ucCmdFifo, &item);
            struct pinItem * it = (struct pinItem *)item;
            pushToCmdFifo(it);

            roomInFifo--;
            //}
            //mecoboStatus = MECOBO_STATUS_READY;
        }

        /*
           if(xbarProgrammed & runItems & !timeStarted) {
           resetTime();
           timeStarted = 1;
           runTime();
           }
           */

        //Check how many samples are in the FIFO, we have to get them anyway
        //so no need to check until we've gotten them anyway.

        if(timeStarted & runItems & (samplesToGet == 0)) {
            samplesToGet = sampleFifoDataCount();
        }


        //sampleFifoDataCount();
        if ((samplesToGet>0) & timeStarted & runItems & !fifoFull(&ucSampleFifo)) {
            //Push samples into fifo

            uint16_t * memctrl = (uint16_t*)EBI_ADDR_BASE;// getChannelAddress(0); //this is the EBI interface
            uint16_t data = memctrl[6]; //get next sample from EBI INTERFACE
            uint8_t tableIndex = (data >> tableIndexShift); //top 3 bits of word is index in fpga controller fetch table

            struct sampleValue sample; 
            sample.sampleNum = numSamples++;
            sample.channel = has_daughterboard ? fpgaTableToChannel[tableIndex] : tableIndex;
            sample.value = (int16_t)data;

            if (DEBUG_PRINTING) printf("s %x, ch %x, v %x i %u\n", sample.sampleNum, sample.channel, sample.value, tableIndex);

            fifoInsert(&ucSampleFifo, &sample);

            samplesToGet--;

        }
    }
}


void softReset()
{
    uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;
    cmdInterfaceAddr[7] = 0xD00D;   //SOFT SYSTEM RESET
    printf("SOFT SYSTEM RESET, ALL FIFOs CLEARED\n");
    //USBTIMER_DelayMs(1);
}

void runTime()
{
    uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;
    cmdInterfaceAddr[7] = 0xDEAD;   //start time
    printf("t run, tim: %x\n", cmdInterfaceAddr[9]);
}
void resetTime()
{
    uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;
    printf("t res, tim: %x\n", cmdInterfaceAddr[9]);
    cmdInterfaceAddr[7] = 0xBEEF;   //reset and stop time
}

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
                    if(DEBUG_PRINTING) printf("currentPack.data malloc null :( \n");
                } 

                USBD_Read(CDC_EP_DATA_OUT, currentPack.data, currentPack.size, UsbDataReceived);
            } else {
                //Only header command, so we can parse it, and will not wait for data.
                execCurrentPack();
                //And wait for new header.
                if (USBD_GetUsbState() == USBD_STATE_CONFIGURED) {
                    USBD_Read(CDC_EP_DATA_OUT, inBuffer, 8, UsbHeaderReceived); //get new header.
                }
            }
        } else {
            USBD_Read(CDC_EP_DATA_OUT, inBuffer, 8, UsbHeaderReceived);
        }
    }

    return USB_STATUS_OK;
}


//Set up callback to always wait for a new pack of 8 bytes
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
        USBD_Read(CDC_EP_DATA_OUT, inBuffer, 8, UsbHeaderReceived); //get new header.
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
        //don't free this command it just sends back a status
        //    if(packToSend.command != USB_CMD_STATUS) {
        if(packToSend.data != NULL) {
            free(packToSend.data);
        } else {
            if(DEBUG_PRINTING) printf("Tried to free NULL-pointer\n");
        }
        //   }

        //Reset sample counter now.
        if(packToSend.command == USB_CMD_GET_INPUT_BUFFER){
            numSamples = 0;
        }
    }
    sendInProgress = 0;

    return USB_STATUS_OK;
}
/*
   void DmaUsbTxDone(unsigned int channel, int primary, void *user)
   {
   (void) channel;
   (void) primary;
   (void) user;

   INT_Disable();
   INT_Enable();
   }
   */

void UsbStateChange(USBD_State_TypeDef oldState, USBD_State_TypeDef newState)
{
    (void) oldState;
    if (newState == USBD_STATE_CONFIGURED) {
        USBD_Read(CDC_EP_DATA_OUT, inBuffer, 8, UsbHeaderReceived);
    }

}

inline uint32_t get_bit(uint32_t val, uint32_t bit) 
{
    return (val >> bit) & 0x1;
}


void setupInput(FPGA_IO_Pins_TypeDef channel, int sampleRate, uint32_t startTime, uint32_t endtime)
{
    mecoboStatus = MECOBO_STATUS_BUSY;

    //Hack to avoid start time 0 which is a special sentinent value that indicates 
    //that no recording has been scheduled on that pin.
    if(startTime == 0) { startTime = 1; }
    //We are in setup phase. maybe ok.

    command(0, SAMPLE_COLLECTOR_ADDR, SAMPLE_COLLECTOR_REG_NEW_UNIT, channel);  //set up the sample collector to this channel
    //memctrl[4] = channel;
    fpgaTableToChannel[fpgaTableIndex] = (uint8_t)channel;
    if(DEBUG_PRINTING) printf("Rec Channel %u added, index %u, table entry %d\n", channel, fpgaTableIndex, (uint8_t)fpgaTableToChannel[fpgaTableIndex]);

    //How many samples?
    //The overflow register is what decides this.
    //We set it to something that sort of achieves the wished sample rate.
    //If we want for instance 44.1KHz sample rate, the counter is
    //roughly 1701, so actual sample rate is about 44.09
    float overflow = 0;
    if (sampleRate != 0) {
        overflow = (75000000)/((float)sampleRate);
    } else {
        printf("omg sampleRate is 0. setting cowardly to 1000");
        overflow = 1000.0;
    }
    fpgaTableIndex++;
    //printf("Estimated %u samples for rate %d, of %f, channel %d, duration %d\n", numSamples, sampleRate, overflow, channel, duration);

    //If it's a ADC channel we set up the ADC here in stead of all this other faff.
    if ((AD_CHANNELS_START <= channel) && (channel <= (AD_CHANNELS_END))) {

        //find the ADC's internal number for the channel to program it.
        uint16_t boardChan = channel - AD_CHANNELS_START;
        //board 0
        if(boardChan < 16) {
            //Program sequence register with channel.
            adcSequence[0] |= 0xE000 | (1 << (12 - boardChan));
            if(DEBUG_PRINTING) printf("ADCregister write channel %x, %x\n", boardChan, adcSequence[0]);

            //Range register 1

            //resetTime();
            //runTime();
            //if(!adc_rr_wr)
            //command(0, channel, AD_REG_PROGRAM, 0x1AAA0);
            //USBTIMER_DelayMs(1);

            //Range register 2
            //command(0, channel, AD_REG_PROGRAM, 0x1CAA0);
            //USBTIMER_DelayMs(1);

            command(0, channel, AD_REG_OVERFLOW, (uint32_t)overflow);
            command(0, 255, 0, 0);
            command(0, channel, AD_REG_ENDTIME, (uint32_t)endtime);
            command(0, 255, 0, 0);
            command(0, channel, AD_REG_REC_START_TIME, (uint32_t)startTime);
            command(0, 255, 0, 0);
            //command(0, channel, AD_REG_DIVIDE, (uint32_t)1);

            command(0, channel, AD_REG_PROGRAM, 0x10000|(uint32_t)adcSequence[0]);
            command(0, 255, 0, 0);

            //Program the ADC to use this channel as well now. Result in two comp, internal ref.
            //sequencer on.
            //Control register
            command(0, channel, AD_REG_PROGRAM, 0x18014);
            command(0, 255, 0, 0);

            //resetTime();
            if(DEBUG_PRINTING) printf("ADC programmed to new sequences\n");
        }
    }

    //Digital channel.
    else {
        if(DEBUG_PRINTING) printf("Recording dig ch: %x\n", channel);
        command(0, channel, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_RESET);  //reset pin controller to put it in idle
        command(0, channel, PINCONTROL_REG_SAMPLE_RATE, (uint32_t)overflow);
        command(0, channel, PINCONTROL_REG_END_TIME, (uint32_t)endtime);
        command(0, channel, PINCONTROL_REG_REC_START_TIME, (uint32_t)startTime);

        //command(0, channel, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_INPUT_STREAM);

        /*
           addr[PINCONTROL_CMD_LOCAL_CMD] = CMD_RESET;
           addr[PINCONFIG_SAMPLE_RATE] = (uint16_t)overflow;
           addr[PINCONTROL_CMD_LOCAL_CMD] = CMD_INPUT_STREAM;
           */
    }


}


inline void startInput()
{
    printf("Starting sampling\n");
    command(0, SAMPLE_COLLECTOR_ADDR, SAMPLE_COLLECTOR_REG_LOCAL_CMD, SAMPLE_COLLECTOR_CMD_START_SAMPLING);  //this sends STAT SAMPLING COMMAND
}


void execCurrentPack()
{
    executeInProgress = 1;
    if(currentPack.command == USB_CMD_STATUS) {

        struct mecoboStatus * s = (struct mecoboStatus *)malloc(sizeof(struct mecoboStatus));
        s->state = (uint8_t)mecoboStatus;
        s->foo = (ucCmdFifo.numElements == 0) ? 0 : 1;
        s->samplesInBuffer = (uint16_t)ucSampleFifo.numElements + (uint16_t)sampleFifoDataCount(); //check if the sample fifo is empty yet.
        s->roomInCmdFifo = (uint16_t)((uint16_t)ucCmdFifo.size - (uint16_t)ucCmdFifo.numElements);

        sendPacket(sizeof(struct mecoboStatus), USB_CMD_STATUS, (uint8_t*)s);
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

            printf("ADDING A PIN CONFIG PIN %u\n", item.pin);
            //Hack to avoid starting this if time is set to 0.
            if (item.startTime == 0) item.startTime = 1;

            fifoInsert(&ucCmdFifo, &item);


            if(DEBUG_PRINTING) printf("Item %d added to ucFifo, chan %d, starting at %x, ending at %x, samplerate %d\n", 0, (unsigned int)item.pin, (unsigned int)item.startTime, (unsigned int)item.endTime, (unsigned int)item.sampleRate);
        } else {
            if(DEBUG_PRINTING) printf("Curr data NULL\n");
        }
    }

    else if (currentPack.command == USB_CMD_SETUP_RECORDING) 
    {
        printf("SETUP_RECORDING\n");
        struct pinItem item;
        if(currentPack.data != NULL) {
            uint32_t * d = (uint32_t *)(currentPack.data);
            item.pin = (d[PINCONFIG_DATA_FPGA_PIN]);  //pin is channel, actually
            item.sampleRate = d[PINCONFIG_DATA_SAMPLE_RATE];
            item.endTime    = 75000*d[PINCONFIG_END_TIME];
            item.startTime    = (75000*d[PINCONFIG_START_TIME]);
            item.type = d[PINCONFIG_DATA_TYPE];

            setupInput(item.pin, item.sampleRate, item.startTime, item.endTime);
            printf("USB_CMD_SETUP_RECORDING pin %u\n", (unsigned int)item.pin);
        }
    }

    else if(currentPack.command == USB_CMD_RUN_SEQ)
    {
        if(!setupFinished) {
            printf("ERROR: Won't run without setup finished\n");
            return;
        } else {
            printf("---- SEQUENCE RUN STARTED -----\n");
        }
        runTime();
        runItems = 1;
        timeStarted = 1;
    }

    else if(currentPack.command == USB_CMD_PROGRAM_XBAR)
    {
        printf("Configuring XBAR\n");

        //Data in the buffer is the order we would like the
        //bytes to be IN the xbar.
        //since the command function takes a 32 bit 
        //integer assumed to be stored in Little-Endian,
        //we'll access the buffer like that, and 
        //the order that is sent to the FPGA 
        //is actually correct
        uint16_t * d = (uint16_t*)(currentPack.data);

        int j = 0;
        for(int i = 0; i < 16; i++) {
            command2x16(0, XBAR_CONTROLLER_ADDR, i, __builtin_bswap16(d[j]), __builtin_bswap16(d[j+1]));
            j += 2;
        }
        printf("Sent %d bytes\n", j*2);

        command(0, XBAR_CONTROLLER_ADDR, XBAR_REG_LOCAL_CMD, 0x1);

        //We need to wait here until the XBAR is configured. 
        //TODO: GET THE READ BUS BACK SO WE CAN WAIT HERE UNTIL CONFIG IS DONE
        //runTime();
        USBTIMER_DelayMs(1);

        printf("\nXBAR configured\n");

        xbarProgrammed = 1;
        mecoboStatus = MECOBO_STATUS_XBAR_CONFIGURED;
    }


    else if(currentPack.command == USB_CMD_UPDATE_REGISTER)
    {
        if(DEBUG_PRINTING) printf("Updating Register\n");
        int * d = (int*)(currentPack.data);
        if(DEBUG_PRINTING) printf("REG %d: %d\n", d[0], d[1]); 
        DACreg[d[0]] = d[1];
        registersUpdated[d[0]] = 1;
    }


    else if(currentPack.command == USB_CMD_LED) {
        /* Start output from pin controllers */
        uint32_t * d = (uint32_t *)(currentPack.data);
        led(d[LED_SELECT], d[LED_MODE]);
    }

    else if(currentPack.command == USB_CMD_RESET_ALL) {
        //mecoboStatus = MECOBO_STATUS_BUSY;

        printf("----------------- RESETING MECOBO ------------------\n");
        resetTime();
        softReset();   //This will clear the command fifo, etc.

        /*
           if(has_daughterboard) {
           setupDAC();
           setupADC();
           }
           */

        //USBTIMER_DelayMs(10);
        startInput();  //start sample collector
        //command(0, SAMPLE_COLLECTOR_ADDR, SAMPLE_COLLECTOR_REG_LOCAL_CMD, SAMPLE_COLLECTOR_CMD_RESET);  
        runItems = 0;
        //Reset state
        numSamples = 0;
        adcSequence[0] = 0;

        samplesToGet = 0;

        fifoReset(&ucCmdFifo);
        fifoReset(&ucSampleFifo);

        fpgaTableIndex = 0;
        for(int q = 0; q < MAX_INPUT_CHANNELS; q++) {
            fpgaTableToChannel[q] = 0;
        }

        //Set all DAC outputs to 0V
        if(has_daughterboard){
            for(int i = DA_CHANNELS_START; i < DA_CHANNELS_START + 8; i++) {
                uint32_t wrd = getDACword(i, 128);
                command(0, DAC_CONTROLLER_ADDR, DAC_REG_LOAD_VALUE, wrd);
            }
        }
        //resetAllPins();

        timeStarted = 0;
        xbarProgrammed = 0;
        setupFinished = 0;

        feedFpgaCmdFifo = 1;
        sampleFifoEmpty = 1;


        printf("\n\n---------------- MECOBO RESET DONE ------------------\n\n");
        mecoboStatus = MECOBO_STATUS_RESET_COMPLETE;

    }

    else if(currentPack.command == USB_CMD_GET_INPUT_BUFFER_SIZE) {

        uint32_t * retSamples = (uint32_t*)malloc(4);

        *retSamples = min(ucSampleFifo.numElements, USB_MAX_SAMPLES);

        //if(DEBUG_PRINTING) printf("%u smpl uC q\n", (unsigned int)*retSamples);
        sendPacket(4, USB_CMD_GET_INPUT_BUFFER_SIZE, (uint8_t*)(retSamples));
    }

    else if(currentPack.command == USB_CMD_GET_INPUT_BUFFER) {

        //    uint16_t * memctrl = (uint16_t*)EBI_ADDR_BASE;// getChannelAddress(0); //this is the EBI interface

        //Send back the whole sending buffer.
        //Data contains the number of samples to xfer.
        uint32_t * txSamples = (uint32_t *)(currentPack.data);
        int bytes = sizeof(struct sampleValue) * *txSamples;
        struct sampleValue * samples = (struct sampleValue *)malloc(bytes);
        if(!samples) {
            printf("ERROR: MALLOC FAILED. We're probably out of memory!\n");
        }
        printf("Shipping %d bytes\n", bytes);

        for(unsigned int i = 0; i < *txSamples; i++) {
            struct sampleValue * sv;
            fifoGet(&ucSampleFifo, (void*)&sv);
            samples[i] = *sv;
        }

        sendPacket(bytes, USB_CMD_GET_INPUT_BUFFER, (uint8_t*)samples);

        if(DEBUG_PRINTING) printf("USB has been instructed to send data. Returning to main loop\n");
    }



    else if(currentPack.command == USB_CMD_LOAD_BITFILE) {
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
        printf("f1\n");
        printf("Got data, current offset %u\n", (unsigned int)bitfileOffset);
    }


    else if(currentPack.command == USB_CMD_PROGRAM_FPGA) {
        programFPGA();
        bitfileOffset = 0;
    }

    //The intention is for this to be run before other items.
    else if (currentPack.command == USB_CMD_LOAD_SETUP) { 
        printf("USB_CMD_LOAD_SETUP\n");

        //resetTime();

        mecoboStatus = MECOBO_STATUS_BUSY;
        if (runItems) {
            printf("ERROR: LOAD SETUP WHILE SCHEDULE IS RUNNING\n");
        }


        while(ucCmdFifo.numElements > 0) {
            printf("setup item\n");
            void * item = NULL;
            fifoGet(&ucCmdFifo, &item);
            struct pinItem * it = (struct pinItem *)item;
            pushToCmdFifo(it);
        }

        //reset the microcontroller ... shouldn't be needed, but hey.
        //
        //Wait until the fpga cmd fifo is empty here so that we know that we are all
        //out of setup items in the fifo and most things are ready to go. this will
        //only work if we setup some busy signals.
        fifoReset(&ucCmdFifo);


        //uC-queue now empty and we can start loading all the other commands.
        mecoboStatus = MECOBO_STATUS_SETUP_LOADED;

        //Preload FIFO with a sample counter reset thingy. First thing that happens when a new sequence
        //starts is to remove any samples in the FIFO.
        //command(0, SAMPLE_COLLECTOR_ADDR, SAMPLE_COLLECTOR_REG_LOCAL_CMD, SAMPLE_COLLECTOR_CMD_RES_SAMPLE_FIFO);

    } else if (currentPack.command == USB_CMD_PRELOAD_FIFO) {

        //preload the fifo with commands as well
        printf("uc cmd fifo preload: %d\n", ucCmdFifo.numElements);
        while(ucCmdFifo.numElements > 0) {
            void * item = NULL;
            fifoGet(&ucCmdFifo, &item);
            struct pinItem * it = (struct pinItem *)item;
            pushToCmdFifo(it);
        }


        mecoboStatus = MECOBO_STATUS_FIFO_PRELOADED;
        printf("---- SETUP FINISHED -----\n");

        //Start the sample collector!


        setupFinished = 1;
    }

    //if/else done.
    executeInProgress = 0;

    if(currentPack.size > 0) {
        if(currentPack.data != NULL) {
            free(currentPack.data);
        } else {
            printf("FREE CURRENT PACK NULL\n");
        }
    }
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
    USBD_Write(CDC_EP_DATA_IN, packToSend.data, packToSend.size, UsbDataSent);
}

void resetAllPins()
{
    printf("Reseting all digital pin controllers\n");
    for(int j = 0; j < 50; j++) {
        command(0, 255, 0, 0);
        command(0, j, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_RESET);
        command(0, 255, 0, 0);
    }
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
            GPIO_PinModeSet(gpioPortD, 5, gpioModePushPull,mode);  //Led U3
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

    printf("ir: %x\n", pin);
    printf("sr: %x\n", statusReg);
    checkStatusReg();
}

inline uint16_t sampleFifoDataCount()
{
    uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;
    uint16_t d = cmdInterfaceAddr[EBI_SAMPLE_FIFO_DATA_COUNT];
    //printf("%u\n", d);
    return d;
}

inline uint16_t cmdFifoDataCount()
{
    uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;
    uint16_t d = cmdInterfaceAddr[EBI_CMD_FIFO_DATA_COUNT];
    return d;
}


void checkStatusReg() 
{
    uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;
    statusReg = cmdInterfaceAddr[0];
    if (statusReg & STATUS_REG_CMD_FIFO_ALMOST_FULL_BIT) {
        printf("CMD FIFO almost full, no more feeding\n");
        feedFpgaCmdFifo = 0;
    }

    if (statusReg & STATUS_REG_CMD_FIFO_EMPTY) {
        //printf("CMD FIFO empty\n");
        feedFpgaCmdFifo = 1;
    } 
    if (statusReg & STATUS_REG_CMD_FIFO_ALMOST_EMPTY) {
        //printf("CMD FIFO almost empty\n");
        feedFpgaCmdFifo = 1;
    }

    if (statusReg & STATUS_REG_SAMPLE_FIFO_EMPTY) {
        sampleFifoEmpty = 1;
    } else {
        sampleFifoEmpty = 0;
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
    //so. we order manually.
    //
    //The FPGA cmdInterface is ordered from MSB to LSB. There's 5
    //words of 2 bytes each.


    cmdInterfaceAddr[1] = serialCmd[1];
    //printf("in %x out %x\n", serialCmd[1], cmdInterfaceAddr[12]);

    cmdInterfaceAddr[2] = serialCmd[0];  //flippery to put them in correct order on bus 
    //printf("in %x out %x\n", serialCmd[0], cmdInterfaceAddr[12]);

    cmdInterfaceAddr[3] = cmd->data[0];  //will be put on bus in network order
    //printf("in %x out %x\n", cmd->data[0], cmdInterfaceAddr[12]);

    cmdInterfaceAddr[4] = cmd->data[1];  //will be put on bus in network order
    //printf("in %x out %x\n", cmd->data[1], cmdInterfaceAddr[12]);

    cmdInterfaceAddr[5] = serialCmd[4];  // I've ordered the struct so that the last word of the cmd is first addr, then controller.
    //printf("in %x out %x\n", serialCmd[4], cmdInterfaceAddr[12]);

    /*
       printf("s: %x\n", cmdInterfaceAddr[0]);
       printf("pushing word %u : %x\n", 0, serialCmd[1]);
       printf("pushing word %u : %x\n", 1, serialCmd[0]);
       printf("pushing dword %u : %x\n", 2, cmd->data[0]);
       printf("pushing dword %u : %x\n", 3, cmd->data[1]);
       printf("pushing word %u : %x\n", 4, serialCmd[4]);
       */
}



void command(uint32_t startTime, uint8_t controller, uint8_t reg, uint32_t data) {
    struct fifoCmd cmd;
    uint16_t * data16 = (uint16_t*)(&data);
    cmd.startTime = (uint32_t)(startTime);
    cmd.controller = controller; 
    cmd.addr = reg;
    cmd.data[0] = data16[1]; 
    cmd.data[1] = data16[0]; 
    putInFifo(&cmd);
}

void command2x16(uint32_t startTime, uint8_t controller, uint8_t reg, uint16_t data1, uint16_t data2) {
    struct fifoCmd cmd;
    cmd.startTime = (uint32_t)(startTime);
    cmd.controller = controller; 
    cmd.addr = reg;
    cmd.data[0] = data1; 
    cmd.data[1] = data2; 
    putInFifo(&cmd);
}

void pushToCmdFifo(struct pinItem * item)
{
    //struct fifoCmd cmd = makeCommand(item->startTime, (uint8_t)item->pin, 0x0, 0x0);

    printf("fifo: %u, pin %u\n", (unsigned int)item->startTime, (unsigned int)item->pin);
    switch(item->type) {

        case PINCONFIG_DATA_TYPE_DIGITAL_CONST:
            if (item->constantValue == 1) {
                command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_REC_START_TIME, 0);
                command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_NCO_COUNTER, (uint32_t)item->nocCounter);
                command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_END_TIME, (uint32_t)item->endTime);
                command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_CONST);
            } else {
                command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_REC_START_TIME, 0);
                command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_NCO_COUNTER, (uint32_t)item->nocCounter);
                command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_END_TIME, (uint32_t)item->endTime);
                command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_RESET);
            }
            break;


        case PINCONFIG_DATA_TYPE_DIGITAL_OUT:
            command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_NCO_COUNTER, (uint32_t)item->nocCounter);
            command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_REC_START_TIME, 0);
            command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_END_TIME, (uint32_t)item->endTime);
            command(item->startTime, (uint8_t)item->pin, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_START_OUTPUT);

            break;

        case PINCONFIG_DATA_TYPE_RECORD_ANALOGUE:
        case PINCONFIG_DATA_TYPE_RECORD:

            //command(item->startTime, SAMPLE_COLLECTOR_ADDR, SAMPLE_COLLECTOR_REG_LOCAL_CMD, SAMPLE_COLLECTOR_CMD_START_SAMPLING);  //this sends STAT SAMPLING COMMAND
            break; 

        case PINCONFIG_DATA_TYPE_DAC_CONST:
            if(DEBUG_PRINTING) printf(">> CONST DAC, t%u, ch %u, %u\n", item->startTime, item->pin, item->constantValue);
            //setVoltage(item->pin, item->constantValue);
            uint32_t wrd = getDACword(item->pin, item->constantValue);
            command(item->startTime, DAC_CONTROLLER_ADDR, DAC_REG_LOAD_VALUE, wrd);
            break;



        default:
            break;
    }
}


//XBAR
void resetXbar()
{
    for(int i = 0; i < 16; i++) {
        command(0, XBAR_CONTROLLER_ADDR, i, 0);
    }
    command(0, XBAR_CONTROLLER_ADDR, XBAR_REG_LOCAL_CMD, 0x1);
}




