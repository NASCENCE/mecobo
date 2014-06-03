#define noGUI
#define WITHEM
using CGPIP2;
using emInterfaces;
using EMServer;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
#if GUI
using System.Windows.Forms;
#endif
using System.Xml.Serialization;
using Thrift.Protocol;
using Thrift.Transport;

namespace EMUtils
{
    public class Experiment_Pong
    {
        //Define a common random number generator
        //You can set a fixed random seed in the constructor
        public class RandomSource
        {
            public static Random RNG = new Random();

            public static emWaveFormType RandomWaveFormType()
            {
                emWaveFormType[] AllowedTypes = new[] { emWaveFormType.ARBITRARY, emWaveFormType.PWM, emWaveFormType.SAW, emWaveFormType.SINE };
                return AllowedTypes[RNG.Next(0, AllowedTypes.Length)];
            }

            public static emSequenceOperationType RandomSequenceOperationType()
            {
                emSequenceOperationType[] AllowedTypes = new[] { emSequenceOperationType.CONSTANT};// emSequenceOperationType.ARBITRARY, emSequenceOperationType.CONSTANT, emSequenceOperationType.CONSTANT_FROM_REGISTER, emSequenceOperationType.DIGITAL, emSequenceOperationType.PREDEFINED };
                return AllowedTypes[RNG.Next(0, AllowedTypes.Length)];
            }
        }

        public class Individual : IComparable
        {
            public static void Save(Individual Ind, string FileName)
            {
                Ind.Save(FileName);
            }

            public Individual() { }

            public double Fitness;  // The fitness measure
            public ulong EvaluationIndex = 0;  //Counter for when it was evauated

            public int ListenPin = 0;
            public int PaddlePin = 1;
            public int BallPin = 2;
            public List<emSequenceItem> Genotype = null;
            public double Threshold;

            public void Save(string FileName)
            {
                TextWriter TW = new StreamWriter(FileName);
                TW.WriteLine(Fitness);
                TW.WriteLine(EvaluationIndex);
                TW.WriteLine(ListenPin);
                TW.WriteLine(BallPin);
                TW.WriteLine(Threshold);
                TW.WriteLine(Genotype.Count);
                for (int i = 0; i < Genotype.Count; i++)
                {
                    var Stream = new FileStream(String.Format("{0}_{1:000}", FileName, i),FileMode.Create);
                    TProtocol tProtocol = new TBinaryProtocol(new TStreamTransport(Stream, Stream));
                    this.Genotype[i].Write(tProtocol);
                    Stream.Close();
                }
                TW.Close();
            }

            public static Individual Load(string FileName)
            {
                try
                {
                    Reporting.Say("Loading " + FileName);
                    string[] Lines = File.ReadAllLines(FileName);
                    Individual Ind = new Individual();
                    Ind.Fitness = double.Parse(Lines[0]);
                    Ind.EvaluationIndex = ulong.Parse(Lines[1]);
                    Ind.ListenPin = int.Parse(Lines[2]);
                    Ind.BallPin = int.Parse(Lines[3]);
                    Ind.Threshold = double.Parse(Lines[4]);
                    int GenotypeCount = int.Parse(Lines[5]);
                    Ind.Genotype = new List<emSequenceItem>();
                    for (int i = 0; i < GenotypeCount; i++)
                    {
                        Reporting.Say("\tLoading " + String.Format("{0}_{1:000}", FileName, i));
                        var Stream = new FileStream(String.Format("{0}_{1:000}", FileName, i), FileMode.Open);
                        TProtocol tProtocol = new TBinaryProtocol(new TStreamTransport(Stream, Stream));
                        emSequenceItem I = new emSequenceItem();
                        I.Read(tProtocol);
                        Ind.Genotype.Add(I);
                    }
                    Reporting.Say("Finished loading");

                    return Ind;
                }
                catch (Exception e)
                {
                    return null;
                }
            }

