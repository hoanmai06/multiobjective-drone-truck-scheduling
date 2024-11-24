#include "local_search.h"
#include "log.h"

bool is_better(const Fitness& lhs, const Fitness& rhs) {
    return (lhs[0] < rhs[0] && lhs[1] <= rhs[1]) || (lhs[0] <= rhs[0] && lhs[1] < rhs[1]);
}

void first_improvement_permutation_swap_hill_climbing(Individual& individual, const Problem& problem) {
    Fitness current_fitness = fitness(individual, problem);

    for (int i = 0; i < individual.permutation_gene.size(); ++i) {
        for (int j = i + 1; j < individual.permutation_gene.size(); ++j) {
            Individual clone = individual;
            std::swap(clone.permutation_gene[i], clone.permutation_gene[j]);
            if (!is_valid(clone, problem)) continue;

            Fitness clone_fitness = fitness(clone, problem);
            if (is_better(clone_fitness, current_fitness)) {
                current_fitness = std::move(clone_fitness);
                std::swap(individual.permutation_gene[i], individual.permutation_gene[j]);

//                print(current_fitness);
            }
        }
    }
}
