// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "emEvolvableMotherboard.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "channelmap.h"
#include "../mecoprot.h"
#include <map>
#include <queue>
#include <thread>
#include <chrono>
#include <thread>


#include "Mecobo.h"
#include "USB.h"

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


  int port;
  int boardAddr;

  std::thread runThread;

  steady_clock::time_point sequenceRunStart;
  int lastSequenceItemEnd;

  int64_t time;
  std::vector<emSequenceItem> seqItems;
  
  channelMap xbar;

  //Keeps recordings of all the recording pins.
  std::map<int, std::vector<uint32_t>> rec;
  //std::vector<int> recPins;
  std::vector<emSequenceItem> itemsInFlight;

  public:
  Mecobo * mecobo;
  //constructor for this class
  emEvolvableMotherboardHandler(int force, std::string bitfilename, bool daughterboard) {
    // Your initialization goes here
    time = 0;
    std::cout << "Starting USB subsystem." << std::endl;
    mecobo = new Mecobo(daughterboard);
    if (force)  {
      mecobo->programFPGA(bitfilename.c_str());
    }  
  }

  int32_t ping() {
    // Your implementation goes here
    printf("ping\n");
    return 0;
  }

  void setLED(const int32_t index, const bool state) {
    state ? mecobo->setLed(index, 1) : mecobo->setLed(index, 0);
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
    //mecobo->discharge();
    mecobo->reset();
    clearSequences();
    std::cout << "Board reset and sequences cleared" << std::endl;
    return true;
  }

  bool reprogramme(const std::string& bin, const int32_t length) {
    std::cout << "Reprogrammed called. Not implemented yet." << std::endl;
    return true;
  }

  void getDebugState(emDebugInfo& _return) {
    _return.stateBlob = std::string("Not implemented yet\n");
  }

  void clearSequences() {
    printf("Clearing sequence queue.\n");
    seqItems.clear();
  }

  void runSequences() {
    
    emException err;
    //Reset board?
    mecobo->reset();
    //Sort the sequence before we submit the items to the board.
    std::cout << "Scheduling sequences on board." << std::endl;
    if (seqItems.size() > 50) {
      err.Reason = "No more than 50 items please!";
      err.Source = "runSequences()";
      throw err;
    }
    std::sort(seqItems.begin(), seqItems.end(), 
        [](emSequenceItem const & a, emSequenceItem const & b) { return a.startTime < b.startTime; });
    

    //Find the last item as well.
    lastSequenceItemEnd = -1;
    for (auto item : seqItems) {
      setupItem(item);

      if(item.endTime > lastSequenceItemEnd) {
        lastSequenceItemEnd = item.endTime;
      }
    }

    std::cout << "Instructing Mecobo to run scheduled sequence items." << std::endl;
    mecobo->runSchedule();
    sequenceRunStart = steady_clock::now();
  } 


  void stopSequences() {
    std::cout << "stopSequences does nothing yet." << std::endl;
  }

  void joinSequences() {
    //Hang around until things are done.
    std::cout << "Join called. Blocking until all items have run to completion." << std::endl;
    std::cout << "Last item ends at " << lastSequenceItemEnd << std::endl;
    int totalWaitMs = lastSequenceItemEnd;
    unsigned int items = 0;
  
    //Wait for at least the lastSeq endtime.
    std::this_thread::sleep_for(std::chrono::milliseconds(lastSequenceItemEnd)); 

    while((items = mecobo->status().itemsInQueue) > 0) {
      //Ask again in 10ms -- should ve very close to done. 
      std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
      totalWaitMs += 10;
      if (totalWaitMs > (lastSequenceItemEnd*2)) {
        std::cout << "We waited long enough (twice!). Exit timeout. NumItems: " << items << std::endl;
        break;
      }
    }
  }

  void appendSequenceAction(const emSequenceItem& Item) {
    std::cout << "Appending action " << Item.operationType << std::endl;
    //append to vector that we will sort when doing runSequences (where we will do most work)
    seqItems.push_back(Item);
    return;
  }

  void getRecording(emWaveForm& _return, const int32_t srcPin) {

    std::vector<int32_t> r = mecobo->getSampleBuffer(srcPin);
    std::cout << "Pin " << srcPin << " has " << r.size() << " samples" << std::endl;

    _return.SampleCount = r.size();
    _return.Samples = r;
  }


  void clearRecording(const int32_t srcPin) {
    // Your implementation goes here
    rec[srcPin].clear();
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

  void setConfigRegister(const int32_t index, const int32_t value) {
    if((value < 0) || (value > 255)) {

      emException err;
      err.Reason = "Value in config registers must be between 0 and 255";
      err.Source = "setConfigRegister(register index, value)";
      throw err;
    }
    std::cout << "Setting config register " << index << " to " << value << std::endl;
    mecobo->updateRegister(index, value);
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
        for(auto p : item.pin) {
          std::cout << "CONSTANT. Amplitude:" << item.amplitude << " Pin: " << p << \
            "Start:" << item.startTime << "End: " << item.endTime << std::endl;
        }
        mecobo->scheduleConstantVoltage(item.pin, (int)item.startTime, (int)item.endTime, (int)item.amplitude);
        break;

      case emSequenceOperationType::type::CONSTANT_FROM_REGISTER:
        for(auto p : item.pin) {
          std::cout << "CONSTANT FROM REG. Amplitude:" << item.amplitude << " Pin: " << p << \
            "Start:" << item.startTime << "End: " << item.endTime << std::endl;
        }
        mecobo->scheduleConstantVoltageFromRegister(item.pin, (int)item.startTime, (int)item.endTime, (int)item.ValueSourceRegister);
        break;


      case emSequenceOperationType::type::RECORD:

        if (item.waveFormType == emWaveFormType::PWM) {
          for(auto p : item.pin) {
            std::cout << "RECORDING [digital] added on pin " << p << ". Start: " << item.startTime << ", End: " << item.endTime <<", Freq: " << item.frequency << " Gives sample divisor [debug]:" << sampleDiv << std::endl;
          }
          mecobo->scheduleDigitalRecording(item.pin, item.startTime, item.endTime, item.frequency);

        } else {
          for(auto p : item.pin) {
            std::cout << "RECORDING [analogue] added on pin " << p << ". Start: " << item.startTime << ", End: " << item.endTime <<", Freq: " << item.frequency << " Gives sample divisor [debug]:" << sampleDiv << std::endl;
          }
          mecobo->scheduleRecording(item.pin, item.startTime, item.endTime, item.frequency);
        }
        //Error checking.
        //
        /*
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
           }*/

        break;

      case emSequenceOperationType::type::DIGITAL:
        for(auto p : item.pin) {
          std::cout << "DIGITAL OUT. Freq:" << item.frequency << "Cycle: " << item.cycleTime << "Pin: " << p << \
            "Start:" << item.startTime << "End: " << item.endTime << std::endl;
        }
        mecobo->scheduleDigitalOutput(item.pin, (int)item.startTime, (int)item.endTime, (int)item.frequency, (int)item.cycleTime);
        break;



      //YAY DOUBLE CASE SWITCH CASE.
      case emSequenceOperationType::type::PREDEFINED:
        switch (item.waveFormType) {
          case emWaveFormType::SINE:
          for(auto p : item.pin) {
        	  std::cout << "PREDEFINED SINE on " << p << std::endl;
          }
                mecobo->scheduleSine(item.pin, item.startTime, item.endTime, item.frequency, item.amplitude, item.phase);
            break;

          case emWaveFormType::PWM:
            for (auto p: item.pin) {
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

        	std::cout << "PREDEFINED PWM added on pin " << p << ": Freq:" << item.frequency << ", duty" << duty << " Antiduty: " << aduty << std::endl;
        	//submitItem(item.pin, item.startTime, item.endTime,  (uint32_t)duty, (uint32_t)aduty, 0x1, 0x0, PINCONFIG_DATA_TYPE_PREDEFINED_PWM, item.amplitude);
            }
        	  mecobo->schedulePWMoutput(item.pin, item.startTime, item.endTime, item.amplitude);
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }

  }
};

