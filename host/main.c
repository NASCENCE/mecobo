#include <sys/time.h>
#include <stdio.h>
#include <libusb.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <vector>
#include <bitset>
#include <tuple>
#include <iostream>
#include <random>
#include <algorithm>
#include <cmath>
#include "mecohost.h"
#include "../uc/mecoprot.h"

#define NUM_ENDPOINTS 4


const int genomeSize = 11 * 16 * 2;
const int resultSize = 2048;


char eps[NUM_ENDPOINTS];
int getPin(FPGA_IO_Pins_TypeDef pin, uint32_t * val);

static inline uint32_t get_bit(uint32_t val, uint32_t bit);
int experiment_foo();
int experiment_ca();
int setReg(uint32_t data);
int programFPGA(const char * filename);
int startOutput (FPGA_IO_Pins_TypeDef pin);

struct libusb_device_handle * mecoboHandle;
struct libusb_device * mecobo;
void getEndpoints(char * endpoints, struct libusb_device * dev, int interfaceNumber)
{
  //We know which interface we want the endpoints for (0x1), so
  //we'll just run through the descriptors and dig them out.

  //get Configuration 0 form device
  struct libusb_config_descriptor * config;
  libusb_get_active_config_descriptor(dev, &config);

  //Get the interface 0x1 descp
  struct libusb_interface_descriptor interface = config->interface[1].altsetting[0];
  for(int ep = 0; ep < interface.bNumEndpoints; ++ep) {
    if(interface.endpoint[ep].bEndpointAddress & 0x80) {
      printf("Found input endpoint with address %x\n", interface.endpoint[ep].bEndpointAddress);
    } else {
      printf("Found output with address %x\n", interface.endpoint[ep].bEndpointAddress);
    }
    endpoints[ep] = (char)interface.endpoint[ep].bEndpointAddress;
  }
}

bool sortIndividual(const std::tuple<std::bitset<genomeSize>, double> &a,
                    const std::tuple<std::bitset<genomeSize>, double> &b) {
  return std::get<1>(a) > std::get<1>(b);
}

std::vector<std::tuple<std::bitset<genomeSize>, double>> selection(
    std::vector< std::tuple< std::bitset<genomeSize>, double >>
    & fitpop)
{
  //elitism, return half of the best ones. 
  std::sort(fitpop.begin(), fitpop.end(), sortIndividual);

  std::vector<std::tuple<std::bitset<genomeSize>, double>> ret;
  for(int i = 0; i < fitpop.size()/2; i++) {
    ret.push_back(fitpop[i]);
  }
  return ret;
}

std::vector<std::bitset<genomeSize>> mutate(
    std::vector< std::tuple< std::bitset<genomeSize>, double >>
    & population,
    int popSize)
{
  std::vector<std::bitset<genomeSize>> ret;

  //Linear scaling of parent selection probability
  //based on fitness.
  //TODO: fails if max - min = 0
  double fitRange = std::get<1>(population.front())-std::get<1>(population.back());

  std::default_random_engine generator;
  
  std::vector<double> intervals = {0.0f, (double)population.size()};
  std::vector<double> weights = {std::get<1>(population.front()), std::get<1>(population.back())};

  std::piecewise_linear_distribution<double> distro(intervals.begin(), intervals.end(), weights.begin());


  std::uniform_int_distribution<int> uniformDistro20p(0,4);
  for(int i = 0; i < popSize; i++) {
    //Do two dice rolls
    auto parentA = std::get<0>(population[(int)distro(generator)]);
    auto parentB = std::get<0>(population[(int)distro(generator)]);
    //We have indices of parents, now 
    //do a 1 point cross-over between them.
    std::bitset<genomeSize> child;
    int b;
    for(b = 0; b < genomeSize/2; b++) {
      child[b]                = parentA[b];
      child[b+(genomeSize/2)] = parentB[b];
    }

    //Do some mutation as well (20%)
    for(b = 0; b < genomeSize; b++) {
      if(uniformDistro20p(generator) == 0) {
        child[b] = !child[b];
      }
    }

    ret.push_back(child);
  }


  return ret;
}

