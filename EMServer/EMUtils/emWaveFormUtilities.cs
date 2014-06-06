using emInterfaces;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace EMUtils
{
    public class emWaveFormUtilities
    {
        public static void SaveWaveForm(emWaveForm WaveForm, string FileName)
        {
            TextWriter tw = new StreamWriter(FileName);
            tw.WriteLine("LENGTH\t" + WaveForm.SampleCount);
            tw.WriteLine("RATE\t" + WaveForm.Rate);
            for (int i = 0; i < WaveForm.SampleCount; i++)
            {
                tw.WriteLine(WaveForm.Samples[i]);               
            }
            tw.Close();
        }

        public static emWaveForm LoadWaveForm(string FileName)
        {
            string[] Lines = File.ReadAllLines(FileName);
            emWaveForm W = new emWaveForm();
            W.Samples = new List<int>();
            foreach (string Line in Lines)
            {
                if (Line.StartsWith("LENGTH"))
                    W.SampleCount = int.Parse(Line.Split('\t')[1]);
                else if (Line.StartsWith("RATE"))
                    W.Rate = int.Parse(Line.Split('\t')[1]);
                else
                    W.Samples.Add(int.Parse(Line));
            }
            return W;
        }

        public static void stats(emWaveForm W, out double Frequency, out double DutyCycle)
        {
            List<int> DownUpTransitions = new List<int>();
            ulong LowCount = 0;
            for (int i = 0; i < W.SampleCount-1; i++)
            {
                if (W.Samples[i] == 0 && W.Samples[i + 1] > 0)
                    DownUpTransitions.Add(i);
                if (W.Samples[i] <= 0)
                    LowCount++;
            }

            double WaveTime = (double)W.SampleCount / W.Rate;
            Frequency = DownUpTransitions.Count / WaveTime;

            DutyCycle = (double)LowCount / W.SampleCount;
        }

        internal static emWaveForm absDifference(emWaveForm emWaveForm1, emWaveForm emWaveForm2)
        {
            if (emWaveForm1.SampleCount != emWaveForm2.SampleCount)
            {
                emException Err = new emException();
                Err.ExceptionType = emExceptionType.CRITICAL;
                Err.Reason = "absDifference: Buffers neeed to be the same length";
                throw Err;
            }

            if (emWaveForm1.Rate != emWaveForm2.Rate)
            {
                emException Err = new emException();
                Err.ExceptionType = emExceptionType.CRITICAL;
                Err.Reason = "absDifference: Buffers neeed to be the same rate";
                throw Err;
            }
            
            emWaveForm R = new emWaveForm();
            R.Rate = emWaveForm1.Rate;
            R.SampleCount = emWaveForm1.SampleCount;
            R.Samples = new List<int>();

            for (int i = 0; i < emWaveForm1.SampleCount; i++)
                R.Samples.Add(Math.Abs(emWaveForm1.Samples[i] - emWaveForm2.Samples[1]));


            return R;
        }

        internal static double sum(emWaveForm emWaveForm)
        {
            double Acc = 0;
            for (int i = 0; i < emWaveForm.SampleCount; i++) Acc += emWaveForm.Samples[i];
            return Acc;
        }

        internal static double sumSquaredDifference(emWaveForm emWaveForm1, emWaveForm emWaveForm2)
        {
            if (emWaveForm1.SampleCount != emWaveForm2.SampleCount)
            {
                emException Err = new emException();
                Err.ExceptionType = emExceptionType.CRITICAL;
                Err.Reason = "sumSquaredDifference: Buffers neeed to be the same length";
                throw Err;
            }

            if (emWaveForm1.Rate != emWaveForm2.Rate)
            {
                emException Err = new emException();
                Err.ExceptionType = emExceptionType.CRITICAL;
                Err.Reason = "sumSquaredDifference: Buffers neeed to be the same rate";
                throw Err;
            }

            double Acc = 0;

            for (int i = 0; i < emWaveForm1.SampleCount; i++)
                Acc+=(Math.Pow(emWaveForm1.Samples[i] - emWaveForm2.Samples[1],2));

            return Acc;
        }

        internal static emWaveForm add(emWaveForm emWaveForm1, emWaveForm emWaveForm2)
        {
            if (emWaveForm1.SampleCount != emWaveForm2.SampleCount)
            {
                emException Err = new emException();
                Err.ExceptionType = emExceptionType.CRITICAL;
                Err.Reason = "add: Buffers neeed to be the same length";
                throw Err;
            }

            if (emWaveForm1.Rate != emWaveForm2.Rate)
            {
                emException Err = new emException();
                Err.ExceptionType = emExceptionType.CRITICAL;
                Err.Reason = "add: Buffers neeed to be the same rate";
                throw Err;
            }

            emWaveForm R = new emWaveForm();
            R.Rate = emWaveForm1.Rate;
            R.SampleCount = emWaveForm1.SampleCount;
            R.Samples = new List<int>();

            for (int i = 0; i < emWaveForm1.SampleCount; i++)
                R.Samples.Add(emWaveForm1.Samples[i] + emWaveForm2.Samples[1]);

            return R;
        }

        internal static emWaveForm subtract(emWaveForm emWaveForm1, emWaveForm emWaveForm2)
        {
            if (emWaveForm1.SampleCount != emWaveForm2.SampleCount)
            {
                emException Err = new emException();
                Err.ExceptionType = emExceptionType.CRITICAL;
                Err.Reason = "subtract: Buffers neeed to be the same length";
                throw Err;
            }

            if (emWaveForm1.Rate != emWaveForm2.Rate)
            {
                emException Err = new emException();
                Err.ExceptionType = emExceptionType.CRITICAL;
                Err.Reason = "subtract: Buffers neeed to be the same rate";
                throw Err;
            }

            emWaveForm R = new emWaveForm();
            R.Rate = emWaveForm1.Rate;
            R.SampleCount = emWaveForm1.SampleCount;
            R.Samples = new List<int>();

            for (int i = 0; i < emWaveForm1.SampleCount; i++)
                R.Samples.Add(emWaveForm1.Samples[i] - emWaveForm2.Samples[1]);

            return R;
        }

        internal static void setValues(emWaveForm emWaveForm, int value)
        {
            for (int i = 0; i < emWaveForm.SampleCount; i++)
            {
                emWaveForm.Samples[i] = value;
            }
        }

        internal static emWaveForm threshold(emWaveForm emWaveForm, int thresholdValue, int maxValue, bool invert)
        {
            emWaveForm R = new emWaveForm();
            R.Rate = emWaveForm.Rate;
            R.SampleCount = emWaveForm.SampleCount;
            R.Samples = new List<int>();

            for (int i = 0; i < emWaveForm.SampleCount; i++)
            {
                if (emWaveForm.Samples[i] < thresholdValue)
                {
                    if (invert) R.Samples.Add(maxValue); else R.Samples.Add(0);
                }
                else
                {
                   if (invert) R.Samples.Add(0); else  R.Samples.Add(maxValue); 
                }
            }

            return R;
        }

        internal static void minMax(emWaveForm emWaveForm, out int minValue, out int maxValue)
        {
            minValue = emWaveForm.Samples.Min();
            maxValue = emWaveForm.Samples.Max();
        }


        internal static emWaveForm normalize(emWaveForm emWaveForm, int minValue, int maxValue)
        {
            int emWaveFormMin; int emWaveFormMax;
            minMax(emWaveForm, out emWaveFormMin, out emWaveFormMax);

            emWaveForm R = new emWaveForm();
            R.SampleCount = emWaveForm.SampleCount;
            R.Rate = emWaveForm.Rate;
            R.Samples = new List<int>();
            
            double r = emWaveFormMax - emWaveFormMin;
            double r2 = maxValue - minValue;

            for (int i = 0; i < emWaveForm.SampleCount; i++)
            {
                double p = (double)(emWaveForm.Samples[i] - emWaveFormMin) / r;
                double v = (p * r2) + minValue;
                R.Samples.Add((int)v);
            }

            return R;
        }

        internal static emWaveForm quantize(emWaveForm emWaveForm, int minValue, int maxValue, int levels)
        {
            throw new NotImplementedException();
        }

        internal static emWaveForm resample(emWaveForm emWaveForm, int newLength)
        {
            throw new NotImplementedException();
        }

        internal static emWaveForm medianFilter(emWaveForm emWaveForm, int filterSize)
        {
            emWaveForm R = new emWaveForm();
            R.SampleCount = emWaveForm.SampleCount;
            R.Rate = emWaveForm.Rate;
            R.Samples = new List<int>();

            int WindowSize = filterSize / 2;
            

            for (int i = 0; i < emWaveForm.SampleCount; i++)
            {
                List<int> Window = new List<int>();

                for (int j = i - WindowSize; j <= i + WindowSize; j++)
                {
                    if (j >= 0 && j < emWaveForm.SampleCount)
                        Window.Add(emWaveForm.Samples[j]);
                }
                Window.Sort();
                emWaveForm.Samples.Add(Window[Window.Count / 2]);
            }

            return R;
        }
    }

    public class emDataApiImp : emDataApi.Iface
    {
        public static Dictionary<string, emWaveForm> Buffers = new Dictionary<string, emWaveForm>();

        private static long Index = 0;

        public string createBuffer(long length)
        {
            emWaveForm w = new emWaveForm();
            w.Samples = new List<int>((int)length);
            w.SampleCount = (int)length;
            string BufferName = String.Format("{0:0000000000000000000}", Index++);
            Buffers.Add(BufferName, w);
            return BufferName;
        }

        public void destroyBuffer(string bufferName)
        {
            if (Buffers.ContainsKey(bufferName)) Buffers.Remove(bufferName);
        }

        public emWaveForm getBuffer(string bufferName)
        {
            if (Buffers.ContainsKey(bufferName))
                return Buffers[bufferName];
            emException Err = new emException();
            Err.ExceptionType = emExceptionType.CRITICAL;
            Err.Reason = "Buffer " + bufferName + " not found. Did you delete it? Did you use this API to create it?";            
            throw Err;
        }

        public string setBuffer(emWaveForm samples)
        {
            string BufferName = String.Format("{0:0000000000000000000}", Index++);
            Buffers.Add(BufferName,samples);
            return BufferName;
        }

        public string cloneBuffer(string bufferName)
        {
            emWaveForm Src = getBuffer(bufferName);
            emWaveForm Dest = new emWaveForm();
            Dest.SampleCount = Src.SampleCount;
            Dest.Rate = Src.Rate;
            Dest.Samples = new List<int>();
            if (Src.Samples!=null)
                Dest.Samples.AddRange(Src.Samples);
            return setBuffer(Dest);
        }

        public void renameBuffer(string oldBufferName, string newBufferName)
        {
            emWaveForm Src = getBuffer(oldBufferName);
            Buffers.Add(newBufferName, Src);
        }

        public void saveBuffer(string bufferName, string fileName)
        {
            emWaveFormUtilities.SaveWaveForm(getBuffer(bufferName), fileName);
        }

        public string loadBuffer(string fileName)
        {
            return setBuffer(emWaveFormUtilities.LoadWaveForm(fileName));
        }

        public string absDifference(string bufferA, string bufferB)
        {
            return setBuffer(emWaveFormUtilities.absDifference(getBuffer(bufferA), getBuffer(bufferB)));
        }

        public double sum(string bufferName)
        {
            return emWaveFormUtilities.sum(getBuffer(bufferName));
        }

        public double sumSquaredDifference(string bufferA, string bufferB)
        {
            return emWaveFormUtilities.sumSquaredDifference(getBuffer(bufferA), getBuffer(bufferB));
        }

        public string subtract(string bufferA, string bufferB)
        {
            return setBuffer(emWaveFormUtilities.subtract(getBuffer(bufferA), getBuffer(bufferB)));
        }

        public string add(string bufferA, string bufferB)
        {
            return setBuffer(emWaveFormUtilities.add(getBuffer(bufferA), getBuffer(bufferB)));
        }

        public void setValues(string bufferName, int value)
        {
            emWaveFormUtilities.setValues(getBuffer(bufferName), value);
        }

        public string threshold(string bufferName, int thresholdValue, int maxValue, bool invert)
        {
            return setBuffer(emWaveFormUtilities.threshold(getBuffer(bufferName), thresholdValue, maxValue, invert));
        }

        public string normalize(string bufferName, int minValue, int maxValue)
        {
            return setBuffer(emWaveFormUtilities.normalize(getBuffer(bufferName), minValue, maxValue));
        }

        public string quantize(string bufferName, int minValue, int maxValue, int levels)
        {
            return setBuffer(emWaveFormUtilities.quantize(getBuffer(bufferName), minValue, maxValue, levels));
        }

        public string resample(string bufferName, int newLength)
        {
            return setBuffer(emWaveFormUtilities.resample(getBuffer(bufferName), newLength));
        }

        public string medianFilter(string bufferName, int fitlerSize)
        {
            return setBuffer(emWaveFormUtilities.medianFilter(getBuffer(bufferName), fitlerSize));
        }
    }
}
