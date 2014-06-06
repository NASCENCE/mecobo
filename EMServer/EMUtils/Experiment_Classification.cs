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
using System.Windows.Forms;
using System.Xml.Serialization;

namespace EMUtils
{
    public class Experiment_Classification
    {
        public class DataSet
        {
            public int[,] RawData =
            {
                {1,2,2,2,1},
                {1,1,1,1,3},
                {1,1,1,2,2},
                {1,1,2,1,3},
                {1,1,2,2,1},
                {1,2,1,1,3},
                {1,2,1,2,2},
                {1,2,2,1,3},                
                {2,1,1,1,3},
                {2,1,1,2,2},
                {2,1,2,1,3},
                {2,1,2,2,1},
                {2,2,1,1,3},
                {2,2,1,2,2},
                {2,2,2,1,3},
                {2,2,2,2,3},
                {3,1,1,1,3},
                {3,1,1,2,3},
                {3,1,2,1,3},
                {3,1,2,2,1},
                {3,2,1,1,3},
                {3,2,1,2,2},
                {3,2,2,1,3},
                {3,2,2,2,3}
            };

            public int AttributeCount
            {
                get { return 4; }
            }

            public int OutputCount
            {
                get { return 1; }
            }

            public int InstanceCount
            {
                get { return 24; }
            }
        }


        public enum GeneType
        {
            INPUT_0,  //0
            INPUT_1,  //1
            INPUT_2,  //2
            INPUT_3,  //3       
            OUTPUT_0, //4
            OUTPUT_1,  //5
            OUTPUT_2,  //6
            CONST_0,  //7
            CONST_1,  //8
            CONST_2,  //9
            CONST_3, //10
            CONST_4, //11
        }

        public class Individual : IComparable
        {
            public double Fitness;  // The fitness measure
            public ulong EvaluationIndex = 0;  //Counter for when it was evauated

            public GeneType[] Genotype_Types = null;
            public int[] Genotype_FreqScalars = null;
            public int[] Genotype_Mag;

