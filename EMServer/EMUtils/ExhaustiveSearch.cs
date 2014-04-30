using emInterfaces;
using EMServer;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EMUtils
{
    class ExhaustiveSearch
    {
        public static void Shuffle(List<int> L)
        {
            Random RNG = new Random(12245);

            for (int i = 0; i < L.Count; i++)
            {
                int Temp = L[i];
                L.RemoveAt(i);
                L.Insert(RNG.Next(0, L.Count), Temp);
            }
        }

        public static void Go()
        {
            bool Send = true;
            Reporting.LogToFile("./", "exhaustiveSearch");
            emEvolvableMotherboard.Client Motherboard = null;

            if (Send)
            {
                Motherboard = emUtilities.Connect();
                Motherboard.ping();
            }
            int MaxPin = 11;
            int MaxConfigs = (int)Math.Pow(2, MaxPin+1);
            int RunTime = 128;
            List<int> Configs = new List<int>();
            for (int Config = 0; Config < MaxConfigs; Config++)
                Configs.Add(Config);
            Shuffle(Configs);

            List<int> OutputPins = new List<int>();
            for (int pin = 0; pin <= MaxPin; pin++)
                OutputPins.Add(pin);
        //    OutputPins.Clear(); OutputPins.Add(2);
            Shuffle(OutputPins);

            int Counter = 0;

            foreach (int outputPin in OutputPins)
            {
                foreach (int Config in Configs)
                {
                    Counter++;
                    if (Send)
                    {
                        Motherboard.clearSequences();
                         Motherboard.reset();
                    }


                    string OutputString = "";
                    OutputString += String.Format("{0:000.00}% ", ((double)Counter/(Configs.Count * OutputPins.Count)) *100);
                    OutputString += String.Format("{0,-6} ", Config);
                    OutputString += String.Format("{0,-3} ", outputPin);

                    for(int parse=0;parse<2;parse++)
                    for (int pin = 0; pin <= MaxPin; pin++)
                    {
                        emSequenceItem Item = new emSequenceItem();
                        Item.Pin = new List<int>(); Item.Pin.Add(pin);
                        Item.StartTime = 0;
                        Item.EndTime = RunTime;
                        Item.OperationType = emSequenceOperationType.CONSTANT;
                        Item.WaveFormType = emWaveFormType.PWM;
                        Item.Frequency = 5000;
                        Item.Phase = 0;
                        Item.CycleTime = 100;

                        if (pin == outputPin  && parse==0)
                        {
                            OutputString += ("*");
                        }
                        else if ((Config & (1 << pin)) > 0 && parse == 0)
                        {
                            Item.Amplitude = 1;
                            OutputString += ("1");
                        }
                        else if  (parse==0)
                        {
                            Item.Amplitude = 0;
                            OutputString += ("0");
                        }

                        if (pin != outputPin && parse == 0)
                        {
                            if (Send)
                            {
                                Motherboard.appendSequenceAction(Item);
                            }
                        }

                        if (pin == outputPin && parse==1)
                        {
                            Item = new emSequenceItem();
                            Item.Pin = new List<int>(); Item.Pin.Add(pin) ;
                            Item.StartTime = 0;
                            Item.EndTime = RunTime;
                            Item.Frequency = 5000;
                            Item.OperationType = emSequenceOperationType.RECORD;
                            Item.Phase = 0;
                            Item.CycleTime = 50;
                            if (Send)
                            {
                                Motherboard.appendSequenceAction(Item);
                            }
                        }


                       
                    }

                    OutputString += " ";
                    emWaveForm RecordedSignal = null;
                    if (Send)
                    {
                        Motherboard.runSequences();
                        Motherboard.joinSequences();
                        RecordedSignal = Motherboard.getRecording(outputPin);
                        if (RecordedSignal == null) throw new Exception("Failed to get recorded signal");
                        OutputString += String.Format("{0,-6} ", RecordedSignal.SampleCount);
                     
                        int OneCount = 0;
                        for (int i = 0; i < RecordedSignal.Samples.Count; i++)
                            if (RecordedSignal.Samples[i] > 0) OneCount++;
                        OutputString += String.Format("{0,-6} ", OneCount);


                        OneCount = 0;
                        for (int i = RecordedSignal.Samples.Count / 4; i < 3 *( RecordedSignal.Samples.Count / 4); i++)
                            if (RecordedSignal.Samples[i] > 0) OneCount++;
                        OutputString += String.Format("{0,-6} ", OneCount);
                        OutputString += String.Format("{0,-6} ", ((OneCount > RecordedSignal.Samples.Count / 4) ? "1" : "0"));
                    }

                    Reporting.Say(OutputString);
                }
            }

        }
    }
}
