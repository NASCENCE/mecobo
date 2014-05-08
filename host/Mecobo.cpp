/*
 * Mecobo.cpp
 *
 *  Created on: May 7, 2014
 *      Author: oddrune
 */

#include "Mecobo.h"
#include <stdexcept>


Mecobo::Mecobo (USB & usb)
{
  this->usb = usb;
  hasDaughterboard = true;
}

Mecobo::~Mecobo ()
{
  // TODO Auto-generated destructor stub
}

void Mecobo::scheduleConstantVoltage(int pin, int start, int end, int amplitude)
{

  FPGA_IO_Pins_TypeDef channel = (FPGA_IO_Pins_TypeDef)0;
  //Find a channel (or it might throw an error).
  if(hasDaughterboard) {
    channel = xbar.getChannel(pin);
  } else {
    channel = (FPGA_IO_Pins_TypeDef)pin;
  }

  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_CONST] = (uint32_t)amplitude;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_DAC_CONST;

  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);
  sendPacket(&p);
}

void Mecobo::scheduleRecording(int pin, int start, int end, int frequency)
{

  FPGA_IO_Pins_TypeDef channel = (FPGA_IO_Pins_TypeDef)0;
  //Find a channel (or it might throw an error).
  if(hasDaughterboard) {
    channel = xbar.getChannel(pin);
  } else {
    channel = (FPGA_IO_Pins_TypeDef)pin;
  }

  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_RECORD_ANALOGUE;

  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);
  sendPacket(&p);
}


void Mecobo::createMecoPack(struct mecoPack * packet, uint8_t * data,  uint32_t dataSize, uint32_t command)
{
  if(dataSize > 0) {
    packet->data = (uint8_t*)malloc(dataSize);
    memcpy(packet->data, data, dataSize);
  } else {
    packet->data = NULL;
  }

  packet->size = dataSize;
  packet->command = command;
}


void Mecobo::sendPacket(struct mecoPack * packet)
{
  //First, send header (fixed 8 bytes)
  //Create a buffer of data to send.
  uint32_t toSend[2];
  toSend[0] = packet->size;
  toSend[1] = packet->command;

  //Send header to prepare board for data.
  usb.sendBytesDefaultEndpoint((uint8_t*)toSend, 8);
  usb.sendBytesDefaultEndpoint(packet->data, packet->size);
}


void Mecobo::programFPGA(const char * filename)
{
  FILE* bitfile;


#ifdef WIN32
  int openResult = fopen_s(&bitfile, filename, "rb");
  perror ("programFPGA");
#else
  bitfile = fopen(filename, "rb");
#endif

  printf("Programming FPGA\n");
  fseek(bitfile, 0L, SEEK_END);
  long nBytes = ftell(bitfile);
  rewind(bitfile);

  int packsize = 32*1024;
  int nPackets = nBytes / packsize;
  int rest = nBytes % (packsize);
  printf("supposed to have ballpark 6,440,432 bits. have %ld\n", nBytes * 8);
  printf("file is %ld bytes, sending %d packets of %d bytes and one pack of %d bytes\n",
        nBytes, nPackets, packsize, rest);
  uint8_t * bytes;
  bytes = (uint8_t *)malloc(nBytes);

  fread(bytes, 1, nBytes, bitfile);

  struct mecoPack send;
  int i;
  for(i =0; i < nPackets; i++) {
    printf("Sending pack %d of %d, %d bytes of %ld for fpga programming\n", i + 1, nPackets, packsize, nBytes);
    printf("position %u in array\n", (i * packsize));
    createMecoPack(&send, bytes + (i*packsize), packsize, USB_CMD_PROGRAM_FPGA);
    sendPacket(&send);
  }
  //Send the rest if there is any.
  if(rest > 0) {
    printf("Sending the rest pack, position %u, size %d\n", (i*packsize), rest);
    struct mecoPack lol;
    createMecoPack(&lol, bytes + (i*packsize), rest, USB_CMD_PROGRAM_FPGA);
    sendPacket(&lol);
  }

  free(bytes);
  printf("\n");
  fclose(bitfile);

}

