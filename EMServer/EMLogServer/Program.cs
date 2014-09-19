using emInterfaces;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Thrift.Server;
using Thrift.Transport;

namespace EMLogServer
{
    class Program
    {
        static void Main(string[] args)
        {
            Settings.LoadSettings(Settings.DefaultSettingsFileName);

            emLogServerImplementation handler = new emLogServerImplementation();
            emLogServer.Processor processor = new emLogServer.Processor(handler);
            TServerTransport serverTransport = new TServerSocket(Settings.GetSetting<int>("LogServerPort"));
            TServer server = new TSimpleServer(processor, serverTransport);

            Reporting.Say("Starting log server on port " + Settings.GetSetting<int>("LogServerPort"));
            server.Serve();
        }
    }
}
