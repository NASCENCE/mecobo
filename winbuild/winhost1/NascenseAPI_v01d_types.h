/**
 * Autogenerated by Thrift Compiler (0.9.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef NascenseAPI_v01d_TYPES_H
#define NascenseAPI_v01d_TYPES_H

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>



namespace emInterfaces {

struct emExceptionType {
  enum type {
    CRITICAL = 0,
    NONFATAL = 1
  };
};

extern const std::map<int, const char*> _emExceptionType_VALUES_TO_NAMES;

struct emLogEventType {
  enum type {
    emNULL = 0,
    CONFIG = 1,
    emERROR = 2,
    MISC = 3,
    COMMAND = 4,
    RESPONSE = 5,
    DONOTLOG = 6,
    DISPLAYASMESSAGEBOX = 7
  };
};

extern const std::map<int, const char*> _emLogEventType_VALUES_TO_NAMES;

struct emSequenceOperationType {
  enum type {
    emNULL = 0,
    ARBITRARY = 1,
    RECORD = 2,
    WAIT = 3,
    PREDEFINED = 4,
    CONSTANT = 5
  };
};

extern const std::map<int, const char*> _emSequenceOperationType_VALUES_TO_NAMES;

struct emWaveFormType {
  enum type {
    emNULL = 0,
    ARBITRARY = 1,
    PWM = 2,
    SAW = 3,
    SINE = 4
  };
};

extern const std::map<int, const char*> _emWaveFormType_VALUES_TO_NAMES;

typedef struct _emLogServerSettings__isset {
  _emLogServerSettings__isset() : logServer(false), experimentName(false) {}
  bool logServer;
  bool experimentName;
} _emLogServerSettings__isset;

class emLogServerSettings {
 public:

  static const char* ascii_fingerprint; // = "07A9615F837F7D0A952B595DD3020972";
  static const uint8_t binary_fingerprint[16]; // = {0x07,0xA9,0x61,0x5F,0x83,0x7F,0x7D,0x0A,0x95,0x2B,0x59,0x5D,0xD3,0x02,0x09,0x72};

  emLogServerSettings() : logServer(), experimentName() {
  }

  virtual ~emLogServerSettings() throw() {}

  std::string logServer;
  std::string experimentName;

  _emLogServerSettings__isset __isset;

  void __set_logServer(const std::string& val) {
    logServer = val;
  }

  void __set_experimentName(const std::string& val) {
    experimentName = val;
  }

  bool operator == (const emLogServerSettings & rhs) const
  {
    if (!(logServer == rhs.logServer))
      return false;
    if (!(experimentName == rhs.experimentName))
      return false;
    return true;
  }
  bool operator != (const emLogServerSettings &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const emLogServerSettings & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(emLogServerSettings &a, emLogServerSettings &b);

typedef struct _emException__isset {
  _emException__isset() : errorCode(false), Reason(false), Source(false), exceptionType(false) {}
  bool errorCode;
  bool Reason;
  bool Source;
  bool exceptionType;
} _emException__isset;

class emException : public ::apache::thrift::TException {
 public:

  static const char* ascii_fingerprint; // = "8E595A1DCD897D76B9D95487E55A8A4E";
  static const uint8_t binary_fingerprint[16]; // = {0x8E,0x59,0x5A,0x1D,0xCD,0x89,0x7D,0x76,0xB9,0xD9,0x54,0x87,0xE5,0x5A,0x8A,0x4E};

  emException() : errorCode(0), Reason(), Source(), exceptionType((emExceptionType::type)0) {
  }

  virtual ~emException() throw() {}

  int32_t errorCode;
  std::string Reason;
  std::string Source;
  emExceptionType::type exceptionType;

  _emException__isset __isset;

  void __set_errorCode(const int32_t val) {
    errorCode = val;
  }

  void __set_Reason(const std::string& val) {
    Reason = val;
  }

  void __set_Source(const std::string& val) {
    Source = val;
  }

  void __set_exceptionType(const emExceptionType::type val) {
    exceptionType = val;
  }

  bool operator == (const emException & rhs) const
  {
    if (!(errorCode == rhs.errorCode))
      return false;
    if (!(Reason == rhs.Reason))
      return false;
    if (!(Source == rhs.Source))
      return false;
    if (!(exceptionType == rhs.exceptionType))
      return false;
    return true;
  }
  bool operator != (const emException &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const emException & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(emException &a, emException &b);

typedef struct _emWaveForm__isset {
  _emWaveForm__isset() : Samples(false), Rate(false), SampleCount(false) {}
  bool Samples;
  bool Rate;
  bool SampleCount;
} _emWaveForm__isset;

class emWaveForm {
 public:

  static const char* ascii_fingerprint; // = "3A97DBFE4BA367F203F94B8299BEAECA";
  static const uint8_t binary_fingerprint[16]; // = {0x3A,0x97,0xDB,0xFE,0x4B,0xA3,0x67,0xF2,0x03,0xF9,0x4B,0x82,0x99,0xBE,0xAE,0xCA};

  emWaveForm() : Rate(0), SampleCount(0) {
  }

  virtual ~emWaveForm() throw() {}

  std::vector<int32_t>  Samples;
  int32_t Rate;
  int32_t SampleCount;

  _emWaveForm__isset __isset;

  void __set_Samples(const std::vector<int32_t> & val) {
    Samples = val;
  }

  void __set_Rate(const int32_t val) {
    Rate = val;
  }

  void __set_SampleCount(const int32_t val) {
    SampleCount = val;
  }

  bool operator == (const emWaveForm & rhs) const
  {
    if (!(Samples == rhs.Samples))
      return false;
    if (!(Rate == rhs.Rate))
      return false;
    if (!(SampleCount == rhs.SampleCount))
      return false;
    return true;
  }
  bool operator != (const emWaveForm &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const emWaveForm & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(emWaveForm &a, emWaveForm &b);

typedef struct _emSequenceItem__isset {
  _emSequenceItem__isset() : operationType(false), pin(false), startTime(false), endTime(false), frequency(true), phase(true), cycleTime(true), amplitude(true), waveFormType(false), waveForm(false), waitForTrigger(true) {}
  bool operationType;
  bool pin;
  bool startTime;
  bool endTime;
  bool frequency;
  bool phase;
  bool cycleTime;
  bool amplitude;
  bool waveFormType;
  bool waveForm;
  bool waitForTrigger;
} _emSequenceItem__isset;

class emSequenceItem {
 public:

  static const char* ascii_fingerprint; // = "DC4895B095EBF30E67E39EC74456F780";
  static const uint8_t binary_fingerprint[16]; // = {0xDC,0x48,0x95,0xB0,0x95,0xEB,0xF3,0x0E,0x67,0xE3,0x9E,0xC7,0x44,0x56,0xF7,0x80};

  emSequenceItem() : operationType((emSequenceOperationType::type)0), pin(0), startTime(0), endTime(0), frequency(0), phase(0), cycleTime(0), amplitude(0), waveFormType((emWaveFormType::type)0), waitForTrigger(-1) {
  }

  virtual ~emSequenceItem() throw() {}

  emSequenceOperationType::type operationType;
  int32_t pin;
  int64_t startTime;
  int64_t endTime;
  int32_t frequency;
  int32_t phase;
  int32_t cycleTime;
  int32_t amplitude;
  emWaveFormType::type waveFormType;
  emWaveForm waveForm;
  int32_t waitForTrigger;

  _emSequenceItem__isset __isset;

  void __set_operationType(const emSequenceOperationType::type val) {
    operationType = val;
  }

  void __set_pin(const int32_t val) {
    pin = val;
  }

  void __set_startTime(const int64_t val) {
    startTime = val;
  }

  void __set_endTime(const int64_t val) {
    endTime = val;
  }

  void __set_frequency(const int32_t val) {
    frequency = val;
  }

  void __set_phase(const int32_t val) {
    phase = val;
  }

  void __set_cycleTime(const int32_t val) {
    cycleTime = val;
  }

  void __set_amplitude(const int32_t val) {
    amplitude = val;
  }

  void __set_waveFormType(const emWaveFormType::type val) {
    waveFormType = val;
  }

  void __set_waveForm(const emWaveForm& val) {
    waveForm = val;
  }

  void __set_waitForTrigger(const int32_t val) {
    waitForTrigger = val;
  }

  bool operator == (const emSequenceItem & rhs) const
  {
    if (!(operationType == rhs.operationType))
      return false;
    if (!(pin == rhs.pin))
      return false;
    if (!(startTime == rhs.startTime))
      return false;
    if (!(endTime == rhs.endTime))
      return false;
    if (!(frequency == rhs.frequency))
      return false;
    if (!(phase == rhs.phase))
      return false;
    if (!(cycleTime == rhs.cycleTime))
      return false;
    if (!(amplitude == rhs.amplitude))
      return false;
    if (!(waveFormType == rhs.waveFormType))
      return false;
    if (!(waveForm == rhs.waveForm))
      return false;
    if (!(waitForTrigger == rhs.waitForTrigger))
      return false;
    return true;
  }
  bool operator != (const emSequenceItem &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const emSequenceItem & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(emSequenceItem &a, emSequenceItem &b);

typedef struct _emDebugInfo__isset {
  _emDebugInfo__isset() : stateBlob(false), stateBlobLength(false), values(false) {}
  bool stateBlob;
  bool stateBlobLength;
  bool values;
} _emDebugInfo__isset;

class emDebugInfo {
 public:

  static const char* ascii_fingerprint; // = "0AD85E73ED11FD8C9FB0DA32F9512610";
  static const uint8_t binary_fingerprint[16]; // = {0x0A,0xD8,0x5E,0x73,0xED,0x11,0xFD,0x8C,0x9F,0xB0,0xDA,0x32,0xF9,0x51,0x26,0x10};

  emDebugInfo() : stateBlob(), stateBlobLength(0) {
  }

  virtual ~emDebugInfo() throw() {}

  std::string stateBlob;
  int32_t stateBlobLength;
  std::map<std::string, std::string>  values;

  _emDebugInfo__isset __isset;

  void __set_stateBlob(const std::string& val) {
    stateBlob = val;
  }

  void __set_stateBlobLength(const int32_t val) {
    stateBlobLength = val;
  }

  void __set_values(const std::map<std::string, std::string> & val) {
    values = val;
  }

  bool operator == (const emDebugInfo & rhs) const
  {
    if (!(stateBlob == rhs.stateBlob))
      return false;
    if (!(stateBlobLength == rhs.stateBlobLength))
      return false;
    if (!(values == rhs.values))
      return false;
    return true;
  }
  bool operator != (const emDebugInfo &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const emDebugInfo & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

void swap(emDebugInfo &a, emDebugInfo &b);

} // namespace

#endif
