using EMServer;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace EMUtils
{
    class SettingsHelper
    {
        public static bool IsMicrosoftCLR()
        {
            return (Type.GetType("Mono.Runtime") == null);
        }

        public static void Initialize()
        {
            Reporting.Say("\tLoading settings from " + Settings.DefaultSettingsFileName);
            //Load settings
            if (File.Exists(Settings.DefaultSettingsFileName))
                Settings.LoadSettings(Settings.DefaultSettingsFileName);
            else
            {
                Settings.DefaultSettings();
                Settings.SaveSettings(Settings.DefaultSettingsFileName);
            }

            if (Settings.GetSetting<bool>("LogClient"))
            {
                Reporting.LogToFile(Settings.GetSetting<string>("LogClientPath"), "EMClient");
                Reporting.Say("\tLogging to file " + Reporting.CurrentLogFileName);
            }

            //List all settings

            foreach (string Key in Settings._Settings.Keys)
                Reporting.Say("\tSETTING\t" + Key + "\t" + Settings._Settings[Key].ToString());

            Reporting.Say("Microsoft CLR? " + (IsMicrosoftCLR() ? "YES" : "NO"));
        }
    }
}
