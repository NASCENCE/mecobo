// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "emEvolvableMotherboard.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "mecohost.h"
#include "../mecoprot.h"
#include <map>
#include <queue>
#include <thread>

#include "emMotherboard.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::emInterfaces;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

class emEvolvableMotherboardHandler : virtual public emEvolvableMotherboardIf {


  shared_ptr<emMotherboard> mobo;

  std::thread runThread;

  int64_t time;
  std::vector<emSequenceItem> seqItems;

  std::map<int, std::queue<emSequenceItem>> pinSeq;
  std::map<int, std::vector<uint32_t>> rec;
  //std::vector<int> recPins;
  std::vector<emSequenceItem> itemsInFlight;

  public:
  emEvolvableMotherboardHandler(shared_ptr<emMotherboard> mobo) {
    // Your initialization goes here
    time = 0;
    this->mobo = mobo;
  }

  int32_t ping() {
    // Your implementation goes here
    printf("ping\n");
    return 0;
  }

  void setLED(const int32_t index, const bool state) {
    state ? moboSetLed(index, 0) : moboSetLed(index, 1);
    std::cout << "Led " << index << "is " << state << std::endl;
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
    resetAllPins();
    printf("reset\n");
    return true;
  }

  bool reprogramme(const std::string& bin, const int32_t length) {
    // Your implementation goes here
    printf("reprogramme\n");
    return true;
  }

  void getDebugState(emDebugInfo& _return) {
    // Your implementation goes here
    printf("getDebugState\n");
  }

  void clearSequences() {
    // Your implementation goes here
    
    printf("Clearing sequence queue.\n");
    pinSeq.clear();
    seqItems.clear();
  }

  void runSequences() {
    
    //Reset board?
    reset();
    //Sort the sequence before we submit the items to the board.
    std::cout << "Submitting sequence to board." << std::endl;
    std::sort(seqItems.begin(), seqItems.end(), 
        [](emSequenceItem const & a, emSequenceItem const & b) { return a.startTime < b.startTime; });
    
    int lastEnd = -1;
    for (auto item : seqItems) {
      setupItem(item);
      if(item.endTime > lastEnd) {
        lastEnd = item.endTime;
        std::cout << "Last item ends at" << lastEnd << std::endl;
      }
    }
    
    std::cout << "Running sequences." << std::endl;
    steady_clock::time_point start = steady_clock::now();
    steady_clock::time_point end = steady_clock::now();
    evoMoboRunSeq();

    //If it took a little time to schedule that last item, add a little slack.
    lastEnd += 10;
    while(duration_cast<milliseconds>(end - start).count() < lastEnd) {
      end = steady_clock::now();
    }
    std::cout << "-- Sequence done --" << std::endl;
    std::vector<sampleValue> samples;
    getSampleBuffer(samples);
    for(auto s : samples) {
      //std::cout << "Samples for pin " << s.pin << ":" << s.sampleNum << std::endl;
      rec[s.pin].push_back(s.value);
    }

    std::cout << "Sequence done, all qeueues empty." << std::endl;
    std::cout << "Clearing sequences for you." << std::endl;
    clearSequences();
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
    //TODO: Lots of error checking and all that jazzy.
    std::cout << "Appending action" << std::endl;
    
    //append to vector that we will sort when doing runSequences (where we will do most work)
    seqItems.push_back(Item);
    return;
  }

  void getRecording(emWaveForm& _return, const int32_t srcPin) {
    // Your implementation goes here
    std::vector<int32_t> v;
    std::vector<sampleValue> samples;

    std::cout << "There are " << rec[srcPin].size() << "samples for pin " << srcPin << std::endl;
    for(auto s : rec[srcPin]) {
      //std::cout << s.sampleNum << std::endl;
      v.push_back(s);
    }

    emWaveForm r;
    r.SampleCount = v.size();
    r.Samples = v;
    _return = r;

    rec[srcPin].clear();
  }


  void clearRecording(const int32_t srcPin) {
    // Your implementation goes here
    printf("clearRecording\n");
  }

  int32_t getTemperature() {
    // Your implementation goes here
    printf("getTemperature\n");
    return 0;
  }

  void setLogServer(const emLogServerSettings& logServer) {
    // Your implementation goes here
    printf("setLogServer\n");
  }

