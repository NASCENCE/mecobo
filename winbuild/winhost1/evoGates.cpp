#include "emEvolvableMotherboard.h"

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <vector>
#include <iostream>
#include <fstream>

#include <random>
#include <algorithm>
#include <map>

#include <bitset>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;


using namespace emInterfaces;

int main(void)
{
  boost::shared_ptr<TSocket> socket(new TSocket("localhost", 9090));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  
  emEvolvableMotherboardClient client(protocol);
  transport->open();
  client.ping();

  std::vector<std::bitset<11>> outputs;

  for(int rec = 0; rec < 12; rec++) {
    for(int j = 0; j < 2048; j++) {
      std::bitset<11> f ((unsigned long long)j);

      //Reset!
      for(int pin = 0; pin < 12; pin++) {
        emSequenceItem s;
        s.pin = pin;
        s.startTime = 0;
        s.endTime = 1000;
        s.amplitude = 0;
        s.operationType = emSequenceOperationType::type::CONSTANT;
        client.appendSequenceAction(s);
      }
      client.runSequences ();

      client.reset();

      //setup pins for the actual pattern
      int ai = 0;
      for(int pin = 0; pin < 12; pin++) {

        emSequenceItem s;
        if(pin == rec) {
          //ignore 6th bit as output.
          s.pin = rec;
          s.startTime = 0;
          s.endTime = 1024;
          s.frequency = 50000;
          s.operationType = emSequenceOperationType::type::RECORD;
          std::cout << "# ";
        } else {
          s.pin = pin;
          s.startTime = 0;
          s.endTime = 100;
          s.amplitude = f[ai];
          s.operationType = emSequenceOperationType::type::CONSTANT;
          std::cout << f[ai] << " ";
          ai++;
        }

        client.appendSequenceAction(s);
      }
      client.runSequences ();

      emWaveForm r;
      client.getRecording(r, rec);

      std::cout << "=> ";
      std::map<int, int> vals;
      for (auto i : r.Samples) {
        vals[i]++;
        std::cout << i;
      }
      std::cout << std::endl;
    }
    /*
    if (vals[1] > 100) {
      std::cout << " 1" << std::endl;
    } else {
      std::cout << " 0" << std::endl;
    }*/


    //for(auto s : r.Samples) {
    //  std::cout << s;
   // }
    client.clearSequences();
    client.reset();
  }

  std::cout << std::endl;
  transport->close();

  return 0;
}