double measuredFitness(std::bitset<resultSize> individual, double lambda) {
  int pos = 0;
  double quint = 0;
  for(int i = 0; i < resultSize-1; i+=2) {
    if(!individual[i] && !individual[i+1]) {
      quint++;
    }
  }
  //std::cout << quint << std::endl;
  //4^5 = 1024 
  double measuredLambda = (1024.0f-quint)/1024.0f;
  double fitness =  (1.0f - std::abs(lambda - measuredLambda));
  return fitness;
}

std::vector<std::bitset<resultSize>> runPopulation(std::vector<std::bitset<genomeSize>> population)
{
  std::vector<std::bitset<resultSize>> results;

  std::vector<FPGA_IO_Pins_TypeDef> outPins = {
      FPGA_F16,
      FPGA_F17,
      FPGA_G14,
      FPGA_G16,
      FPGA_H16,
      FPGA_H17,
      FPGA_H15,
      FPGA_L12,
      FPGA_H14,
      FPGA_K14,
      FPGA_K12
    };
    
    std::vector<FPGA_IO_Pins_TypeDef> inPins = {
      FPGA_J16
    };


    //Setup input pins
    for(FPGA_IO_Pins_TypeDef inPin : inPins) {
      setPin(inPin, 0x1, 0x1, 0x1, 0xFF);  //sample rate ... something reasonable?
    }
 
    for(std::bitset<genomeSize> individual : population) {
      int pinNum = 0;
      std::bitset<16> duty;
      std::bitset<16> antiduty;
      for(FPGA_IO_Pins_TypeDef pin : outPins) {
        //Build two bit slices for pin config
        int slicePos = pinNum * 32;
        for(int i = 0; i < 16; i++) {
          duty[i] = individual[slicePos + i];
        }
        for(int i = 0; i < 16; i++ ) {
          antiduty[i] = individual[slicePos + 16 + i];
        }

        setPin(pin, duty.to_ulong(), antiduty.to_ulong(), 0x1, 0x0);
        //std::cout << "Setting " << pin << ": " << \
        //  duty.to_ulong() << ": " << \
        //  antiduty.to_ulong() << std::endl;

        startOutput(pin);
        pinNum++;
      }
      for(FPGA_IO_Pins_TypeDef inPin : inPins) {
        startInput(inPin); //flips buffers. The next buffer should now be clean.
      }

      //Collect at least 1024 samples.
      //Only one pin, so it's quite easy.
      struct mecoboDev dev;
      std::vector<sampleValue> allSamples;
      while(allSamples.size() < resultSize) {
        std::vector<sampleValue> s;
        getMecoboStatus(&dev);
        if(dev.bufElements >= 128) {
          getSampleBuffer(s);
          //append sample
          allSamples.insert(allSamples.end(),
                            s.begin(),
                            s.end());
        }
      }
      //Construct a bitset  from the collected samples
      std::bitset<resultSize> b;
      int pos = 0;
      for(sampleValue sv : allSamples) {
        //std::cout << sv.value << std::endl;
        if(pos < 2048)
          b[pos++] = sv.value ? true:false;
      }
      //std::cout << "Result" << b << std::endl;
      results.push_back(b);
    }
  return results;
}




int main(int argc, char ** argv) {

  uint32_t progFpga = 0;
  //Command line arguments
  if (argc > 1) {
    for(int i = 0; i < argc; i++) {
      if(strcmp(argv[i], "-f") == 0) {
          progFpga = 1;
      }
    }
 }


  libusb_context * ctx = NULL;

  int r;
  ssize_t cnt;

  r = libusb_init(&ctx);
  if(r < 0) {
    printf("Init Error\n"); //there was an error
    return 1;
  }
  libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

  mecoboHandle = libusb_open_device_with_vid_pid(ctx, 0x2544, 0x3);
  mecobo = libusb_get_device(mecoboHandle);	

  libusb_detach_kernel_driver(mecoboHandle, 0x1);	
  if(libusb_claim_interface(mecoboHandle, 0x1) != 0) {
    printf("Could not claim interface 0 of Mecobo\n");
  }

  getEndpoints(eps, mecobo, 0x1);

  double start = omp_get_wtime();
 
  //setReg(42);

  if(progFpga) 
    programFPGA("mecobo.bin");

  //experiment_foo();
  experiment_ca();

  double end = omp_get_wtime();
  /*	
      for(int q = 0; q < transfered; q++) {
      printf("%x,", rcv[q]);
      }
      printf("\n");
      */
  //printf("Rate: %f KB/s\n", ((bytes)/(double)(end-start))/(double)1024);

  libusb_release_interface(mecoboHandle, 0x1);
  libusb_attach_kernel_driver(mecoboHandle, 0x1);	

  libusb_close(mecoboHandle);

  libusb_exit(ctx); //close the session

  return 0;
}