  private:
  std::string stringRepItem(emSequenceItem & em) 
  {
    std::string ret;
    ret.append("-- ");
    return ret;
  }
  /*
    void sanitizeItem(emSequenceItem & em) {
      switch(item.operationType) {
        case emSequenceOperationType::type::PWM:
          break;
        default:
          break;
      } 
    }
    */
  void setupItem(emSequenceItem item) {
    double period; //= 1.0f/(double)s.frequency;
    int32_t duty; // = period * (25*1000000);
    int32_t aduty; // = period * (25*1000000);
    uint32_t sampleDiv = ((50*1000000)/(double)item.frequency);
    emException err;

    switch(item.operationType) {
      case emSequenceOperationType::type::CONSTANT:
        std::cout << "CONSTANT added: " << item.amplitude << " on pin " << item.pin << std::endl;
        submitItem((FPGA_IO_Pins_TypeDef)item.pin, item.startTime, item.endTime, item.amplitude, 0, 0x1, 0x0, PINCONFIG_DATA_TYPE_DIRECT_CONST, item.amplitude);
        break;
      case emSequenceOperationType::type::PREDEFINED:
        //Since it's predefined, we have a waveFormType
        if(item.waveFormType == emWaveFormType::PWM) {
          period = 1.0/(double)item.frequency;

          duty =  (item.cycleTime/100.0)*(period * (50*1000000));
          aduty = ((100.0f - item.cycleTime)/100.0) * period * (50*1000000);


          if(item.frequency < 382) {
            std::cout << "Frequency is too low:" << item.frequency << std::endl;
            std::cout << "Please set frequency to over 400Hz.";
            
            err.Reason = "Frequency too low; under 400Hz"; //reason;
            err.Source = "emMotherboard sequencer";
            throw err;
            break;
          }
          std::cout << "PREDEFINED PWM added: Freq:" << item.frequency << ", duty" << duty << " Antiduty: " << aduty << std::endl;
          submitItem((FPGA_IO_Pins_TypeDef)item.pin, item.startTime, item.endTime,  (uint32_t)duty, (uint32_t)aduty, 0x1, 0x0, PINCONFIG_DATA_TYPE_PREDEFINED_PWM, item.amplitude);
        }
        break;
      case emSequenceOperationType::type::RECORD:
        std::cout << "RECORDING added on pin " << item.pin << ". Start: " << item.startTime << ", End: " << item.endTime <<", Freq: " << item.frequency << " Gives sample divisor [debug]:" << sampleDiv << std::endl;
        if(sampleDiv <= 1) {
          err.Reason = "samplerate too high";
          err.Source = "emMotherboard";
          throw err;
          break;
        } else if (sampleDiv > 65535) {
          err.Reason = "samplerate too low";
          err.Source = "emMotherboard";
          throw err;
          break;
        }
        submitItem((FPGA_IO_Pins_TypeDef)item.pin, item.startTime, item.endTime, 1, 1, 1, sampleDiv, PINCONFIG_DATA_TYPE_RECORD, item.amplitude);
        break;
      default:
        break;
    }

  }
};

int main(int argc, char **argv) {


  std::cout << "Hi, I'm the evolutionary motherboard." << std::endl;
  std::cout << "I was built on " << __DATE__ << " at " __TIME__ << std::endl;
  shared_ptr<emMotherboard> em(new emMotherboard());
  uint32_t forceProgFpga = 1;  //default program fpga.
  //Command line arguments
  if (argc > 1) {
    for(int i = 0; i < argc; i++) {
      if(strcmp(argv[i], "-f") == 0) {
        forceProgFpga = 0;
      }
    }
  }
  

  int port = 9090;

  shared_ptr<emEvolvableMotherboardHandler> handler(new emEvolvableMotherboardHandler(em));
  shared_ptr<TProcessor> processor(new emEvolvableMotherboardProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);

  std::cout << "Starting USB..." << std::endl;
  startUsb();
  std::cout << "Done!" << std::endl;

  if(forceProgFpga) {
    programFPGA("mecobo.bin");
  } else {
    //Check if FPGA is configured
    /*
    if(em != NULL) {
      if(!(em->isFpgaConfigured())) {
        programFPGA("mecobo.bin");
      }
    }
    */
  }

  std::cout << "Starting thrift server. (Silence ensues)." << std::endl;
  server.serve();

  std::cout << "Stopping USB..." << std::endl;
  stopUsb();
  std::cout << "Done!" << std::endl;

  return 0;
}



