// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "emEvolvableMotherboard.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::emInterfaces;

class emEvolvableMotherboardHandler : virtual public emEvolvableMotherboardIf {
 public:
  emEvolvableMotherboardHandler() {
    // Your initialization goes here
  }

  int32_t ping() {
    // Your implementation goes here
    printf("ping\n");
  }

  void setLED(const int32_t index, const bool state) {
    // Your implementation goes here
    printf("setLED\n");
  }

  void getMotherboardID(std::string& _return) {
    // Your implementation goes here
    printf("getMotherboardID\n");
  }

  void getMotherboardState(std::string& _return) {
    // Your implementation goes here
    printf("getMotherboardState\n");
  }

  void getLastError(std::string& _return) {
    // Your implementation goes here
    printf("getLastError\n");
  }

  bool reset() {
    // Your implementation goes here
    printf("reset\n");
  }

  bool reprogramme(const std::string& bin, const int32_t length) {
    // Your implementation goes here
    printf("reprogramme\n");
  }

  void getDebugState(emDebugInfo& _return) {
    // Your implementation goes here
    printf("getDebugState\n");
  }

  void clearSequences() {
    // Your implementation goes here
    printf("clearSequences\n");
  }

  void runSequences() {
    // Your implementation goes here
    printf("runSequences\n");
  }

  void stopSequences() {
    // Your implementation goes here
    printf("stopSequences\n");
  }

  void joinSequences() {
    // Your implementation goes here
    printf("joinSequences\n");
  }

  void appendSequenceAction(const emSequenceItem& Item) {
    // Your implementation goes here
    printf("appendSequenceAction\n");
  }

  void getRecording(emWaveForm& _return, const int32_t srcPin) {
    // Your implementation goes here
    printf("getRecording\n");
  }

  void clearRecording(const int32_t srcPin) {
    // Your implementation goes here
    printf("clearRecording\n");
  }

  int32_t getTemperature() {
    // Your implementation goes here
    printf("getTemperature\n");
  }

  void setLogServer(const emLogServerSettings& logServer) {
    // Your implementation goes here
    printf("setLogServer\n");
  }

  void setConfigRegister(const int32_t index, const int32_t value) {
    // Your implementation goes here
    printf("setConfigRegister\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  shared_ptr<emEvolvableMotherboardHandler> handler(new emEvolvableMotherboardHandler());
  shared_ptr<TProcessor> processor(new emEvolvableMotherboardProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

