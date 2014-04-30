using emInterfaces;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace EMClient
{
    class emUtilities
    {
        public static void SaveWaveForm(emWaveForm WaveForm, string FileName)
        {
            TextWriter tw = new StreamWriter(FileName);
            for (int i = 0; i < WaveForm.SampleCount; i++)
            {
                tw.WriteLine(i + "," + WaveForm.Samples[i]);
                Console.WriteLine("\t" + i + "," + WaveForm.Samples[i]);
            }
            tw.Close();
        }
    }
}
