#include "emEvolvableMotherboard.h"

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <vector>
#include <iostream>

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
  boost::shared_ptr<TSocket> socket(new TSocket("dmlab02.idi.ntnu.no", 9090));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  
  emEvolvableMotherboardClient client(protocol);
  transport->open();
  client.ping();

  std::vector<std::bitset<12>> outputs;

  for(int i = 0; i < 4096; i++) {
    std::bitset<12> f ((unsigned long long)i);
   
    //setup pins for all 
    for(int i = 0; i < 12; i++) {
      
      emSequenceItem s;

      if(i == 6) {
        //ignore 6th bit as output.
        s.pin = i;
        s.startTime = 0;
        s.endTime = 256;
        s.frequency = 8000;
        s.operationType = emSequenceOperationType::type::RECORD;

      } else {
        s.pin = i;
        s.startTime = 0;
        s.endTime = 256;
        s.amplitude = f[i];
        s.operationType = emSequenceOperationType::type::CONST;
      }
    
      client.appendSequenceAction(s);
    }
    client.runSequences();

    emWaveForm r;
    client.getRecording(r, 6);

    for(auto s : r.Samples) {
      std::cout << s;
    }
    client.clearSequences();
    client.reset();
  }

  std::cout << std::endl;
  transport->close();

  return 0;
}
