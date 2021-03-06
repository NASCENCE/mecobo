#include "em_ebi.h"
#include "em_timer.h"
#include "em_common.h"
#include "mecobo.h"

static EBI_Init_TypeDef ebiConfig = {   

  ebiModeD16,      /* 8 bit address, 8 bit data */  \
    ebiActiveLow,     /* ARDY polarity */              \
    ebiActiveLow,     /* ALE polarity */               \
    ebiActiveLow,     /* WE polarity */                \
    ebiActiveLow,     /* RE polarity */                \
    ebiActiveLow,     /* CS polarity */                \
    ebiActiveLow,     /* BL polarity */                \
    false,            /* enable BL */                  \
    false,            /* enable NOIDLE */              \
    false,             /* enable ARDY */                \
    false,            /* don't disable ARDY timeout */ \
    EBI_BANK0,        /* enable bank 0 */              \
    EBI_CS0,          /* enable chip select 0 */       \
    0,                /* addr setup cycles */          \
    0,                /* addr hold cycles */           \
    false,            /* do not enable half cycle ALE strobe */ \
    1,                /* read setup cycles */          \
    3,                /* read strobe cycles */         \
    1,                /* read hold cycles */           \
    false,            /* disable page mode */          \
    false,            /* disable prefetch */           \
    false,            /* do not enable half cycle REn strobe */ \
    1,                /* write setup cycles */         \
    3,                /* write strobe cycles */        \
    1,                /* write hold cycles */          \
    false,            /* do not disable the write buffer */ \
    false,            /* do not enable halc cycle WEn strobe */ \
    ebiALowA0,        /* ALB - Low bound, address lines */ \
    ebiAHighA20,       /* APEN - High bound, address lines */   \
    ebiLocation1,     /* Use Location 0 */             \
    true,             /* enable EBI */                 \
};

static EBI_Init_TypeDef ebiConfigSRAM1 = {   

  ebiModeD16,      /* 8 bit address, 8 bit data */  \
    ebiActiveLow,     /* ARDY polarity */              \
    ebiActiveLow,     /* ALE polarity */               \
    ebiActiveLow,     /* WE polarity */                \
    ebiActiveLow,     /* RE polarity */                \
    ebiActiveHigh,     /* CS polarity */                \
    ebiActiveLow,     /* BL polarity */                \
    false,            /* enable BL */                  \
    false,            /* enable NOIDLE */              \
    false,            /* enable ARDY */                \
    false,            /* don't disable ARDY timeout */ \
    EBI_BANK1,        /* enable bank 0 */              \
    EBI_CS1,          /* enable chip select 0 */       \
    0,                /* addr setup cycles */          \
    0,                /* addr hold cycles */           \
    false,            /* do not enable half cycle ALE strobe */ \
    1,                /* read setup cycles */          \
    3,                /* read strobe cycles */         \
    1,                /* read hold cycles */           \
    false,            /* disable page mode */          \
    false,            /* disable prefetch */           \
    false,            /* do not enable half cycle REn strobe */ \
    1,                /* write setup cycles */         \
    3,                /* write strobe cycles */        \
    1,                /* write hold cycles */          \
    true,            /* do not disable the write buffer */ \
    false,            /* do not enable halc cycle WEn strobe */ \
    ebiALowA0,        /* ALB - Low bound, address lines */ \
    ebiAHighA20,       /* APEN - High bound, address lines */   \
    ebiLocation1,     /* Use Location 0 */             \
    true,             /* enable EBI */                 \
};


static EBI_Init_TypeDef ebiConfigSRAM2 = {   

  ebiModeD16,      /* 8 bit address, 8 bit data */  \
    ebiActiveLow,     /* ARDY polarity */              \
    ebiActiveLow,     /* ALE polarity */               \
    ebiActiveLow,     /* WE polarity */                \
    ebiActiveLow,     /* RE polarity */                \
    ebiActiveHigh,     /* CS polarity */                \
    ebiActiveLow,     /* BL polarity */                \
    false,            /* enable BL */                  \
    false,            /* enable NOIDLE */              \
    false,            /* enable ARDY */                \
    false,            /* don't disable ARDY timeout */ \
    EBI_BANK2,        /* enable bank 0 */              \
    EBI_CS2,          /* enable chip select 0 */       \
    0,                /* addr setup cycles */          \
    0,                /* addr hold cycles */           \
    false,            /* do not enable half cycle ALE strobe */ \
    1,                /* read setup cycles */          \
    3,                /* read strobe cycles */         \
    1,                /* read hold cycles */           \
    false,            /* disable page mode */          \
    false,            /* disable prefetch */           \
    false,            /* do not enable half cycle REn strobe */ \
    1,                /* write setup cycles */         \
    3,                /* write strobe cycles */        \
    1,                /* write hold cycles */          \
    true,            /* do not disable the write buffer */ \
    false,            /* do not enable halc cycle WEn strobe */ \
    ebiALowA0,        /* ALB - Low bound, address lines */ \
    ebiAHighA20,       /* APEN - High bound, address lines */   \
    ebiLocation1,     /* Use Location 0 */             \
    true,             /* enable EBI */                 \
};


static TIMER_Init_TypeDef timerInit = {
  .enable     = true, 
  .debugRun   = true, 
  .prescale   = timerPrescale64,
  .clkSel     = timerClkSelHFPerClk, 
  .fallAction = timerInputActionNone, 
  .riseAction = timerInputActionNone, 
  .mode       = timerModeUp, 
  .dmaClrAct  = false,
  .quadModeX4 = false, 
  .oneShot    = false, 
  .sync       = false, 
};


