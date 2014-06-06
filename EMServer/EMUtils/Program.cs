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

            ///PongForm PF = new PongForm();
            //PF.ShowDialog();
            //EMUtils.Experiment_Pong.Pong.Go();
            EMUtils.Experiment_Gates2.Go();
            //MainForm MF = new MainForm();
            //MF.ShowDialog();
          
        }
    }
}
