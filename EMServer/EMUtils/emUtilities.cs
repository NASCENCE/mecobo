using emInterfaces;
using EMServer;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using Thrift.Protocol;
using Thrift.Transport;

namespace EMUtils
{
    class emUtilities
    {
        private static emEvolvableMotherboard.Client Motherboard = null;

        public static void Disconnect()
        {
            emUtilities.Motherboard = null;
        }

        public static emEvolvableMotherboard.Client Connect()
        {
            int MaxRetry = 10;
            for (int i = 0; i < MaxRetry; i++)
            {
                Motherboard = _Connect();
                if (Motherboard != null) return Motherboard;
                Reporting.Say("Retrying connection attempt ... " + (i+1) + " of " + MaxRetry);
                Thread.Sleep(10000);
            }

            return Motherboard;
        }

        public static emEvolvableMotherboard.Client _Connect()
        {
            if (Motherboard != null)
                return emUtilities.Motherboard;
           
            try
            {
                TSocket transport = new TSocket(Settings.GetSetting<string>("Server"), Settings.GetSetting<int>("ServerPort"));
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
                return null;
            }
            return Motherboard;
        }

        private static emDataApi.Client DataApi = null;

        public static emDataApi.Client ConnectToDataApi()
        {
            if (DataApi != null)
                return emUtilities.DataApi;

            try
            {
                TSocket transport = new TSocket(Settings.GetSetting<string>("DataApiServer"), Settings.GetSetting<int>("ServerPort"));
                transport.Open();
                TBinaryProtocol protocol = new TBinaryProtocol(transport);
                DataApi = new emDataApi.Client(protocol);
                Stopwatch PingTimer = new Stopwatch(); PingTimer.Start();
              
            }
            catch (Exception err)
            {
                Reporting.Say("Failed to connect to the DataApi. Check the server is running, and then check your conf file");
                Reporting.Say(err.ToString());
                return null;
            }
            return DataApi;
        }

      
    }
}
