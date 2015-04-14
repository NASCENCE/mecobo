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
#include <algorithm>

//For windows :(
#undef min
#undef max

Mecobo::Mecobo (bool daughterboard)
{
  hasDaughterboard = daughterboard;
  if (hasDaughterboard) {
    std::cout << "Mecobo initialized" << std::endl;
  } else {
    std::cout << "Mecobo initialized without daughterboard" << std::endl;
  }

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

  //int divisor = (int)(60000000/frequency); //(int)pow(2, 17-(frequency/1000.0));
  //std::cout << "Divisor found:" << divisor << std::endl;
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
  data[PINCONFIG_DATA_SAMPLE_RATE] = frequency;

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
  printf("ProgramFPGA\n");
  FILE* bitfile;

  int onlyProgram = 0;
  if(filename == NULL) {
    printf("No filename, assuming bitfile in NOR FLASH, Instructing uC to program\n");
    onlyProgram = 1;
  }

  if(!onlyProgram) {

#ifdef WIN32
    int openResult = fopen_s(&bitfile, filename, "rb");
    perror ("programFPGA");
#else
    bitfile = fopen(filename, "rb");
#endif

    printf("Programming FPGA with bitfile %s\n", filename);
    fseek(bitfile, 0L, SEEK_END);
    long nBytes = ftell(bitfile);
    rewind(bitfile);

    int packsize = 32*1024;
    int nPackets = nBytes / packsize;
    int rest = nBytes % (packsize);
    printf("supposed to have ballpark 6,440,432 bits. have %ld\n", 8*nBytes * 8);
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
      createMecoPack(&send, bytes + (i*packsize), packsize, USB_CMD_LOAD_BITFILE);
      sendPacket(&send);
    }
    //Send the rest if there is any.
    if(rest > 0) {
      printf("Sending the rest pack, position %u, size %d\n", (i*packsize), rest);
      struct mecoPack lol;
      createMecoPack(&lol, bytes + (i*packsize), rest, USB_CMD_LOAD_BITFILE);
      sendPacket(&lol);
    }

    printf("Data send finished\n");
    free(bytes);
    fclose(bitfile);
    
      }
  struct mecoPack lol;
  createMecoPack(&lol, NULL, 0, USB_CMD_PROGRAM_FPGA);
  sendPacket(&lol);
}

std::vector<int32_t> Mecobo::getSampleBuffer(int materialPin)
{

  //This holds all the samples.
  std::vector<sampleValue> samples;

  //Send request for buffer size
  struct mecoPack pack;
  createMecoPack(&pack, 0, 0, USB_CMD_GET_INPUT_BUFFER_SIZE);
  sendPacket(&pack);

  //Retrieve bytes.
  uint32_t nSamples = 0;
  int usbTransfers = 1;
  int remainderSamples = 0;
  usb.getBytesDefaultEndpoint((uint8_t *)&nSamples, 4);
  
  int maxSamplesPerTx = (64000/sizeof(sampleValue));

  //Number of samples ready to be fetched.
  std::cout << "SampleBufferSize available: " << nSamples << std::endl;
  if(nSamples == 0) {
    std::cout << "WARNING: No samples collected. Something might be wrong, but there might just be no new samples as well." << std::endl;
    return pinRecordings[materialPin];
  }

  //max tx size is 64k
  usbTransfers = nSamples/maxSamplesPerTx;
  remainderSamples = nSamples%maxSamplesPerTx;
  std::cout << "Splitting in "<< usbTransfers << " transfers and " << remainderSamples << " remainder" << std::endl;
  //additional transfer for the remainder
  if (remainderSamples > 0) {
    usbTransfers++;
  }

  //Do the required number of transfers
  for(int p = 1; p <= usbTransfers; p++) {

    int thisTxBytes = nSamples * sizeof(sampleValue);
    int thisTxSamples = nSamples;
    if(remainderSamples > 0) {
      //last tx
      if(p == usbTransfers) {
        thisTxBytes = remainderSamples * sizeof(sampleValue);
        thisTxSamples = remainderSamples;
      } else {
        thisTxBytes = maxSamplesPerTx * sizeof(sampleValue);
        thisTxSamples = maxSamplesPerTx;
      }
    } else {
      thisTxBytes = nSamples * sizeof(sampleValue);
      thisTxSamples = nSamples;
    }
    
    
    
    sampleValue* collectedSamples = new sampleValue[thisTxSamples];

    std::cout << "Receiving bytes:" << thisTxBytes << std::endl;

    //Ask for samples back.
    createMecoPack(&pack, (uint8_t*)&thisTxSamples, 4, USB_CMD_GET_INPUT_BUFFER);
    sendPacket(&pack);

    usb.getBytesDefaultEndpoint((uint8_t*)collectedSamples, thisTxBytes);

    sampleValue * casted = (sampleValue *)collectedSamples;
    for(int i = 0; i < (int)thisTxSamples; i++) {
      samples.push_back(casted[i]);
    }

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
    if(hasDaughterboard) {
      std::vector<int> pin = xbar.getPin((FPGA_IO_Pins_TypeDef)s.channel);
      //std::cout << "Channel "<< (unsigned int)s.channel << " gotten" << std::endl;
      for (auto p : pin) {
        //std::cout << "Pin "<< (unsigned int)p << " mapped" << std::endl;
        //Cast from 13 bit to 32 bit two's complement int.
        int v = signextend<signed int, 13>(0x00001FFF & (int32_t)s.value);
        //std::cout << "Val: " << s.value << "signex: "<< v << std::endl;
        pinRecordings[(int)p].push_back(v);
      }
    } else {
      pinRecordings[s.channel].push_back(s.value);
    }
  }
  return pinRecordings[materialPin];
}

