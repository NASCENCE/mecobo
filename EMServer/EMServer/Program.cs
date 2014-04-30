using emInterfaces;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Thrift;
using Thrift.Server;
using Thrift.Transport;
namespace EMServer
{
    class Program
    {
        public static void Initialize()
        {
            Reporting.Say("\tLoading settings from " + Settings.DefaultSettingsFileName);
            //Load settings
            if (File.Exists(Settings.DefaultSettingsFileName))
                Settings.LoadSettings(Settings.DefaultSettingsFileName);
            else
                Settings.DefaultSettings();

            if (Settings.GetSetting<bool>("LogServer"))
            {
                Reporting.LogToFile(Settings.GetSetting<string>("LogServerPath"), "EMServer");
                Reporting.Say("\tLogging to file " + Reporting.CurrentLogFileName);
            }

            //List all settings

            foreach (string Key in Settings._Settings.Keys)
                Reporting.Say("\tSETTING\t" + Key + "\t" + Settings._Settings[Key].ToString());

            
        }

        public static void CreateEMServer()
        {
            
            EMAPIImplementation handler = new EMAPIImplementation();
            emEvolvableMotherboard.Processor processor = new emEvolvableMotherboard.Processor(handler);
            TServerTransport serverTransport = new TServerSocket(Settings.GetSetting<int>("ServerPort"));
            TServer server = new TSimpleServer(processor, serverTransport);

            Reporting.Say("Starting server on port " + Settings.GetSetting<int>("ServerPort"));
            server.Serve();
        }

        public static void CreateDataServer()
        {

            emDataApiImp handler = new emDataApiImp();
            emInterfaces.emDataApi.Processor processor = new emDataApi.Processor(handler);
            TServerTransport serverTransport = new TServerSocket(Settings.GetSetting<int>("DataAPIServerPort"));
            TServer server = new TSimpleServer(processor, serverTransport);

            Reporting.Say("Starting server on port " + Settings.GetSetting<int>("DataAPIServerPort"));
            server.Serve();
        }

        static void Main(string[] args)
        {
            Reporting.Say("EM Server startup");
            Initialize();
            ///CreateEMServer();
            CreateDataServer();
        }
    }
}
