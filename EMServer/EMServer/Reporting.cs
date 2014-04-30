using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.IO;

namespace EMServer
{
    public class Reporting
    {
        public static Mutex ReportingLock = new Mutex();
        public static void LogToFile(string Root, string BaseFileName)
        {

            if (!Directory.Exists(Root))
                Directory.CreateDirectory(Root);

            int Index = 0;
            string FileName = String.Format(Root+"//"+BaseFileName+"_{0:000000000}.txt", Index);

            while (File.Exists(FileName))
            {
                Index++;
                FileName = String.Format(Root + "//" + BaseFileName + "__{0:000000000}.txt", Index);
            }

            TextFile = new StreamWriter(FileName);
            Reporting.CurrentLogFileName = FileName;
        }

        public static TextWriter TextFile = null;
        public static string CurrentLogFileName = "";

        public static void Say(string Text)
        {
            ReportingLock.WaitOne();


            Console.ForegroundColor = ConsoleColor.Blue;
            Console.Write(DateTime.Now.ToLongTimeString());
            Console.Write('\t');
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine(Text);

            if (TextFile != null)
            {
                TextFile.Write(DateTime.Now.ToLongDateString());
                TextFile.Write('\t');
                TextFile.Write(DateTime.Now.ToLongTimeString());
                TextFile.Write('\t');
                TextFile.WriteLine(Text);
                TextFile.Flush();
            }
            
            ReportingLock.ReleaseMutex();
        }

        static Reporting()
        {
            //Console.WindowWidth = 120;
            //Console.BufferHeight = 3000;
        }
    }
}