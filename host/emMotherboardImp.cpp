#include "emMotherboard.h"
#include "mecohost.h"
#include <iostream>

emMotherboard::emMotherboard()
{
  fpgaConfigured = false;
  currentBufferElements = 0;
}

bool emMotherboard::isFpgaConfigured()
{
  return UsbIsFpgaConfigured();
}
