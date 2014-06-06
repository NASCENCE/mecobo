#include "emEvolvableMotherboard.h"

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <vector>
#include <iostream>

#include <random>
#include <algorithm>
#include <map>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;


using namespace emInterfaces;

void connectivityTest(emEvolvableMotherboardClient & cli) 
{
  //Set all pins high
  for(int i = 0; i< 14; i++) {
    emSequenceItem out;
    out.pin = i;
    out.startTime = 0;
    out.endTime = 1000;
    out.amplitude = 1.0;
    out.operationType = emSequenceOperationType::type::CONSTANT;
    cli.appendSequenceAction(out);

    //Do a recording.
    emSequenceItem rec;
    rec.pin = 0;
    rec.startTime = 0;
    rec.endTime = 1000;
    rec.frequency = 5000;
    rec.operationType = emSequenceOperationType::type::RECORD;
    cli.appendSequenceAction(rec);

    cli.runSequences();

    emWaveForm r;
    cli.getRecording(r, 0);
 
    std::map<int, int> vals;
    for (auto i : r.Samples) {
      vals[i]++;
   }
    if (vals[1] > 0) {
      std::cout << "Connectivity good for pin" << i << std::endl;
    } else {
      std::cout << "no connection" << std::endl;
    }

  }
}

void pinTest(emEvolvableMotherboardClient & cli) 
{
  int offset = 6;
  for(int i = 0; i < offset; i++) {
    emSequenceItem out;
    out.pin = i;
    out.amplitude = 1;
    out.startTime = 0;
    out.endTime = 256;
    out.operationType = emSequenceOperationType::type::CONSTANT;
    cli.appendSequenceAction(out);

    //Do a recording.
    emSequenceItem rec;
    rec.pin = i + offset;
    rec.startTime = 0;
    rec.endTime = 256;
    rec.frequency = 5000;
    rec.operationType = emSequenceOperationType::type::RECORD;
    cli.appendSequenceAction(rec);

    cli.runSequences();

    emWaveForm r;
    cli.getRecording(r, i + offset);
 
    std::map<int, int> vals;
    for (auto l : r.Samples) {
      vals[l]++;
    }
    
    std::cout << std::endl;
    if (vals[1] > 0) {
      std::cout << "Connectivity OK for pins" << i << "and " << i + offset << std::endl;
    } else {
      std::cout << "Connectivity BAD for pins" << i << "and " << i + offset << std::endl;
    }
  }
}

void testLoopbackCONSTANT(emEvolvableMotherboardClient & cli, int pin1, int pin2) 
{
  //std::default_random_engine generator;
  std::random_device generator;
  std::uniform_int_distribution<int> uniformDistro(0,1);

  //Generate a random sequence of 20 const sequences, each 10ms long.
  //Sample this for 1 second

  emSequenceItem rec;
  rec.pin = pin2;
  rec.startTime = 0;
  rec.endTime = 1000;
  rec.frequency = 8000;
  rec.operationType = emSequenceOperationType::type::RECORD;
  cli.appendSequenceAction(rec);

  std::vector<int> amps;
  for(int i = 0; i < 10; i++) 
    amps.push_back((int)uniformDistro(generator));

  for(int i = 0; i < 10; i++) {
    emSequenceItem out;
    out.pin = pin1;
    out.startTime = i*100;
    out.endTime = (i*100)+100;
    out.amplitude = amps[i];
    out.operationType = emSequenceOperationType::type::CONSTANT;
    cli.appendSequenceAction(out);
  }

  cli.runSequences();

  emWaveForm r;
  cli.getRecording(r, pin2);

  std::vector<int> measuredResults;
  int subSize = r.Samples.size() / 10;
  for(int i = 0;  i < 10; i++) {
    std::vector<int> subVector;
    std::map<int, int> mode;
    //This gets the frequency of each measured amplitude
    for(int s = i * subSize; s < (i*subSize) + subSize; s++) {
      subVector.push_back(r.Samples[s]);
      mode[r.Samples[s]]++;
    }
    measuredResults.push_back(
        std::max_element(mode.begin(), mode.end(), 
        [](const std::pair<int, unsigned>& p1, const std::pair<int, unsigned>& p2) {
                return p1.second < p2.second; })->first
        );
  }
  if(std::equal(measuredResults.begin(), measuredResults.end(), amps.begin())) {
    std::cout << "TEST SUCCEEDED" << std::endl;
  } else {
    std::cout << "TEST FAILED" << std::endl;
  }
  cli.clearSequences();

}

int main(void)
{
  boost::shared_ptr<TSocket> socket(new TSocket("localhost", 9090));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  
  emEvolvableMotherboardClient client(protocol);
  transport->open();
  client.ping();
/*
  for(int i = 0; i < 10; i++) {
    testLoopbackCONSTANT(client, 12,13);
  }
  */
  for(int i = 0; i < 100; i++)
    pinTest(client);
/*  
  emSequenceItem rec;
  rec.pin = 1;
  rec.startTime = 0;
  rec.endTime = 1000;
  rec.frequency = 2000;
  rec.operationType = emSequenceOperationType::type::RECORD;

  emSequenceItem out;
  out.pin = 0;
  out.startTime = 0;
  out.endTime = 1000;
  out.frequency = 500;
  out.cycleTime = 50;
  out.operationType = emSequenceOperationType::type::PREDEFINED;
  out.waveFormType = emWaveFormType::PWM;

  client.appendSequenceAction(out);
  client.appendSequenceAction(rec);
  client.runSequences();
  
  emWaveForm r;
  client.getRecording(r, 1);

  for(auto s : r.Samples) {
    std::cout << s;
  }
  std::cout << std::endl;
*/
  transport->close();

  return 0;
}
