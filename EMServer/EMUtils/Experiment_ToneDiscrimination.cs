using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using emInterfaces;
using EMServer;
using System.Windows.Forms;
using System.Diagnostics;
using CGPIP2;
using System.Threading;
using System.IO;

namespace EMUtils
{
    public class Experiment_ToneDiscrimination
    {
        //Define a common random number generator
        //You can set a fixed random seed in the constructor
        public class RandomSource
        {
            public static Random RNG = new Random();
        }

        //Define the individual's representation
        public class Individual : IComparable
        {
            public double Fitness;  // The fitness measure
            public ulong EvaluationIndex = 0;  //Counter for when it was evauated
            public List<string> Report = new List<string>();
            public List<emSequenceItem> Genotype = null; //The gentoype is a list of signals to output
            public int ListenPin = 8; //We also can evolve the pin it uses to get the material's output from
            public int InputPin0 = 0;
            public List<emWaveForm> Output = new List<emWaveForm>();
            public List<emSequenceItem> OutputedSequences;
            public int CorrectClass = 0;

            //Need to define a mutation operator
            public void Mutate(double P)
            {
                Fitness = double.NaN;  //Reset the fitness
                this.Report.Clear();

                this.Output.Clear();
                this.CorrectClass = 0;


                if (RandomSource.RNG.NextDouble() < P)
                    this.ListenPin = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];

