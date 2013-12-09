#ifndef __GA_H__
#define __GA_H__

#include <vector>
#include <bitset>
#include <tuple>
#include <iostream>
#include <random>
#include <algorithm>
#include <cmath>

#include "emEvolvableMotherboard.h"

const int genomeSize = 11 * 16 * 2;
const int resultSize = 2048;

typedef std::bitset<genomeSize> genomeType;

class geneticAlgorithm {
  public:
    //geneticAlgorithm(emEvolvableMotherboardClient& client);

  private:
    //emEvolvableMotherboardClient & client;
};

std::vector<genomeType> ca_run(
    std::string logfile,
    std::vector<genomeType> population, 
    double wantedLambda, 
    int popSize);


double getAverageFitness(std::vector<std::tuple<genomeType, double>> population);

bool sortIndividual(const std::tuple<genomeType, double> &a,
                    const std::tuple<genomeType, double> &b);

std::vector<std::tuple<genomeType, double>> selection(
    std::vector< std::tuple< genomeType, double >> & fitpop);

std::vector<genomeType> mutate(
    std::vector< std::tuple< genomeType, double >>& population,
    int popSize);

double measuredFitness(
    std::bitset<resultSize> individual, 
    double lambda);

std::vector<std::bitset<resultSize>> runPopulation(
    std::vector<genomeType> population);


#endif
