using emInterfaces;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EMServer
{
    class EMAPIImplementation : emEvolvableMotherboard.Iface
    {
        int emEvolvableMotherboard.Iface.ping()
        {
            Reporting.Say("PING!");
            return 0;
        }

        string emEvolvableMotherboard.Iface.getMotherboardID()
        {
            throw new NotImplementedException();
        }

        string emEvolvableMotherboard.Iface.getMotherboardState()
        {
            throw new NotImplementedException();
        }

        string emEvolvableMotherboard.Iface.getLastError()
        {
            throw new NotImplementedException();
        }

        bool emEvolvableMotherboard.Iface.reset()
        {
            throw new NotImplementedException();
        }

        void emEvolvableMotherboard.Iface.clearSequences()
        {
            throw new NotImplementedException();
        }

        void emEvolvableMotherboard.Iface.runSequences()
        {
            throw new NotImplementedException();
        }

        void emEvolvableMotherboard.Iface.stopSequences()
        {
            throw new NotImplementedException();
        }

    

        void emEvolvableMotherboard.Iface.appendSequenceAction(emSequenceItem Item)
        {
            throw new NotImplementedException();
        }

        emWaveForm emEvolvableMotherboard.Iface.getRecording(int srcPin)
        {
            emWaveForm Test = new emWaveForm();
            Test.Samples = new List<int>();
            for (int i = 0; i < 1000; i++)
                Test.Samples.Add(i);
            Test.Rate = 100;
            Test.SampleCount = Test.Samples.Count;
            return Test;
        }

        void emEvolvableMotherboard.Iface.clearRecording(int srcPin)
        {
            throw new NotImplementedException();
        }

    

        

        public void setLogServer(emLogServerSettings logServer)
        {
            throw new NotImplementedException();
        }


        public void setLED(int index, bool state)
        {
            throw new NotImplementedException();
        }

        public bool reprogramme(byte[] bin, int length)
        {
            throw new NotImplementedException();
        }

        public emDebugInfo getDebugState()
        {
            throw new NotImplementedException();
        }

        public void joinSequences()
        {
            throw new NotImplementedException();
        }

        public int getTemperature()
        {
            throw new NotImplementedException();
        }


        void emEvolvableMotherboard.Iface.setLED(int index, bool state)
        {
            throw new NotImplementedException();
        }

        bool emEvolvableMotherboard.Iface.reprogramme(byte[] bin, int length)
        {
            throw new NotImplementedException();
        }

        emDebugInfo emEvolvableMotherboard.Iface.getDebugState()
        {
            throw new NotImplementedException();
        }

        void emEvolvableMotherboard.Iface.joinSequences()
        {
            throw new NotImplementedException();
        }

        int emEvolvableMotherboard.Iface.getTemperature()
        {
            throw new NotImplementedException();
        }

        void emEvolvableMotherboard.Iface.setLogServer(emLogServerSettings logServer)
        {
            throw new NotImplementedException();
        }
    }
}
