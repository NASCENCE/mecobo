using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using EMServer;
using Thrift.Protocol;
using Thrift.Transport;
using System.Threading;
using System.Diagnostics;
using emInterfaces;

namespace EMClient
{
    class Program
    {
        public static bool IsMicrosoftCLR()
        {
            return (Type.GetType("Mono.Runtime") == null);
        }

        public static void Initialize()
        {
            Reporting.Say("\tLoading settings from " + Settings.DefaultSettingsFileName);
            //Load settings
            if (File.Exists(Settings.DefaultSettingsFileName))
                Settings.LoadSettings(Settings.DefaultSettingsFileName);
            else
            {
                Settings.DefaultSettings();
                Settings.SaveSettings(Settings.DefaultSettingsFileName);
            }

            if (Settings.GetSetting<bool>("LogClient"))
            {
                Reporting.LogToFile(Settings.GetSetting<string>("LogClientPath"), "EMClient");
                Reporting.Say("\tLogging to file " + Reporting.CurrentLogFileName);
            }

            //List all settings

            foreach (string Key in Settings._Settings.Keys)
                Reporting.Say("\tSETTING\t" + Key + "\t" + Settings._Settings[Key].ToString());

            Reporting.Say("Microsoft CLR? " + (IsMicrosoftCLR() ? "YES" : "NO"));
        }

        static void Main(string[] args)
        {
            Reporting.Say("Simple test client");

            try
            {
                Initialize();
            }
            catch (Exception err)
            {
                Reporting.Say("Failed to init the software. Check the conf file");
                Reporting.Say(err.ToString());
                return;
            }

            emEvolvableMotherboard.Client Motherboard = null;

            try
            {
                TSocket transport = new TSocket("dmlab02.idi.ntnu.no", Settings.GetSetting<int>("ServerPort"), 10000);
                
                transport.Open();
                TBinaryProtocol protocol = new TBinaryProtocol(transport);
                Motherboard = new emEvolvableMotherboard.Client(protocol);
                Stopwatch PingTimer = new Stopwatch(); PingTimer.Start();
                Reporting.Say("Ping response = " + Motherboard.ping());
                PingTimer.Stop();
                Reporting.Say("Ping response took " + PingTimer.ElapsedMilliseconds + "ms or " + PingTimer.ElapsedTicks + " ticks");
            }
            catch (Exception err)
            {
                Reporting.Say("Failed to connect to the motherboard. Check the server is running, and then check your conf file");
                Reporting.Say(err.ToString());
                return;
            }

            Stopwatch ConfigTimer = new Stopwatch();
            ConfigTimer.Start();
            //Schedule tones
            int ToneCount = 6;
            int ToneLength = 100;
            int Counter = 0;
            for (int p = 1; p < 10; p++)
            {
                for (int i = 0; i < ToneCount; i++)
                {
                    emSequenceItem Item = new emSequenceItem();
                    Item.OperationType = emSequenceOperationType.PREDEFINED;
                    Item.WaveFormType = emWaveFormType.PWM;

                    Item.StartTime = i * ToneLength;
                    Item.EndTime = Item.StartTime + ToneLength;
                    Item.Pin = new List<int>(); Item.Pin.Add( p);
                    Item.Phase = 0;
                    Item.Amplitude = int.MaxValue;
                    Item.Frequency = i % 2 == 0 ? 1000 *(p + 1) : 100 *(p + 1);
                    Item.CycleTime = 50;
                    Motherboard.appendSequenceAction(Item);
                    Counter++;
                    
                }
            }

            emSequenceItem ActionRecord = new emSequenceItem();
            ActionRecord.Frequency = 10000;
            ActionRecord.Pin = new List<int>(); ActionRecord.Pin.Add(0);
            ActionRecord.StartTime = 0;
            ActionRecord.EndTime = (ToneCount * ToneLength);
            ActionRecord.OperationType = emSequenceOperationType.RECORD;
            Motherboard.appendSequenceAction(ActionRecord);
            Counter++;

            ConfigTimer.Stop();
            Reporting.Say("Configuring took " + ConfigTimer.ElapsedMilliseconds + " ms for " + Counter+" items");

      
            Reporting.Say("Running sequences....");
            Stopwatch SeqTimer = new Stopwatch(); SeqTimer.Start();
            Motherboard.runSequences();
            SeqTimer.Stop();
            Reporting.Say("Seqeunces ended! And took " + SeqTimer.ElapsedMilliseconds+" ms to run");
            Reporting.Say("Ping response = " + Motherboard.ping());
   

            //em.joinSequence();

            emWaveForm WaveForm = Motherboard.getRecording(0);
            Console.WriteLine("WaveForm contains " + WaveForm.SampleCount + " samples");
            emUtilities.SaveWaveForm(WaveForm, "waveform1.csv");
        }



    }
}
