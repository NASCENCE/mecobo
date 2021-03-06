/**
 * Autogenerated by Thrift Compiler (0.9.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using Thrift;
using Thrift.Collections;
using System.Runtime.Serialization;
using Thrift.Protocol;
using Thrift.Transport;

namespace emInterfaces
{

  /// <summary>
  /// Debug struct
  /// </summary>
  #if !SILVERLIGHT
  [Serializable]
  #endif
  public partial class emDebugInfo : TBase
  {
    private byte[] _stateBlob;
    private int _stateBlobLength;
    private Dictionary<string, string> _values;

    public byte[] StateBlob
    {
      get
      {
        return _stateBlob;
      }
      set
      {
        __isset.stateBlob = true;
        this._stateBlob = value;
      }
    }

    public int StateBlobLength
    {
      get
      {
        return _stateBlobLength;
      }
      set
      {
        __isset.stateBlobLength = true;
        this._stateBlobLength = value;
      }
    }

    public Dictionary<string, string> Values
    {
      get
      {
        return _values;
      }
      set
      {
        __isset.values = true;
        this._values = value;
      }
    }


    public Isset __isset;
    #if !SILVERLIGHT
    [Serializable]
    #endif
    public struct Isset {
      public bool stateBlob;
      public bool stateBlobLength;
      public bool values;
    }

    public emDebugInfo() {
    }

    public void Read (TProtocol iprot)
    {
      TField field;
      iprot.ReadStructBegin();
      while (true)
      {
        field = iprot.ReadFieldBegin();
        if (field.Type == TType.Stop) { 
          break;
        }
        switch (field.ID)
        {
          case 1:
            if (field.Type == TType.String) {
              StateBlob = iprot.ReadBinary();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 2:
            if (field.Type == TType.I32) {
              StateBlobLength = iprot.ReadI32();
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          case 3:
            if (field.Type == TType.Map) {
              {
                Values = new Dictionary<string, string>();
                TMap _map8 = iprot.ReadMapBegin();
                for( int _i9 = 0; _i9 < _map8.Count; ++_i9)
                {
                  string _key10;
                  string _val11;
                  _key10 = iprot.ReadString();
                  _val11 = iprot.ReadString();
                  Values[_key10] = _val11;
                }
                iprot.ReadMapEnd();
              }
            } else { 
              TProtocolUtil.Skip(iprot, field.Type);
            }
            break;
          default: 
            TProtocolUtil.Skip(iprot, field.Type);
            break;
        }
        iprot.ReadFieldEnd();
      }
      iprot.ReadStructEnd();
    }

    public void Write(TProtocol oprot) {
      TStruct struc = new TStruct("emDebugInfo");
      oprot.WriteStructBegin(struc);
      TField field = new TField();
      if (StateBlob != null && __isset.stateBlob) {
        field.Name = "stateBlob";
        field.Type = TType.String;
        field.ID = 1;
        oprot.WriteFieldBegin(field);
        oprot.WriteBinary(StateBlob);
        oprot.WriteFieldEnd();
      }
      if (__isset.stateBlobLength) {
        field.Name = "stateBlobLength";
        field.Type = TType.I32;
        field.ID = 2;
        oprot.WriteFieldBegin(field);
        oprot.WriteI32(StateBlobLength);
        oprot.WriteFieldEnd();
      }
      if (Values != null && __isset.values) {
        field.Name = "values";
        field.Type = TType.Map;
        field.ID = 3;
        oprot.WriteFieldBegin(field);
        {
          oprot.WriteMapBegin(new TMap(TType.String, TType.String, Values.Count));
          foreach (string _iter12 in Values.Keys)
          {
            oprot.WriteString(_iter12);
            oprot.WriteString(Values[_iter12]);
          }
          oprot.WriteMapEnd();
        }
        oprot.WriteFieldEnd();
      }
      oprot.WriteFieldStop();
      oprot.WriteStructEnd();
    }

    public override string ToString() {
      StringBuilder sb = new StringBuilder("emDebugInfo(");
      sb.Append("StateBlob: ");
      sb.Append(StateBlob);
      sb.Append(",StateBlobLength: ");
      sb.Append(StateBlobLength);
      sb.Append(",Values: ");
      sb.Append(Values);
      sb.Append(")");
      return sb.ToString();
    }

  }

}