int main(int argc, char **argv) {


  std::cout << "Hi, I'm the evolutionary motherboard." << std::endl;
  std::cout << "I was built on " << __DATE__ << " at " __TIME__ << std::endl;


  uint32_t forceProgFpga = 1;  //default program fpga.
  bool daughterboard = true;
  std::string bitfilename = std::string("mecobo.bin");
  //Command line arguments
  if (argc > 1) {
    for(int i = 0; i < argc; i++) {
      if(strcmp(argv[i], "-f") == 0) {
        forceProgFpga = 0;
      }
      if(strcmp(argv[i], "-b") == 0) {
        forceProgFpga = 1;
        bitfilename = std::string(argv[++i]);
        printf("Programming FPGA with bitfile %s\n", bitfilename.c_str());
      }

      if(strcmp(argv[i], "-n") == 0) {
        std::cout << "Option -n passed, host assumes no daughterboard" << std::endl;
        daughterboard = false;
      
      }
    }
  }

  shared_ptr<emEvolvableMotherboardHandler> handler(new emEvolvableMotherboardHandler(forceProgFpga, bitfilename, daughterboard));
  shared_ptr<TProcessor> processor(new emEvolvableMotherboardProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(handler->mecobo->getPort()));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);

  //std::cout << "Starting thrift server, listening at port " << port << std::endl;
  std::cout << "Thrift starting" << std::endl;
  server.serve();
  std::cout << "EvoMaterio exiting. Sayonara." << std::endl;

  return 0;
}



