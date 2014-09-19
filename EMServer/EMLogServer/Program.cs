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
            TSocket transport = new TSocket("localhost", Settings.GetSetting<int>("LogServerPort"));
                transport.Open();
                TBinaryProtocol protocol = new TBinaryProtocol(transport);
                LogServerClient = new emLogServer.Client(protocol);
                

            string LogServerExperimentName = LogServerClient.createUniqueExperimentName("Test");
            LogServerSettings = LogServerClient.getLogServerSettings(LogServerExperimentName);
            LogServerClient.log(LogServerSettings, "Hello world", emLogEventType.MISC);
            LogServerClient.log(LogServerSettings, DateTime.Now.ToLongDateString(), emLogEventType.COMMAND);
            LogServerClient.log(LogServerSettings, DateTime.Now.ToLongTimeString(), emLogEventType.COMMAND);
        }

        static void Main(string[] args)
        {
            Settings.LoadSettings(Settings.DefaultSettingsFileName);

            foreach(string s in args)
                if (s == "test")
                {
                    RunTest();
                    return;
                }

            RunServer();
        }
    }
}