int createMecoPack(struct mecoPack * packet, uint8_t * data,  uint32_t dataSize, uint32_t command)
{
  if(dataSize > 0) {
    packet->data = (uint8_t*)malloc(dataSize);
    memcpy(packet->data, data, dataSize);
  } else {
    packet->data = NULL;
  }

  packet->size = dataSize;
  packet->command = command;
  return 0;
}

int setReg(uint32_t data) 
{
    struct mecoPack p;
    createMecoPack(&p, (uint8_t *)(&data), 4, USB_CMD_CONFIG_REG);
    sendPacket(&p, eps[2]);
}

int setPin( FPGA_IO_Pins_TypeDef pin, 
            uint32_t duty,
            uint32_t antiduty,
            uint32_t cycles,
            uint32_t sampleRate)
{

  uint32_t data[USB_PACK_SIZE_BYTES];

  data[PINCONFIG_DATA_FPGA_PIN] = pin;
  data[PINCONFIG_DATA_DUTY] = duty;
  data[PINCONFIG_DATA_ANTIDUTY] = antiduty;
  data[PINCONFIG_DATA_CYCLES] = cycles;
  data[PINCONFIG_DATA_SAMPLE_RATE] = sampleRate;

  data[PINCONFIG_DATA_RUN_INF] = 0x1; //debug!

  //printf("sending pinconfig on pin:%x\n", pin);
  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, USB_PACK_SIZE_BYTES, USB_CMD_CONFIG_PIN);

  sendPacket(&p, eps[2]);
}

int startOutput (FPGA_IO_Pins_TypeDef pin)
{
  uint32_t data;
  data = pin;
  struct mecoPack p;
  createMecoPack(&p, (uint8_t*)&data, 4, USB_CMD_START_OUTPUT);
  sendPacket(&p, eps[2]);
}

int startInput (FPGA_IO_Pins_TypeDef pin)
{
  uint32_t data;
  data = pin;
  struct mecoPack p;
  createMecoPack(&p, (uint8_t*)&data, 4, USB_CMD_STREAM_INPUT);
  sendPacket(&p, eps[2]);
}


int getPin(FPGA_IO_Pins_TypeDef pin, uint32_t * val) 
{
  //Construct packet to get a pin
  uint32_t data[1];
  data[PINCONFIG_DATA_FPGA_PIN] = pin;
  struct mecoPack p;
  createMecoPack(&p, (uint8_t *)data, 4, USB_CMD_READ_PIN);
  sendPacket(&p, eps[2]);

  //Get data back.
  int bytesRemaining = 4*3;
  int transfered = 0;
  uint8_t * rcv = (uint8_t*)malloc(12);
  while(bytesRemaining > 0) {
    libusb_bulk_transfer(mecoboHandle, eps[0], rcv, 12, &transfered, 0);
    bytesRemaining -= transfered;
  }
  memcpy(val, rcv, 12);
  free(rcv);
}

int sendPacket(struct mecoPack * packet, uint8_t endpoint) 
{
  //First, send header (fixed 8 bytes)
  //Create a buffer of data to send.
  uint32_t toSend[2];
  toSend[0] = packet->size;
  toSend[1] = packet->command;

  //printf("Sending header\n");
  int transfered = 0;
  int remaining = 8;
  while(remaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, (uint8_t*)toSend, 8, &transfered, 0);
    remaining -= transfered;
    //printf("Sent bytes of header, %u\n", transfered);
  } 
  //Send data afterwards.
  //printf("Sending data, size %u\n", packet->size);
  transfered = 0;
  remaining = packet->size;
  while(remaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, packet->data, packet->size, &transfered, 0);
    remaining -= transfered;
    //printf("Sent bytes of data, %u\n", transfered);
  }

  return 0;
}

