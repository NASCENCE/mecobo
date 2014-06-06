/*
 * Mecobo.cpp
 *
 *  Created on: May 7, 2014
 *      Author: oddrune
 */

#include "Mecobo.h"
#include <stdexcept>
#include <cmath>
#include <chrono>
#include <thread>


Mecobo::Mecobo ()
{
  hasDaughterboard = true;
  std::cout << "Mecobo initialized" << std::endl;
}

Mecobo::~Mecobo ()
{
  // TODO Auto-generated destructor stub
}

void Mecobo::scheduleConstantVoltage(std::vector<int> pins, int start, int end, int amplitude)
{

  FPGA_IO_Pins_TypeDef channel = (FPGA_IO_Pins_TypeDef)0;
  //Find a channel (or it might throw an error).

  if(hasDaughterboard) {
  channel = xbar.getChannelForPins(pins, PINCONFIG_DATA_TYPE_DAC_CONST);
  } else {
    channel = (FPGA_IO_Pins_TypeDef)pins[0];
  }

  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_CONST] = (int32_t)amplitude;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_DAC_CONST;
  data[PINCONFIG_DATA_SAMPLE_RATE] = 0;

  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);
  sendPacket(&p);
  std::cout << "ITEM SCHEDULED ON MECOBO" << std::endl;
}


void Mecobo::scheduleConstantVoltageFromRegister(std::vector<int> pin, int start, int end, int reg)
{

  FPGA_IO_Pins_TypeDef channel = (FPGA_IO_Pins_TypeDef)0;
  //Find a channel (or it might throw an error).

  if(hasDaughterboard) {
  channel = xbar.getChannelForPins(pin, PINCONFIG_DATA_TYPE_DAC_CONST);
  } else {
    channel = (FPGA_IO_Pins_TypeDef)pin[0];
  }

  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_CONST] = (int32_t)reg;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_CONSTANT_FROM_REGISTER;
  data[PINCONFIG_DATA_SAMPLE_RATE] = 0;

  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);
  sendPacket(&p);
  std::cout << "CONSTANT_FROM_REGISTER SCHEDULED ON MECOBO" << std::endl;
}

