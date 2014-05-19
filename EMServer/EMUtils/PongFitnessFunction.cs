using emInterfaces;
using EMServer;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

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
                emSequenceOperationType[] AllowedTypes = new[] { emSequenceOperationType.CONSTANT,  emSequenceOperationType.DIGITAL};// emSequenceOperationType.ARBITRARY, emSequenceOperationType.CONSTANT, emSequenceOperationType.CONSTANT_FROM_REGISTER, emSequenceOperationType.DIGITAL, emSequenceOperationType.PREDEFINED };
                return AllowedTypes[RNG.Next(0, AllowedTypes.Length)];
            }
        }

        public class Individual : IComparable
        {
            public double Fitness;  // The fitness measure
            public ulong EvaluationIndex = 0;  //Counter for when it was evauated

            public int ListenPin = 0;
            public int PaddlePin = 1;
            public int BallPin = 2;
            public List<emSequenceItem> Genotype = null; 

            public Individual Clone()
            {
                Individual NewInd = new Individual();
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

                if (Other.EvaluationIndex < this.EvaluationIndex)
                    return -1;
                if (Other.EvaluationIndex > this.EvaluationIndex)
                    return 1;
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
                    SI.Pin = new List<int>();
                    int PinCount = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxPinsPerSequenceItem);
                    for (int p = 1; p < PinCount; p++)                    
                    {
                        if (RandomSource.RNG.NextDouble() < MutationRate)
                        {
                            SI.Pin.RemoveAt(p);
                            if (RandomSource.RNG.NextDouble() < MutationRate || SI.Pin.Count==0)
                            {
                                int P = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                                if (!SI.Pin.Contains(P))
                                    SI.Pin.Add(P);
                            }
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
                        SI.Amplitude = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxAmplitude);
                    }
                    if (RandomSource.RNG.NextDouble() < MutationRate)
                    {
                        SI.Frequency = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                    }
                    if (RandomSource.RNG.NextDouble() < MutationRate)
                    {
                        SI.Phase = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPhase);
                    }
                    SI.CycleTime = 50;

                    
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
                this.Individuals.Clear(); this.Individuals.Add(Best);
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
            public static int PopulationSize = 5;
            public static int ItemsInGenotype = 10;
            public static int[] AvailablePins = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
            public static int MaxPinsPerSequenceItem = 3;
            public static int MaxTime = 128;
            public static int MaxAmplitude = 2;
            public static int MinFrequency = 500;
            public static int MaxFrequency = 10000;
            public static int MaxPhase = 10;

            public static Individual RandomIndividual()
            {
                ItemsInGenotype = AvailablePins.Length;
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
                } while (Ind.PaddlePin == Ind.ListenPin || Ind.PaddlePin==Ind.BallPin);


                Ind.Genotype = new List<emSequenceItem>();

                for (int g = 0; g < IndividualAndPopulationFactory.ItemsInGenotype; g++)
                {
                    emSequenceItem SI = new emSequenceItem();
                    SI.Pin = new List<int>();
                    int PinCount = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxPinsPerSequenceItem);
                    for (int p = 1; p < PinCount; p++)
                    {
                        int P = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                        if (!SI.Pin.Contains(P))
                            SI.Pin.Add(P);
                    }

                    SI.StartTime = 0;
                    SI.EndTime = -1;
                    SI.OperationType = RandomSource.RandomSequenceOperationType();
                    SI.WaveFormType = RandomSource.RandomWaveFormType();
                    SI.Amplitude = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxAmplitude);
                    SI.Frequency = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                    SI.Phase = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPhase);
                    SI.CycleTime = 50;

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
                this.Motherboard = emUtilities.Connect();
                this.Motherboard.ping();
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

            public void ApplyConfigFromIndividual(Individual Ind)
            {
                List<int> PinsUsed = new List<int>();

                //Set up where we read back the values from the EM
                emSequenceItem RecordItem = new emSequenceItem();
                RecordItem.StartTime = 0;
                RecordItem.EndTime = -1;
                RecordItem.Pin = new List<int>(); RecordItem.Pin.Add(Ind.ListenPin);
                RecordItem.Frequency = 10000;
                RecordItem.OperationType = emSequenceOperationType.RECORD;

                emSequenceItem PaddleItem = new emSequenceItem();
                PaddleItem.StartTime = 0;
                PaddleItem.EndTime = -1;
                PaddleItem.Pin =new List<int>(); PaddleItem.Pin.Add(Ind.PaddlePin);
                PaddleItem.OperationType = emSequenceOperationType.CONSTANT_FROM_REGISTER;
                PaddleItem.ValueSourceRegister = 0;

                emSequenceItem BallItem = new emSequenceItem();
                BallItem.StartTime = 0;
                BallItem.EndTime = -1;
                BallItem.Pin = new List<int>(); BallItem.Pin.Add(Ind.BallPin);
                BallItem.OperationType = emSequenceOperationType.CONSTANT_FROM_REGISTER;
                BallItem.ValueSourceRegister = 1;

                this.Motherboard.appendSequenceAction(RecordItem);
                this.Motherboard.appendSequenceAction(PaddleItem);
                this.Motherboard.appendSequenceAction(BallItem);


                //We will prevent things shorting out a bit
                List<int> ForbiddenPins = new List<int>();
                ForbiddenPins.AddRange(RecordItem.Pin);
                //ForbiddenPins.AddRange(PaddleItem.Pin);
                //ForbiddenPins.AddRange(BallItem.Pin);                

                //Set up the individuals' config instructions
                for (int i = 0; i < Ind.Genotype.Count; i++)
                {
                    Ind.Genotype[i].StartTime = -1;
                    Ind.Genotype[i].EndTime = -1;
                    for (int pIndex = 0; pIndex < Ind.Genotype[i].Pin.Count;pIndex++ )
                    {
                        if (ForbiddenPins.Contains(Ind.Genotype[i].Pin[pIndex])) continue;
                        PinsUsed.Add(Ind.Genotype[i].Pin[pIndex]);
                        if (Ind.Genotype[i].OperationType == emSequenceOperationType.DIGITAL)
                        {
                            if (Ind.Genotype[i].Amplitude >= 1)
                                Ind.Genotype[i].CycleTime = 100;
                            else
                                Ind.Genotype[i].CycleTime = 0;
                        }

                        this.Motherboard.appendSequenceAction(Ind.Genotype[i]);
                    }
                }
                
                this.Motherboard.runSequences();
                
            }
            Stopwatch LastReadStopWatch = null;
            public double UpdateStep(Individual Ind, double PaddlePosition, double BallYPosition)
            {
                emWaveForm EmOutput = this.Motherboard.getRecording(Ind.ListenPin);
                if (EmOutput.SampleCount < 10) return 0;
                int CountHigh = 0;
                int Threshold = 0;
                double[] SampleValues = new double[EmOutput.Samples.Count];
                for (int i = 0; i < EmOutput.Samples.Count; i++)
                    SampleValues[i] = (5d / 4096d) * EmOutput.Samples[i];
                for (int i = 0; i < EmOutput.Samples.Count; i++) if (SampleValues[i] >= Threshold) CountHigh++;
                if (CountHigh > EmOutput.Samples.Count / 2)
                    return 1;
                else
                    return -1;
            }

            public double TestIndividual(Individual Ind)
            {                
                Ind.Fitness = 0;
                Ind.EvaluationIndex = this.EvaluationCounter++;

                Motherboard.reset();

                ApplyConfigFromIndividual(Ind);

                int PositionsToTest = 8;

                for (int i = 0; i < PositionsToTest; i++)
                {
                    double PaddlePosition = RandomSource.RNG.NextDouble();
                    double BallYPosition = RandomSource.RNG.NextDouble();
                    double Action = UpdateStep(Ind, PaddlePosition, BallYPosition);
                    if (Action == 0)
                        Ind.Fitness = 0;
                    else if (PaddlePosition < BallYPosition && Action < 0)
                        Ind.Fitness++;
                    else if (PaddlePosition >= BallYPosition && Action >= 0)
                        Ind.Fitness++;
                    else
                        throw new Exception("Unhandled condition");
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


                Stopwatch Timer = new Stopwatch();
                Timer.Start();
                string OutputFileName = "";
                for (int Epoch = 0; Epoch < 1000; Epoch++)
                {
                    FitFunc.Motherboard.setLED(4, false);
                    FitFunc.TestPopulation(Pop);
                    FitFunc.Motherboard.setLED(4, true);
                    FitFunc.Motherboard.setLED(5, true);
                    Pop.Sort();
                    Individual BestInd = Pop.Individuals[0];
                    Reporting.Say(string.Format("{0,-10}\t{1,-10}\t{2,-10}\t{3,-10}", Epoch, BestInd.Fitness, BestInd.EvaluationIndex, Timer.ElapsedMilliseconds));



                    Application.DoEvents();

                    Pop.Simple1PlusNEvoStrat(0.1);

                    FitFunc.Motherboard.setLED(5, false);

                }
            }
        }

    }
}
