// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "emLogServer.h"
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

class emLogServerHandler : virtual public emLogServerIf {
 public:
  emLogServerHandler() {
    // Your initialization goes here
  }

  void createUniqueExperimentName(std::string& _return, const std::string& baseName) {
    // Your implementation goes here
    printf("createUniqueExperimentName\n");
  }

  void getLogServerSettings(emLogServerSettings& _return, const std::string& uniqueExperimentName) {
    // Your implementation goes here
    printf("getLogServerSettings\n");
  }

  void log(const emLogServerSettings& logServer, const std::string& message, const emLogEventType::type messageType) {
    // Your implementation goes here
    printf("log\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  shared_ptr<emLogServerHandler> handler(new emLogServerHandler());
  shared_ptr<TProcessor> processor(new emLogServerProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

