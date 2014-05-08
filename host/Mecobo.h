/*
 * Mecobo.h
 *
 *  Created on: May 7, 2014
 *      Author: oddrune
 */

#ifndef MECOBO_H_
#define MECOBO_H_

#include <map>
#include <vector>
#include "emEvolvableMotherboard.h"
#include "USB.h"
#include "../mecoprot.h"
#include "channelmap.h"

class Mecobo
{
private:

  void createMecoPack(struct mecoPack * packet, uint8_t * data,  uint32_t dataSize, uint32_t command);

  bool hasDaughterboard;
  USB usb;
  channelMap xbar;
  std::map<int, std::vector<uint32_t>> rec;

public:
  Mecobo (USB & channel);
  virtual
  ~Mecobo ();

  void sendPacket(struct mecoPack * packet);
  bool isFpgaConfigured();
  void programFPGA(const char * filename);
  //Convenience method for sending the emSequenceItems.
  //TODO: I don't like to mention emSequenceItem here. Coupling seems to tight. Oh well.
  //void submitSequenceItem(emInterfaces::emSequenceItem & item);

  //Various board capabilities
  void scheduleConstantVoltage(int pin, int start, int end, int amplitude);
  void scheduleRecording(int pin, int start, int end, int frequency);

  void scheduleDigitalRecording(int pin, int start, int end, int frequency);
  void scheduleDigitalOutput(int pin, int start, int end, int frequency, int dutyCycle);
  void schedulePWMoutput(int pin, int start, int end, int pwmValue);
  void scheduleSine(int pin, int start, int end, int frequency, int amplitude, int phase);


  std::vector<sampleValue> getSampleBuffer();
  void reset();
  void setLed(int a, int b);

};


#endif /* MECOBO_H_ */