            public Individual Clone()
            {
                Individual NewInd = new Individual();
                NewInd.BallPin = this.BallPin;
                NewInd.EvaluationIndex = this.EvaluationIndex;
                NewInd.Fitness = this.Fitness;
                NewInd.Genotype = new List<emSequenceItem>();
                foreach (emSequenceItem I in this.Genotype)
                {
                    emSequenceItem C = new emSequenceItem();
                    C.Amplitude = I.Amplitude;
                    C.CycleTime = I.CycleTime;
                    C.EndTime = I.EndTime;
                    C.Frequency = I.Frequency;
                    C.OperationType = I.OperationType;
                    C.Phase = I.Phase;
                    C.Pin = new List<int>();
                    C.Pin.AddRange(I.Pin);
                    C.StartTime = I.StartTime;
                    C.ValueSourceRegister = I.ValueSourceRegister;
                    C.WaitForTrigger = I.WaitForTrigger;
                    C.WaveForm = I.WaveForm;
                    C.WaveFormType = I.WaveFormType;
                    NewInd.Genotype.Add(C);
                }
                NewInd.ListenPin = this.ListenPin;
                NewInd.PaddlePin = this.PaddlePin;

                return NewInd;
            }

            //define the comparison.  // Bigger fitness is better
            public int CompareTo(object obj)
            {
                Individual Other = (Individual)obj;
                if (Other.Fitness < this.Fitness)
                    return -1;
                else if (Other.Fitness > this.Fitness)
                    return 1;
                /*
                if (Other.EvaluationIndex < this.EvaluationIndex)
                    return -1;
                if (Other.EvaluationIndex > this.EvaluationIndex)
                    return 1;*/
                return 0;
            }

