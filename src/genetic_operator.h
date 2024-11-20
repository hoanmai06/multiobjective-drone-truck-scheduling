#ifndef TINH_TOAN_TIEN_HOA_GENETIC_OPERATOR_H
#define TINH_TOAN_TIEN_HOA_GENETIC_OPERATOR_H

#include "genetic.h"

Individual create_random_individual(const Problem& problem);
Population create_random_population(int population_size, const Problem& problem);

std::pair<Individual, Individual> crossover(const Individual &parent1,
                                            const Individual &parent2);

void mutation(Individual& individual);

#endif //TINH_TOAN_TIEN_HOA_GENETIC_OPERATOR_H
