using EMServer;
using System;
using System.Collections.Generic;
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

            MainForm MF = new MainForm();
            MF.ShowDialog();
          
        }
    }
}