            public void Mutate(double MutationRate)
            {
                Individual Ind = this; ///code borrowed from the factory class....
                Ind.Fitness = double.NaN;

                if (RandomSource.RNG.NextDouble() < MutationRate)
                {
                    Ind.ListenPin = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                }

                if (RandomSource.RNG.NextDouble() < MutationRate)
                {
                    do
                    {
                        Ind.BallPin = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                    } while (Ind.BallPin == Ind.ListenPin);
                }

                if (RandomSource.RNG.NextDouble() < MutationRate)
                {
                    do
                    {
                        Ind.PaddlePin = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                    } while (Ind.PaddlePin == Ind.ListenPin || Ind.PaddlePin == Ind.BallPin);
                }



                for (int g = 0; g < Ind.Genotype.Count; g++)
                {
                    emSequenceItem SI = Ind.Genotype[g];


                    for (int p = 1; p < SI.Pin.Count; p++)
                    {
                        if (RandomSource.RNG.NextDouble() < MutationRate)
                        {
                            SI.Pin.RemoveAt(p);
                            int P = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                            while(SI.Pin.Contains(P))
                                P = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                            SI.Pin.Add(P);
                        }
                    }

                    SI.StartTime = 0;
                    SI.EndTime = -1;
                    if (RandomSource.RNG.NextDouble() < MutationRate)
                    {
                        SI.OperationType = RandomSource.RandomSequenceOperationType();
                    }
                    if (RandomSource.RNG.NextDouble() < MutationRate)
                    {
                        SI.WaveFormType = RandomSource.RandomWaveFormType();
                    }
                    if (RandomSource.RNG.NextDouble() < MutationRate)
                    {
                        SI.Amplitude = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxAmplitude);
                    }
                    if (RandomSource.RNG.NextDouble() < MutationRate)
                    {
                        SI.Frequency = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                    }
                    if (RandomSource.RNG.NextDouble() < MutationRate)
                    {
                        SI.Phase = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPhase);
                    }
                    if (RandomSource.RNG.NextDouble() < MutationRate)
                    {
                        SI.CycleTime = RandomSource.RNG.Next(1, 100);
                    }


                }


            }
        }

        public class Population
        {
            public List<Individual> Individuals = null;

            public void Sort()
            {
                this.Individuals.Sort();
            }

            public void Simple1PlusNEvoStrat(double MutationRate)
            {
                this.Sort();
                Individual Best = this.Individuals[0];
                this.Individuals.Clear(); 
                this.Individuals.Add(Best);
                for (int i = 0; i < IndividualAndPopulationFactory.PopulationSize - 1; i++)
                {
                    Individual Child = Best.Clone();
                    Child.Mutate(MutationRate);
                    this.Individuals.Add(Child);
                }
            }
        }

        public class IndividualAndPopulationFactory
        {
            public static int PopulationSize = 2;
            public static int ItemsInGenotype = 4;
            public static int[] AvailablePins = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,12,13,14,15};
            public static int MaxPinsPerSequenceItem = 1;
            public static int MaxTime = 128;
            public static int MaxAmplitude = 254;
            public static int MinFrequency = 500;
            public static int MaxFrequency = 10000;
            public static int MaxPhase = 10;

            public static Individual RandomIndividual()
            {

                Individual Ind = new Individual();
                Ind.Fitness = double.NaN;

                Ind.ListenPin = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];


                do
                {
                    Ind.BallPin = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                } while (Ind.BallPin == Ind.ListenPin);

                do
                {
                    Ind.PaddlePin = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                } while (Ind.PaddlePin == Ind.ListenPin || Ind.PaddlePin == Ind.BallPin);


                Ind.Genotype = new List<emSequenceItem>();

                for (int g = 0; g < IndividualAndPopulationFactory.ItemsInGenotype; g++)
                {
                    emSequenceItem SI = new emSequenceItem();
                    SI.Pin = new List<int>();
                    int PinCount = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPinsPerSequenceItem);
                    for (int p = 0; p < PinCount; p++)
                    {
                        int P = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                        if (!SI.Pin.Contains(P))
                            SI.Pin.Add(P);
                    }

                    SI.StartTime = 0;
                    SI.EndTime = -1;
                    SI.OperationType = RandomSource.RandomSequenceOperationType();
                    SI.WaveFormType = RandomSource.RandomWaveFormType();
                    SI.Amplitude = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxAmplitude);
                    SI.Frequency = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                    SI.Phase = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPhase);
                    SI.CycleTime = RandomSource.RNG.Next(1, 100); ;

                    Ind.Genotype.Add(SI);
                }

                return Ind;
            }

            public static Population RandomPopulation(int Size)
            {
                Population Pop = new Population();
                Pop.Individuals = new List<Individual>();

                for (int i = 0; i < Size; i++)
                    Pop.Individuals.Add(RandomIndividual());

                return Pop;
            }
        }

        public class PongFitnessFunction
        {
            public ulong EvaluationCounter = 0;
            public emEvolvableMotherboard.Client Motherboard = null;

            public PongFitnessFunction()
            {
#if WITHEM
                this.Motherboard = emUtilities.Connect();
                 this.Motherboard.ping();
#endif
            }

            private Thread _TestPopulationThread = null;
            private Population PopToTest = null;

            public void TestPopulationThread()
            {
                foreach (Individual Ind in PopToTest.Individuals)
                    TestIndividual(Ind);
            }

            public void TestPopulation(Population Pop)
            {
                this.PopToTest = Pop;
                _TestPopulationThread = new Thread(TestPopulationThread);
                _TestPopulationThread.Start();
                _TestPopulationThread.Join();
            }

            public bool ApplyConfigFromIndividual(Individual Ind)
            {
                List<int> PinsUsed = new List<int>();

                this.Motherboard.clearSequences();
                

                //Ind.ListenPin = 8;

                //Set up where we read back the values from the EM
                emSequenceItem RecordItem = new emSequenceItem();
                RecordItem.StartTime = 0;
                RecordItem.EndTime = -1;
                RecordItem.Pin = new List<int>(); RecordItem.Pin.Add(Ind.ListenPin);
                RecordItem.Frequency = 20000;
                RecordItem.OperationType = emSequenceOperationType.RECORD;

                emSequenceItem PaddleItem = new emSequenceItem();
                PaddleItem.StartTime = 0;
                PaddleItem.EndTime = -1;
                PaddleItem.Pin = new List<int>(); PaddleItem.Pin.Add(Ind.PaddlePin);
                PaddleItem.OperationType = emSequenceOperationType.CONSTANT_FROM_REGISTER;
                PaddleItem.ValueSourceRegister = 0;

                emSequenceItem BallItem = new emSequenceItem();
                BallItem.StartTime = 0;
                BallItem.EndTime = -1;
                BallItem.Pin = new List<int>(); BallItem.Pin.Add(Ind.BallPin);
                BallItem.OperationType = emSequenceOperationType.CONSTANT_FROM_REGISTER;
                BallItem.ValueSourceRegister = 1;

                #if WITHEM
                this.Motherboard.appendSequenceAction(RecordItem);
                this.Motherboard.appendSequenceAction(PaddleItem);
                this.Motherboard.appendSequenceAction(BallItem);
#endif

                //We will prevent things shorting out a bit
                List<int> ForbiddenPins = new List<int>();
                ForbiddenPins.AddRange(RecordItem.Pin);
                //ForbiddenPins.AddRange(PaddleItem.Pin);
                //ForbiddenPins.AddRange(BallItem.Pin);                

                //Set up the individuals' config instructions
                for (int i = 0; i < Ind.Genotype.Count; i++)
                {
                    Ind.Genotype[i].StartTime = 0;
                    Ind.Genotype[i].EndTime = -1;

                /*    if (Ind.Genotype[i].OperationType == emSequenceOperationType.DIGITAL)
                    {
                        if (Ind.Genotype[i].Amplitude >= 1)
                            Ind.Genotype[i].CycleTime = 100;
                        else
                            Ind.Genotype[i].CycleTime = 0;
                    }*/
                    PinsUsed = new List<int>();
                    for (int pIndex = 0; pIndex < Ind.Genotype[i].Pin.Count; pIndex++)
                    {
                        if (ForbiddenPins.Contains(Ind.Genotype[i].Pin[pIndex])) continue;
                        //if (ForbiddenPins.Contains(Ind.Genotype[i].Pin[pIndex]))
                        {
                            //  Ind.Genotype[i].Pin.RemoveAt(pIndex);

                        }
                        PinsUsed.Add(Ind.Genotype[i].Pin[pIndex]);


                        Console.Write(i + "\t");
                        for (int pIndex2 = 0; pIndex2 < Ind.Genotype[i].Pin.Count; pIndex2++)
                        {
                            Console.Write(Ind.Genotype[i].Pin[pIndex2] + ",");
                        }
                        Console.WriteLine("\t" + Ind.Genotype[i].OperationType + "\t" + Ind.Genotype[i].CycleTime + "\t" + Ind.Genotype[i].Amplitude);
                        #if WITHEM
                        this.Motherboard.appendSequenceAction(Ind.Genotype[i]);
#endif
                    }
                }

                try
                {
                    #if WITHEM
                    this.Motherboard.runSequences();
#endif
                    return true;
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.ToString());
                    return false;
                }

            }
            Stopwatch LastReadStopWatch = null;
            DateTime LastGot = DateTime.Now;
            public double UpdateStep(Individual Ind, double PaddlePosition, double BallYPosition, out double AvgVoltage)
            {
                this.Motherboard.setConfigRegister(0, 1 + (int)(254d * PaddlePosition));
                this.Motherboard.setConfigRegister(1, 1 + (int)(254d * BallYPosition));
                AvgVoltage = 0;
                emWaveForm EmOutput = null;//
                while (EmOutput == null || EmOutput.SampleCount < 10)
                {
                    Thread.Sleep(100);
                    #if WITHEM
                    EmOutput = this.Motherboard.getRecording(Ind.ListenPin);                    
#else
                    EmOutput = new emWaveForm();
                    EmOutput.Samples = new List<int>(1000);
#endif
                    if (EmOutput.Samples.Count < 10)
                    {
                        Console.WriteLine("NOT MANY SAMPLES!!!");
                        
                    }
                }

                int CountHigh = 0;
                double Threshold = 0.05;

                List<double> SampleValues = new List<double>();
                int StartIndex = 9 * (EmOutput.Samples.Count / 10);
                for (int i = StartIndex; i < EmOutput.Samples.Count; i++)
                    SampleValues.Add((5d / 4096d) * EmOutput.Samples[i]);


                //AvgVoltage = SampleValues.Average();
                SampleValues.Sort();
                AvgVoltage = SampleValues[SampleValues.Count / 2];


                Console.Write("\tP={0:0.00} B={1:0.00} ", PaddlePosition, BallYPosition);
                Console.Write("  #Samples={0:0000} Avg={1:0.00}v  Time={2:000}ms  ",
                (EmOutput.SampleCount - StartIndex),
                    AvgVoltage,
                        DateTime.Now.Subtract(LastGot).TotalMilliseconds);

                

                LastGot = DateTime.Now;


                if (AvgVoltage > Threshold)
                    return 1;
                else
                    return -1;
            }

            public double[,] TestStates =
                new double[,]
                {
                    //ball, paddle
                    {0.1,0.1},
                    {0.1,0.9},
                    {0.9,0.9},
                    {0.9,0},
                    {0.4,1},
                    {0.6,0.1},
                    {0.25,0.1},
                    {0.25,0.9},
                    {0.25,0.6},
                    {0.75,0.2},
                    {0.1,0.5},
                    {0.25,0.25}
                };

            public void Shuffle()
            {
                return;
                for (int k = 0; k < 10; k++)
                {
                    int i0 = RandomSource.RNG.Next(0,(int)TestStates.GetLongLength(0));
                    int i1 = RandomSource.RNG.Next(0, (int)TestStates.GetLongLength(0));
                    double t0 = TestStates[i0, 0];
                    double t1 = TestStates[i0, 1];
                    TestStates[i0, 0] = TestStates[i1, 0];
                    TestStates[i0, 1] = TestStates[i1, 1];
                    TestStates[i1, 0] = t0;
                    TestStates[i1, 1] = t1;
                }
            }

            public double TestIndividual(Individual Ind)
            {
                Ind.Fitness = 0;
/*
                for (int i = 0; i < Ind.Genotype.Count; i++)
                    Ind.Fitness += Ind.Genotype[i].Frequency;
                return Ind.Fitness;*/
                Ind.EvaluationIndex = this.EvaluationCounter++;
                ConfusionMatrix CM = new ConfusionMatrix();
                Motherboard.reset();


          

                if (!ApplyConfigFromIndividual(Ind))
                {
                    Ind.Fitness = -1;
                    return Ind.Fitness; ;
                }
                Thread.Sleep(200);
                Shuffle();
                List<double> OutputVoltages = new List<double>();
                int RepeatCounts = 2;
                int PositionsToTest = (int)TestStates.GetLongLength(0);
                for (int r = 0; r < RepeatCounts; r++)
                {
                    
                    for (int i = 0; i < PositionsToTest; i++)
                    {
                        double PaddlePosition = TestStates[i, 1];// RandomSource.RNG.NextDouble();
                        double BallYPosition = TestStates[i, 0];// i % 2 == 0 ? PaddlePosition - (PaddlePosition * RandomSource.RNG.NextDouble()) : PaddlePosition + (PaddlePosition * RandomSource.RNG.NextDouble());
                        double AvgOutputVoltage = 0;
                        double Action = UpdateStep(Ind, PaddlePosition, BallYPosition, out AvgOutputVoltage);
                        OutputVoltages.Add(AvgOutputVoltage);
                        if (Action < 0) Action = -1; else Action = 1;
                        double ExpectedAction = PaddlePosition < BallYPosition ? -1 : 1;
                        
                        Console.Write("\t\tACTION={0,2},EXPECTED={1,2} ", (int)Action, (int)ExpectedAction);
                        if (ExpectedAction < 0 && Action < 0)
                        {
                            Console.Write(" TP CORRECT");
                            CM.TP++;
                        }
                        if (ExpectedAction >= 0 && Action >= 0)
                        {
                            Console.Write(" TN CORRECT");
                            CM.TN++;
                        }
                        if (ExpectedAction >= 0 && Action < 0)
                        {
                            Console.Write(" FP WRONG");
                            CM.FP++;
                        }
                        if (ExpectedAction < 0 && Action >= 0)
                        {
                            Console.Write(" FN WRONG");
                            CM.FN++;
                        }
                        Console.WriteLine();
                    }
                }
                Ind.Fitness = Math.Abs(CM.MCC);

                
                double TestThreshold = -5;
                while (TestThreshold < 5)
                {
                    ConfusionMatrix TCM = new ConfusionMatrix();
                    int Index = 0;
                    for (int r = 0; r < RepeatCounts; r++)
                    {
                        for (int i = 0; i < PositionsToTest; i++)
                        {
                            double PaddlePosition = TestStates[i, 1];// RandomSource.RNG.NextDouble();
                            double BallYPosition = TestStates[i, 0];// i % 2 == 0 ? PaddlePosition - (PaddlePosition * RandomSource.RNG.NextDouble()) : PaddlePosition + (PaddlePosition * RandomSource.RNG.NextDouble());
                            double AvgOutputVoltage = 0;
                            double Action = OutputVoltages[Index];
                            Index++;
                            if (Action < TestThreshold) Action = -1; else Action = 1;
                            double ExpectedAction = PaddlePosition < BallYPosition ? -1 : 1;


                            if (ExpectedAction < 0 && Action < 0)
                            {
                                TCM.TP++;
                            }
                            if (ExpectedAction >= 0 && Action >= 0)
                            {
                                TCM.TN++;
                            }
                            if (ExpectedAction >= 0 && Action < 0)
                            {
                                TCM.FP++;
                            }
                            if (ExpectedAction < 0 && Action >= 0)
                            {
                                TCM.FN++;
                            }
                        }
                    }
                    if (Math.Abs(TCM.MCC) > Ind.Fitness)
                    {
                        Console.WriteLine("\t\t\tBEST THRESHOLD = " + TestThreshold + " gives " + Ind.Fitness);
                        Ind.Fitness = Math.Abs(TCM.MCC);
                        Ind.Threshold = TestThreshold;
                    }

                    TestThreshold += 0.01;
                }
                return Ind.Fitness;
            }
        }


        public class Pong
        {
            public static void Go()
            {

                string WorkingFolder = String.Format("./Pong/");
                if (!Directory.Exists(WorkingFolder))
                    Directory.CreateDirectory(WorkingFolder);
                Directory.SetCurrentDirectory(WorkingFolder);
                Population Pop = EMUtils.Experiment_Pong.IndividualAndPopulationFactory.RandomPopulation(IndividualAndPopulationFactory.PopulationSize);
                PongFitnessFunction FitFunc = new PongFitnessFunction();


                for (int i = 0; i < 25; i++)
                {
//                    FitFunc.TestIndividual(Pop.Individuals[0]);
  //                  Reporting.Say(i + "\t" + Pop.Individuals[0].Fitness);
                    Console.WriteLine("**********************");
                }
    //            return;
                if (File.Exists("PONGBESTIND"))
                {
                    Individual B = Individual.Load("PONGBESTIND");
                    if (B != null)
                    {
                        B.EvaluationIndex = 0;
                        Pop.Individuals.Add(B);
                    }
                }

                Stopwatch Timer = new Stopwatch();
                Timer.Start();
                string OutputFileName = "";
                for (int Epoch = 0; Epoch < 1000; Epoch++)
                {
                    #if WITHEM
                    FitFunc.Motherboard.setLED(0, false);
#endif
                    FitFunc.TestPopulation(Pop);
#if WITHEM
                    FitFunc.Motherboard.setLED(0, true);
                    FitFunc.Motherboard.setLED(1, true);
#endif
                    Pop.Sort();
                    Individual BestInd = Pop.Individuals[0];
                    BestInd.Save("PONGBESTIND");
                    Individual.Save(BestInd, "PongBestInd.xml");
                    Reporting.Say(string.Format("{0,-10}\t{1,-10}\t{2,-10}\t{3,-10}", Epoch, BestInd.Fitness, BestInd.EvaluationIndex, Timer.ElapsedMilliseconds));


#if GUI
                    Application.DoEvents();
#endif
                    if (BestInd.Fitness >= 0.95)
                        break;

                    Pop.Simple1PlusNEvoStrat(0.1);
                    #if WITHEM
                    FitFunc.Motherboard.setLED(1, false);
#endif

                }

#if GUI
                PongForm PF = new PongForm();
                PF.FitnessFunction = FitFunc;
                Pop.Sort();
                PF.Ind = Pop.Individuals[0]; ;
                PF.ShowDialog();
#endif
            }
        }

    }
}
