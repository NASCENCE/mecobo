#include "ga.h"

#include "mecohost.h"


std::vector<std::bitset<resultSize>> runPopulation(std::vector<genomeType> population)
{
  std::vector<std::bitset<resultSize>> results;

  std::vector<FPGA_IO_Pins_TypeDef> outPins = {
      FPGA_F16,
      FPGA_F17,
      FPGA_G14,
      FPGA_G16,
      FPGA_H16,
      FPGA_H17,
      FPGA_H15,
      FPGA_L12,
      FPGA_H14,
      FPGA_K14,
      FPGA_K12
    };
    
    std::vector<FPGA_IO_Pins_TypeDef> inPins = {
      FPGA_J16
    };


    //Setup input pins
    for(FPGA_IO_Pins_TypeDef inPin : inPins) {
      setPin(inPin, 0x1, 0x1, 0x1, 0xFF);  //sample rate ... something reasonable?
    }
 
    for(genomeType individual : population) {
      int pinNum = 0;
      std::bitset<16> duty;
      std::bitset<16> antiduty;
      for(FPGA_IO_Pins_TypeDef pin : outPins) {
        //Build two bit slices for pin config
        int slicePos = pinNum * 32;
        for(int i = 0; i < 16; i++) {
          duty[i] = individual[slicePos + i];
        }
        for(int i = 0; i < 16; i++ ) {
          antiduty[i] = individual[slicePos + 16 + i];
        }

        setPin(pin, duty.to_ulong(), antiduty.to_ulong(), 0x1, 0x0);
        //std::cout << "Setting " << pin << ": " << \
        //  duty.to_ulong() << ": " << \
        //  antiduty.to_ulong() << std::endl;

        startOutput(pin);
        pinNum++;
      }
      for(FPGA_IO_Pins_TypeDef inPin : inPins) {
        startInput(inPin); //flips buffers. The next buffer should now be clean.
      }

      //Collect at least 1024 samples.
      //Only one pin, so it's quite easy.
      struct mecoboDev dev;
      std::vector<sampleValue> allSamples;
      while(allSamples.size() < resultSize) {
        std::vector<sampleValue> s;
        getMecoboStatus(&dev);
        if(dev.bufElements >= 128) {
          getSampleBuffer(s);
          //append sample
          allSamples.insert(allSamples.end(),
                            s.begin(),
                            s.end());
        }
      }
      //Construct a bitset  from the collected samples
      std::bitset<resultSize> b;
      int pos = 0;
      for(sampleValue sv : allSamples) {
        //std::cout << sv.value << std::endl;
        if(pos < 2048)
          b[pos++] = sv.value ? true:false;
      }
      //std::cout << "Result" << b << std::endl;
      results.push_back(b);
    }
  return results;
}

double getAverageFitness(std::vector<std::tuple<genomeType, double>> population) {
  double avgFit = 0.0f;
  for(auto individual : population) {
    avgFit += std::get<1>(individual);
  }
  return avgFit/population.size();
}


bool sortIndividual(const std::tuple<genomeType, double> &a,
                    const std::tuple<genomeType, double> &b) {
  return std::get<1>(a) > std::get<1>(b);
}

std::vector<std::tuple<genomeType, double>> selection(
    std::vector< std::tuple< genomeType, double >>
    & fitpop)
{
  //elitism, return half of the best ones. 
  std::sort(fitpop.begin(), fitpop.end(), sortIndividual);

  std::vector<std::tuple<genomeType, double>> ret;
  for(int i = 0; i < fitpop.size()/2; i++) {
    ret.push_back(fitpop[i]);
  }
  return ret;
}

