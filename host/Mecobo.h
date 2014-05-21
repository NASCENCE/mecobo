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
  void setXbar(std::vector<uint8_t> & bytes);
  bool hasDaughterboard;
  USB usb;
  channelMap xbar;

  //Recordings will be -5 to 5V.
  std::map<int, std::vector<int32_t>> pinRecordings;

public:
  Mecobo ();
  virtual
  ~Mecobo ();

  void sendPacket(struct mecoPack * packet);
  bool isFpgaConfigured();
  void programFPGA(const char * filename);
  //Convenience method for sending the emSequenceItems.
  //TODO: I don't like to mention emSequenceItem here. Coupling seems to tight. Oh well.
  //void submitSequenceItem(emInterfaces::emSequenceItem & item);

  //Various board capabilities
  void scheduleConstantVoltage(std::vector<int> pins, int start, int end, int amplitude);
  void scheduleRecording(std::vector<int> pins, int start, int end, int frequency);

  void scheduleDigitalRecording(std::vector<int> pins, int start, int end, int frequency);
  void scheduleDigitalOutput(std::vector<int> pins, int start, int end, int frequency, int dutyCycle);
  void schedulePWMoutput(std::vector<int> pins, int start, int end, int pwmValue);
  void scheduleSine(std::vector<int> pins, int start, int end, int frequency, int amplitude, int phase);
  void scheduleConstantVoltageFromRegister(std::vector<int> pins, int start, int end, int reg);
  

  void runSchedule();
  void clearSchedule();

  //Retrieves the sample buffer that's built up currently. This will of course cause a little bit of delay.
  std::vector<int32_t> getSampleBuffer(int pin);

  void reset();
  void setLed(int a, int b);
  void updateRegister(int index, int value);

};

template <typename T, unsigned B>
inline T signextend(const T x)
{
    struct {T x:B;} s;
      return s.x = x;

}

#endif /* MECOBO_H_ */
