#include "em_ebi.h"
#include "em_timer.h"
#include "em_common.h"
#include "descriptors.h"
#include "mecobo.h"

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
    1,                /* read setup cycles */          \
    2,                /* read strobe cycles */         \
    0,                /* read hold cycles */           \
    false,            /* disable page mode */          \
    false,            /* disable prefetch */           \
    false,            /* do not enable half cycle REn strobe */ \
    1,                /* write setup cycles */         \
    2,                /* write strobe cycles */        \
    0,                /* write hold cycles */          \
    false,            /* do not disable the write buffer */ \
    false,            /* do not enable halc cycle WEn strobe */ \
    ebiALowA0,        /* ALB - Low bound, address lines */ \
    ebiAHighA18,       /* APEN - High bound, address lines */   \
    ebiLocation1,     /* Use Location 0 */             \
    true,             /* enable EBI */                 \
};

TIMER_Init_TypeDef timerInit = {
  .enable     = true, 
  .debugRun   = true, 
  .prescale   = timerPrescale1024,
  .clkSel     = timerClkSelHFPerClk, 
  .fallAction = timerInputActionNone, 
  .riseAction = timerInputActionNone, 
  .mode       = timerModeUp, 
  .dmaClrAct  = false,
  .quadModeX4 = false, 
  .oneShot    = false, 
  .sync       = false, 
};

const USBD_Init_TypeDef initstruct = 
{
  .deviceDescriptor    = &deviceDesc,
  .configDescriptor    = configDesc,
  .stringDescriptors   = strings,
  .numberOfStrings     = sizeof(strings)/sizeof(void*),
  .callbacks           = &callbacks,
  .bufferingMultiplier = bufferingMultiplier,
  .reserved            = 0
};

const USBD_Callbacks_TypeDef callbacks = 
{
  .usbReset        = NULL,
  .usbStateChange  = UsbStateChange,
  .setupCmd        = NULL,
  .isSelfPowered   = NULL,
  .sofInt          = NULL
};

