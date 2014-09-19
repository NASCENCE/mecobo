using EMServer;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EMUtils
{
    
    class Program
    {
        [STAThread] 
        static void Main(string[] args)
        {
            try
            {
                SettingsHelper.Initialize();
            }
            catch (Exception err)
            {
                Reporting.Say("Failed to init the software. Check the conf file");
                Reporting.Say(err.ToString());
                return;
            }

            PongForm PF = new PongForm();
            PF.ShowDialog();
            EMUtils.Experiment_Pong.Pong.Go();

            JSONHelper J = new JSONHelper();
            J.WriteKeyValuePair("hello", "world");
            J.WriteStartArray("array1");
            for(int i=0;i<5;i++)
                J.WriteValue(i.ToString());
            J.WriteEndArray();
            J.JW.WritePropertyName("b");
            J.JW.WriteStartObject();
            J.WriteKeyValuePair("test", "me");
            J.JW.WriteEndObject();
            J.WriteStartArray("array2");
            for (int i = 0; i < 5; i++)
                J.WriteValue(i.ToString());
            J.WriteEndArray();
            Console.WriteLine(J.FinishAndGetString());
            
            EMUtils.Experiment_Gates.Go();

        
            //MainForm MF = new MainForm();
            //MF.ShowDialog();
          
            }
    }
}
