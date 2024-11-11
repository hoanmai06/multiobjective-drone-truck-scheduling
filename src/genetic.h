#ifndef TINH_TOAN_TIEN_HOA_GENETIC_H
#define TINH_TOAN_TIEN_HOA_GENETIC_H

#include "problem.h"

#include <functional>
#include <limits>
#include <vector>
#include <utility>

using PermutationGene = std::vector<int>;
using BinaryGene = std::vector<bool>;
using Fitness = std::pair<double, double>;

class Individual {
public:
    const Problem& problem;

    PermutationGene permutation_gene;
    BinaryGene binary_gene;
    Fitness fitness = {std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};

    // Tạo ra một individual rỗng có permutation_gene dài bằng số khách hàng và binary_gene dài bằng số khách hàng trừ 1
    explicit Individual(const Problem& problem) : problem(problem) {
        permutation_gene.resize(problem.customer_count());
        binary_gene.resize(problem.customer_count() - 1);
    }
};

Individual encode(const Solution& solution);
Solution decode(const Individual& individual);

bool is_drone_routes_valid(const Individual& individual);
bool is_valid(const Individual& individual);
Fitness fitness(const Individual& individual);

using CrossoverAlgorithm = std::function<Individual(const Individual&, const Individual&)>;
using MutationAlgorithm = std::function<Individual(const Individual&)>;

struct GeneticAlgorithmOptions {
    double crossover_rate = 0.95;
    double mutation_rate = 0.1;

    CrossoverAlgorithm crossover = nullptr;
    MutationAlgorithm mutation = nullptr;
};

Solution non_dominated_sorting_genetic_algorithm(const Problem& problem, const GeneticAlgorithmOptions& options);

#endif //TINH_TOAN_TIEN_HOA_GENETIC_H