int getPacket(struct mecoPack * packet)
{
  return 0;
}

bool sortValues(sampleValue i, sampleValue j) {
  return i.sampleNum < j.sampleNum;
}




typedef std::vector<std::bitset<genomeSize>> kuk;
kuk ca_run(
    std::vector<std::bitset<genomeSize>> population, 
    double wantedLambda, int popSize)
{
  std::vector<std::bitset<resultSize>> res;
  //Run! Run! Run!
  std::vector<double> generationFitness;
  bool terminate = false;
  int gen = 0;
  double lastAvg = 0.0f;
  std::tuple<std::bitset<genomeSize>, double> allTimeBest;
  while(!terminate) {
    res = runPopulation(population);

    std::vector<double> fitness;
    for(std::bitset<resultSize> result : res) {
      fitness.push_back(measuredFitness(result, wantedLambda));
    }
    std::vector<std::tuple<std::bitset<genomeSize>, double>> fitPop;
    int p = 0;
    for(double f : fitness) {
      fitPop.push_back(std::make_tuple(population[p++], f));
    }

    //Select
    auto bestPop = selection(fitPop);
    
    //Find average fitness increase
    double avgFit = 0.0f;
    for(auto b : bestPop) {
      //Pick out the all time best while we're at it.
      if(std::get<1>(b) > std::get<1>(allTimeBest)) {
        allTimeBest = b;
      }
      avgFit += std::get<1>(b);
    }
    generationFitness.push_back(avgFit/bestPop.size());
    std::cout <<    gen++                     << " " << \
      "Avg: " <<    generationFitness.back() << " " << \
      "Low: " <<    std::get<1>(bestPop.back()) << " " <<\
      "High: "<<    std::get<1>(bestPop.front()) << " " <<\
      "ATB: " <<    std::get<1>(allTimeBest) <<  \
      std::endl;

    //Check if average fitness increase over the last 20 generations is very small,
    //if so, we are done. But only if we have enough generations.
    int maxGen = 10;
    if(generationFitness.size() > maxGen) {
      double avg = 0.0f;
      for(int i = generationFitness.size()-maxGen; i < generationFitness.size(); i++) {
        avg += generationFitness[i];
      }
      avg = avg/(double)maxGen;
      double diff = lastAvg - avg;
      if((diff >= 0.0f) && (diff <= 0.0001f)) {
        terminate = true;
      }
      lastAvg = avg;
    }

    //Mutate
    auto newPop = mutate(bestPop, popSize);
    //New population!
    population = newPop;
  }

  //Finish up.
  std::cout << "All time best individual: " << std::get<0>(allTimeBest) << std::endl;
  std::cout << "Fitness: " << std::get<1>(allTimeBest) << std::endl;
  return population;
}

int experiment_ca()
{
  int popSize = 16;
  //11 pins,32 bit per pin, implicitly mapped 1-1 to outPins defined below.
  std::vector<std::bitset<genomeSize>> population;
    std::default_random_engine randEng;
    std::uniform_int_distribution<> dis(0, 1);

    for(int p = 0; p < popSize; p++) {
      std::bitset<genomeSize> gene;
      for(int i = 0; i < genomeSize; i++) {
        gene[i] = dis(randEng);
      }
      population.push_back(gene);
    }
    auto seeded = ca_run(population, 0.999, 16);
    
    ca_run(seeded, 0.01, 16);
    return 0;
}