std::vector<genomeType> mutate(
    std::vector< std::tuple< genomeType, double >>
    & population,
    int popSize)
{
  std::vector<genomeType> ret;

  //Linear scaling of parent selection probability
  //based on fitness.
  //TODO: fails if max - min = 0
  double fitRange = std::get<1>(population.front())-std::get<1>(population.back());

  std::default_random_engine generator;
  
  std::vector<double> intervals = {0.0f, (double)population.size()};
  std::vector<double> weights = {std::get<1>(population.front()), std::get<1>(population.back())};

  std::piecewise_linear_distribution<double> distro(intervals.begin(), intervals.end(), weights.begin());


  std::uniform_int_distribution<int> uniformDistro20p(0,4);
  for(int i = 0; i < popSize; i++) {
    //Do two dice rolls
    auto parentA = std::get<0>(population[(int)distro(generator)]);
    auto parentB = std::get<0>(population[(int)distro(generator)]);
    //We have indices of parents, now 
    //do a 1 point cross-over between them.
    genomeType child;
    int b;
    for(b = 0; b < genomeSize/2; b++) {
      child[b]                = parentA[b];
      child[b+(genomeSize/2)] = parentB[b];
    }

    //Do some mutation as well (20%)
    for(b = 0; b < genomeSize; b++) {
      if(uniformDistro20p(generator) == 0) {
        child[b] = !child[b];
      }
    }

    ret.push_back(child);
  }


  return ret;
}

double measuredFitness(std::bitset<resultSize> individual, double lambda) {
  int pos = 0;
  double quint = 0;
  for(int i = 0; i < resultSize-1; i+=2) {
    if(!individual[i] && !individual[i+1]) {
      quint++;
    }
  }
  //std::cout << quint << std::endl;
  //4^5 = 1024 
  double measuredLambda = (1024.0f-quint)/1024.0f;
  double fitness =  (1.0f - std::abs(lambda - measuredLambda));
  return fitness;
}

std::vector<genomeType> ca_run(
    std::vector<genomeType> population, 
    double wantedLambda, int popSize)
{
  std::vector<std::bitset<resultSize>> res;
  //Run! Run! Run!
  std::vector<double> generationFitness;
  bool terminate = false;
  int gen = 0;
  double lastAvg = 0.0f;
  std::tuple<genomeType, double> allTimeBest;
  while(!terminate) {
    //Run population
    res = runPopulation(population);
    //Measure fitness for run
    std::vector<double> fitness;
    for(std::bitset<resultSize> result : res) {
      fitness.push_back(measuredFitness(result, wantedLambda));
    }

    //Make <genome, fitness> tuples. 
    std::vector<std::tuple<genomeType, double>> fitPop;
    int p = 0;
    for(double f : fitness) {
      fitPop.push_back(std::make_tuple(population[p++], f));
    }

    //Select the best individuals
    auto bestPop = selection(fitPop);

    //Check if the best from the selection (selection returns sorted best -> worst)
    if (std::get<1>(allTimeBest) < std::get<1>(bestPop.front())) {
      allTimeBest = bestPop.front();
    }
     
    //Find average fitness increase
    double avgFit = getAverageFitness(bestPop);

    //Store the average fitness, used for termincation
    generationFitness.push_back(avgFit);

    std::cout <<    gen++                     << " " << \
      "Avg: " <<    generationFitness.back() << " " << \
      "Low: " <<    std::get<1>(bestPop.back()) << " " <<\
      "High: "<<    std::get<1>(bestPop.front()) << " " <<\
      "ATB: " <<    std::get<1>(allTimeBest) <<  \
      std::endl;

    //Check if average fitness increase over the last 10 generations is very small,
    //if so, we are done. But only if we have enough generations.
    int maxGen = 10;
    if(generationFitness.size() > maxGen) {
      double avg = 0.0f;
      for(int i = generationFitness.size()-maxGen; i < generationFitness.size(); i++) {
        avg += generationFitness[i];
      }
      avg = avg/(double)maxGen;
      double diff = lastAvg - avg;
      if((diff >= 0.0f) && (diff <= 0.0001f)) {
        terminate = true;
      }
      lastAvg = avg;
    }

    //Mutate
    auto newPop = mutate(bestPop, popSize);
    //New population!
    population = newPop;
  }

  //Finish up.
  std::cout << "All time best individual: " << std::get<0>(allTimeBest) << std::endl;
  std::cout << "Fitness: " << std::get<1>(allTimeBest) << std::endl;
  return population;
}



