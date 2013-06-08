#include "em_device.h"
#include "em_cmu.h"
#include "em_dma.h"
#include "em_dma.h"
#include "em_gpio.h"
#include "em_int.h"
#include "dmactrl.h"
#include "em_usb.h"
#include "em_usart.h"
#include "bsp.h"
#include "bsp_trace.h"

#include <stdlib.h>
#include "mecobo.h"
#include "queue.h"


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
/**************************************************************************//**
 * @brief main - the entrypoint after reset.
 *****************************************************************************/
int main(void)
{
    BSP_Init(BSP_INIT_DEFAULT);
    /* Enable HFXO as high frequency clock, HFCLK (depending on external oscillator this will probably be 48MHz) */
    //CMU->OSCENCMD = CMU_OSCENCMD_HFXOEN;
    //while (!(CMU->STATUS & CMU_STATUS_HFXORDY)) ;
    //CMU->CMD = CMU_CMD_HFCLKSEL_HFXO;
    /* No LE clock source selected */

    /* Enable GPIO clock */
    //        CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

    USBD_Init(&initstruct);

    /*
     * When using a debugger it is practical to uncomment the following three
     * lines to force host to re-enumerate the device.
     */
    USBD_Disconnect();
    USBTIMER_DelayMs(1000);
    USBD_Connect();

    /* Setup DMA */
//    setupDma();

    //queueInit(&dataIn, 1024*2);
    int bufferSize = 47;
    uint8_t * byteBuffer = malloc(bufferSize);
    int i;
    for(i = 0; i < bufferSize; i++) {
        byteBuffer[i] = 0;
    }
    int bufferPos = 0;

    uint8_t * sendBuff = malloc(bufferSize);
    
    //read and write in loop	
    for (;;)
    {
        //Read head
        usbRxActive = 1;
        int readCount = 0;
        while(usbRxActive) {
            //This essentially attempts to fill the byteBuffer until it succeeds.
            USBD_Read(EP_DATA_OUT1, byteBuffer, bufferSize, UsbDataReceived);
            readCount++;
        }
        
        int b;
        for(b = 0; b < bufferSize; b++) {
            sendBuff[b] = byteBuffer[b] + 1;
        }
        sendBuff[0] = readCount;
        usbTxActive = 1;
        while(usbTxActive) {
            //Try to write data until success!
            USBD_Write(EP_DATA_IN1, sendBuff, bufferSize, UsbDataSent);
        }
    }
}

int UsbDataReceived(USB_Status_TypeDef status,
                            uint32_t xf,
                            uint32_t remaining) 
{
    //(void) xf;
    (void) remaining;
    if ((status == USB_STATUS_OK) && (xf > 0)) {
        usbRxActive = 0;
    } 
    return USB_STATUS_OK;
}

int UsbDataSent(USB_Status_TypeDef status,
        uint32_t xf,
        uint32_t remaining)
{
    (void) xf;
    (void) remaining;

    if (status == USB_STATUS_OK) 
    {
        usbTxActive = 0;
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

