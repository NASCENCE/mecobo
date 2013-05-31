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


/*** Typedef's and defines. ***/

/* Define USB endpoint addresses */
#define EP_DATA_OUT1       0x01  /* Endpoint for USB data reception.       */
#define EP_DATA_OUT2       0x02  /* Endpoint for USB data reception.       */
#define EP_DATA_IN1        0x81  /* Endpoint for USB data transmission.    */
#define EP_DATA_IN2        0x82  /* Endpoint for USB data transmission.    */
#define EP_NOTIFY         0x82  /* The notification endpoint (not used).  */

#define BULK_EP_SIZE     USB_MAX_EP_SIZE  /* This is the max. ep size.    */

#include "descriptors.h"


/**************************************************************************//**
 * @brief main - the entrypoint after reset.
 *****************************************************************************/
int main(void)
{
   BSP_Init(BSP_INIT_DEFAULT);   /* Initialize DK board register access */
   CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

  USBD_Init(&initstruct);

  /*
   * When using a debugger it is practical to uncomment the following three
   * lines to force host to re-enumerate the device.
   */
  USBD_Disconnect();
  USBTIMER_DelayMs(1000);
  USBD_Connect();

  
  uint8_t data[BULK_EP_SIZE];
  uint8_t buffer[512*128];
  //read and write in loop	
  for (;;)
  {
	USBD_Read(EP_DATA_OUT1, (void*)buffer, 4096, NULL);
	//USBD_Write(EP_DATA_IN1, (void*)buffer, 64, NULL);
  }
}
