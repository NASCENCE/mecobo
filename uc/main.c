////LEDS on front case (left to right):
//GPIO_PinModeSet(gpioPortD, 4, gpioModePushPull, 1);  //Led 1
//GPIO_PinModeSet(gpioPortB, 11, gpioModePushPull, 1);  //Led 2
//GPIO_PinModeSet(gpioPortD, 8, gpioModePushPull, 1);  //Led 3
//GPIO_PinModeSet(gpioPortB, 8, gpioModePushPull, 1);  //Led 4

//LEDS on board "north to south"
//A10 : U0
//A9: U1
//B12: U2
//D5: U3
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

#define FPGA_SYSTEM_CLK_RATE 75000000  //FPGA runs at 100MHz
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

static int ad_and_da_configured = 0;
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
static int samplesToGet = 0;

//Are we programming the FPGA
int fpgaConfigured = 0;
uint32_t bitfileOffset = 0;
uint32_t lastPackSize = 0;

static int firstSampleDiscarded = 0;

#define MAX_INPUT_CHANNELS 50
static int fpgaTableIndex = 0;
static uint16_t fpgaTableToChannel[MAX_INPUT_CHANNELS];

static volatile uint16_t * cmdInterfaceAddr = (uint16_t*)EBI_ADDR_BASE;

#define NUM_DAC_REGS 4
uint16_t DACreg[NUM_DAC_REGS];
uint8_t registersUpdated[NUM_DAC_REGS];

//Circular buffer in SRAM1, modulo MAX_SAMPLES
static const int MAX_SAMPLES = 43689; //SRAM1_BYTES/sizeof(struct sampleValue);
#define MAX_CHANNELS 150

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
    infop("Print output online\n");

    //Small check for endinanness
    uint32_t integer = 0xDEADBABE;
    uint8_t * int8 = (uint8_t*)&integer;
    if(int8[0] == 0xDE) infop("Arch is BIG ENDIAN\n");
    if(int8[0] == 0xBE) infop("Arch is LITTLE ENDIAN\n");


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

    //Make sure NOR has come up.
    for(int norwait = 0; norwait < 10000000; norwait++);


    has_daughterboard = 0;
    int skip_boot_tests= 0;

    if(!skip_boot_tests) {

        //testNOR();
        testRam();
    }



    infop("Initializing USB\n");
    USBD_Init(&initstruct);
    infop("USB Initialized.\n");
    USBD_Disconnect();
    USBTIMER_DelayMs(100);
    USBD_Connect();
    led(BOARD_LED_U3, 1);

    setupADC();
    setupDAC();
 
    infop("Cycling LEDs :-)\n");
    //Cycle leds.
        for(int i = 0; i < 48; i++) {
            led(i%8, 1);
            USBTIMER_DelayMs(60);
            led(i%8, 0);
        }


    infop("It's just turtles all the way down.\n");
    infop("I'm the mecobo firmware running on the evolutionary motherboard 3.5new.\n");
    infop("I was built %s, git commit %s\n", __DATE__, BUILD_VERSION);
    infop("Entering main loop.\n");


    fifoInit(&ucCmdFifo, UC_CMD_FIFO_SIZE, sizeof(struct pinItem), NULL);
    fifoInit(&ucSampleFifo, 10000, sizeof(struct sampleValue), (uint8_t *)SRAM2_START);

    /* MAIN LOOP */
    resetTime();

    int roomInFifo = FPGA_CMD_FIFO_SIZE - cmdFifoDataCount();


    struct sampleValue sample; 
    uint16_t tableIndex;
    uint16_t data;

    for (;;) {

        //feed if fifo is more than half empty
        if(roomInFifo < 200) {
            roomInFifo = FPGA_CMD_FIFO_SIZE - cmdFifoDataCount();
        }
        //We want to be able to feed the fifo even if the sequenc run is not started
        if (setupFinished && (roomInFifo > 0) && (ucCmdFifo.numElements > 0)) {
            //mecoboStatus = MECOBO_STATUS_BUSY;
            // while(ucCmdFifo.numElements > 0) {
            void * item = NULL;
            fifoGet(&ucCmdFifo, &item);
            struct pinItem * it = (struct pinItem *)item;
            pushToCmdFifo(it);

            roomInFifo--;
        }

        //Check how many samples are in the FIFO, we have to get them anyway
        //so no need to check until we've gotten them anyway.

        if(timeStarted && (samplesToGet <= 0)) {
            samplesToGet = sampleFifoDataCount();
        }


        //sampleFifoDataCount();
        if ((samplesToGet>0) && timeStarted && !fifoFull(&ucSampleFifo)) {


            debug("samplesToget: %d\n", samplesToGet);
            //Push samples into fifo
            //sample.sampleNum = numSamples++;
            //for(int i = 0; i < samplesToGet; i++) {

             //   if(!fifoFull(&ucSampleFifo)) {
                    if(!firstSampleDiscarded) {
                        firstSampleDiscarded = 1;
                        ebir(6);
                    } else {
                        data = ebir(6);
                        
                        int tableIndexShift = has_daughterboard ? 13 : 1;   //the 1 is for just looking at the controller ID
                        tableIndex = (data >> tableIndexShift); //top 3 bits of word is index in fpga controller fetch table

                        sample.channel = has_daughterboard ? fpgaTableToChannel[tableIndex] : tableIndex;
                        sample.value = (int16_t)data;
                        fifoInsert(&ucSampleFifo, &sample);  
                        samplesToGet--;
                    }
                  //  }
            //}
            debug("d %x s %x, ch %x, v %d i %u\n", data, sample.sampleNum, sample.channel, sample.value, tableIndex);



        }
    }
}


