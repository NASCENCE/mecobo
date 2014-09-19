using emInterfaces;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace EMUtils
{
    class JSONHelper
    {
        StringBuilder JSB;
        StringWriter SW;
        public JsonWriter JW;

        public JSONHelper()
        {
            JSB = new StringBuilder();
            SW = new StringWriter(JSB);
            JW = new JsonTextWriter(SW);
            JW.Formatting = Formatting.Indented;
            JW.WriteStartObject();
        }

        public string FinishAndGetString()
        {
            JW.WriteEndObject();
            return JSB.ToString();
        }

        public void WritePropertyName(string N)
        {
            JW.WritePropertyName(N);
        }

        public void WriteValue(string V)
        {
            JW.WriteValue(V);
        }
        
        public void WriteKeyValuePair(string Name, string Value)
        {
            this.WritePropertyName(Name);
            this.WriteValue(Value);
        }


        public void WriteStartArray(string p)
        {
            this.WritePropertyName(p);
            this.JW.WriteStartArray();
        }

        public void WriteEndArray()
        {
            this.JW.WriteEndArray();
        }

        public void StartSubObject(string p)
        {
            this.WritePropertyName(p);
            this.JW.WriteStartObject();
        }

        public void EndSubObject()
        {
            this.JW.WriteEndObject();
        }


        public void WriteSub(string N)
        {
            
        }

        public void Write(List<emSequenceItem> SequenceItems)
        {            
            StartSubObject("Seq");
            int i = 0;
            foreach (emSequenceItem Item in SequenceItems)
            {
                StartSubObject("SI" + i);
                i++;
                WriteKeyValuePair("PC", Item.Pin.Count.ToString());
                for(int p=0;p<Item.Pin.Count;p++)
                WriteKeyValuePair("Pin"+p, Item.Pin[p].ToString());
                WriteKeyValuePair("Amp",Item.Amplitude.ToString());
                WriteKeyValuePair("CT",Item.CycleTime.ToString());
                WriteKeyValuePair("End",Item.EndTime.ToString());
                WriteKeyValuePair("Str",Item.StartTime.ToString());
                WriteKeyValuePair("Fre",Item.Frequency.ToString());
                WriteKeyValuePair("Opt",Item.OperationType.ToString());
                WriteKeyValuePair("Pha",Item.Phase.ToString());
                WriteKeyValuePair("VSR",Item.ValueSourceRegister.ToString());
                WriteKeyValuePair("WFT",Item.WaitForTrigger.ToString());                
                WriteKeyValuePair("Wav",Item.WaveFormType.ToString());
                EndSubObject();
            }
            EndSubObject();
        }
    }
}