void Mecobo::scheduleRecording(std::vector<int> pin, int start, int end, int frequency)
{

  FPGA_IO_Pins_TypeDef channel = (FPGA_IO_Pins_TypeDef)0;
  //Find a channel (or it might throw an error).

  int divisor = (int)(60000000/frequency); //(int)pow(2, 17-(frequency/1000.0));
  std::cout << "Divisor found:" << divisor << std::endl;
  if(hasDaughterboard) {
    channel = xbar.getChannelForPins(pin, PINCONFIG_DATA_TYPE_RECORD_ANALOGUE);
  } else {
    channel = (FPGA_IO_Pins_TypeDef)pin[0];
  }

  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_RECORD_ANALOGUE;
  data[PINCONFIG_DATA_SAMPLE_RATE] = divisor;

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

std::vector<int32_t> Mecobo::getSampleBuffer(int materialPin)
{
  std::vector<sampleValue> samples;

  //Send request for buffer size
  struct mecoPack pack;
  createMecoPack(&pack, 0, 0, USB_CMD_GET_INPUT_BUFFER_SIZE);
  sendPacket(&pack);

  //Don't know.
  pinRecordings.clear();
  //Retrieve bytes.
  uint32_t nSamples = 0;
  usb.getBytesDefaultEndpoint((uint8_t *)&nSamples, 4);

  //Got the input buffer size back, now we collect it.
  std::cout << "SampleBufferSize available: " << nSamples << std::endl;
  if(nSamples == 0) {
    return pinRecordings[materialPin];
  } else {
    if(nSamples >= 64000/sizeof(sampleValue)) {
      nSamples = 64000/sizeof(sampleValue);
      std::cout << "WARNING: Receiving less (" << nSamples << ") than what we could because of USB." << std::endl;
    } 
    int totalBytes = nSamples * sizeof(sampleValue);

    sampleValue* collectedSamples = new sampleValue[nSamples];

    std::cout << "Receiving bytes:" << totalBytes << std::endl;

    //Ask for samples back.
    //uint32_t ch = xbar.getChannel(materialPin);
    createMecoPack(&pack, (uint8_t*)&nSamples, 4, USB_CMD_GET_INPUT_BUFFER);
    sendPacket(&pack);

    usb.getBytesDefaultEndpoint((uint8_t*)collectedSamples, totalBytes);

    sampleValue * casted = (sampleValue *)collectedSamples;
    for(int i = 0; i < (int)nSamples; i++) {
      samples.push_back(casted[i]);
    }

    //std::sort(samples.begin(), samples.end(), 
     //   [](sampleValue const & a, sampleValue const & b) { return a.sampleNum < b.sampleNum; });
    
    delete[] collectedSamples;
  }

  //TODO: Sort the samples here, we could have a wrapped-around buffer.
  //Assumtion is that the sequence numbers from the FPGA are always
  //increasing, which can also be false, so this is "best effort"
  //to recreate the wave, but the general trend exists of course.
  
  //The samples we get back are associated with a certain channel,
  //we need to convert it to pins to see what the user actually
  //expected out.
  for(auto s : samples) {
    //Since one channel can be on many pins we'll collect them all.
    std::vector<int> pin = xbar.getPin((FPGA_IO_Pins_TypeDef)s.channel);
    for (auto p : pin) {
      //Cast from 13 bit to 32 bit two's complement int.
      int v = signextend<signed int, 13>(0x00001FFF & (int32_t)s.value);
      //std::cout << "Val: " << s.value << "signex: "<< v << std::endl;
      pinRecordings[p].push_back(v);
    }
  }

  return pinRecordings[materialPin];
}

void Mecobo::discharge()
{
  xbar.reset();
  std::vector<int> pins;
  for(int i = 0; i < 15; i++) {pins.push_back(i);}
  scheduleDigitalOutput(pins, 0, 500, 0, 0);
  runSchedule();
}

void Mecobo::reset()
{
  xbar.reset();
  pinRecordings.clear();
  struct mecoPack p;
  createMecoPack(&p, 0, 0, USB_CMD_RESET_ALL);
  sendPacket(&p);
  //Wait a while between polling the status to be nice.
  while(this->status().state == MECOBO_STATUS_BUSY) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

bool
Mecobo::isFpgaConfigured ()
{
  return true;
}

mecoboStatus
Mecobo::status()
{
  struct mecoPack p;
  createMecoPack(&p, 0, 0, USB_CMD_STATUS);
  sendPacket(&p);
  mecoboStatus status;
  usb.getBytesDefaultEndpoint((uint8_t*)&status, sizeof(mecoboStatus));

  return status;
}

void
Mecobo::scheduleDigitalRecording (std::vector<int> pin, int start, int end, int frequency)
{
  FPGA_IO_Pins_TypeDef channel = (FPGA_IO_Pins_TypeDef)0;
  //Find a channel (or it might throw an error).

  if(hasDaughterboard) {
    channel = xbar.getChannelForPins(pin, PINCONFIG_DATA_TYPE_DIGITAL_OUT);
  } else {
    channel = (FPGA_IO_Pins_TypeDef)pin[0];
  }


  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_RECORD;
  data[PINCONFIG_DATA_SAMPLE_RATE] = (int)frequency;

  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);
  sendPacket(&p);
}


void
Mecobo::scheduleDigitalOutput (std::vector<int> pin, int start, int end, int frequency,
			       int dutyCycle)
{
  FPGA_IO_Pins_TypeDef channel = (FPGA_IO_Pins_TypeDef)0;
  //Find a channel (or it might throw an error).

  //Find a channel (or it might throw an error).
  if(hasDaughterboard) {
    channel = xbar.getChannelForPins(pin, PINCONFIG_DATA_TYPE_DIGITAL_OUT);
  } else {
    channel = (FPGA_IO_Pins_TypeDef)pin[0];
  }

  int period = 0;
  if(frequency != 0) {
    period = (int)(75000000/frequency); //(int)pow(2, 17-(frequency/1000.0));
  }
  
  int duty = period * ((double)dutyCycle/100.0);

  std::cout << "p:" << period << "d:" << duty << "ad:" << period - duty << std::endl;
  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_DUTY] = duty;
  data[PINCONFIG_DATA_ANTIDUTY] = period - duty;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_DIGITAL_OUT;

  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);
  sendPacket(&p);

}

void
Mecobo::schedulePWMoutput (std::vector<int> pin, int start, int end, int pwmValue)
{
  throw std::runtime_error("PWM not implemented yet :/");
  //submitItem(item.pin, item.startTime, item.endTime,  (uint32_t)duty, (uint32_t)aduty, 0x1, 0x0, PINCONFIG_DATA_TYPE_PREDEFINED_PWM, item.amplitude);
}

void
Mecobo::scheduleSine (std::vector<int> pin, int start, int end, int frequency, int amplitude,
		      int phase)
{

  //TODO: Support phase.

  FPGA_IO_Pins_TypeDef channel = (FPGA_IO_Pins_TypeDef)0;
  //Find a channel (or it might throw an error).

  //Find a channel (or it might throw an error).
  if(hasDaughterboard) {
  channel = xbar.getChannelForPins(pin, PINCONFIG_DATA_TYPE_DAC_CONST);
  } else {
    channel = (FPGA_IO_Pins_TypeDef)pin[0];
  }

  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_CONST] = (uint32_t)amplitude;
  data[PINCONFIG_DATA_SAMPLE_RATE] = frequency;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_PREDEFINED_SINE;

  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);
  sendPacket(&p);
}

void
Mecobo::runSchedule ()
{
  //Set up the crossbar for this sequence.
  std::cout << "Setting up XBAR" << std::endl;
  std::vector<uint8_t> test = xbar.getXbarConfigBytes();
  this->setXbar(test);
  while(status().state == MECOBO_STATUS_BUSY) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }

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

void Mecobo::updateRegister(int index, int value)
{
  struct mecoPack p;
  int32_t data[2];
  data[0] = index;
  data[1] = value;
  createMecoPack(&p, (uint8_t*)data, 8, USB_CMD_UPDATE_REGISTER);
  sendPacket(&p);
}
