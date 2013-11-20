#ifndef __EMMOTHERBOARD_H__
#define __EMMOTHERBOARD_H__

class emMotherboard {
  bool fpgaConfigured;
  int currentBufferElements;

  public:
    emMotherboard();

    bool isFpgaConfigured();
};
#endif
