using emInterfaces;
using EMServer;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace EMUtils
{
    class Classification
    {
        public static int num_input = 4;// For Iris and contact lense it is 4
        public static int num_output = 3;// For Iris and contact lense it is 3
        public static bool ignoreFirst;
        public static int run = 1;
        public static int dataset=1;
        //Maximum and minimum values of input variables
        public static int[] max;
        public static int[] min;
       
        //Array of genotypes
        public static gene_item[,] genotypes;
        //class of genotype
        public class gene_item
        {
            public int pin_no, sig_type, amplitude, frequency, phase, cycle;

        }
        public class RandomSource
        {
            public static Random RNG = new Random();
        }

    

        //Define the individual's representation
        public class Individual
        {
            public double TrainingFitness;  // The fitness measure
            public double TestingFitness;  // The fitness measure
            public int correctTrainingClass, correctTestingClass;
            public ulong EvaluationIndex = 0;  //Counter for when it was evauated

            public List<emSequenceItem> Genotype = null; //The gentoype is a list of signals to output
            public int[] ListenPins = new int[num_output]; //We also can evolve the pin it uses to get the material's output from

            public emWaveForm[] Output = new emWaveForm[num_output];
            public List<emSequenceItem> OutputedSequences;
            public emWaveForm Output2;

            public int[] InputPins = new int[num_input];

            public List<string> Report = new List<string>();

           
            public void Mutate(int pop_num)//Mutates the genotypes
            {
                int mutate_gene_num = RandomSource.RNG.Next(0, 11);
                int mutate_gene = RandomSource.RNG.Next(0, 5);
              
                int i = 0;


                if (mutate_gene == 0)
                {
                    int pin_num = RandomSource.RNG.Next(0, 11);


                    while (pin_num == genotypes[pop_num, mutate_gene_num].pin_no)
                    {
                        pin_num = RandomSource.RNG.Next(0, 11);
                    }


                    int this_pin_num = genotypes[pop_num, mutate_gene_num].pin_no;
              


                    for (i = 0; i <= 11; i++)
                    {
                        if (genotypes[pop_num, i].pin_no == pin_num)
                        {
                            genotypes[pop_num, mutate_gene_num].pin_no = pin_num;
                            genotypes[pop_num, i].pin_no = this_pin_num;
                            break;
                        }
                    }


                }


                if (mutate_gene == 1)
                {
                  
                    if (genotypes[pop_num, mutate_gene_num].sig_type == 0)
                        genotypes[pop_num, mutate_gene_num].sig_type = 1;
                    else
                        genotypes[pop_num, mutate_gene_num].sig_type = 0;
                }
                if (mutate_gene == 2)
                {
                   

                    if (genotypes[pop_num, mutate_gene_num].amplitude == 0)
                        genotypes[pop_num, mutate_gene_num].amplitude = 1;
                    else
                    {

                        genotypes[pop_num, mutate_gene_num].amplitude = 0;

                    
                    }
                }
                int mutate3;
                if (mutate_gene == 3)
                {
                   

                    mutate3 = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                    while (mutate3 == genotypes[pop_num, mutate_gene_num].frequency)
                    {
                        mutate3 = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);

                    }

                    genotypes[pop_num, mutate_gene_num].frequency = mutate3;

                

                }
                if (mutate_gene == 4)
                {
                   

                    mutate3 = RandomSource.RNG.Next(0, 100);
                    while (mutate3 == genotypes[pop_num, mutate_gene_num].cycle)
                    {
                        mutate3 = RandomSource.RNG.Next(0, 100);

                    }

                    genotypes[pop_num, mutate_gene_num].cycle = mutate3;

                   

                }
                if (mutate_gene == 5)
                {
                  

                    mutate3 = RandomSource.RNG.Next(1, 10);
                    while (mutate3 == genotypes[pop_num, mutate_gene_num].phase)
                    {
                        mutate3 = RandomSource.RNG.Next(1, 10);

                    }

                    genotypes[pop_num, mutate_gene_num].phase = mutate3;

                  

                }

             
                //Loads mutated genotype in pins

                this.load_pins(pop_num);



            }

            private void load_pins(int pop_num)
            {
                //First 4  pins are input pins
                for (int i = 0; i < num_input; i++)
                {

                    this.InputPins[i] = genotypes[pop_num, i].pin_no;

                }

                //NextNext 5 pins are configuration voltages
                for (int i = 0; i < this.Genotype.Count; i++)
                {
                    this.Genotype[i].Pin = genotypes[pop_num, num_input + i].pin_no;
                    this.Genotype[i].Amplitude = genotypes[pop_num, num_input + i].amplitude;
                    this.Genotype[i].Frequency = genotypes[pop_num, num_input + i].frequency;
                    this.Genotype[i].CycleTime = genotypes[pop_num, num_input + i].cycle;
                    this.Genotype[i].Phase = genotypes[pop_num, num_input + i].phase;
                    if (genotypes[pop_num, num_input + i].sig_type == 0)
                        this.Genotype[i].OperationType = emSequenceOperationType.CONSTANT;
                    else
                        this.Genotype[i].OperationType = emSequenceOperationType.PREDEFINED;

                }

                //Last 3 pins are outputs

                for (int i = 0; i < num_output; i++)
                {

                    this.ListenPins[i] = genotypes[pop_num, num_input + this.Genotype.Count + i].pin_no;

                }


            }

            //Copies genotypes of pin configurations from one individual to another
            public Individual Clone(int pop_num, int copy_pop_num)
            {
                Individual NewInd = new Individual();
                NewInd.TrainingFitness = this.TrainingFitness;
                NewInd.correctTrainingClass = this.correctTrainingClass;
                //    NewInd.TestingFitness = this.TestingFitness;
                NewInd.EvaluationIndex = this.EvaluationIndex;
                NewInd.Report.AddRange(this.Report);
                NewInd.Genotype = new List<emSequenceItem>();
                int start_index = num_input;
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

                    genotypes[pop_num, start_index + i].pin_no = genotypes[copy_pop_num, start_index + i].pin_no;
                    genotypes[pop_num, start_index + i].amplitude = genotypes[copy_pop_num, start_index + i].amplitude;
                    genotypes[pop_num, start_index + i].frequency = genotypes[copy_pop_num, start_index + i].frequency;
                    genotypes[pop_num, start_index + i].cycle = genotypes[copy_pop_num, start_index + i].cycle;
                    genotypes[pop_num, start_index + i].phase = genotypes[copy_pop_num, start_index + i].phase;


                    genotypes[pop_num, start_index + i].sig_type = genotypes[copy_pop_num, start_index + i].sig_type;
                }
                NewInd.Output = this.Output;
                Array.Copy(this.ListenPins, NewInd.ListenPins, this.ListenPins.Length);
                Array.Copy(this.InputPins, NewInd.InputPins, this.InputPins.Length);

             
                start_index = 0;
                for (int i = 0; i < num_input; i++)
                {
                    NewInd.InputPins[i] = this.InputPins[i];

                    genotypes[pop_num, start_index + i].pin_no = genotypes[copy_pop_num, start_index + i].pin_no;
                    genotypes[pop_num, start_index + i].amplitude = genotypes[copy_pop_num, start_index + i].amplitude;
                    genotypes[pop_num, start_index + i].frequency = genotypes[copy_pop_num, start_index + i].frequency;
                    genotypes[pop_num, start_index + i].cycle = genotypes[copy_pop_num, start_index + i].cycle;
                    genotypes[pop_num, start_index + i].phase = genotypes[copy_pop_num, start_index + i].phase;


                    genotypes[pop_num, start_index + i].sig_type = genotypes[copy_pop_num, start_index + i].sig_type;
                }


                start_index = num_input + this.Genotype.Count;

                for (int i = 0; i < num_output; i++)
                {
                    NewInd.ListenPins[i] = this.ListenPins[i];
                    genotypes[pop_num, start_index + i].pin_no = genotypes[copy_pop_num, start_index + i].pin_no;
                    genotypes[pop_num, start_index + i].amplitude = genotypes[copy_pop_num, start_index + i].amplitude;
                    genotypes[pop_num, start_index + i].frequency = genotypes[copy_pop_num, start_index + i].frequency;
                    genotypes[pop_num, start_index + i].cycle = genotypes[copy_pop_num, start_index + i].cycle;
                    genotypes[pop_num, start_index + i].phase = genotypes[copy_pop_num, start_index + i].phase;


                    genotypes[pop_num, start_index + i].sig_type = genotypes[copy_pop_num, start_index + i].sig_type;
                  
                }


                return NewInd;
            }

          
        }

        public class Population
        {
            public List<Individual> Individuals = null;

          
            public Individual getBest() //Gets best individual depending on training fitness
            {
                Individual Best = new Individual();
                double max_fit = 0.0;
                for (int i = 0; i < IndividualAndPopulationFactory.PopulationSize; i++)
                {
                    if (max_fit <= this.Individuals[i].TrainingFitness)
                    {
                        max_fit = this.Individuals[i].TrainingFitness;
                        Best = this.Individuals[i].Clone(0, i);
                    }

                }
                return Best;
            }

            //Copies best individual to other individuals and then mutates those individuals
            public void Simple1PlusNEvoStrat(double MutationRate)
            {
                // this.Sort();
                /// Individual Best = this.Individuals[0];
                Individual Best = getBest();
                this.Individuals.Clear(); this.Individuals.Add(Best);
                for (int i = 0; i < IndividualAndPopulationFactory.PopulationSize - 1; i++)
                {
                    Individual Child = Best.Clone(i + 1, 0);
                    Child.Mutate(i + 1);
                    this.Individuals.Add(Child);
                }
            }
        }


        //Class for storing data
        public class ClassificationInstance
        {
            public int[] param = new int[num_input];

            public int expected_class;


            public ClassificationInstance(int[] param1, int expected_class)
            {
                for (int i = 0; i < num_input; i++)
                    this.param[i] = param1[i];
                this.expected_class = expected_class;
            }

            //Calculating the result, but it is not used now
            public int result(double percentage)
            {
                int predicted = 0;
                if (percentage <= 33.33)
                    predicted = 1;
                if (percentage > 33.33 && percentage <= 66.66)
                    predicted = 2;
                if (percentage > 66.66)
                    predicted = 3;
                return predicted;
            }


               //Another way of calculating the result, but it is not used now
            public int result(FitnessFunction fitnessFunction)
            {
                int predicted = 0;
               

                if (fitnessFunction.results[0] == 1)
                    predicted = 1;
                else if (fitnessFunction.results[1] == 1)
                    predicted = 2;
                else if (fitnessFunction.results[2] == 1)
                    predicted = 3;
                return predicted;
            }
        }

        //Class for contact lense
        public class ClassificationContactLense
        {
            public int[] param = new int[num_input];

            public int expected_class;


            public ClassificationContactLense(int[] param1, int expected_class)
            {
                for (int i = 0; i < num_input; i++)
                    this.param[i] = param1[i];
                this.expected_class = expected_class;
            }

             //Calculating the result, but it is not used now
            public int result(double percentage)
            {
                int predicted = 0;
                if (percentage <= 33.33)
                    predicted = 1;
                if (percentage > 33.33 && percentage <= 66.66)
                    predicted = 2;
                if (percentage > 66.66)
                    predicted = 3;
                return predicted;
            }


               //Another way of calculating the result, but it is not used now
            public int result(FitnessFunction fitnessFunction)
            {
                int predicted = 0;
               

                if (fitnessFunction.results[0] == 1)
                    predicted = 1;
                else if (fitnessFunction.results[1] == 1)
                    predicted = 2;
                else if (fitnessFunction.results[2] == 1)
                    predicted = 3;
                return predicted;
            }
        }

       
       
       //Reading data
      
        public class FitnessFunction
        {
            public ulong EvaluationCounter = 0;
            public emEvolvableMotherboard.Client Motherboard = null;

           
          
            public List<ClassificationInstance> TrainingTestCases = new List<ClassificationInstance>();
            public List<ClassificationInstance> TestingTestCases = new List<ClassificationInstance>();

        

            private double bestFitness;

            private Thread _TestPopulationThread = null;
            private Population PopToTest = null;
            public int[] results = new int[num_output];


            public FitnessFunction()
            {
               
                this.Motherboard = emUtilities.Connect();
                this.Motherboard.ping();

                string tr_path="", te_path="";

                switch(dataset)
                {
                    case 1:
                       tr_path="C:\\ContactLense\\Training.txt";
                       te_path="C:\\ContactLense\\Testing.txt";
                        break;

                    case 2:
                          tr_path="C:\\Iris\\Training.txt";
                          te_path="C:\\Iris\\Testing.txt";
                        break;

                }
              
                string[] training_data = File.ReadAllLines(tr_path);
                string[] testing_data = File.ReadAllLines(te_path);
                for (int r = 0; r < training_data.Length; r++)
                {
                    string[] words = training_data[r].Split(',');
                    int[] param = new int[words.Length];
                    int i = 0;
                    if (words[0].Trim() != "")
                    {
                        foreach (string word in words)
                        {
                            switch(dataset)
                            {
                                case 1:
                                     param[i] =int.Parse(word);
                                    break;
                                case 2:
                                    param[i] = (int)(10.0*double.Parse(word));//Made Iris input variables integer and multiplied by 10
                                    break;
                            }
                            i++;

                        }

                        TrainingTestCases.Add(new ClassificationInstance(param, int.Parse(words[words.Length - 1])));
                      
                    }
                   
                }

                for (int r = 0; r < testing_data.Length; r++)
                {
                    string[] words = testing_data[r].Split(',');
                    int[] param = new int[words.Length];
                    int i = 0;
                    if (words[0].Trim() != "")
                    {

                        foreach (string word in words)
                        {
                             switch(dataset)
                             {
                                case 1:
                                     param[i] =int.Parse(word);
                                    break;
                                case 2:
                                    param[i] = (int)(10.0*double.Parse(word));
                                    break;
                            }
                            i++;
                        }

                        TestingTestCases.Add(new ClassificationInstance(param, int.Parse(words[words.Length - 1])));
                       
                    }
                }

               
            }



            public void TestPopulationThread()
            {
                if (ignoreFirst == false)
                    TestIndividual(PopToTest.Individuals[0]);

                for (int i = 1; i < PopToTest.Individuals.Count; i++)
                {
                    TestIndividual(PopToTest.Individuals[i]);
                }
            }

            public void TestPopulation(Population Pop)
            {
                this.PopToTest = Pop;
                _TestPopulationThread = new Thread(TestPopulationThread);
                _TestPopulationThread.Start();
                _TestPopulationThread.Join();
            }

            public void TestPopulationThread_test()
            {
                if (ignoreFirst == false)
                    TestIndividual(PopToTest.Individuals[0]);

                for (int i = 1; i < PopToTest.Individuals.Count; i++)
                {
                    TestIndividual_test(PopToTest.Individuals[i]);
                }
            }

            public void TestPopulation_test(Population Pop)
            {
                this.PopToTest = Pop;
                _TestPopulationThread = new Thread(TestPopulationThread_test);
                _TestPopulationThread.Start();
                _TestPopulationThread.Join();
            }
            public void TestIndividual(Individual Ind)
            {
                
                Ind.TrainingFitness = 0.0;
                Ind.correctTrainingClass = 0;
                Ind.EvaluationIndex = this.EvaluationCounter++;
                double TP = 0.0, TN = 0.0, FP = 0.0, FN = 0.0;

                //Schedule tones

                long FinalTime = 0;
                long StartTime = 0;
                long TestCaseLength = IndividualAndPopulationFactory.MaxTime;
                FinalTime = IndividualAndPopulationFactory.MaxTime;
                //    ConfusionMatrix CM = new ConfusionMatrix();
                int Index = 0;

                //Executes for each instance of trainind dataset
             
                foreach (ClassificationInstance testCase in TrainingTestCases) 
                {

                    this.Motherboard.reset();
                    this.Motherboard.clearSequences();

                    Index++;
                    Ind.OutputedSequences = new List<emSequenceItem>();
                    List<int> PinsUsed = new List<int>();
                    for (int i = 0; i < num_input; i++)
                    {
                        emSequenceItem Item = new emSequenceItem();
                        Item.StartTime = 1;//  StartTime;
                        Item.EndTime = FinalTime; ;// StartTime + TestCaseLength;
                        Item.Pin = Ind.InputPins[i];
                        Item.Amplitude = 1;
                        Item.OperationType = emSequenceOperationType.PREDEFINED;
                        Item.WaveFormType = emWaveFormType.PWM;
                        Item.CycleTime = 50;
                        //Input mapping
                        Item.Frequency = (10000 - 500) * testCase.param[i] / (max[i] - min[i]) + ((500 * max[i] - 10000 * min[i]) / (max[i] - min[i]));

                        if (Item.Frequency < 500)
                            Item.Frequency = 500;
                        
                        this.Motherboard.appendSequenceAction(Item);
                        Ind.OutputedSequences.Add(Item);


                        FinalTime = Item.EndTime;
                        StartTime = Item.EndTime;

                    }
                    for (int i = 0; i < num_input; i++)
                    {
                        emSequenceItem Item = new emSequenceItem();
                        Item.StartTime = FinalTime;
                        Item.EndTime = FinalTime + 1;
                        Item.Pin = Ind.InputPins[i];
                        Item.OperationType = emSequenceOperationType.emNULL;
                        this.Motherboard.appendSequenceAction(Item);
                        Ind.OutputedSequences.Add(Item);
                    }


                    foreach (int p in Ind.InputPins)
                        PinsUsed.Add(p);

                    for (int i = 0; i < num_output; i++)
                    {
                        //Set up where we read back the values from the EM
                        emSequenceItem RecordItem = new emSequenceItem();
                        RecordItem.StartTime = 0;
                        RecordItem.EndTime = IndividualAndPopulationFactory.MaxTime;
                        RecordItem.Pin = Ind.ListenPins[i];
                        RecordItem.Frequency = 25000;
                        RecordItem.OperationType = emSequenceOperationType.RECORD;


                        if (PinsUsed.Contains(Ind.ListenPins[i])) throw new Exception("Pin used multiple times!");
                        PinsUsed.Add(RecordItem.Pin);


                        this.Motherboard.appendSequenceAction(RecordItem);
                        Ind.OutputedSequences.Add(RecordItem);
                    }




                    //Set up the individuals' config instructions
                    for (int i = 0; i < Ind.Genotype.Count; i++)
                    {

                        if (PinsUsed.Contains(Ind.Genotype[i].Pin)) throw new Exception("Pin used multiple times!");
                        PinsUsed.Add(Ind.Genotype[i].Pin);
                        this.Motherboard.appendSequenceAction(Ind.Genotype[i]);
                        Ind.OutputedSequences.Add(Ind.Genotype[i]);

                    }


                    //Run the test, wait for the mobo to let us know it has finished
                    this.Motherboard.runSequences();
                    double percentage = 0.0;

                    //Analyse the data
                    emWaveForm[] RecordedSignal = new emWaveForm[num_output];
                    double max_freq = 0.0;
                    int max_freq_index = 0, start_index = -1;
                    double max_average_gap = 0.0;
                    double[] average_gaps = new double[num_output];
                    int predicted = 1;
                    for (int o = 0; o < num_output; o++)
                    {
                        RecordedSignal[o] = this.Motherboard.getRecording(Ind.ListenPins[o]);
                        if (RecordedSignal[o] == null) throw new Exception("Failed to get recorded signal");
                        if (RecordedSignal[o].Samples.Count == 0 || RecordedSignal[o].SampleCount != RecordedSignal[o].Samples.Count)
                            Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal[o].SampleCount + " " + RecordedSignal[o].Samples.Count);
                        if (RecordedSignal[o].Samples.Count > 300000)
                        {
                            Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal[o].SampleCount + " " + RecordedSignal[o].Samples.Count);
                            Ind.Output[o] = RecordedSignal[o];

                            return;
                        }


                       //Gets the summation of consecutive transition gaps
                        int sum_gap = 0, gap_count = 0;
                        start_index = -1;
                        for (int i = 0; i < RecordedSignal[o].SampleCount - 1; i++)
                        {
                            if (RecordedSignal[o].Samples[i] == 0 && RecordedSignal[o].Samples[i + 1] == 1)
                            {
                                if (start_index == -1)
                                    start_index = i;
                                else
                                {
                                    sum_gap += (i - start_index);
                                    start_index = i;
                                }

                                gap_count++;
                            }

                        }

                        if (gap_count < 2)
                            average_gaps[o] = 0;
                        else
                            average_gaps[o] = sum_gap / gap_count;

                        //Predict class based on maximum average gap
                        if (max_average_gap < average_gaps[o])
                        {
                            max_average_gap = average_gaps[o];
                            predicted = o + 1;
                        }
                    }

                    int Expected = testCase.expected_class;
                   
                    if (Expected == predicted)
                    {
                        TP = TP + 1.0; //For correctly predicted class
                        TN += 2.0;// For correctly not predicting other two classes
                        Ind.correctTrainingClass++;
                    }
                    else
                    {
                        FN += 1.0; //For incorrectly not predicting the right class
                        FP += 1.0;// For incorrectly predicted wrong class
                        TN += 1.0; // For correctly not predicting the other class
                    }

                   

                }

                Ind.TrainingFitness = (TP / (TP + FP)) * (TN / (TN + FN)); // Calculating training fitness

            }

            //Calculating testing fitness same as training fitness
            public void TestIndividual_test(Individual Ind)
            {
               
                Ind.EvaluationIndex = this.EvaluationCounter++;

                Ind.TestingFitness = 0.0;
                Ind.correctTestingClass = 0;
                //Schedule tones

                long FinalTime = 0;
                long StartTime = 0;
                long TestCaseLength = IndividualAndPopulationFactory.MaxTime;
                FinalTime = IndividualAndPopulationFactory.MaxTime;
                //    ConfusionMatrix CM = new ConfusionMatrix();
                int Index = 0;

                double TP = 0.0, TN = 0.0, FP = 0.0, FN = 0.0;
               
                foreach (ClassificationInstance testCase in TestingTestCases)
                {

                    //   foreach (ClassificationCancer testCase in TrainingTestCases){
                    this.Motherboard.reset();
                    this.Motherboard.clearSequences();

                    Index++;
                    Ind.OutputedSequences = new List<emSequenceItem>();
                    List<int> PinsUsed = new List<int>();
                    for (int i = 0; i < num_input; i++)
                    {
                        emSequenceItem Item = new emSequenceItem();
                        Item.StartTime = 1;//  StartTime;
                        Item.EndTime = FinalTime; ;// StartTime + TestCaseLength;
                        Item.Pin = Ind.InputPins[i];
                        Item.Amplitude = 1;
                        Item.OperationType = emSequenceOperationType.PREDEFINED;
                        Item.WaveFormType = emWaveFormType.PWM;
                        Item.CycleTime = 50;
                        Item.Frequency = (10000 - 500) * testCase.param[i] / (max[i] - min[i]) + ((500 * max[i] - 10000 * min[i]) / (max[i] - min[i]));

                        if (Item.Frequency < 500)
                            Item.Frequency = 500;
                     
                        this.Motherboard.appendSequenceAction(Item);
                        Ind.OutputedSequences.Add(Item);


                        FinalTime = Item.EndTime;
                        StartTime = Item.EndTime;

                    }
                    for (int i = 0; i < num_input; i++)
                    {
                        emSequenceItem Item = new emSequenceItem();
                        Item.StartTime = FinalTime;
                        Item.EndTime = FinalTime + 1;
                        Item.Pin = Ind.InputPins[i];
                        Item.OperationType = emSequenceOperationType.emNULL;
                        this.Motherboard.appendSequenceAction(Item);
                        Ind.OutputedSequences.Add(Item);
                    }


                    foreach (int p in Ind.InputPins)
                        PinsUsed.Add(p);

                    for (int i = 0; i < num_output; i++)
                    {
                        //Set up where we read back the values from the EM
                        emSequenceItem RecordItem = new emSequenceItem();
                        RecordItem.StartTime = 0;
                        RecordItem.EndTime = IndividualAndPopulationFactory.MaxTime;
                        RecordItem.Pin = Ind.ListenPins[i];
                        RecordItem.Frequency = 25000;
                        RecordItem.OperationType = emSequenceOperationType.RECORD;


                        if (PinsUsed.Contains(Ind.ListenPins[i])) throw new Exception("Pin used multiple times!");
                        PinsUsed.Add(RecordItem.Pin);


                        this.Motherboard.appendSequenceAction(RecordItem);
                        Ind.OutputedSequences.Add(RecordItem);
                    }




                    //Set up the individuals' config instructions
                    for (int i = 0; i < Ind.Genotype.Count; i++)
                    {

                        if (PinsUsed.Contains(Ind.Genotype[i].Pin)) throw new Exception("Pin used multiple times!");
                        PinsUsed.Add(Ind.Genotype[i].Pin);
                        this.Motherboard.appendSequenceAction(Ind.Genotype[i]);
                        Ind.OutputedSequences.Add(Ind.Genotype[i]);

                    }


                    //Run the test, wait for the mobo to let us know it has finished
                    this.Motherboard.runSequences();
                    double percentage = 0.0;

                    //Analyse the data
                    emWaveForm[] RecordedSignal = new emWaveForm[num_output];
                    double max_freq = 0.0;
                    int max_freq_index = 0;
                    double max_average_gap = 0.0;
                    double[] average_gaps = new double[num_output];
                    int predicted = 1;
                    for (int o = 0; o < num_output; o++)
                    {
                        RecordedSignal[o] = this.Motherboard.getRecording(Ind.ListenPins[o]);
                        if (RecordedSignal[o] == null) throw new Exception("Failed to get recorded signal");
                        if (RecordedSignal[o].Samples.Count == 0 || RecordedSignal[o].SampleCount != RecordedSignal[o].Samples.Count)
                            Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal[o].SampleCount + " " + RecordedSignal[o].Samples.Count);
                        if (RecordedSignal[o].Samples.Count > 300000)
                        {
                            Reporting.Say("WARNING!\tBuffer size = " + RecordedSignal[o].SampleCount + " " + RecordedSignal[o].Samples.Count);
                            Ind.Output[o] = RecordedSignal[o];

                            return;
                        }


                        int sum_gap = 0, gap_count = 0;
                        int start_index = -1;
                        for (int i = 0; i < RecordedSignal[o].SampleCount - 1; i++)
                        {
                            if (RecordedSignal[o].Samples[i] == 0 && RecordedSignal[o].Samples[i + 1] == 1)
                            {
                                if (start_index == -1)
                                    start_index = i;
                                else
                                {
                                    sum_gap += (i - start_index);
                                    start_index = i;
                                }

                                gap_count++;
                            }

                        }

                        if (gap_count < 2)
                            average_gaps[o] = 0;
                        else
                            average_gaps[o] = sum_gap / gap_count;

                        if (max_average_gap < average_gaps[o])
                        {
                            max_average_gap = average_gaps[o];
                            predicted = o + 1;
                        }

                    }

                  
                    int Expected = testCase.expected_class;
                 
                    if (Expected == predicted)
                    {
                        TP += 1.0;
                        TN += 2.0;
                        Ind.correctTestingClass++;
                    }
                    else
                    {
                        TN += 1.0;
                        FP += 1.0;
                        FN += 1.0;
                    }


                }
              

                Ind.TestingFitness = (TP / (TP + FP)) * (TN / (TN + FN));

                string path_name = "";

                switch (dataset)
                {
                    case 1:
                        path_name = "C:\\ContactLense\\result" + run.ToString() + ".txt";
                        break;
                    case 2:
                        path_name = "C:\\Iris\\result" + run.ToString() + ".txt";
                        break;
                }
                using (StreamWriter w = File.AppendText(path_name))
                {

                    w.WriteLine("Testing Fitness: " + Ind.TestingFitness.ToString() + " correct class: " + Ind.correctTestingClass.ToString());
                }


            }



        }


        public class IndividualAndPopulationFactory
        {
            public static int PopulationSize = 5;
            public static int ItemsInGenotype = 12 - num_input - num_output;
            public static int[] AvailablePins = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
            public static int MaxTime = 128;
            public static int MaxAmplitude = 1;
            public static int MaxFrequency = 10000;
            public static int MinFrequency = 500;
            public static int MaxPhase = 10;

            
        
            public static Individual RandomIndividual(int pop_num)
            {
                ItemsInGenotype = 12 - num_input - num_output;
                Individual Ind = new Individual();

                Ind.Genotype = new List<emSequenceItem>();
                List<int> UnusedPins = new List<int>();
                UnusedPins.AddRange(AvailablePins);
                int start_index = num_input + ItemsInGenotype;


                for (int pin = 0; pin < Ind.ListenPins.Length; pin++)
                {

                    Ind.ListenPins[pin] = UnusedPins[RandomSource.RNG.Next(0, UnusedPins.Count)];
                    UnusedPins.Remove(Ind.ListenPins[pin]);
                    genotypes[pop_num, start_index + pin].pin_no = Ind.ListenPins[pin];



                    genotypes[pop_num, start_index + pin].sig_type = RandomSource.RNG.NextDouble() < 0.5 ? 0 : 1;
                    genotypes[pop_num, start_index + pin].amplitude = RandomSource.RNG.Next(0, 1);

                    if (pop_num == 0)
                        genotypes[pop_num, start_index + pin].sig_type = 1;

                    if (genotypes[pop_num, start_index + pin].sig_type == 1)
                        genotypes[pop_num, start_index + pin].amplitude = 1;

                    genotypes[pop_num, start_index + pin].frequency = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                    genotypes[pop_num, start_index + pin].phase = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPhase);
                    genotypes[pop_num, start_index + pin].cycle = RandomSource.RNG.Next(0, 10);


                }
                start_index = 0;
                for (int pin = 0; pin < Ind.InputPins.Length; pin++)
                {
                    Ind.InputPins[pin] = UnusedPins[RandomSource.RNG.Next(0, UnusedPins.Count)];
                    UnusedPins.Remove(Ind.InputPins[pin]);

                    genotypes[pop_num, start_index + pin].pin_no = Ind.InputPins[pin];



                    genotypes[pop_num, start_index + pin].sig_type = RandomSource.RNG.NextDouble() < 0.5 ? 0 : 1;
                    genotypes[pop_num, start_index + pin].amplitude = RandomSource.RNG.Next(0, 1);

                    if (pop_num == 0)
                        genotypes[pop_num, start_index + pin].sig_type = 1;

                    if (genotypes[pop_num, start_index + pin].sig_type == 1)
                        genotypes[pop_num, start_index + pin].amplitude = 1;

                    genotypes[pop_num, start_index + pin].frequency = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                    genotypes[pop_num, start_index + pin].phase = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPhase);
                    genotypes[pop_num, start_index + pin].cycle = RandomSource.RNG.Next(0, 10);

                }
                start_index = num_input;
                for (int i = 0; i < ItemsInGenotype; i++)
                {
                    emSequenceItem Item = new emSequenceItem();


                    Item.Pin = UnusedPins[RandomSource.RNG.Next(0, UnusedPins.Count)];
                    UnusedPins.Remove(Item.Pin);
                    Item.StartTime = 0;
                    Item.EndTime = IndividualAndPopulationFactory.MaxTime;


                    // Item.OperationType = emSequenceOperationType.CONSTANT;// RandomSource.RNG.NextDouble() < 0.5 ? emSequenceOperationType.CONST : emSequenceOperationType.PREDEFINED;
                    Item.OperationType = RandomSource.RNG.NextDouble() < 0.5 ? emSequenceOperationType.CONSTANT : emSequenceOperationType.PREDEFINED;
                    Item.Amplitude = RandomSource.RNG.Next(0, IndividualAndPopulationFactory.MaxAmplitude);
                    Item.WaveFormType = emWaveFormType.PWM;
                    if (Item.OperationType == emSequenceOperationType.PREDEFINED)
                        Item.Amplitude = IndividualAndPopulationFactory.MaxAmplitude;

                    Item.Frequency = RandomSource.RNG.Next(IndividualAndPopulationFactory.MinFrequency, IndividualAndPopulationFactory.MaxFrequency);
                    Item.Phase = RandomSource.RNG.Next(1, IndividualAndPopulationFactory.MaxPhase);
                    Item.CycleTime = RandomSource.RNG.Next(0, 100);
                    Ind.Genotype.Add(Item);

                    //  genotypes[ pop_num, i] = new gene_item();
                    genotypes[pop_num, start_index + i].pin_no = Item.Pin;
                    genotypes[pop_num, start_index + i].amplitude = Item.Amplitude;
                    genotypes[pop_num, start_index + i].frequency = Item.Frequency;
                    genotypes[pop_num, start_index + i].cycle = Item.CycleTime;
                    genotypes[pop_num, start_index + i].phase = Item.Phase;

                    if (Item.OperationType == emSequenceOperationType.CONSTANT)
                        genotypes[pop_num, start_index + i].sig_type = 0;
                    else
                        genotypes[pop_num, start_index + i].sig_type = 1;


                }


                return Ind;
            }

            public static Population RandomPopulation(int Size)
            {
                Population Pop = new Population();
                Pop.Individuals = new List<Individual>();

                for (int i = 0; i < Size; i++)
                    Pop.Individuals.Add(RandomIndividual(i));

                return Pop;
            }

            public static Population AddIndividuals(List<Individual> BestInds)
            {
                Population Pop = new Population();
                //  Pop.Individuals = BestInds;
                Pop.Individuals = new List<Individual>();

                for (int i = 0; i < BestInds.Count; i++)
                    Pop.Individuals.Add(BestInds[i]);

                return Pop;
            }
        }
        public static void initialize_min_max()
        {
           switch(dataset)
           {
              case 1://Contact lense
                   max=new int[4]{ 3, 2, 2, 2 };
                   min=new int[4] { 1, 1, 1, 1 };
                   break;
               case 2://Iris
                   max = new int[4]{ 79, 44, 69, 25 };
                   min = new int[4]{ 43, 20, 10, 1 };
                   break;
           }
        }
      
        public static void Go(int DataSet, int epoch_size)
        {
            
            DataSet=DataSet;
            initialize_min_max();
            FitnessFunction FitFunc = new FitnessFunction();

            genotypes = new gene_item[IndividualAndPopulationFactory.PopulationSize, 12];

            for (int j = 0; j < IndividualAndPopulationFactory.PopulationSize; j++)
            {
                for (int k = 0; k < 12; k++)
                {
                    genotypes[j, k] = new gene_item();
                }
            }
            for (run = 1; run <= 20; run++)
            {
               
                Population Pop = IndividualAndPopulationFactory.RandomPopulation(IndividualAndPopulationFactory.PopulationSize);
               
                ignoreFirst = false;
                for (int Epoch = 1; Epoch <= epoch_size; Epoch++)
                {
                    // ignoreFirst is used so that after the first generation, the first individual will not be evaluated again. The first one is the best one from last generation
                    if (Epoch > 0)
                        ignoreFirst = true;

                    FitFunc.TestPopulation(Pop);
                    
                    Individual BestInd = Pop.getBest();

                    string path_name="";

                    switch(DataSet)
                    {
                        case 1:
                            path_name="C:\\ContactLense\\result" + run.ToString() + ".txt";
                            break;
                        case 2:
                            path_name="C:\\Iris\\result" + run.ToString() + ".txt";
                            break;

                    }
               //     Reporting.Say(string.Format("STATUS\t{0,-10}\t{1:0.0000}\t{2,-10}\t{3,-10}", Epoch, BestInd.TrainingFitness, BestInd.EvaluationIndex, Timer.ElapsedMilliseconds));
                    using (StreamWriter w = File.AppendText(path_name))
                    {

                      

                        w.WriteLine("Epoch: " + Epoch.ToString() + "Training fitness: " + BestInd.TrainingFitness.ToString() + " correct classes: " + BestInd.correctTrainingClass.ToString());
                        w.WriteLine("Input pins: " + BestInd.InputPins[0].ToString() + " " + BestInd.InputPins[1].ToString() + " " + BestInd.InputPins[2].ToString() + " " + BestInd.InputPins[3].ToString() + " Output pin: " + BestInd.ListenPins[0].ToString());
                        w.WriteLine("Genotypes: ");
                        for (int i = 0; i < BestInd.Genotype.Count; i++)
                        {
                            if (BestInd.Genotype[i].OperationType == emSequenceOperationType.CONSTANT)
                                w.WriteLine("Pin:" + BestInd.Genotype[i].Pin.ToString() + " Operation type: " + BestInd.Genotype[i].OperationType.ToString() + " Amplitude: " + BestInd.Genotype[i].Amplitude.ToString());
                            else
                                w.WriteLine("Pin:" + BestInd.Genotype[i].Pin.ToString() + " Operation type: " + BestInd.Genotype[i].OperationType.ToString() + " Wave form type: " + BestInd.Genotype[i].WaveFormType.ToString() + " Amplitude: " + BestInd.Genotype[i].Amplitude.ToString() + " Frequency: " + BestInd.Genotype[i].Frequency.ToString() + " Phase: " + BestInd.Genotype[i].Phase.ToString() + " CycleTime: " + BestInd.Genotype[i].CycleTime.ToString());
                        }
                        w.WriteLine(" ");

                      
                    }


                  
                    Application.DoEvents();

                  
                    //The evolutionary run will be finished if it reaches final generation number or training fitness becomes 1.0, that means if it predicts all instances of training dataset correctly
                    if (Epoch == epoch_size || BestInd.TrainingFitness == 1.0)
                    {

                        FitFunc.TestIndividual_test(BestInd);//It then runs the instances of testing dataset with the best configuration.
                      
                        break;
                    }
                    Pop.Simple1PlusNEvoStrat(0.8);
                    
                }


            }
         

        }

       
    }
}