void softReset()
{
    ebiw(7, 0xD00D);
    infop("Soft system reset.\n");
}


void runTime()
{
    ebiw(7, 0xDEAD);
    infop("T run, tim: %x\n", cmdInterfaceAddr[9]);
}

void resetTime()
{
    ebiw(7, 0xBEEF);
    infop("T res, tim: %x\n", cmdInterfaceAddr[9]);
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
            debug("Expected header of size 8, got %u.\n", (unsigned int)xf);
        }
    }

    //Check that we're still good, and get some data for this pack.
    if (USBD_GetUsbState() == USBD_STATE_CONFIGURED) {
        if(gotHeader) {
            if(currentPack.size > 0) {
                //Go to data collection.
                currentPack.data = (uint8_t*)malloc(currentPack.size);
                if(currentPack.data == NULL) {
                    debug("currentPack.data malloc null :( \n");
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
            debug("Tried to free NULL-pointer\n");
        }

    }
    sendInProgress = 0;

    return USB_STATUS_OK;
}

void UsbStateChange(USBD_State_TypeDef oldState, USBD_State_TypeDef newState)
{
    (void) oldState;
    if (newState == USBD_STATE_CONFIGURED) {
        USBD_Read(CDC_EP_DATA_OUT, inBuffer, 8, UsbHeaderReceived);
    }

}

void ebiw(uint32_t addr, uint16_t data) {
    *(cmdInterfaceAddr + addr) = data;
    trace("w %x %x\n", (uint32_t)(addr), data);
}

uint16_t ebir(uint32_t addr) {
    uint16_t d = cmdInterfaceAddr[addr];
    trace("r %x %x\n", addr, d);
    return d;
}


void setupInput(FPGA_IO_Pins_TypeDef channel, int sampleRate, uint32_t startTime, uint32_t endtime)
{
    mecoboStatus = MECOBO_STATUS_BUSY;

    //Hack to avoid start time 0 which is a special sentinent value that indicates 
    //that no recording has been scheduled on that pin.
    if(startTime == 0) { startTime = 1; }
    //We are in setup phase. maybe ok.

    command(0, SAMPLE_COLLECTOR_ADDR, SAMPLE_COLLECTOR_REG_NEW_UNIT, channel);  //set up the sample collector to this channel
    fpgaTableToChannel[fpgaTableIndex] = (uint16_t)channel;
    debug("Rec Channel %u added, index %u, table entry %d\n", channel, fpgaTableIndex, (uint16_t)fpgaTableToChannel[fpgaTableIndex]);

    //How many samples?
    //The overflow register is what decides this.
    
    double overflow = 0;
    if (sampleRate != 0) {
        overflow = (float)FPGA_SYSTEM_CLK_RATE/(float)sampleRate;   //this is the period length of the 100MHz sample rate counter clock
        infop("Overflow register: %f\n", overflow);
    } else {
        debug("SampleRate is 0. Set to 1000\n");
        overflow = 1000.0;
    }
    fpgaTableIndex++;

    //If it's a ADC channel we set up the ADC here in stead of all this other faff.
    if ((AD_CHANNELS_START <= channel) && (channel <= (AD_CHANNELS_END))) {

        //find the ADC's internal number for the channel to program it.
        uint16_t boardChan = channel - AD_CHANNELS_START;
        //board 0
        if(boardChan < 16) {
            //Program sequence register with channel.
            adcSequence[0] |= 0xE000 | (1 << (12 - boardChan));
            debug("ADCregister write channel %x, %x\n", boardChan, adcSequence[0]);

            command(0, channel, AD_REG_OVERFLOW, (uint32_t)overflow);
            command(0, channel, AD_REG_ENDTIME, (uint32_t)endtime);
            command(0, channel, AD_REG_REC_START_TIME, (uint32_t)startTime);
            //command(0, channel, AD_REG_DIVIDE, (uint32_t)1);

            command(0, channel, AD_REG_PROGRAM, 0x10000|(uint32_t)adcSequence[0]);

            //Program the ADC to use this channel as well now. Result in two comp, internal ref.
            //sequencer on.
            //Control register
            command(0, channel, AD_REG_PROGRAM, 0x18014);

            debug("ADC programmed to new sequences\n");
        }
    }

    //Digital channel.
    else {
        debug("Recording dig ch: %x\n", channel);
        command(0, channel, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_RESET);  //reset pin controller to put it in idle
        command(0, channel, PINCONTROL_REG_SAMPLE_RATE, (uint32_t)overflow);
        command(0, channel, PINCONTROL_REG_END_TIME, (uint32_t)endtime);
        command(0, channel, PINCONTROL_REG_REC_START_TIME, (uint32_t)startTime);
    }


}


inline void startInput()
{
    command(0, SAMPLE_COLLECTOR_ADDR, SAMPLE_COLLECTOR_REG_LOCAL_CMD, SAMPLE_COLLECTOR_CMD_RES_SAMPLE_FIFO);  //this sends STAT SAMPLING COMMAND
    infop("Starting sampling\n");
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
            item.startTime  = d[PINCONFIG_START_TIME];   //start time is in microseconds
            item.endTime    = d[PINCONFIG_END_TIME];   //microsecs
            item.constantValue = d[PINCONFIG_DATA_CONST];
            item.type = d[PINCONFIG_DATA_TYPE];
            item.sampleRate = d[PINCONFIG_DATA_SAMPLE_RATE];
            item.nocCounter = d[PINCONFIG_DATA_NOC_COUNTER];

            //Hack to avoid starting this if time is set to 0.
            if (item.startTime == 0) item.startTime = 1;

            fifoInsert(&ucCmdFifo, &item);


            debug("Item added to ucFifo, chan %d, starting at %d, ending at %d\n", (unsigned int)item.pin, (unsigned int)item.startTime, (unsigned int)item.endTime);
        } else {
            debug("Curr data NULL\n");
        }
    }

    else if (currentPack.command == USB_CMD_SETUP_RECORDING) 
    {
        debug("SETUP_RECORDING\n");
        struct pinItem item;
        if(currentPack.data != NULL) {
            uint32_t * d = (uint32_t *)(currentPack.data);
            item.pin = (d[PINCONFIG_DATA_FPGA_PIN]);  //pin is channel, actually
            item.sampleRate = d[PINCONFIG_DATA_SAMPLE_RATE];
            item.endTime    = d[PINCONFIG_END_TIME];
            item.startTime    = d[PINCONFIG_START_TIME];
            item.type = d[PINCONFIG_DATA_TYPE];

            setupInput(item.pin, item.sampleRate, item.startTime, item.endTime);
            debug("USB_CMD_SETUP_RECORDING pin %u\n", (unsigned int)item.pin);
        }
    }

    else if(currentPack.command == USB_CMD_RUN_SEQ)
    {
        if(!setupFinished) {
            infop("ERROR: Won't run without setup finished\n");
            return;
        } else {
            infop("---- SEQUENCE RUN STARTED -----\n");
        }
        runTime();
        timeStarted = 1;
    }

    else if(currentPack.command == USB_CMD_PROGRAM_XBAR)
    {
        infop("Configuring XBAR\n");

        //The data that comes in are 32 16-bit words
        //where we want to send word 0 first, word 1 second, etc...
        uint16_t * d = (uint16_t*)(currentPack.data);

        //the command interface uses 32 bit words for some reason so 
        //we'll interate in steps of 2 words.
        int j = 0;
        for(int i = 0; i < 16; i++) {
            //command2x16(0, XBAR_CONTROLLER_ADDR, i, __builtin_bswap16(d[j]), __builtin_bswap16(d[j+1]));
            command2x16(0, XBAR_CONTROLLER_ADDR, i, d[j], d[j+1]);
            j += 2;
        }

        command(0, XBAR_CONTROLLER_ADDR, XBAR_REG_LOCAL_CMD, 0x1);

        //We need to wait here until the XBAR is configured. 
        //TODO: GET THE READ BUS BACK SO WE CAN WAIT HERE UNTIL CONFIG IS DONE
        USBTIMER_DelayMs(10);

        infop("\nXBAR configured\n");

        xbarProgrammed = 1;
        mecoboStatus = MECOBO_STATUS_XBAR_CONFIGURED;
    }


    else if(currentPack.command == USB_CMD_UPDATE_REGISTER)
    {
        debug("Updating Register\n");
        int * d = (int*)(currentPack.data);
        debug("REG %d: %d\n", d[0], d[1]); 
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

        uint32_t * d = (uint32_t *)(currentPack.data);
        if(*d >= 1) {
            has_daughterboard = 1;

            if(!ad_and_da_configured) {
               setupDAC();
               setupADC();
               ad_and_da_configured = 1;
            }

        } else {
            has_daughterboard = 0;
        }


        infop("----------------- RESETING MECOBO ------------------\n");
        if(has_daughterboard) 
        {
            infop("DAUGHTERBOARD: PRESENT\n");
        } else {
            infop("DAUGHTERBOARD: NONE\n");
        }
        resetTime();
        softReset();   //This will clear the command fifo, etc.

        startInput();  //start sample collector

        adcSequence[0] = 0;


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

        firstSampleDiscarded = 0;
        timeStarted = 0;
        xbarProgrammed = 0;
        setupFinished = 0;

        feedFpgaCmdFifo = 1;
        sampleFifoEmpty = 1;

        samplesToGet = 0;

        infop("\n\n---------------- MECOBO RESET DONE ------------------\n\n");
        mecoboStatus = MECOBO_STATUS_RESET_COMPLETE;

    }

    else if(currentPack.command == USB_CMD_GET_INPUT_BUFFER_SIZE) {

        uint32_t * retSamples = (uint32_t*)malloc(4);

        *retSamples = min(ucSampleFifo.numElements, USB_MAX_SAMPLES);

        sendPacket(4, USB_CMD_GET_INPUT_BUFFER_SIZE, (uint8_t*)(retSamples));
    }

    else if(currentPack.command == USB_CMD_GET_INPUT_BUFFER) {


        //Send back the whole sending buffer.
        //Data contains the number of samples to xfer.
        uint32_t * txSamples = (uint32_t *)(currentPack.data);
        int bytes = sizeof(struct sampleValue) * *txSamples;
        struct sampleValue * samples = (struct sampleValue *)malloc(bytes);
        if(!samples) {
            infop("ERROR: MALLOC FAILED. We're probably out of memory!\n");
        }
        debug("Shipping %d bytes\n", bytes);

        for(unsigned int i = 0; i < *txSamples; i++) {
            struct sampleValue * sv;
            fifoGet(&ucSampleFifo, (void*)&sv);
            samples[i] = *sv;
        }

        sendPacket(bytes, USB_CMD_GET_INPUT_BUFFER, (uint8_t*)samples);

        debug("USB has been instructed to send data. Returning to main loop\n");
    }



    else if(currentPack.command == USB_CMD_LOAD_BITFILE) {


        //bitfileoffset magic.
        if(bitfileOffset == NOR_FLASH_NODB_POS) {
            eraseNorChip();  //we want to chuck in a new file, kill this.
            //TODO: fix per-region flash kill
        }
        if(bitfileOffset == NOR_FLASH_DB_POS) {
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
        debug("Got data, current offset %u\n", (unsigned int)bitfileOffset);
    }


    else if(currentPack.command == USB_CMD_PROGRAM_FPGA) {
        programFPGA();
        bitfileOffset = 0;
    }

    //The intention is for this to be run before other items.
    else if (currentPack.command == USB_CMD_LOAD_SETUP) { 
        infop("USB_CMD_LOAD_SETUP\n");


        mecoboStatus = MECOBO_STATUS_BUSY;
    
        while(ucCmdFifo.numElements > 0) {
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
        while(ucCmdFifo.numElements > 0) {
            void * item = NULL;
            fifoGet(&ucCmdFifo, &item);
            struct pinItem * it = (struct pinItem *)item;
            pushToCmdFifo(it);
        }


        mecoboStatus = MECOBO_STATUS_FIFO_PRELOADED;
        infop("---- SETUP FINISHED -----\n");

        setupFinished = 1;
    }

    //if/else done.
    executeInProgress = 0;

    if(currentPack.size > 0) {
        if(currentPack.data != NULL) {
            free(currentPack.data);
        } else {
            infop("ERROR: FREE CURRENT PACK NULL\n");
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
    sendInProgress = 1;
    USBD_Write(CDC_EP_DATA_IN, packToSend.data, packToSend.size, UsbDataSent);
}

void resetAllPins()
{
    infop("Reseting all digital pin controllers\n");
    for(int j = 0; j < 50; j++) {
        command(0, j, PINCONTROL_REG_LOCAL_CMD, PINCONTROL_CMD_RESET);
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
    //Light the beacon!
    int toggle = 0;

    autoSelectNor(); //set NOR in read mode
    infop("Programming FPGA!\n");
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
        debug("Waiting for initb and done\n");
    }

    while(GPIO_PinInGet(gpioPortD, 15)) {
        debug("Waiting for INIT_B to go high\n");
    }

    //set prog b high again (activate programming)
    GPIO_PinOutSet(gpioPortC, 3);
    //wait until initB goes high again (fpga ready for data)
    while(!GPIO_PinInGet(gpioPortD, 15)) {
        debug("Waiting for initb to go high again\n");
    }
    //Bring front led 2 on to indicate programming.

    GPIO_PinModeSet(gpioPortD, 6, gpioModeInput, 1);

    //uint32_t * n32 = (uint32_t *)(NOR_START + 4); //this is where we store the length
    //uint32_t bitfileLength = *n32; //stored at the 4 bytes
    //int nb = 0;


    struct NORFileTableEntry * entries = (struct NORFileTableEntry *)NOR_START;
    infop("Found a bitfile, size %u\n", (unsigned int)entries[0].size);
    //TODO: Select which bitfile to use here. For now,
    //assume only 1 bitfile present.
    uint8_t * nor = (uint8_t *)(NOR_START + entries[0].offset); //this is where data starts

    uint32_t nb = 0;
    infop("Programming bitfile\n");
    while(nb < entries[0].size) {
        uint8_t bait = nor[nb];

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
            led(BOARD_LED_U3, toggle);
            debug("Last byte programmed: %x\n", bait);
            if(!GPIO_PinInGet(gpioPortD, 15)) {
                infop("CRC error during config, INITB went high!\n");
                return;
            } else {
                debug("BYTE OK\n");
            }
            toggle = !toggle;
        }
    }

    infop("FPGA programming done\n");
    led(BOARD_LED_U3, 0);
}

void eraseNorChip()
{
    uint16_t * nor = (uint16_t *)NOR_START;
    infop("Doing chip erase\n");
    nor[0x555] = 0xCA;
    nor[0x2AA] = 0x35;
    nor[0x555] = 0x80;
    nor[0x555] = 0xCA;
    nor[0x2AA] = 0x35;
    nor[0x555] = 0x10;

    //while(NORPollData(1, 0));
    NORBusy();

    infop("Done\n");
}



void NORBusy() {

    uint16_t * nor = (uint16_t *)NOR_START;
#define TOGGLE_BIT 0x20
    uint8_t status = 0;
    uint8_t done = 0;
    int counter = 0;
    int toggle = 1;
    while(!done) {
        status = nor[0];
        //read once more
        if((nor[0] & TOGGLE_BIT) == (status & TOGGLE_BIT)) {
            done = 1;
        }
        if (((counter++)%100000) == 0) {
            debug("NOR busy... %x\n", status);
            led(BOARD_LED_U2, toggle);
            toggle = !toggle;
        }
    }
    infop("Final status %x\n", nor[0]);
    led(BOARD_LED_U2, 0);

    return;
}

void resetNor()
{
    infop("NOR RESET\n");
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
    debug("NOR: Entering enhanced mode\n");
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
    debug("NOR: Exit enhanced mode\n");
}


/* Writes 256 16-bit words of data to the NOR */
void write256Buffer(uint16_t * data, uint32_t offset)
{
    uint16_t * nor = (uint16_t *)NOR_START;
    /* Set in auto mode select mode */
    //autoSelectNor();

    uint32_t bad = offset & 0xFFFFFF00;
    debug("Writing 512 bytes from data %p at offset 0x%x, block address %x\n", data, (unsigned int)offset, (unsigned int)bad);

    //Now start command
    nor[bad] = 0x53;

    for (int i = 0; i < 256; i++) {
        nor[offset + i] = data[i];
    }


    nor[bad] = 0x49;

    //Wait until we no longer toggle
    while(NORToggling());

    //NORError();

    debug("Wrote bytes\n");
}


#define TOGGLE_BIT 0x20
int NORToggling() 
{
    uint16_t * nor = (uint16_t *)NOR_START;
    uint16_t a = nor[0];
    uint16_t b = nor[0];

    if ((a & TOGGLE_BIT) != (b & TOGGLE_BIT)) {
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
        infop("Error detected. Status reg: %x\n", status);
        return 1;
    }
    else return 0;
}


void fpgaIrqHandler(uint8_t pin) {

    infop("irq: reg: %x\n", pin);
    infop("irq: status: %x\n", statusReg);
    checkStatusReg();
}

int sampleFifoDataCount()
{
    return (int)ebir(EBI_SAMPLE_FIFO_DATA_COUNT);
}

inline uint16_t cmdFifoDataCount()
{
    return (int)ebir(EBI_CMD_FIFO_DATA_COUNT);
}


void checkStatusReg() 
{
    statusReg = cmdInterfaceAddr[0];
    if (statusReg & STATUS_REG_CMD_FIFO_ALMOST_FULL_BIT) {
        infop("CMD FIFO almost full, no more feeding\n");
        feedFpgaCmdFifo = 0;
    }

    if (statusReg & STATUS_REG_CMD_FIFO_EMPTY) {
        feedFpgaCmdFifo = 1;
    } 
    if (statusReg & STATUS_REG_CMD_FIFO_ALMOST_EMPTY) {
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


    ebiw(1, serialCmd[1]);
    ebiw(2, serialCmd[0]);
    ebiw(3, cmd->data[0]);
    ebiw(4, cmd->data[1]);
    ebiw(5, serialCmd[4]);

    /*
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
    */
}



//The write to the FPGA will make the data into a big endian word.
//the uc is little endian so the least significant byte of a 32 bit word 
//is stored first in memory:
//addr: 0x0 0x1 0x2  0x3
//byte: 1    2   3   4
//
//the fpga wants things the other way around. 
//
//this is a little wonky, but the EBI interface will but the bits 
//in msb to lsb order on the data lines, and we write in 16 bit words. 
//
//we thus need to write the highest (in memory addr) two bytes of the 32 bit word FIRST since they
//are more significant. this happens in cmd.data[0] = data16[1]  -- highest two
//bytes to FIRST addr.
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
    cmd.data[0] = data1; //TODO: Figure out why this is correct.
    cmd.data[1] = data2; 
    debug("reg: %x F1: %x F2: %x\n", reg, data2, data1);
    putInFifo(&cmd);
}

void pushToCmdFifo(struct pinItem * item)
{
    //struct fifoCmd cmd = makeCommand(item->startTime, (uint8_t)item->pin, 0x0, 0x0);

    debug("fifo: %u, pin %u\n", (unsigned int)item->startTime, (unsigned int)item->pin);
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
            debug(">> CONST DAC, t%x, ch %x, %x\n", (unsigned int)item->startTime, item->pin, item->constantValue);
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




