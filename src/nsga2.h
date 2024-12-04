#ifndef TINH_TOAN_TIEN_HOA_NSGA2_H
#define TINH_TOAN_TIEN_HOA_NSGA2_H

#include "problem.h"
#include "genetic.h"

#include <pagmo/pagmo.hpp>

struct GeneticAlgorithmOptions {
    int population_size = 100;
    int max_population_count = 500;

    double crossover_rate = 0.95;
    double mutation_rate = 0.1;

    int max_same_parent_crossover_retry_count = 10;
    int max_create_offspring_retry_count = 10;

    bool force_mutation_on_bad_crossover = true;

    int local_search_period = 10;

    PopulationInitializationAlgorithm initialization = nullptr;
    CrossoverAlgorithm crossover = nullptr;
    MutationAlgorithm mutation = nullptr;
    RepairAlgorithm repair = nullptr;
    IndividualPostprocessingAlgorithm postprocessing = [] (Individual&, const Problem&) {};
    IndividualPostprocessingAlgorithm local_search = [] (Individual&, const Problem&) {};
};

class NSGA2Population {
public:
    Population individual_list;
    std::vector<Fitness> fitness_list;
    std::vector<int> ranks;

    [[nodiscard]] std::size_t size() const {
        return individual_list.size();
    }

    void reserve(std::size_t n) {
        individual_list.reserve(n);
        fitness_list.reserve(n);
        ranks.reserve(n);
    }

    void resize(std::size_t n) {
        individual_list.resize(n);
        fitness_list.resize(n);
        ranks.resize(n);
    }

    void recalculate_rank() {
        auto pagmo_result = pagmo::fast_non_dominated_sorting(fitness_list);
        std::vector<std::vector<std::size_t>> fronts_indices = std::get<0>(pagmo_result);
        for (int front = 0; front < fronts_indices.size(); ++front) {
            for (int index = 0; index < fronts_indices[front].size(); ++index) {
                ranks[index] = front;
            }
        }
    }

    NSGA2Population() = default;

    explicit NSGA2Population(Population&& pop, const Problem& problem) : individual_list(pop) {
        fitness_list.reserve(individual_list.size());
        ranks.resize(individual_list.size());

        for (const Individual& individual : individual_list) {
            fitness_list.push_back(std::move(fitness(individual, problem)));
        }

        recalculate_rank();
    }
};

Population nsga2(const Problem& problem, const GeneticAlgorithmOptions& options);

#endif //TINH_TOAN_TIEN_HOA_NSGA2_H