void Mecobo::discharge()
{
  if(hasDaughterboard) {
    xbar.reset();
    std::vector<int> pins;
    for(int i = 0; i < 15; i++) {pins.push_back(i);}
    scheduleDigitalOutput(pins, 0, 500, 0, 0);
    runSchedule();
  }
}

void Mecobo::reset()
{
  if (hasDaughterboard) {
    std::cout << "Reseting XBAR..."; 
    xbar.reset();
    std::cout << "DONE." << std::endl;
  }

  pinRecordings.clear();
  struct mecoPack p;
  createMecoPack(&p, 0, 0, USB_CMD_RESET_ALL);
  sendPacket(&p);
  //Wait a while between polling the status to be nice.
  std::cout << "Waiting for uC to complete reset" << std::endl;
  while(this->status().state == MECOBO_STATUS_BUSY) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::cout << "Reset complete!" << std::endl;
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
    channel = (FPGA_IO_Pins_TypeDef)pin[0];  //Note that pin is a list, but always pick the first. 
  }


  int divisor = (int)(75000000/frequency); //(int)pow(2, 17-(frequency/1000.0));

  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_RECORD;
  data[PINCONFIG_DATA_SAMPLE_RATE] = (int)divisor;

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

  /*
  int period = 0;
  if(frequency != 0) {
    period = std::min(65500, (int)(75000000/frequency)); //(int)pow(2, 17-(frequency/1000.0));
  }
  
  int duty = std::min(65500, int(period * ((double)dutyCycle/100.0)));
  */
  //2^32/75*10^6 = 57.26623061333333333333
  double magic = 57.26623061333333333333;

  int countervalue = (int)(magic * frequency);

  std::cout << " ----  Counter value: " << countervalue << std::endl;
  //std::cout << "p:" << period << "d:" << duty << "ad:" << period - duty << std::endl;
  uint32_t data[USB_PACK_SIZE_BYTES/4];
  data[PINCONFIG_START_TIME] = start;
  data[PINCONFIG_END_TIME] = end;
  data[PINCONFIG_DATA_FPGA_PIN] = channel;
  data[PINCONFIG_DATA_NOC_COUNTER] = countervalue;
  //data[PINCONFIG_DATA_DUTY] = duty;
  //data[PINCONFIG_DATA_ANTIDUTY] = period - duty;
  data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_DIGITAL_OUT;
  if(dutyCycle==100) {
    data[PINCONFIG_DATA_CONST] = frequency;
    data[PINCONFIG_DATA_TYPE] = PINCONFIG_DATA_TYPE_DIGITAL_CONST;
  }

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
  if(hasDaughterboard){
    std::cout << "Setting up XBAR" << std::endl;
    std::vector<uint8_t> test = xbar.getXbarConfigBytes();
    this->setXbar(test);
    while(status().state == MECOBO_STATUS_BUSY) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }

  struct mecoPack p;
  createMecoPack(&p, NULL, 0, USB_CMD_RUN_SEQ);
  sendPacket(&p);
}

void
Mecobo::setXbar(std::vector<uint8_t> & bytes)
{
  if(hasDaughterboard) {
    struct mecoPack p;
    createMecoPack(&p, bytes.data(), 64, USB_CMD_PROGRAM_XBAR);
    sendPacket(&p);
  } else {
    std::cout << "Tried to set XBAR without daughterboard" << std::endl;
  }
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

int
Mecobo::getPort()
{
  return 9090 + this->usb.getUsbAddress();
}
