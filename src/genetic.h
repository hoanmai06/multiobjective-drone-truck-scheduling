#ifndef TINH_TOAN_TIEN_HOA_GENETIC_H
#define TINH_TOAN_TIEN_HOA_GENETIC_H

#include "problem.h"

#include <functional>
#include <limits>
#include <vector>
#include <utility>

using PermutationGene = std::vector<int>;
using BinaryGene = std::vector<bool>;
using Fitness = std::vector<double>;

class Individual {
public:
    PermutationGene permutation_gene;
    BinaryGene binary_gene;

    void resize(int customer_count) {
        permutation_gene.resize(customer_count);
        binary_gene.resize(customer_count - 1);
    }

    Individual() = default;

    explicit Individual(int customer_count) {
        permutation_gene.resize(customer_count);
        binary_gene.resize(customer_count);
    }

    // Tạo ra một individual rỗng có permutation_gene dài bằng số khách hàng và binary_gene dài bằng số khách hàng trừ 1
    explicit Individual(const Problem& problem) {
        permutation_gene.resize(problem.customer_count());
        binary_gene.resize(problem.customer_count() - 1);
    }
};

using Population = std::vector<Individual>;
using PopulationInitializationAlgorithm = std::function<Population(int population_size, const Problem&)>;
using CrossoverAlgorithm = std::function<std::pair<Individual, Individual>(const Individual&, const Individual&)>;
using MutationAlgorithm = std::function<void(Individual&)>;
using RepairAlgorithm = std::function<Individual(Individual&, const Problem&)>;

Individual encode(const Solution& solution);
Solution decode(const Individual& individual, const Problem& problem);

bool is_drone_routes_valid(const Individual& individual, const Problem& problem);
bool is_valid(const Individual& individual, const Problem& problem);
Fitness fitness(const Individual& individual, const Problem& problem);

#endif //TINH_TOAN_TIEN_HOA_GENETIC_H
