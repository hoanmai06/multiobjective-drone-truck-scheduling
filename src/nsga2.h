#ifndef TINH_TOAN_TIEN_HOA_NSGA2_H
#define TINH_TOAN_TIEN_HOA_NSGA2_H

#include "problem.h"
#include "genetic.h"

struct GeneticAlgorithmOptions {
    int population_size = 100;
    int max_population_count = 500;

    double crossover_rate = 0.95;
    double mutation_rate = 0.1;

    PopulationInitializationAlgorithm initialization = nullptr;
    SelectionAlgorithm selection = nullptr;
    CrossoverAlgorithm crossover = nullptr;
    MutationAlgorithm mutation = nullptr;
};

Population nsga2(const Problem& problem, const GeneticAlgorithmOptions& options);

#endif //TINH_TOAN_TIEN_HOA_NSGA2_H
