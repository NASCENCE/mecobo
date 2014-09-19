using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;


namespace EMLogServer
{
    public class Settings
    {
        public static Dictionary<string, object> _Settings = new Dictionary<string, object>();
      
        public static T GetSetting<T>(string Name)
        {
            return (T)_Settings[Name];
        }

        public static void SetSetting<T>(string Name, T Value)
        {
            if (_Settings.ContainsKey(Name))
            {
                _Settings[Name] = Value;
            }
            else
            {
                _Settings.Add(Name, Value);
                Console.WriteLine("Adding new setting " + Name + "\t" + Value.ToString());
            }
        }

        public static void DefaultSettings()
        {            
           SetSetting<int>("ServerPort", 9095);            
        }

        public static void SaveSettings(string FileName)
        {
            TextWriter tw = new StreamWriter(FileName);
            foreach (string Key in _Settings.Keys)
            {
                tw.Write(Key);
                tw.Write('\t');
                tw.Write(_Settings[Key]);
                tw.Write('\t');
                tw.Write(_Settings[Key].GetType().ToString());
                tw.WriteLine();

            }
            tw.Close();
        }

        public static string DefaultSettingsFileName = "LogServerSettings.conf";

        public static void LoadSettings(string Filename)
        {
            if (!File.Exists(Filename))
            {
                DefaultSettings();
                Console.WriteLine("Unable to find " + Filename + " using defaults...");
                return;
            }

            DefaultSettingsFileName = Filename;
            string[] Lines = File.ReadAllLines(Filename);
            _Settings = new Dictionary<string, object>();

            foreach (string Line in Lines)
            {
                if (Line.StartsWith("#") || Line.StartsWith("//")) continue;
                string[] Tokens = Line.Split('\t');
                if (Tokens[2] == "System.Int32")
                    _Settings.Add(Tokens[0], int.Parse(Tokens[1]));
                else if (Tokens[2] == "System.Single")
                    _Settings.Add(Tokens[0], float.Parse(Tokens[1]));
                else if (Tokens[2] == "System.Double")
                    _Settings.Add(Tokens[0], double.Parse(Tokens[1]));
                else if (Tokens[2] == "System.String")
                    _Settings.Add(Tokens[0], (Tokens[1]));
                else if (Tokens[2] == "System.Boolean")
                    _Settings.Add(Tokens[0], bool.Parse(Tokens[1]));
                else if (Tokens[2] == "System.Byte")
                    _Settings.Add(Tokens[0], byte.Parse(Tokens[1]));
                else
                    throw new Exception("Unable to parse settings file");
                

            }
        }
    }
}
