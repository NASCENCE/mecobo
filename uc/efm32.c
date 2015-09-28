
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
#include "gpiointerrupt.h"

#include "struct_init.h"

extern void fpgaIrqHandler(uint8_t pin);

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

  TIMER_Init(TIMER1, &timerInit);
  TIMER_Init(TIMER2, &timerInit);
  //special NOR SETUP
  EBI_Init_TypeDef ebiConfigNOR = EBI_INIT_DEFAULT;

  //EBI_Init(&ebiConfigNOR);
  ebiConfigNOR.mode = ebiModeD16;
  ebiConfigNOR.wePolarity = ebiActiveLow;
  ebiConfigNOR.rePolarity = ebiActiveLow;
  ebiConfigNOR.csPolarity = ebiActiveLow;

  //ebiConfigNOR.addrSetupCycles = 1;
 // ebiConfigNOR.addrHoldCycles = 3;

  ebiConfigNOR.banks = EBI_BANK3;
  ebiConfigNOR.csLines = EBI_CS3;

  ebiConfigNOR.aLow = ebiALowA0;
  ebiConfigNOR.aHigh = ebiAHighA23;
  ebiConfigNOR.location = ebiLocation1;

  //ebiConfig.addrHoldCycles  = 5;
  //ebiConfig.addrSetupCycles = 5;

  /* Read cycle times */
  ebiConfigNOR.readStrobeCycles = 11;
  ebiConfigNOR.readHoldCycles   = 2;
  ebiConfigNOR.readSetupCycles  = 2;

  /* Write cycle times */
  ebiConfigNOR.writeStrobeCycles = 11;
  ebiConfigNOR.writeHoldCycles   = 2;
  ebiConfigNOR.writeSetupCycles  = 2;

  /* Configure GPIO pins as push pull */ /* EBI AD9..15 */
  GPIO_PinModeSet( gpioPortA,  0, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortA,  1, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortA,  2, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortA,  3, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortA,  4, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortA,  5, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortA,  6, gpioModePushPull, 0 );

  /* EBI AD8 */
  GPIO_PinModeSet( gpioPortA, 15, gpioModePushPull, 0 );

  /* EBI CS0-CS3 */
  GPIO_PinModeSet( gpioPortD,  9, gpioModePushPull, 1 );
  GPIO_PinModeSet( gpioPortD, 10, gpioModePushPull, 1 );
  GPIO_PinModeSet( gpioPortD, 11, gpioModePushPull, 1 );
  GPIO_PinModeSet( gpioPortD, 12, gpioModePushPull, 1 );

  /* EBI AD0..7 */
  GPIO_PinModeSet( gpioPortE,  8, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortE,  9, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortE, 10, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortE, 11, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortE, 12, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortE, 13, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortE, 14, gpioModePushPull, 0 );
  GPIO_PinModeSet( gpioPortE, 15, gpioModePushPull, 0 );

  //chipsels
  GPIO_PinModeSet( gpioPortD,  9, gpioModePushPull, 1 ); //cs0
  GPIO_PinModeSet( gpioPortD, 10, gpioModePushPull, 1 ); //cs1
  GPIO_PinModeSet( gpioPortD, 11, gpioModePushPull, 1 ); //cs2
  GPIO_PinModeSet( gpioPortD, 12, gpioModePushPull, 1 ); //cs3

  //wen ren
  GPIO_PinModeSet( gpioPortF,  8, gpioModePushPull, 1 );
  GPIO_PinModeSet( gpioPortF,  9, gpioModePushPull, 1 );

  
  /* Initialize GPIO interrupt dispatcher */
  printf("Initializing interrupts\n");
  GPIOINT_Init();
  GPIO_PinModeSet( gpioPortD, 14, gpioModeInput, 0); //irq
  GPIOINT_CallbackRegister(14, fpgaIrqHandler);

  /* Set rising edge interrupt for both ports */
  GPIO_IntConfig(gpioPortD, 14, true, false, true);

  EBI_Init(&ebiConfig);
  EBI_Init(&ebiConfigSRAM1);
  EBI_Init(&ebiConfigSRAM2);
  EBI_Init(&ebiConfigNOR);
}