            public void Mutate(double Rate)
            {
                this.Fitness = double.NaN;
                this.EvaluationIndex = 0;

                for (int i = 0; i < Genotype_Types.Length; i++)  //for each element in the genotype
                {
                    for (int j = 0; j < 3; j++)  //for each gene
                    {
                        if (RandomSource.RNG.NextDouble() > Rate) continue;  //see if we need to mutate it

                        if (j == 0)  //magnitude
                        {
                            this.Genotype_Mag[i] = RandomSource.RNG.NextDouble() < 0.5 ? 0 : 1;
                        }
                        else if (j == 1)
                        {
                            this.Genotype_FreqScalars[i] = RandomSource.RNG.Next(0, 1000);
                        }
                        else if (j == 2)
                        {
                            //where to swap to?
                            int D = RandomSource.RNG.Next(0, this.Genotype_Types.Length);
                            GeneType Temp = this.Genotype_Types[D];
                            this.Genotype_Types[D] = this.Genotype_Types[i];
                            this.Genotype_Types[i] = Temp;
                        }
                    }
                }
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

            public Individual Clone()
            {
                Individual N = new Individual();
                N.Fitness = this.Fitness;
                N.EvaluationIndex = this.EvaluationIndex;
                N.Genotype_Types = new GeneType[this.Genotype_Types.Length];
                N.Genotype_Mag = new int[this.Genotype_Types.Length];
                N.Genotype_FreqScalars = new int[this.Genotype_Types.Length];
                Array.Copy(this.Genotype_Types, N.Genotype_Types, this.Genotype_Types.Length);
                Array.Copy(this.Genotype_Mag, N.Genotype_Mag, this.Genotype_Types.Length);
                Array.Copy(this.Genotype_FreqScalars, N.Genotype_FreqScalars, this.Genotype_Types.Length);
                return N;
            }

            public static void SaveToFile(string FileName, Individual Ind)
            {
                using (var writer = new System.IO.StreamWriter(FileName))
                {
                    var serializer = new XmlSerializer(Ind.GetType());
                    serializer.Serialize(writer, Ind);
                    writer.Flush();
                }
            }

            public static Individual LoadFromFile(string FileName)
            {
                using (var stream = System.IO.File.OpenRead(FileName))
                {
                var serializer = new XmlSerializer(typeof(Individual));
                return serializer.Deserialize(stream) as Individual;
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

            public void Save(string Folder)
            {
                if (!Directory.Exists(Folder))
                    Directory.CreateDirectory(Folder);
                for (int i = 0; i < Individuals.Count; i++)
                    Individual.SaveToFile(Folder + "/" + i + ".xml", Individuals[i]);
            }

            public void Load(string Folder)
            {
                this.Individuals = new List<Individual>();
                string[] Files = Directory.GetFiles(Folder, "*.xml");
                foreach (string File in Files)
                    Individuals.Add(Individual.LoadFromFile(File));
            }
        }

        //Define a common random number generator
        //You can set a fixed random seed in the constructor
        public class RandomSource
        {
            public static Random RNG = new Random();
        }

        public class FitnessFunction
        {
            public DataSet TestSet = null;
            public ulong EvaluationCounter = 0;
            public emEvolvableMotherboard.Client Motherboard = null;
            private double bestFitness;

            public FitnessFunction()
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


            public void TestIndividual(Individual Ind)
            {

                Ind.Fitness = 0;
                Ind.EvaluationIndex = this.EvaluationCounter++;
                ConfusionMatrix CM = new ConfusionMatrix();
                Console.Write("Testing ");
                for (int tc = 0; tc < this.TestSet.InstanceCount; tc++)
                {
                    //Console.Write(".");
                    this.Motherboard.setLED(1, true);
                    this.Motherboard.reset();
                    this.Motherboard.clearSequences();

                    List<emSequenceItem> OutputItems = new List<emSequenceItem>();
                    Dictionary<int, emSequenceItem> RecordItems = new Dictionary<int, emSequenceItem>();

                    for (int e = 0; e < Ind.Genotype_Types.Length; e++)
                    {

                        if (Ind.Genotype_Types[e] == GeneType.CONST_0 ||
                            Ind.Genotype_Types[e] == GeneType.CONST_1 ||
                            Ind.Genotype_Types[e] == GeneType.CONST_2 ||
                            Ind.Genotype_Types[e] == GeneType.CONST_3)
                        {
                            emSequenceItem Item = new emSequenceItem();
                            Item.StartTime = 0;
                            Item.EndTime = IndividualAndPopulationFactory.MaxTime;
                            Item.Pin = e;
                            Item.Phase = 0;
                            Item.CycleTime = 50;
                            Item.Amplitude = Ind.Genotype_Mag[e];
                            Item.Frequency = 500;
                            Item.OperationType = emSequenceOperationType.PREDEFINED;
                            Item.WaveFormType = emWaveFormType.PWM;
                            
                            OutputItems.Add(Item);
                        }

                        if (Ind.Genotype_Types[e] == GeneType.INPUT_0 ||
                            Ind.Genotype_Types[e] == GeneType.INPUT_1 ||
                            Ind.Genotype_Types[e] == GeneType.INPUT_2 ||
                            Ind.Genotype_Types[e] == GeneType.INPUT_3)
                        {
                            emSequenceItem Item = new emSequenceItem();
                            Item.StartTime = 0;
                            Item.EndTime = IndividualAndPopulationFactory.MaxTime;
                            Item.Pin = e;
                            Item.Phase = 0;
                            Item.CycleTime = 50;
                            Item.Amplitude = 1;
                            Item.Frequency = this.TestSet.RawData[tc, Ind.Genotype_Types[e] - GeneType.INPUT_0] * Ind.Genotype_FreqScalars[e];
                            if (Item.Frequency < 500) Item.Frequency = 500;
                            Item.OperationType = emSequenceOperationType.PREDEFINED;
                            Item.WaveFormType = emWaveFormType.PWM;
                            OutputItems.Add(Item);
                        }

                        if (Ind.Genotype_Types[e] == GeneType.OUTPUT_0 ||
                            Ind.Genotype_Types[e] == GeneType.OUTPUT_1 ||
                            Ind.Genotype_Types[e] == GeneType.OUTPUT_2)
                        {
                            emSequenceItem RecordItem = new emSequenceItem();
                            RecordItem.StartTime = 0;
                            RecordItem.EndTime = IndividualAndPopulationFactory.MaxTime;
                            RecordItem.Pin = e;
                            RecordItem.Frequency = 10000;
                            RecordItem.OperationType = emSequenceOperationType.RECORD;
                            RecordItems.Add(Ind.Genotype_Types[e] - GeneType.OUTPUT_0, RecordItem);
                        }
                    }

                    //Apply output items first
                    foreach (emSequenceItem Item in OutputItems)
                        this.Motherboard.appendSequenceAction(Item);

                    //Then apply recording items
                    foreach (emSequenceItem Item in RecordItems.Values)
                        this.Motherboard.appendSequenceAction(Item);

                    this.Motherboard.setLED(1, false);


                    this.Motherboard.setLED(2, true);
                    //Run the test, wait for the mobo to let us know it has finished
                    this.Motherboard.runSequences();
                    this.Motherboard.setLED(2, false);

                    this.Motherboard.setLED(3, true);
                    //Get the outputs

                    emWaveForm Output0Recording = this.Motherboard.getRecording(RecordItems[0].Pin);
                    emWaveForm Output1Recording = this.Motherboard.getRecording(RecordItems[1].Pin);
                    emWaveForm Output2Recording = this.Motherboard.getRecording(RecordItems[2].Pin);

                    this.Motherboard.setLED(3, false);

                    int Expected = this.TestSet.RawData[tc, 4] - 1;
                    double[] OutputValues = new double[3];
                    OutputValues[0] = Freq(Output0Recording);
                    OutputValues[1] = Freq(Output1Recording);
                    OutputValues[2] = Freq(Output2Recording);

                    int MaxClass = 0;
                    for (int i = 1; i < OutputValues.Length; i++)
                    {
                        if (OutputValues[i] > OutputValues[MaxClass])
                            MaxClass = i;
                    }

                    int Predicted = MaxClass;

                    if (Expected == Predicted)
                    {
                        Ind.Fitness++;
                        Console.Write('!');
                    }
                    else
                    {
                        Console.Write('_');
                    }
                    /*
                    if (Expected == 0 && MaxClass == 0)
                    {
                        CM.TP++;
                        Console.Write(".TP.");
                    }
                    if (Expected != 0 && MaxClass == 0)
                    {
                        CM.FP++;
                        Console.Write(".FP.");
                    }
                    if (Expected == 0 && MaxClass != 0)
                    {
                        CM.FN++;
                        Console.Write(".FN.");
                    }
                    if (Expected != 0 && MaxClass != 0)
                    {

                        CM.TN++;
                        Console.Write(".TN.");
                    }
                    */
                  /*  int TargetClass = 1;
                    if (Expected == TargetClass && MaxClass == TargetClass)
                    {
                        CM.TP++;
                        Console.Write(".TP.");
                    }
                    if (Expected != TargetClass && MaxClass == TargetClass)
                    {
                        CM.FP++;
                        Console.Write(".FP.");
                    }
                    if (Expected == TargetClass && MaxClass != TargetClass)
                    {
                        CM.FN++;
                        Console.Write(".FN.");
                    }
                    if (Expected != TargetClass && MaxClass != TargetClass)
                    {

                        CM.TN++;
                        Console.Write(".TN.");
                    }

                }
                Ind.Fitness = Math.Abs(CM.MCC);
                Console.WriteLine(String.Format(".... Fitness MCC={0:0.00},ACC={1:00.0}% ", Ind.Fitness, CM.Accuracy * 100));
                */
                }
                    Console.WriteLine(String.Format(".... Fitness ACC={0:0.00}% ", ((100d * Ind.Fitness) / this.TestSet.InstanceCount)));
                /*
                int ToneCount = 4;
                ConfusionMatrix CM = new ConfusionMatrix();

                Ind.OutputedSequences = new List<emSequenceItem>();
                long FinalTime = 0;
                for (int t = 0; t < ToneCount; t++)
                {
                    this.Motherboard.reset();

                    this.Motherboard.clearSequences();
                    Ind.OutputedSequences.Clear();

                    this.Motherboard.setLED(1, true);
                    for (int i = 0; i < Ind.Genotype.Count; i++)
                    {

                        if (Ind.Genotype[i].Pin == Ind.InputPin0) continue;
                        if (Ind.Genotype[i].Pin == Ind.ListenPin) continue;

                        this.Motherboard.appendSequenceAction(Ind.Genotype[i]);
                        Ind.OutputedSequences.Add(Ind.Genotype[i]);
                    }


                    emSequenceItem Item = new emSequenceItem();
                    Item.StartTime = 0;
                    Item.EndTime = IndividualAndPopulationFactory.MaxTime;
                    Item.Pin = Ind.InputPin0;
                    Item.Phase = 0;
                    Item.CycleTime = 50;
                    Item.Amplitude = 1;
                    Item.Frequency = t % 2 == 0 ? 5000 : 500;
                    Item.OperationType = emSequenceOperationType.PREDEFINED;
                    Item.WaveFormType = emWaveFormType.PWM;

                    this.Motherboard.appendSequenceAction(Item);
                    Ind.OutputedSequences.Add(Item);

                    //Set up where we read back the values from the EM
                    emSequenceItem RecordItem = new emSequenceItem();
                    RecordItem.StartTime = 0;
                    RecordItem.EndTime = IndividualAndPopulationFactory.MaxTime;
                    RecordItem.Pin = Ind.ListenPin;
                    RecordItem.Frequency = 10000;
                    RecordItem.OperationType = emSequenceOperationType.RECORD;


                    this.Motherboard.appendSequenceAction(RecordItem);
                    Ind.OutputedSequences.Add(RecordItem);
                    this.Motherboard.setLED(1, false);

                    this.Motherboard.setLED(2, true);
                    //Run the test, wait for the mobo to let us know it has finished
                    this.Motherboard.runSequences();
                    this.Motherboard.setLED(2, false);
                    // this.Motherboard.joinSequences();


                    this.Motherboard.setLED(3, true);
                    //Analyse the data
                    emWaveForm RecordedSignal = this.Motherboard.getRecording(RecordItem.Pin);
                    if (RecordedSignal == null) throw new Exception("Failed to get recorded signal");
                    if (RecordedSignal.Samples.Count == 0 || RecordedSignal.SampleCount != RecordedSignal.Samples.Count)
                        Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal.SampleCount + " " + RecordedSignal.Samples.Count);
                    if (RecordedSignal.Samples.Count > 300000)
                    {
                        Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal.SampleCount + " " + RecordedSignal.Samples.Count);
                        Ind.Output.Add(RecordedSignal);
                        Ind.Fitness = -1;
                        return;
                    }

                    double[] SumOfSignals = new double[2];
                    int ChunkSize = RecordedSignal.SampleCount;
                    int CountOne = 0;
                    int CountZero = 0;
                    bool Expected = t % 2 == 0;

                    for (int j = ChunkSize / 4; j < 3 * ChunkSize / 4; j++)
                    {
                        bool Predicted = RecordedSignal.Samples[j] == 1;
                        if (RecordedSignal.Samples[j] == 1) CountOne++;
                        else if (RecordedSignal.Samples[j] == 0) CountZero++;

                        if (Expected && Predicted) CM.TP++;
                        else if (!Expected && !Predicted) CM.TN++;
                        else if (Expected && !Predicted) CM.FN++;
                        else if (!Expected && Predicted) CM.FP++;
                    }

                    Ind.Output.Add(RecordedSignal);
                    bool OverallPred = CountOne >= (0.7f * (CountOne + CountZero));
                    Ind.Report.Add(String.Format(" TESTCASE {0,-2} {1,-8},{2,-8}/{3,-8} {7:0.00} Exp={4,-5} Pred={5,-5} {6,-5}", t, CountZero, CountOne, ChunkSize, (t % 2 == 0), OverallPred, OverallPred == (t % 2 == 0), (double)CountOne / (CountOne + CountZero)));
                    if (OverallPred == (t % 2 == 0)) Ind.CorrectClass++;

                }
                Ind.Fitness = Math.Abs(CM.Accuracy);

                if (double.IsNaN(Ind.Fitness)) Ind.Fitness = -1;
                this.Motherboard.setLED(3, false);*/
            }

            public double Freq(emWaveForm W)
            {
                int C = 0;

                int t = W.Samples[0];
                List<int> Ts = new List<int>();
                for (int i = 1; i < W.SampleCount; i++)
                {
                    if (W.Samples[i] == 1 && W.Samples[i - 1] == 0)
                    {
                        C++;
                        t = W.Samples[i];
                        Ts.Add(i);
                    }
                    if (W.Samples[i] != 0 && W.Samples[i] != 1)
                    {
                        throw new Exception("Samples are not 0 or 1");
                    }
                }

                if (Ts.Count < 2)
                    return 0;

                double SumGap = 0;

                for (int i = 1; i < Ts.Count; i++)
                {
                    SumGap += Math.Abs(Ts[i - 1] - Ts[i]);
                }

                return SumGap / Ts.Count;
            }

            public int CountTransitions(emWaveForm W)
            {
                int C = 0;

                int t = W.Samples[0];

                for (int i = 1; i < W.SampleCount; i++)
                {
                    if (W.Samples[i] != t)
                    {
                        C++;
                        t = W.Samples[i];
                    }
                }

                return C;
            }
        }



        public class IndividualAndPopulationFactory
        {
            public static int PopulationSize = 5;
            public static int ItemsInGenotype = 10;
            public static int[] AvailablePins = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
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

                Ind.Genotype_Types = new GeneType[ItemsInGenotype];
                Ind.Genotype_FreqScalars = new int[ItemsInGenotype];
                Ind.Genotype_Mag = new int[ItemsInGenotype];

                for (int i = 0; i < ItemsInGenotype; i++)
                {
                    Ind.Genotype_Types[i] = (GeneType)i;
                    Ind.Genotype_Mag[i] = RandomSource.RNG.NextDouble() < 0.5 ? 0 : 1;
                    Ind.Genotype_FreqScalars[i] = RandomSource.RNG.Next(0, 1000);
                }

                Ind.Mutate(1);

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

        public static emWaveFormVisualizer wfv = new emWaveFormVisualizer();
        public static emSequenceVisualiser sequenceVisualiser = new emSequenceVisualiser();


        public static void Go()
        {
            string WorkingFolder = String.Format("./Classifier/");
            if (!Directory.Exists(WorkingFolder))
                Directory.CreateDirectory(WorkingFolder);
            Directory.SetCurrentDirectory(WorkingFolder);
            Reporting.LogToFile(WorkingFolder, "log");
            Population Pop = IndividualAndPopulationFactory.RandomPopulation(IndividualAndPopulationFactory.PopulationSize);
            FitnessFunction FitFunc = new FitnessFunction();
            FitFunc.TestSet = new DataSet();
            wfv.Show();
            sequenceVisualiser.Show();
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

                Pop.Save("./Classification_Population/");

                Pop.Simple1PlusNEvoStrat(0.1);

                FitFunc.Motherboard.setLED(5, false);
            }

        }
    }
}
