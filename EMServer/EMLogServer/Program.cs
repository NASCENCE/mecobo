using emInterfaces;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Thrift.Protocol;
using Thrift.Server;
using Thrift.Transport;

namespace EMLogServer
{
    class Program
    {
        public static void RunServer()
        {
            emLogServerImplementation handler = new emLogServerImplementation();
            emLogServer.Processor processor = new emLogServer.Processor(handler);
            TServerTransport serverTransport = new TServerSocket(Settings.GetSetting<int>("LogServerPort"));
            TServer server = new TSimpleServer(processor, serverTransport);

            Reporting.Say("Starting log server on port " + Settings.GetSetting<int>("LogServerPort"));
            server.Serve();
        }

        public static void RunTest()
        {
            emLogServer.Client LogServerClient = null;
            emLogServerSettings LogServerSettings = null;
            TSocket transport = new TSocket("195.176.191.61", 9092);
                transport.Open();
                TBinaryProtocol protocol = new TBinaryProtocol(transport);
                LogServerClient = new emLogServer.Client(protocol);
                

            string LogServerExperimentName = LogServerClient.createUniqueExperimentName("Test");
            LogServerSettings = LogServerClient.getLogServerSettings(LogServerExperimentName);
            for (int i = 0; i < 16; i++)
            {
                LogServerClient.log(LogServerSettings, "Hello world", emLogEventType.MISC);
                LogServerClient.log(LogServerSettings, DateTime.Now.ToLongDateString(), emLogEventType.COMMAND);
                LogServerClient.log(LogServerSettings, DateTime.Now.ToLongTimeString(), emLogEventType.COMMAND);
                Console.Write(i.ToString() + " ");
            }

            
        }

        static void Main(string[] args)
        {
            Settings.LoadSettings(Settings.DefaultSettingsFileName);


            foreach(string s in args)
                if (s == "test")
                {
                    for(int i=0;i<256;i++)
                    RunTest();
                    return;
                }

            RunServer();
        }
    }
}