                if (RandomSource.RNG.NextDouble() < P)
                {
                    do
                    {
                        this.InputPin0 = IndividualAndPopulationFactory.AvailablePins[RandomSource.RNG.Next(0, IndividualAndPopulationFactory.AvailablePins.Length)];
                    } while (this.ListenPin == this.InputPin0);

                }
                for (int i = 0; i < this.Genotype.Count; i++)
                {
                    for (int ParameterToMutate = 0; ParameterToMutate < 1; ParameterToMutate++)
                    {
                        if (RandomSource.RNG.NextDouble() > P)
                            continue;

                        emSequenceItem Item = this.Genotype[i];

                        switch (ParameterToMutate)
                        {
                            case (0):
                                Item.Amplitude = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxAmplitude);
                                break;
                            case (1):
                                Item.OperationType = RandomSource.RNG.NextDouble() < 0.5 ? emSequenceOperationType.CONSTANT : emSequenceOperationType.PREDEFINED;
                                break;

                            case (2):
                                if (RandomSource.RNG.NextDouble() < 0.1)
                                    Item.Frequency = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                                else
                                {
                                    int Noise = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxFrequency / 20) - (IndividualAndPopulationFactory.MaxFrequency / 10);
                                    Item.Frequency += Noise;
                                    if (Item.Frequency < IndividualAndPopulationFactory.MinFrequency) Item.Frequency = IndividualAndPopulationFactory.MinFrequency;
                                    if (Item.Frequency > IndividualAndPopulationFactory.MaxFrequency) Item.Frequency = IndividualAndPopulationFactory.MaxFrequency;
                                }
                                break;
                            case (3):
                                Item.Phase = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxPhase);
                                break;
                            default:
                                throw new Exception("Mutation failed");
                        }

                    }
                }


            }


            //Function to make a copy of an individual
            public Individual Clone()
            {
                Individual NewInd = new Individual();
                NewInd.Fitness = this.Fitness;
                NewInd.EvaluationIndex = this.EvaluationIndex;
                NewInd.CorrectClass = this.CorrectClass;

                NewInd.Genotype = new List<emSequenceItem>();
                for (int i = 0; i < this.Genotype.Count; i++)
                {
                    emSequenceItem NewItem = new emSequenceItem();
                    NewItem.Amplitude = this.Genotype[i].Amplitude;
                    NewItem.CycleTime = this.Genotype[i].CycleTime;
                    NewItem.StartTime = this.Genotype[i].StartTime;
                    NewItem.EndTime = this.Genotype[i].EndTime;
                    NewItem.Frequency = this.Genotype[i].Frequency;
                    NewItem.OperationType = this.Genotype[i].OperationType;
                    NewItem.WaveFormType = this.Genotype[i].WaveFormType;
                    NewItem.Pin = this.Genotype[i].Pin;
                    NewItem.Phase = this.Genotype[i].Phase;

                    NewInd.Genotype.Add(NewItem);
                }
                NewInd.Output.AddRange(this.Output);
                NewInd.ListenPin = this.ListenPin;
                NewInd.InputPin0 = this.InputPin0;

                NewInd.OutputedSequences = this.OutputedSequences;
                NewInd.Report.AddRange(this.Report);
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

        public class FitnessFunction
        {
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

            public void TestIndividual_old(Individual Ind)
            {
                //      if (!double.IsNaN(Ind.Fitness)) return;
                //  Ind.ListenPin = 5;
                //Ind.InputPin0 = 10;
                Ind.Fitness = 0;
                Ind.EvaluationIndex = this.EvaluationCounter++;

                this.Motherboard.reset();

                //Schedule tones
                int ToneCount = 5;
                int ToneLength = IndividualAndPopulationFactory.MaxTime / ToneCount;

                List<int> PinsUsed = new List<int>();
                Ind.OutputedSequences = new List<emSequenceItem>();
                long FinalTime = 0;
                for (int i = 0; i < ToneCount; i++)
                {
                    emSequenceItem Item = new emSequenceItem();
                    Item.StartTime = i * ToneLength;
                    Item.EndTime = Item.StartTime + ToneLength;
                    Item.Pin = new List<int>(); Item.Pin.Add( Ind.InputPin0);
                    Item.Phase = 0;
                    Item.CycleTime = 50;
                    Item.Amplitude = 1;
                    Item.Frequency = i % 2 == 0 ? 5000 : 500;
                    Item.OperationType = emSequenceOperationType.PREDEFINED;
                    Item.WaveFormType = emWaveFormType.PWM;
                    this.Motherboard.appendSequenceAction(Item);
                    Ind.OutputedSequences.Add(Item);
                    FinalTime = Item.EndTime;
                }

                {
                    emSequenceItem Item = new emSequenceItem();
                    Item.StartTime = FinalTime;
                    Item.EndTime = FinalTime + 1;
                    Item.Pin = new List<int>(); Item.Pin.Add(Ind.InputPin0);
                    Item.OperationType = emSequenceOperationType.emNULL;
                    this.Motherboard.appendSequenceAction(Item);
                    Ind.OutputedSequences.Add(Item);
                }


                PinsUsed.Add(Ind.InputPin0);


                //Set up where we read back the values from the EM
                emSequenceItem RecordItem = new emSequenceItem();
                RecordItem.StartTime = 0;
                RecordItem.EndTime = IndividualAndPopulationFactory.MaxTime;
                RecordItem.Pin = new List<int>(); RecordItem.Pin.Add( Ind.ListenPin);
                RecordItem.Frequency = 20000;
                RecordItem.OperationType = emSequenceOperationType.RECORD;

                emSequenceItem RecordItem2 = new emSequenceItem();
                RecordItem2.StartTime = 0;
                RecordItem2.EndTime = IndividualAndPopulationFactory.MaxTime;
                RecordItem2.Pin = new List<int>(); RecordItem2.Pin.Add(11);
                RecordItem2.Frequency = RecordItem.Frequency;
                RecordItem2.OperationType = emSequenceOperationType.RECORD;

                if (PinsUsed.Contains(Ind.ListenPin)) throw new Exception("Pin used multiple times!");
                foreach(int p in RecordItem.Pin)
                    PinsUsed.Add(p);

                this.Motherboard.appendSequenceAction(RecordItem);
                Ind.OutputedSequences.Add(RecordItem);
                //     this.Motherboard.appendSequenceAction(RecordItem2);
                //  Ind.OutputedSequences.Add(RecordItem2);

                //Set up the individuals' config instructions
                for (int i = 0; i < Ind.Genotype.Count; i++)
                {
                    foreach (int p in Ind.Genotype[i].Pin)
                    {
                        if (PinsUsed.Contains(p)) throw new Exception("Pin used multiple times!");
                        PinsUsed.Add(p);
                    }
                    this.Motherboard.appendSequenceAction(Ind.Genotype[i]);
                    Ind.OutputedSequences.Add(Ind.Genotype[i]);
                }

                //Run the test, wait for the mobo to let us know it has finished
                this.Motherboard.runSequences();
                // this.Motherboard.joinSequences();

                //Analyse the data
                emWaveForm RecordedSignal = this.Motherboard.getRecording(RecordItem.Pin[0]);
                if (RecordedSignal == null) throw new Exception("Failed to get recorded signal");
                if (RecordedSignal.Samples.Count == 0 || RecordedSignal.SampleCount != RecordedSignal.Samples.Count)
                    Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal.SampleCount + " " + RecordedSignal.Samples.Count);
                if (RecordedSignal.Samples.Count > 300000)
                {
                    Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal.SampleCount + " " + RecordedSignal.Samples.Count);
                    //Ind.Output = RecordedSignal;
                    Ind.Fitness = -1;
                    return;
                }
                ConfusionMatrix CM = new ConfusionMatrix();
                double[] SumOfSignals = new double[2];
                int ChunkSize = RecordedSignal.SampleCount / ToneCount;
                ulong CountOne = 0;
                ulong CountZero = 0;
                for (int i = 0; i < ToneCount; i++)
                {
                    int p = 0;
                    for (int j = 0; j < ChunkSize; j++)
                    {
                        bool Expected = i % 2 == 0;
                        bool Predicted = RecordedSignal.Samples[p] == 1;
                        if (RecordedSignal.Samples[p] == 1) CountOne++;
                        else if (RecordedSignal.Samples[p] == 0) CountZero++;

                        if (Expected && Predicted) CM.TP++;
                        else if (!Expected && !Predicted) CM.TN++;
                        else if (Expected && !Predicted) CM.FN++;
                        else if (!Expected && Predicted) CM.FP++;

                        p++;
                    }
                }

                Ind.Fitness = Math.Abs(CM.Accuracy);
                if (CountOne < (ulong)(RecordedSignal.SampleCount / 100) || CountZero < (ulong)(RecordedSignal.SampleCount / 100)) Ind.Fitness = 0;
                if (double.IsNaN(Ind.Fitness)) Ind.Fitness = -1;
                //       Ind.Output = RecordedSignal;
                //   Ind.Output2 = this.Motherboard.getRecording(11);

                //    Console.WriteLine("\t\t\t\t" + Ind.Output.SampleCount + "\t" + Ind.Output2.SampleCount);

            }

            public void TestIndividual(Individual Ind)
            {

                Ind.Fitness = 0;
                Ind.EvaluationIndex = this.EvaluationCounter++;



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

                        if (Ind.Genotype[i].Pin.Contains(Ind.InputPin0)) continue;
                        if (Ind.Genotype[i].Pin.Contains(Ind.ListenPin)) continue;

                        this.Motherboard.appendSequenceAction(Ind.Genotype[i]);
                        Ind.OutputedSequences.Add(Ind.Genotype[i]);
                    }


                    emSequenceItem Item = new emSequenceItem();
                    Item.StartTime = 0;
                    Item.EndTime = IndividualAndPopulationFactory.MaxTime;
                    Item.Pin = new List<int>(); Item.Pin.Add( Ind.InputPin0);
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
                    RecordItem.Pin = new List<int>(); RecordItem.Pin.Add( Ind.ListenPin);
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
                    emWaveForm RecordedSignal = this.Motherboard.getRecording(RecordItem.Pin[0]);
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
                this.Motherboard.setLED(3, false);
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
                Ind.Genotype = new List<emSequenceItem>();


                Ind.ListenPin = AvailablePins[RandomSource.RNG.Next(0, AvailablePins.Length)];

                do
                {
                    Ind.InputPin0 = AvailablePins[RandomSource.RNG.Next(0, AvailablePins.Length)];
                } while (Ind.ListenPin == Ind.InputPin0);


                for (int i = 0; i < ItemsInGenotype; i++)
                {
                    emSequenceItem Item = new emSequenceItem();
                    Item.Pin = new List<int>(); Item.Pin.Add( AvailablePins[i]);
                    Item.StartTime = 0;
                    Item.EndTime = IndividualAndPopulationFactory.MaxTime;
                    Item.OperationType = emSequenceOperationType.CONSTANT;// RandomSource.RNG.NextDouble() < 0.5 ? emSequenceOperationType.CONSTANT : emSequenceOperationType.PREDEFINED;
                    Item.WaveFormType = emWaveFormType.PWM;
                    Item.Amplitude = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxAmplitude);
                    Item.Frequency = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                    Item.Phase = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPhase);
                    Item.CycleTime = 50;
                    Ind.Genotype.Add(Item);
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
        public static emWaveFormVisualizer wfv = new emWaveFormVisualizer();
        public static emSequenceVisualiser sequenceVisualiser = new emSequenceVisualiser();

        public static emWaveForm Merge(params emWaveForm[] Waves)
        {
            emWaveForm MergedWave = new emWaveForm();
            MergedWave.Samples = new List<int>();
            foreach (emWaveForm Wave in Waves)
            {
                MergedWave.Samples.AddRange(Wave.Samples);
                MergedWave.SampleCount += Wave.Samples.Count;
            }
            return MergedWave;
        }

        public static void Go()
        {

            string WorkingFolder = String.Format("./ToneDisc/");
            if (!Directory.Exists(WorkingFolder))
                Directory.CreateDirectory(WorkingFolder);
            Directory.SetCurrentDirectory(WorkingFolder);
            Population Pop = IndividualAndPopulationFactory.RandomPopulation(IndividualAndPopulationFactory.PopulationSize);
            FitnessFunction FitFunc = new FitnessFunction();

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

                if (BestInd.Output != null)
                {
                    wfv.Clear();
                    emWaveForm Merged = Merge(BestInd.Output.ToArray());
                    wfv.AddLines(Merged);//, BestInd.Output2);
                    emWaveFormUtilities.SaveWaveForm(Merged, "bestInd.txt");
                    foreach (string S in BestInd.Report)
                        Reporting.Say("\t" + S);
                }
                else
                    Reporting.Say("BestInd.Output is null");


                try
                {
                    if (File.Exists(OutputFileName))
                        File.Delete(OutputFileName);
                }
                catch (Exception e)
                {
                    Reporting.Say(e.ToString());
                }

                try
                {
                    if (BestInd.OutputedSequences != null)
                        OutputFileName = sequenceVisualiser.UpdateViz(BestInd.OutputedSequences);
                }
                catch (Exception e)
                {
                    Reporting.Say(e.ToString());
                }

                if (BestInd.CorrectClass == BestInd.Report.Count())
                {
                    emWaveForm Merged = Merge(BestInd.Output.ToArray());
                    emWaveFormUtilities.SaveWaveForm(Merged, String.Format("tonedisc_{0:000000}_outputs.txt", Epoch));
                    try
                    {
                        string NewFileName = String.Format("tonedisc_{0:000000}_config.pdf", Epoch);
                        
                        if (File.Exists(NewFileName)) File.Delete(NewFileName);
                        File.Copy(OutputFileName, NewFileName);
                    }
                    catch (Exception e)
                    {
                        Reporting.Say(e.ToString());
                    }
                }

                Application.DoEvents();

                Pop.Simple1PlusNEvoStrat(0.1);

                FitFunc.Motherboard.setLED(5, false);
            }
        }
    }
}