int experiment_foo()
{
    //Repeat of experiment 
    std::vector<FPGA_IO_Pins_TypeDef> outPins = {
      FPGA_F16,
      FPGA_F17,
      FPGA_G14,
      FPGA_G16,
      FPGA_H16,
      FPGA_H17,
      FPGA_H15,
      FPGA_L12,
      FPGA_H14,
      FPGA_K14,
      FPGA_K12
    };
    
    std::vector<FPGA_IO_Pins_TypeDef> inPins = {
      FPGA_J16
    };

    //Setup input pins
    for(FPGA_IO_Pins_TypeDef inPin : inPins) {
      setPin(inPin, 0x1, 0x1, 0x1, 0xFFF);
    }

    for(uint16_t gene = 0; gene < 2048; gene++) {
      int bit = 0;
      std::string geneString;
      for(FPGA_IO_Pins_TypeDef pin : outPins) {
        if(get_bit(gene, bit++)) {
          setPin(pin, 0xFFFF, 0x0, 0x1, 0x0);
          geneString += "1 ";
        } else {
          setPin(pin, 0x0, 0xFFFF, 0x1, 0x0);
          geneString += "0 ";
        }
        startOutput(pin);
      }

      //start input
      for(FPGA_IO_Pins_TypeDef inPin : inPins) {
        startInput(inPin); //flips buffers. The next buffer should now be clean.
      }

      //Collect at least 5 samples 
      std::vector<sampleValue> samples;
      struct mecoboDev dev;
      bool done = false;
      while(!done) {
        getMecoboStatus(&dev);
        if(dev.bufElements >= 5) {
          done = true;
          getSampleBuffer(samples);
        }
      }
      for (int s = 0; s < 5; s++) {
        std::cout << geneString << " r: " << samples[s].value << " " << samples[s].sampleNum << " " << samples[s].pin << " " << std::endl;
      }
      samples.clear();
    }
}


static inline uint32_t get_bit(uint32_t val, uint32_t bit) 
{
    return (val >> bit) & 0x1;
}

int programFPGA(const char * filename)
{
  FILE * bitfile;


  bitfile = fopen(filename, "r");
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
    sendPacket(&send, eps[2]);
  }
  //Send the rest if there is any.
  if(rest > 0) {
    printf("Sending the rest pack, position %u, size %d\n", (i*packsize), rest);
    struct mecoPack lol;
    createMecoPack(&lol, bytes + (i*packsize), rest, USB_CMD_PROGRAM_FPGA);
    sendPacket(&lol, eps[2]);
  }

  free(bytes);
  printf("\n");
  fclose(bitfile);
}


int getSampleBuffer(std::vector<sampleValue> & samples)
{
  //Send request for buffer size
  struct mecoPack pack;
  createMecoPack(&pack, 0, 0, USB_CMD_GET_INPUT_BUFFER_SIZE);
  sendPacket(&pack, eps[2]);
  uint32_t size;
  getBytesFromUSB(eps[0], (uint8_t *)&size, 4);
  //Now get data back.
  //uint8_t * data = malloc(sizeof(struct sampleValue)*USB_BUFFER_SIZE);
  
  sampleValue collectedSamples[size];
  createMecoPack(&pack, 0, 0, USB_CMD_GET_INPUT_BUFFER);
  sendPacket(&pack, eps[2]);
  getBytesFromUSB(eps[0], (uint8_t*)collectedSamples, sizeof(sampleValue) * size);
  //std::cout << "Got " << size  << "samples" << std::endl;
  for(int i = 0; i < size; i++) {
    //std::cout << collectedSamples[i].value << std::endl;
    samples.push_back(collectedSamples[i]);
  }
}

int getBytesFromUSB(int endpoint, uint8_t * bytes, int nBytes)
{
  //Get data back.
  //printf("Waiting for %d bytes from meco\n", nBytes);
  int bytesRemaining = nBytes;
  int transfered = 0;
  while(bytesRemaining > 0) {
    libusb_bulk_transfer(mecoboHandle, endpoint, bytes, nBytes, &transfered, 0);
    bytesRemaining -= transfered;
  }
  //printf("    ... got them!\n\n");

}


int getMecoboStatus(struct mecoboDev * dev)
{
  struct mecoPack p;
  uint8_t dat[STATUS_BYTES];
  createMecoPack(&p, 0, 0, USB_CMD_STATUS);
  sendPacket(&p, eps[2]);
  getBytesFromUSB(eps[0], dat, STATUS_BYTES);
  uint32_t * d = (uint32_t *)dat;
  dev->fpgaConfigured = d[0];
  dev->bufElements = d[1];

  return 0;
}