std::vector<int32_t> Mecobo::getSampleBuffer(int i)
{
  std::vector<sampleValue> samples;

  //Send request for buffer size
  struct mecoPack pack;
  createMecoPack(&pack, 0, 0, USB_CMD_GET_INPUT_BUFFER_SIZE);
  sendPacket(&pack);

  //Retrieve bytes.
  uint32_t nSamples = 0;
  usb.getBytesDefaultEndpoint((uint8_t *)&nSamples, 4);

  //Got the input buffer size back, now we collect it.
  std::cout << "SampleBuffer size collected: " << nSamples << std::endl;
  if(nSamples == 0) {
    return pinRecordings[i];
  } else {
    sampleValue* collectedSamples = new sampleValue[nSamples];

    //Ask for samples back.
    createMecoPack(&pack, (uint8_t*)&nSamples, 4, USB_CMD_GET_INPUT_BUFFER);
    sendPacket(&pack);

    //Get bytes back. Note that the USB uses a raw byte pointer.
    std::cout << "Receiving bytes:" << sizeof(sampleValue) * nSamples << std::endl;
    usb.getBytesDefaultEndpoint((uint8_t*)collectedSamples, sizeof(sampleValue) * nSamples);
    std::cout << "Collected from USB" << std::endl;
    sampleValue * casted = (sampleValue *)collectedSamples;

    for(int i = 0; i < (int)nSamples; i++) {
      samples.push_back(casted[i]);
    }

    delete[] collectedSamples;
  }

  //The samples we get back are associated with a certain channel,
  //we need to convert it to pins to see what the user actually
  //expected out.
  for(auto s : samples) {
    //Since one channel can be on many pins we'll collect them all.
    std::vector<int> pin = xbar.getPin((FPGA_IO_Pins_TypeDef)s.channel);
    for (auto p : pin) {
      std::cout << "Samples for channel " << s.channel << "pin: " << p << " :" << s.sampleNum << std::endl;
      pinRecordings[p].push_back((int32_t)s.value);
    }
  }

  return pinRecordings[i];
}

void Mecobo::reset()
{
  struct mecoPack p;
  createMecoPack(&p, 0, 0, USB_CMD_RESET_ALL);
  sendPacket(&p);
}

bool
Mecobo::isFpgaConfigured ()
{
  return true;
}

void
Mecobo::scheduleDigitalRecording (int pin, int start, int end, int frequency)
{
  throw std::runtime_error("digital output Not implemented.");
}

void
Mecobo::scheduleDigitalOutput (int pin, int start, int end, int frequency,
			       int dutyCycle)
{
  throw std::runtime_error("digital output Not implemented.");
}

void
Mecobo::schedulePWMoutput (int pin, int start, int end, int pwmValue)
{
  throw std::runtime_error("PWM not implemented yet :/");
}

void
Mecobo::scheduleSine (int pin, int start, int end, int frequency, int amplitude,
		      int phase)
{
  throw std::runtime_error("Sine not implemented yet :(");
}

void
Mecobo::runSchedule ()
{
  //Set up the crossbar for this sequence.
  std::cout << "Setting up XBAR" << std::endl;
  std::vector<uint8_t> test = xbar.getXbarConfigBytes();
  this->setXbar(test);

  struct mecoPack p;
  createMecoPack(&p, NULL, 0, USB_CMD_RUN_SEQ);
  sendPacket(&p);
}

void
Mecobo::setXbar(std::vector<uint8_t> & bytes)
{
  struct mecoPack p;
  createMecoPack(&p, bytes.data(), 64, USB_CMD_PROGRAM_XBAR);
  sendPacket(&p);
}

void
Mecobo::clearSchedule ()
{
  throw std::runtime_error("clearSchedule() not implemented yet");
}

void Mecobo::setLed(int led, int mode) {
  struct mecoPack p;
  uint32_t dat[2];
  dat[LED_SELECT] = (uint32_t)led;
  dat[LED_MODE] = (uint32_t)mode;
  createMecoPack(&p, (uint8_t*)dat, 8, USB_CMD_LED);
  sendPacket(&p);
}
