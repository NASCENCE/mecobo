/**
 * Autogenerated by Thrift Compiler (0.9.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "NascenseAPI_v01e_types.h"

#include <algorithm>

namespace emInterfaces {

int _kemExceptionTypeValues[] = {
  emExceptionType::CRITICAL,
  emExceptionType::NONFATAL
};
const char* _kemExceptionTypeNames[] = {
  "CRITICAL",
  "NONFATAL"
};
const std::map<int, const char*> _emExceptionType_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(2, _kemExceptionTypeValues, _kemExceptionTypeNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

int _kemLogEventTypeValues[] = {
  emLogEventType::emNULL,
  emLogEventType::CONFIG,
  emLogEventType::emERROR,
  emLogEventType::MISC,
  emLogEventType::COMMAND,
  emLogEventType::RESPONSE,
  emLogEventType::DONOTLOG,
  emLogEventType::DISPLAYASMESSAGEBOX
};
const char* _kemLogEventTypeNames[] = {
  "emNULL",
  "CONFIG",
  "emERROR",
  "MISC",
  "COMMAND",
  "RESPONSE",
  "DONOTLOG",
  "DISPLAYASMESSAGEBOX"
};
const std::map<int, const char*> _emLogEventType_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(8, _kemLogEventTypeValues, _kemLogEventTypeNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

int _kemSequenceOperationTypeValues[] = {
  emSequenceOperationType::emNULL,
  emSequenceOperationType::ARBITRARY,
  emSequenceOperationType::RECORD,
  emSequenceOperationType::WAIT,
  emSequenceOperationType::PREDEFINED,
  emSequenceOperationType::CONSTANT,
  emSequenceOperationType::DIGITAL,
  emSequenceOperationType::CONSTANT_FROM_REGISTER
};
const char* _kemSequenceOperationTypeNames[] = {
  "emNULL",
  "ARBITRARY",
  "RECORD",
  "WAIT",
  "PREDEFINED",
  "CONSTANT",
  "DIGITAL",
  "CONSTANT_FROM_REGISTER"
};
const std::map<int, const char*> _emSequenceOperationType_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(8, _kemSequenceOperationTypeValues, _kemSequenceOperationTypeNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

int _kemWaveFormTypeValues[] = {
  emWaveFormType::emNULL,
  emWaveFormType::ARBITRARY,
  emWaveFormType::PWM,
  emWaveFormType::SAW,
  emWaveFormType::SINE
};
const char* _kemWaveFormTypeNames[] = {
  "emNULL",
  "ARBITRARY",
  "PWM",
  "SAW",
  "SINE"
};
const std::map<int, const char*> _emWaveFormType_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(5, _kemWaveFormTypeValues, _kemWaveFormTypeNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));

const char* emLogServerSettings::ascii_fingerprint = "07A9615F837F7D0A952B595DD3020972";
const uint8_t emLogServerSettings::binary_fingerprint[16] = {0x07,0xA9,0x61,0x5F,0x83,0x7F,0x7D,0x0A,0x95,0x2B,0x59,0x5D,0xD3,0x02,0x09,0x72};

uint32_t emLogServerSettings::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->logServer);
          this->__isset.logServer = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->experimentName);
          this->__isset.experimentName = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t emLogServerSettings::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("emLogServerSettings");

  xfer += oprot->writeFieldBegin("logServer", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->logServer);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("experimentName", ::apache::thrift::protocol::T_STRING, 2);
  xfer += oprot->writeString(this->experimentName);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(emLogServerSettings &a, emLogServerSettings &b) {
  using ::std::swap;
  swap(a.logServer, b.logServer);
  swap(a.experimentName, b.experimentName);
  swap(a.__isset, b.__isset);
}

const char* emException::ascii_fingerprint = "8E595A1DCD897D76B9D95487E55A8A4E";
const uint8_t emException::binary_fingerprint[16] = {0x8E,0x59,0x5A,0x1D,0xCD,0x89,0x7D,0x76,0xB9,0xD9,0x54,0x87,0xE5,0x5A,0x8A,0x4E};

uint32_t emException::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->errorCode);
          this->__isset.errorCode = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->Reason);
          this->__isset.Reason = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->Source);
          this->__isset.Source = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 4:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          int32_t ecast0;
          xfer += iprot->readI32(ecast0);
          this->exceptionType = (emExceptionType::type)ecast0;
          this->__isset.exceptionType = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t emException::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("emException");

  xfer += oprot->writeFieldBegin("errorCode", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32(this->errorCode);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("Reason", ::apache::thrift::protocol::T_STRING, 2);
  xfer += oprot->writeString(this->Reason);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("Source", ::apache::thrift::protocol::T_STRING, 3);
  xfer += oprot->writeString(this->Source);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("exceptionType", ::apache::thrift::protocol::T_I32, 4);
  xfer += oprot->writeI32((int32_t)this->exceptionType);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(emException &a, emException &b) {
  using ::std::swap;
  swap(a.errorCode, b.errorCode);
  swap(a.Reason, b.Reason);
  swap(a.Source, b.Source);
  swap(a.exceptionType, b.exceptionType);
  swap(a.__isset, b.__isset);
}

const char* emWaveForm::ascii_fingerprint = "3A97DBFE4BA367F203F94B8299BEAECA";
const uint8_t emWaveForm::binary_fingerprint[16] = {0x3A,0x97,0xDB,0xFE,0x4B,0xA3,0x67,0xF2,0x03,0xF9,0x4B,0x82,0x99,0xBE,0xAE,0xCA};

uint32_t emWaveForm::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            this->Samples.clear();
            uint32_t _size1;
            ::apache::thrift::protocol::TType _etype4;
            xfer += iprot->readListBegin(_etype4, _size1);
            this->Samples.resize(_size1);
            uint32_t _i5;
            for (_i5 = 0; _i5 < _size1; ++_i5)
            {
              xfer += iprot->readI32(this->Samples[_i5]);
            }
            xfer += iprot->readListEnd();
          }
          this->__isset.Samples = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->Rate);
          this->__isset.Rate = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->SampleCount);
          this->__isset.SampleCount = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t emWaveForm::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("emWaveForm");

  xfer += oprot->writeFieldBegin("Samples", ::apache::thrift::protocol::T_LIST, 1);
  {
    xfer += oprot->writeListBegin(::apache::thrift::protocol::T_I32, static_cast<uint32_t>(this->Samples.size()));
    std::vector<int32_t> ::const_iterator _iter6;
    for (_iter6 = this->Samples.begin(); _iter6 != this->Samples.end(); ++_iter6)
    {
      xfer += oprot->writeI32((*_iter6));
    }
    xfer += oprot->writeListEnd();
  }
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("Rate", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32(this->Rate);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("SampleCount", ::apache::thrift::protocol::T_I32, 3);
  xfer += oprot->writeI32(this->SampleCount);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(emWaveForm &a, emWaveForm &b) {
  using ::std::swap;
  swap(a.Samples, b.Samples);
  swap(a.Rate, b.Rate);
  swap(a.SampleCount, b.SampleCount);
  swap(a.__isset, b.__isset);
}

const char* emSequenceItem::ascii_fingerprint = "9FED06A0F02E214B1032B5045900AD35";
const uint8_t emSequenceItem::binary_fingerprint[16] = {0x9F,0xED,0x06,0xA0,0xF0,0x2E,0x21,0x4B,0x10,0x32,0xB5,0x04,0x59,0x00,0xAD,0x35};

uint32_t emSequenceItem::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          int32_t ecast7;
          xfer += iprot->readI32(ecast7);
          this->operationType = (emSequenceOperationType::type)ecast7;
          this->__isset.operationType = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            this->pin.clear();
            uint32_t _size8;
            ::apache::thrift::protocol::TType _etype11;
            xfer += iprot->readListBegin(_etype11, _size8);
            this->pin.resize(_size8);
            uint32_t _i12;
            for (_i12 = 0; _i12 < _size8; ++_i12)
            {
              xfer += iprot->readI32(this->pin[_i12]);
            }
            xfer += iprot->readListEnd();
          }
          this->__isset.pin = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->startTime);
          this->__isset.startTime = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 4:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->endTime);
          this->__isset.endTime = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 5:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->frequency);
          this->__isset.frequency = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 6:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->phase);
          this->__isset.phase = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 7:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->cycleTime);
          this->__isset.cycleTime = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 8:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->amplitude);
          this->__isset.amplitude = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 9:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          int32_t ecast13;
          xfer += iprot->readI32(ecast13);
          this->waveFormType = (emWaveFormType::type)ecast13;
          this->__isset.waveFormType = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 10:
        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
          xfer += this->waveForm.read(iprot);
          this->__isset.waveForm = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 11:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->waitForTrigger);
          this->__isset.waitForTrigger = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 12:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->ValueSourceRegister);
          this->__isset.ValueSourceRegister = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t emSequenceItem::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("emSequenceItem");

  xfer += oprot->writeFieldBegin("operationType", ::apache::thrift::protocol::T_I32, 1);
  xfer += oprot->writeI32((int32_t)this->operationType);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("pin", ::apache::thrift::protocol::T_LIST, 2);
  {
    xfer += oprot->writeListBegin(::apache::thrift::protocol::T_I32, static_cast<uint32_t>(this->pin.size()));
    std::vector<int32_t> ::const_iterator _iter14;
    for (_iter14 = this->pin.begin(); _iter14 != this->pin.end(); ++_iter14)
    {
      xfer += oprot->writeI32((*_iter14));
    }
    xfer += oprot->writeListEnd();
  }
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("startTime", ::apache::thrift::protocol::T_I64, 3);
  xfer += oprot->writeI64(this->startTime);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("endTime", ::apache::thrift::protocol::T_I64, 4);
  xfer += oprot->writeI64(this->endTime);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("frequency", ::apache::thrift::protocol::T_I32, 5);
  xfer += oprot->writeI32(this->frequency);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("phase", ::apache::thrift::protocol::T_I32, 6);
  xfer += oprot->writeI32(this->phase);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("cycleTime", ::apache::thrift::protocol::T_I32, 7);
  xfer += oprot->writeI32(this->cycleTime);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("amplitude", ::apache::thrift::protocol::T_I32, 8);
  xfer += oprot->writeI32(this->amplitude);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("waveFormType", ::apache::thrift::protocol::T_I32, 9);
  xfer += oprot->writeI32((int32_t)this->waveFormType);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("waveForm", ::apache::thrift::protocol::T_STRUCT, 10);
  xfer += this->waveForm.write(oprot);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("waitForTrigger", ::apache::thrift::protocol::T_I32, 11);
  xfer += oprot->writeI32(this->waitForTrigger);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("ValueSourceRegister", ::apache::thrift::protocol::T_I32, 12);
  xfer += oprot->writeI32(this->ValueSourceRegister);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(emSequenceItem &a, emSequenceItem &b) {
  using ::std::swap;
  swap(a.operationType, b.operationType);
  swap(a.pin, b.pin);
  swap(a.startTime, b.startTime);
  swap(a.endTime, b.endTime);
  swap(a.frequency, b.frequency);
  swap(a.phase, b.phase);
  swap(a.cycleTime, b.cycleTime);
  swap(a.amplitude, b.amplitude);
  swap(a.waveFormType, b.waveFormType);
  swap(a.waveForm, b.waveForm);
  swap(a.waitForTrigger, b.waitForTrigger);
  swap(a.ValueSourceRegister, b.ValueSourceRegister);
  swap(a.__isset, b.__isset);
}

const char* emDebugInfo::ascii_fingerprint = "0AD85E73ED11FD8C9FB0DA32F9512610";
const uint8_t emDebugInfo::binary_fingerprint[16] = {0x0A,0xD8,0x5E,0x73,0xED,0x11,0xFD,0x8C,0x9F,0xB0,0xDA,0x32,0xF9,0x51,0x26,0x10};

uint32_t emDebugInfo::read(::apache::thrift::protocol::TProtocol* iprot) {

  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readBinary(this->stateBlob);
          this->__isset.stateBlob = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->stateBlobLength);
          this->__isset.stateBlobLength = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_MAP) {
          {
            this->values.clear();
            uint32_t _size15;
            ::apache::thrift::protocol::TType _ktype16;
            ::apache::thrift::protocol::TType _vtype17;
            xfer += iprot->readMapBegin(_ktype16, _vtype17, _size15);
            uint32_t _i19;
            for (_i19 = 0; _i19 < _size15; ++_i19)
            {
              std::string _key20;
              xfer += iprot->readString(_key20);
              std::string& _val21 = this->values[_key20];
              xfer += iprot->readString(_val21);
            }
            xfer += iprot->readMapEnd();
          }
          this->__isset.values = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t emDebugInfo::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  xfer += oprot->writeStructBegin("emDebugInfo");

  xfer += oprot->writeFieldBegin("stateBlob", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeBinary(this->stateBlob);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("stateBlobLength", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32(this->stateBlobLength);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("values", ::apache::thrift::protocol::T_MAP, 3);
  {
    xfer += oprot->writeMapBegin(::apache::thrift::protocol::T_STRING, ::apache::thrift::protocol::T_STRING, static_cast<uint32_t>(this->values.size()));
    std::map<std::string, std::string> ::const_iterator _iter22;
    for (_iter22 = this->values.begin(); _iter22 != this->values.end(); ++_iter22)
    {
      xfer += oprot->writeString(_iter22->first);
      xfer += oprot->writeString(_iter22->second);
    }
    xfer += oprot->writeMapEnd();
  }
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(emDebugInfo &a, emDebugInfo &b) {
  using ::std::swap;
  swap(a.stateBlob, b.stateBlob);
  swap(a.stateBlobLength, b.stateBlobLength);
  swap(a.values, b.values);
  swap(a.__isset, b.__isset);
}

} // namespace
