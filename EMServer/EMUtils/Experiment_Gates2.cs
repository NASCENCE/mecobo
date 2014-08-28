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
    public class Experiment_Gates2
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

            public List<emSequenceItem> Genotype = null; //The gentoype is a list of signals to output
            public int ListenPin = 8; //We also can evolve the pin it uses to get the material's output from

            public emWaveForm Output;
            public List<emSequenceItem> OutputedSequences;
            public emWaveForm Output2;

            public int[] InputPins = new int[3];

            public List<string> Report = new List<string>();

            //Need to define a mutation operator
            public void Mutate(double P)
            {
                Fitness = double.NaN;  //Reset the fitness
                Report.Clear();
                //To ensure constraints on the pins, make sure pins are only used once 
                //This will have to change at some point
                List<int> UnusedPins = new List<int>();
                UnusedPins.AddRange(IndividualAndPopulationFactory.AvailablePins);
                for (int i = 0; i < this.Genotype.Count; i++)
                {
                    foreach (int p in this.Genotype[i].Pin)
                        UnusedPins.Remove(p);
                }
                UnusedPins.Remove(this.ListenPin);

                foreach (int p in this.InputPins)
                    UnusedPins.Remove(p);

                for (int i = 0; i < this.Genotype.Count; i++)
                {
                    for (int ParameterToMutate = 0; ParameterToMutate < 2; ParameterToMutate++)
                    {
                        if (RandomSource.RNG.NextDouble() > P)
                            continue;

                        emSequenceItem Item = this.Genotype[i];

                        switch (ParameterToMutate)
                        {
                            case (0):
                                int k = RandomSource.RNG.Next(0, Item.Pin.Count);
                                int PinValue = Item.Pin[k];
                                UnusedPins.Add(PinValue);
                                while(Item.Pin.Contains(PinValue)) Item.Pin.Remove(PinValue);
                                PinValue = UnusedPins[RandomSource.RNG.Next(0, UnusedPins.Count)];
                                Item.Pin.Add(PinValue);
                                UnusedPins.Remove(PinValue);
                                break;
                            case (1):
                                Item.Amplitude = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxAmplitude);
                                break;
                            case (2):
                                Item.OperationType = emSequenceOperationType.CONSTANT;//RandomSource.RNG.NextDouble() < 0.5 ? emSequenceOperationType.CONST : emSequenceOperationType.PREDEFINED;
                                break;
                            case (3):
                                if (RandomSource.RNG.NextDouble() < 0.1)
                                    Item.Frequency = RandomSource.RNG.Next(10, IndividualAndPopulationFactory.MaxFrequency);
                                else
                                {
                                    int Noise = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxFrequency / 20) - (IndividualAndPopulationFactory.MaxFrequency / 10);
                                    Item.Frequency += Noise;
                                }
                                break;


                            default:
                                throw new Exception("Mutation failed");
                        }

                    }
                }

                if (RandomSource.RNG.NextDouble() < P)
                {
                    UnusedPins.Add(this.ListenPin);
                    this.ListenPin = UnusedPins[RandomSource.RNG.Next(0, UnusedPins.Count)];
                    UnusedPins.Remove(this.ListenPin);
                }

                for (int p = 0; p < this.InputPins.Length; p++)
                {
                    if (RandomSource.RNG.NextDouble() < P)
                    {
                        UnusedPins.Add(this.InputPins[p]);
                        this.InputPins[p] = UnusedPins[RandomSource.RNG.Next(0, UnusedPins.Count)];
                        UnusedPins.Remove(this.InputPins[p]);
                    }
                }

            }


            //Function to make a copy of an individual
            public Individual Clone()
            {
                Individual NewInd = new Individual();
                NewInd.Fitness = this.Fitness;
                NewInd.EvaluationIndex = this.EvaluationIndex;
                NewInd.Report.AddRange(this.Report);
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
                    NewItem.Pin = new List<int>();
                    NewItem.Pin.AddRange(this.Genotype[i].Pin);
                    NewItem.Phase = this.Genotype[i].Phase;

                    NewInd.Genotype.Add(NewItem);
                }
                NewInd.Output = this.Output;
                NewInd.ListenPin = this.ListenPin;
                Array.Copy(this.InputPins, NewInd.InputPins, this.InputPins.Length);

                NewInd.OutputedSequences = this.OutputedSequences;

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


        //Define a class for the population
        public class Population
        {
            //List of individuals in this population
            public List<Individual> Individuals = null;

            //Sort the population
            public void Sort()
            {
                this.Individuals.Sort();
            }

            //Perform a single step in the evo strat
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


        //Represents a test case. Populated in the constructor for the fitness function
        public class BoolTestCase
        {
            //The two input values for the gate
            public bool InputA = false;
            public bool InputB = false;
            public bool InputC = false;
            //What output we expect to get
            public bool Expected = false;

            //How many inputs the function has
            public int Arity = 0;

            public BoolTestCase(bool InputA, bool InputB, bool Expected) { this.InputB = InputB; this.InputA = InputA; this.Expected = Expected; Arity = 2; }
            public BoolTestCase(bool InputA, bool Expected) { this.InputA = InputA; this.Expected = Expected; Arity = 1; }
            public BoolTestCase(bool InputA, bool InputB, bool InputC, bool Expected) { this.InputC = InputC; this.InputB = InputB; this.InputA = InputA; this.Expected = Expected; Arity = 2; }

        }

        public enum BooleanFunctionType
        {
            AND,
            OR,
            NOT,
            XOR,
            NOR,
            NAND,
            MUX
        }

        public class FitnessFunction
        {
            public string OutputFolder = "./GatesTrace/";


            public ulong EvaluationCounter = 0;//How many evaluations we have done
            public emEvolvableMotherboard.Client Motherboard = null;  //Connection to the EM

            public List<BoolTestCase> TestCases = new List<BoolTestCase>(); //A list of test cases
            public BooleanFunctionType FunctionType; //What function we are trying to solve

            private double bestFitness; //The best fitness score observed so far

            private Thread _TestPopulationThread = null;  //We test the population on a thread
            private Population PopToTest = null;  //The current population



            /// <summary>
            /// Initializes a new instance of the <see cref="FitnessFunction"/> class.
            /// </summary>
            /// <param name="TargetFunction">The boolean function to learn</param>
            /// <param name="Repeats">The number of times to apply each patten</param>
            /// 
            public FitnessFunction(BooleanFunctionType TargetFunction, int Repeats)
            {
                this.FunctionType = TargetFunction;
                this.Motherboard = emUtilities.Connect();  //Connect to th em
                this.Motherboard.ping(); //Check it's responding ok


                //Build our truth tables
                for (int r = 0; r < Repeats; r++)
                {
                    switch (TargetFunction)
                    {
                        case (BooleanFunctionType.AND):
                            {
                                TestCases.Add(new BoolTestCase(false, false, false));
                                TestCases.Add(new BoolTestCase(false, true, false));
                                TestCases.Add(new BoolTestCase(true, false, false));
                                TestCases.Add(new BoolTestCase(true, true, true));
                                break;
                            }
                        case (BooleanFunctionType.NAND):
                            {
                                TestCases.Add(new BoolTestCase(false, false, true));
                                TestCases.Add(new BoolTestCase(false, true, true));
                                TestCases.Add(new BoolTestCase(true, false, true));
                                TestCases.Add(new BoolTestCase(true, true, false));
                                break;
                            }
                        case (BooleanFunctionType.XOR):
                            {
                                TestCases.Add(new BoolTestCase(false, false, false));
                                TestCases.Add(new BoolTestCase(false, true, true));
                                TestCases.Add(new BoolTestCase(true, false, true));
                                TestCases.Add(new BoolTestCase(true, true, false));
                                break;
                            }
                        case (BooleanFunctionType.OR):
                            {
                                TestCases.Add(new BoolTestCase(false, false, false));
                                TestCases.Add(new BoolTestCase(false, true, true));
                                TestCases.Add(new BoolTestCase(true, false, true));
                                TestCases.Add(new BoolTestCase(true, true, true));
                                break;
                            }
                        case (BooleanFunctionType.NOR):
                            {
                                TestCases.Add(new BoolTestCase(false, false, true));
                                TestCases.Add(new BoolTestCase(false, true, false));
                                TestCases.Add(new BoolTestCase(true, false, false));
                                TestCases.Add(new BoolTestCase(true, true, false));
                                break;
                            }
                        case (BooleanFunctionType.NOT):
                            {
                                TestCases.Add(new BoolTestCase(false, true));
                                TestCases.Add(new BoolTestCase(true, false));
                                break;
                            }
                        case (BooleanFunctionType.MUX):
                            {
                                TestCases.Add(new BoolTestCase(false, false, false, false));
                                TestCases.Add(new BoolTestCase(false, false, true, false));
                                TestCases.Add(new BoolTestCase(false, true, false, true));
                                TestCases.Add(new BoolTestCase(false, true, true, true));
                                TestCases.Add(new BoolTestCase(true, false, false, false));
                                TestCases.Add(new BoolTestCase(true, false, true, true));
                                TestCases.Add(new BoolTestCase(true, true, false, false));
                                TestCases.Add(new BoolTestCase(true, true, true, true));
                                break;
                            }
                        default:
                            throw new NotImplementedException();
                    }
                }

                if (!Directory.Exists(OutputFolder))
                {
                    Directory.CreateDirectory(OutputFolder);
                }
                else
                {
                    string[] Files = Directory.GetFiles(OutputFolder);
                    foreach (string F in Files)
                    {
                        Console.WriteLine("Deleting " + F);
                        File.Delete(F);
                    }
                }


                //Shufflle
                this.ShuffleTestCases();
            }

            /// <summary>
            /// Shuffles the test cases.
            /// </summary>
            public void ShuffleTestCases()
            {
                TestCases = TestCases.OrderBy(a => Guid.NewGuid()).ToList();
            }

            /// <summary>
            /// Tests the population. This should be running as a background thread
            /// </summary>
            public void TestPopulationThread()
            {
                
                foreach (Individual Ind in PopToTest.Individuals)
                {
                    this.Motherboard.setLED(2, true);
                    TestIndividual(Ind);
                    this.Motherboard.setLED(2, false);
                }
                
            }

            /// <summary>
            /// Tests the population (as a background thread)
            /// </summary>
            /// <param name="Pop">The population</param>
            public void TestPopulation(Population Pop)
            {
                this.PopToTest = Pop;
                _TestPopulationThread = new Thread(TestPopulationThread);
                _TestPopulationThread.Start();
                _TestPopulationThread.Join();
            }
            
            public void TestIndividual(Individual Ind)
            {
                //Reset the fitness
                Ind.Fitness = 0;
                //Remember the evaluation index for this individual
                Reporting.Say("<EVALUATION>");
                Ind.EvaluationIndex = this.EvaluationCounter++;
                Reporting.Say("<EVALUATIONINDEX>"+Ind.EvaluationIndex+"</EVALUATIONINDEX>");
                //Reset the EM and clear everything
                this.Motherboard.reset();
                this.Motherboard.clearSequences();

                //Some variables related to the experiment time, used to calculate the length of sequence operations
                long FinalTime = 0;
                long StartTime = 0;
                long TestCaseLength = IndividualAndPopulationFactory.MaxTime;
                FinalTime = IndividualAndPopulationFactory.MaxTime;

                //Store for the fitness results
                CGPIP2.ConfusionMatrix CM = new CGPIP2.ConfusionMatrix();

                
                int Index = 0;

                StringBuilder TraceBuilder = new StringBuilder();
                bool First = true;
                foreach (BoolTestCase testCase in TestCases)
                {

                    //Copy the config to the EM
                    #region CONFIGURE EM SEQUENCE
                    this.Motherboard.clearSequences();
                    Index++;
                    Ind.OutputedSequences = new List<emSequenceItem>();
                    List<int> PinsUsed = new List<int>();
                    {
                        emSequenceItem Item = new emSequenceItem();
                        Item.StartTime = 0;//  StartTime;
                        Item.EndTime = FinalTime;// StartTime + TestCaseLength;
                        Item.Pin = new List<int>(); Item.Pin.Add(Ind.InputPins[0]);
                        Item.Amplitude = testCase.InputA ? 255 : 1;
                        Item.OperationType = emSequenceOperationType.CONSTANT;
                        Item.WaveFormType = emWaveFormType.PWM;
                        Item.CycleTime = 100;
                        this.Motherboard.appendSequenceAction(Item);
                        Ind.OutputedSequences.Add(Item);

                        Item = new emSequenceItem();
                        Item.StartTime = 0;// StartTime;
                        Item.EndTime = FinalTime;//StartTime + TestCaseLength;
                        Item.Pin = new List<int>(); Item.Pin.Add(Ind.InputPins[1]);
                        Item.Amplitude = testCase.InputB ? 255 : 1;
                        Item.OperationType = emSequenceOperationType.CONSTANT;
                        Item.WaveFormType = emWaveFormType.PWM;
                        Item.CycleTime = 100;
                        this.Motherboard.appendSequenceAction(Item);
                        Ind.OutputedSequences.Add(Item);

                        if (testCase.Arity == 3)
                        {
                            Item = new emSequenceItem();
                            Item.StartTime = 0;// StartTime;
                            Item.EndTime = FinalTime;//StartTime + TestCaseLength;
                            Item.Pin = new List<int>(); Item.Pin.Add(Ind.InputPins[2]);
                            Item.Amplitude = testCase.InputC ? 255 : 1;
                            Item.OperationType = emSequenceOperationType.CONSTANT;
                            Item.WaveFormType = emWaveFormType.PWM;
                            Item.CycleTime = 100;
                            this.Motherboard.appendSequenceAction(Item);
                            Ind.OutputedSequences.Add(Item);
                        }

                        FinalTime = Item.EndTime;
                        StartTime = Item.EndTime;


                        TraceBuilder.Append(testCase.InputA + "," + testCase.InputB + "," + testCase.InputC + ",");

                    }
                    {
                        emSequenceItem Item = new emSequenceItem();
                        Item.StartTime = FinalTime;
                        Item.EndTime = FinalTime + 1;
                        Item.Pin = new List<int>(); Item.Pin.Add(Ind.InputPins[0]);
                        Item.OperationType = emSequenceOperationType.emNULL;
                        this.Motherboard.appendSequenceAction(Item);
                        Ind.OutputedSequences.Add(Item);
                    }

                    #endregion

                    //Keep track of which pins the individual used
                    foreach (int p in Ind.InputPins)
                        PinsUsed.Add(p);

                    //Set up where we read back the values from the EM
                    emSequenceItem RecordItem = new emSequenceItem();
                    RecordItem.StartTime = 0;
                    RecordItem.EndTime = IndividualAndPopulationFactory.MaxTime;
                    RecordItem.Pin = new List<int>(); RecordItem.Pin.Add(Ind.ListenPin);
                    RecordItem.Frequency = 5000;
                    RecordItem.OperationType = emSequenceOperationType.RECORD;


                    //  if (PinsUsed.Contains(Ind.ListenPin)) throw new Exception("Pin used multiple times!");
                    foreach (int p in RecordItem.Pin) PinsUsed.Add(p);


                    //Set up the individuals' config instructions
                    for (int i = 0; i < Ind.Genotype.Count; i++)
                    {

                        bool Allow = true;
                        foreach (int p in RecordItem.Pin)
                        {
                            //    if (PinsUsed.Contains(p)) Allow = false;// throw new Exception("Pin used multiple times!");
                            //  PinsUsed.Add(p);
                        }
                        if (Allow)
                        {
                            this.Motherboard.appendSequenceAction(Ind.Genotype[i]);
                            Ind.OutputedSequences.Add(Ind.Genotype[i]);
                        }

                    }

                    this.Motherboard.appendSequenceAction(RecordItem);
                    Ind.OutputedSequences.Add(RecordItem);


                    if (First)
                    {
                        Reporting.Say("<SEQUENCE>");
                        foreach (emSequenceItem SI in Ind.OutputedSequences)
                        {
                            Reporting.Say(String.Format("\tPin={0,-2} Op={1,-9} Type={2,-7} Amp={3,-4} Fre={4,-4} Ph={5,-4}",
                                SI.Pin[0].ToString(),
                                SI.OperationType.ToString(),
                                SI.WaveFormType.ToString(),
                                SI.Amplitude.ToString(),
                                SI.Frequency.ToString(),
                                SI.Phase.ToString()
                                ));
                        }
                        Reporting.Say("</SEQUENCE>");
                        Reporting.Say("<OUTPUT>");
                    }


                    //Run the test, wait for the mobo to let us know it has finished
                    this.Motherboard.setLED(3, true);
                    this.Motherboard.runSequences();


                    this.Motherboard.joinSequences();
                    this.Motherboard.setLED(3, false);
                    //Analyse the data
                    emWaveForm RecordedSignal = this.Motherboard.getRecording(RecordItem.Pin[0]);
                    if (RecordedSignal == null) throw new Exception("Failed to get recorded signal");
                    if (RecordedSignal.Samples.Count == 0 || RecordedSignal.SampleCount != RecordedSignal.Samples.Count)
                        Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal.SampleCount + " " + RecordedSignal.Samples.Count);
                    if (RecordedSignal.Samples.Count > 300000)
                    {
                        Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal.SampleCount + " " + RecordedSignal.Samples.Count);
                        Ind.Output = RecordedSignal;
                        Ind.Fitness = -1;
                        return;
                    }

                    ulong CountOne = 0;
                    for (int i = 0; i < RecordedSignal.SampleCount; i++)
                        if (RecordedSignal.Samples[i] >= 0) CountOne++;

                    
                    
                    bool Predicted = CountOne > (ulong)RecordedSignal.SampleCount / 2;
                    bool Expected = testCase.Expected;
                    double AvgVoltage = RecordedSignal.Samples.Count>1? ((5 * RecordedSignal.Samples.Average())/4096) : 0;
                    Reporting.Say(String.Format("\t\tTest={3,-5},{4,-5},{5,-5} Predicted={0,-5} Expected={1,-5} %High={2:000} Avg={6:00.00}v",
                        Predicted, Expected, (100d * CountOne) / RecordedSignal.SampleCount, testCase.InputA, testCase.InputB, testCase.InputC, AvgVoltage));
                    
                    TraceBuilder.AppendLine(Expected + "," + Predicted);

                    if (Predicted && Expected)
                        CM.TP++;
                    else if (!Predicted && Expected)
                        CM.FN++;
                    else if (!Predicted && !Expected)
                        CM.TN++;
                    else if (Predicted && !Expected)
                        CM.FP++;

                    if (Predicted == Expected)
                        Ind.Fitness++;
                    Ind.Report.Add(String.Format(" TESTCASE {0,-2} {1,-5},{2,-5}=>{3,-5} {4,-5} {5}",
                          Index - 1, testCase.InputA, testCase.InputB, testCase.Expected, Predicted, (testCase.Expected == Predicted ? "PASS" : "FAIL")));
                    Ind.Output = RecordedSignal;


                    First = false;
                }
                Reporting.Say("</OUTPUT>");

                if (double.IsNaN(Ind.Fitness)) Ind.Fitness = -1;
                Ind.Fitness = Math.Abs(CM.MCC);

                TraceBuilder.AppendLine("#fitness," + CM.MCC);
                TraceBuilder.AppendLine("#evaluationindex," + Ind.EvaluationIndex);
                Reporting.Say("<FITNESS>" + CM.MCC + "</FITNESS>");
                Reporting.Say("</TEST>");

                File.WriteAllText(OutputFolder + "/" + Ind.EvaluationIndex, TraceBuilder.ToString());
                Ind.Report.Add("FITNESS\t" + Ind.Fitness + "\t" + Ind.EvaluationIndex);
                Reporting.Say("</EVALUATION>");
            }
            /**
            public void TestIndividual2(Individual Ind)
            {
                //      if (!double.IsNaN(Ind.Fitness)) return;
                //  Ind.ListenPin = 5;
                //Ind.InputPin0 = 10;
                Ind.Fitness = 0;
                Ind.EvaluationIndex = this.EvaluationCounter++;

                this.Motherboard.reset();
                this.Motherboard.clearSequences();
                //Schedule tones





                long FinalTime = 0;
                long StartTime = 0;
                long TestCaseLength = IndividualAndPopulationFactory.MaxTime / TestCases.Count;
                FinalTime = IndividualAndPopulationFactory.MaxTime;
                ConfusionMatrix CM = new ConfusionMatrix();
                Ind.OutputedSequences = new List<emSequenceItem>();
                List<int> PinsUsed = new List<int>();
                emSequenceItem Item = null;
                foreach (BoolTestCase testCase in TestCases)
                {

                    {
                        Item = new emSequenceItem();
                        Item.StartTime = StartTime;
                        Item.EndTime = StartTime + TestCaseLength;
                        Item.Pin = Ind.InputPins[0];
                        Item.Amplitude = testCase.InputA ? 1 : 0;
                        Item.OperationType = emSequenceOperationType.CONSTANT;
                        Item.WaveFormType = emWaveFormType.PWM;
                        this.Motherboard.appendSequenceAction(Item);
                        Ind.OutputedSequences.Add(Item);

                        if (testCase.Arity > 1)
                        {
                            Item = new emSequenceItem();
                            Item.StartTime = StartTime;
                            Item.EndTime = StartTime + TestCaseLength;
                            Item.Pin = Ind.InputPins[1];
                            Item.Amplitude = testCase.InputB ? 1 : 0;
                            Item.OperationType = emSequenceOperationType.CONSTANT;
                            Item.WaveFormType = emWaveFormType.PWM;
                            this.Motherboard.appendSequenceAction(Item);
                            Ind.OutputedSequences.Add(Item);
                        }
                        FinalTime = Item.EndTime;
                        StartTime = Item.EndTime;

                    }

                }

                Item = new emSequenceItem();
                Item.StartTime = FinalTime;
                Item.EndTime = FinalTime + 1;
                Item.Pin = Ind.InputPins[0];
                Item.OperationType = emSequenceOperationType.emNULL;
                this.Motherboard.appendSequenceAction(Item);
                Ind.OutputedSequences.Add(Item);

                foreach (int pin in Ind.InputPins)
                    PinsUsed.Add(pin);


                //Set up where we read back the values from the EM
                emSequenceItem RecordItem = new emSequenceItem();
                RecordItem.StartTime = 0;
                RecordItem.EndTime = IndividualAndPopulationFactory.MaxTime;
                RecordItem.Pin = Ind.ListenPin;
                RecordItem.Frequency = 5000;
                RecordItem.OperationType = emSequenceOperationType.RECORD;

                if (PinsUsed.Contains(Ind.ListenPin)) throw new Exception("Pin used multiple times!");
                PinsUsed.Add(RecordItem.Pin);

                this.Motherboard.appendSequenceAction(RecordItem);
                Ind.OutputedSequences.Add(RecordItem);

                //Set up the individuals' config instructions
                for (int i = 0; i < Ind.Genotype.Count; i++)
                {

                    if (PinsUsed.Contains(Ind.Genotype[i].Pin)) throw new Exception("Pin used multiple times!");
                    PinsUsed.Add(Ind.Genotype[i].Pin);
                    Ind.Genotype[i].WaveFormType = emWaveFormType.PWM;
                    this.Motherboard.appendSequenceAction(Ind.Genotype[i]);
                    Ind.OutputedSequences.Add(Ind.Genotype[i]);

                }

                //Run the test, wait for the mobo to let us know it has finished
                this.Motherboard.runSequences();
                this.Motherboard.joinSequences();

                //Analyse the data
                emWaveForm RecordedSignal = this.Motherboard.getRecording(RecordItem.Pin);
                if (RecordedSignal == null) throw new Exception("Failed to get recorded signal");
                if (RecordedSignal.Samples.Count == 0 || RecordedSignal.SampleCount != RecordedSignal.Samples.Count)
                    Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal.SampleCount + " " + RecordedSignal.Samples.Count);
                if (RecordedSignal.Samples.Count > 300000)
                {
                    Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal.SampleCount + " " + RecordedSignal.Samples.Count);
                    Ind.Output = RecordedSignal;
                    Ind.Fitness = -1;
                    return;
                }

                double[] SumOfSignals = new double[2];
                int ChunkSize = RecordedSignal.SampleCount / TestCases.Count;
                ulong CountOne = 0;
                ulong CountZero = 0;
                int p = 0;
                for (int i = 0; i < TestCases.Count; i++)
                {
                    BoolTestCase testCase = TestCases[i];
                    bool Expected = testCase.Expected;
                    int OnesForThisChunk = 0;
                    for (int j = 0; j < ChunkSize; j++)
                    {

                        if (j > ChunkSize / 4 && j < 3 * ChunkSize / 4)
                        {
                            bool Predicted = RecordedSignal.Samples[p] == 1;
                            if (Predicted) OnesForThisChunk++;
                            if (RecordedSignal.Samples[p] == 1) CountOne++;
                            else if (RecordedSignal.Samples[p] == 0) CountZero++;

                            if (Expected && Predicted) CM.TP++;
                            else if (!Expected && !Predicted) CM.TN++;
                            else if (Expected && !Predicted) CM.FN++;
                            else if (!Expected && Predicted) CM.FP++;
                        }
                        p++;
                    }
                    bool OverallPred = OnesForThisChunk > ChunkSize / 4;
                    if (testCase.Arity == 1)
                        Ind.Report.Add(String.Format(" TESTCASE {0,-2} {1,-5}=>{3,-5} {4,-5} {5}",
                            i, testCase.InputA, testCase.InputB, testCase.Expected, OverallPred, (testCase.Expected == OverallPred ? "PASS" : "FAIL")));
                    if (testCase.Arity == 2)
                        Ind.Report.Add(String.Format(" TESTCASE {0,-2} {1,-5},{2,-5}=>{3,-5} {4,-5} {5}",
                            i, testCase.InputA, testCase.InputB, testCase.Expected, OverallPred, (testCase.Expected == OverallPred ? "PASS" : "FAIL")));
                }

                if (double.IsNaN(Ind.Fitness)) Ind.Fitness = -1;
                Ind.Fitness = (CM.MCC);
                Ind.Report.Add(String.Format("FITNESS\t{0,-10}\tAcc={1:0.0000}\tMCC={2:0.0000}", Ind.EvaluationIndex, CM.Accuracy, CM.MCC));
                Reporting.Say(String.Format("\t\tEVALUATION\t{0,-10}\tAcc={1:0.0000}\tMCC={2:0.0000}", Ind.EvaluationIndex, CM.Accuracy, CM.MCC));
                Ind.Output = RecordedSignal;
            }*/
        }


        public class IndividualAndPopulationFactory
        {
            public static int PopulationSize = 5;  //Number of individuas in our population
            public static int ItemsInGenotype = 5; //Number of evovlable elements in our genotype
            public static int[] AvailablePins = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };  //List of the FPGA pins to work with
            public static int MaxTime = 128; //Max time for evaluating an individual
            public static int MaxAmplitude = 254;
            public static int MaxFrequency = 10000;
            public static int MaxPhase = 10;

            /// <summary>
            /// Generates a random individual
            /// </summary>
            /// <returns></returns>
            public static Individual RandomIndividual()
            {
                Individual Ind = new Individual();
                Ind.Fitness = double.NaN;
                Ind.Genotype = new List<emSequenceItem>();
                List<int> UnusedPins = new List<int>();
                UnusedPins.AddRange(AvailablePins);

                for (int i = 0; i < ItemsInGenotype; i++)
                {
                    emSequenceItem Item = new emSequenceItem();
                    Item.Pin = new List<int>(); Item.Pin.Add(UnusedPins[RandomSource.RNG.Next(0, UnusedPins.Count)]);
                    foreach (int p in Item.Pin)
                        UnusedPins.Remove(p);
                    Item.StartTime = 0;
                    Item.EndTime = IndividualAndPopulationFactory.MaxTime;
                    Item.OperationType = emSequenceOperationType.CONSTANT;// RandomSource.RNG.NextDouble() < 0.5 ? emSequenceOperationType.CONST : emSequenceOperationType.PREDEFINED;
                    Item.WaveFormType = emWaveFormType.PWM;
                    Item.Amplitude = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxAmplitude);
                    Item.Frequency = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxFrequency);
                    Item.Phase = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPhase);
                    Item.CycleTime = 50;
                    Ind.Genotype.Add(Item);
                }
                Ind.ListenPin = UnusedPins[RandomSource.RNG.Next(0, UnusedPins.Count)];
                UnusedPins.Remove(Ind.ListenPin);

                for (int pin = 0; pin < Ind.InputPins.Length; pin++)
                {
                    Ind.InputPins[pin] = UnusedPins[RandomSource.RNG.Next(0, UnusedPins.Count)];
                    UnusedPins.Remove(Ind.InputPins[pin]);
                }

                return Ind;
            }

            /// <summary>
            /// Generates a random population
            /// </summary>
            /// <param name="Size">The size of the population</param>
            /// <returns></returns>
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

        /// <summary>
        /// Main method for the experiment
        /// </summary>
        /// 


        public static void Go()
        {
            IndividualAndPopulationFactory.MaxTime = 128;
            FitnessFunction FitFunc = new FitnessFunction(BooleanFunctionType.MUX, 1);

            Population Pop = IndividualAndPopulationFactory.RandomPopulation(IndividualAndPopulationFactory.PopulationSize);

            // wfv.Show();
            sequenceVisualiser.Show();
            Stopwatch Timer = new Stopwatch();
            Timer.Start();

            for (int Epoch = 0; Epoch < 1000; Epoch++)
            {
                FitFunc.ShuffleTestCases();
                FitFunc.TestPopulation(Pop);
                Pop.Sort();
                Individual BestInd = Pop.Individuals[0];
                Reporting.Say(string.Format("STATUS\t{0,-10}\t{1:0.0000}\t{2,-10}\t{3,-10}", Epoch, BestInd.Fitness, BestInd.EvaluationIndex, Timer.ElapsedMilliseconds));
                foreach (string S in BestInd.Report)
                    Reporting.Say("\t" + S);
            /*
                if (BestInd.Output != null)
                {
                    wfv.Clear();
                    wfv.AddLines(BestInd.Output);
                    emWaveFormUtilities.SaveWaveForm(BestInd.Output, "bestInd.txt");
                }
                else
                    Reporting.Say("BestInd.Output is null");

                */
            /*try
            {
                if (BestInd.OutputedSequences != null)
                    sequenceVisualiser.UpdateViz(BestInd.OutputedSequences);
            }
            catch (Exception e)
            {
                Reporting.Say(e.ToString());
            }
            */
                Application.DoEvents();
                FitFunc.Motherboard.setLED(2, true);
                if (BestInd.Fitness < 0.751)
                    Pop.Simple1PlusNEvoStrat(100);
                else
                    Pop.Simple1PlusNEvoStrat(0.1);
                FitFunc.Motherboard.setLED(2, false);
            }
        }
    }
}
