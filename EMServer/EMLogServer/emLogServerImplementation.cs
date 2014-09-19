using emInterfaces;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace EMLogServer
{
    class emLogServerImplementation : emLogServer.Iface
    {

        string emLogServer.Iface.createUniqueExperimentName(string baseName)
        {
            return String.Format("{0}_{1:0000}{2:00}{3:00}_{4:00}{5:00}{6:00}",
                baseName,
                DateTime.Now.Year,
                DateTime.Now.Month,
                DateTime.Now.Day,
                DateTime.Now.Hour,
                DateTime.Now.Minute,
                DateTime.Now.Second);
        }

        emLogServerSettings emLogServer.Iface.getLogServerSettings(string uniqueExperimentName)
        {
            emLogServerSettings S = new emLogServerSettings();
            S.LogServer = Dns.GetHostName();
            S.ExperimentName = uniqueExperimentName;
            return S;
        }

        void emLogServer.Iface.log(emLogServerSettings logServer, string message, emLogEventType messageType)
        {
            string OutputFolder = "./LogServer/" + logServer.ExperimentName;
            if (!Directory.Exists(OutputFolder))
                Directory.CreateDirectory(OutputFolder);
            string OutputFile = String.Format("{1:0000}{2:00}{3:00}_{4:00}{5:00}{6:00}{7}_{8}.txt",
                "",
                DateTime.Now.Year,
                DateTime.Now.Month,
                DateTime.Now.Day,
                DateTime.Now.Hour,
                DateTime.Now.Minute,
                DateTime.Now.Second,
                DateTime.Now.Millisecond,
                messageType.ToString());
            File.WriteAllText(OutputFolder+"/"+OutputFile, message);
            Console.WriteLine(OutputFile + "\t" + message);
        }
    }
}
