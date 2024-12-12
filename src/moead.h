#ifndef TINH_TOAN_TIEN_HOA_MOEAD_H
#define TINH_TOAN_TIEN_HOA_MOEAD_H

#include "genetic.h"

#include <limits>
#include <numeric>

inline double euclid_2d(const std::vector<double>& lhs, const std::vector<double>& rhs) {
    return std::sqrt((lhs[0] - rhs[0])*((lhs[0] - rhs[0])) + (lhs[1] - rhs[1])*((lhs[1] - rhs[1])));
}

using WeightsInitializationAlgorithm = std::function<std::vector<std::vector<double>>(std::size_t population_size)>;

struct MOEADOptions {
    int population_size = 100;
    int neighbor_count = 10;
    int max_population_count = 500;

    int max_same_parent_crossover_retry_count = 10;
    bool force_mutation_on_bad_crossover = true;

    double crossover_rate = 0.95;
    double mutation_rate = 0.1;

    int maximum_number_of_replaced_solution_each_child = population_size;
    double select_parent_from_whole_population_probability = 0.0;

    PopulationInitializationAlgorithm initialization = nullptr;
    WeightsInitializationAlgorithm generate_weights = nullptr;
    CrossoverAlgorithm crossover = nullptr;
    MutationAlgorithm mutation = nullptr;
    RepairAlgorithm repair = nullptr;
    IndividualPostprocessingAlgorithm postprocessing = [] (Individual&, const Problem&) {};
    IndividualPostprocessingAlgorithm local_search = [] (Individual&, const Problem&) {};
};

class MOEADPopulation {
public:
    Population individual_list;
    std::vector<Fitness> fitness_list;
    std::vector<std::vector<double>> weights;
    std::vector<std::vector<std::size_t>> neighbor_indices;
    Population external_population;
    std::vector<Fitness> external_population_fitness;
    std::vector<double> reference_point;

    [[nodiscard]] std::size_t size() const {
        return individual_list.size();
    }

    explicit MOEADPopulation(const Problem& problem, const MOEADOptions& options) {
        individual_list = options.initialization(options.population_size, problem);
        fitness_list.resize(options.population_size);
        weights = options.generate_weights(options.population_size);

        for (int i = 0; i < individual_list.size(); ++i) {
            if (!is_valid(individual_list[i], problem)) options.repair(individual_list[i], problem);
            fitness_list[i] = fitness(individual_list[i], problem);
        }

        reference_point = {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()};
        for (const Fitness& fitness : fitness_list) {
            if (fitness[0] < reference_point[0]) reference_point[0] = fitness[0];
            if (fitness[1] < reference_point[1]) reference_point[1] = fitness[1];
        }

        // Khởi tạo tập hàng xóm
        neighbor_indices.resize(weights.size());
        for (std::size_t i = 0; i < weights.size(); ++i) {
            neighbor_indices[i].resize(weights.size());
            std::iota(neighbor_indices[i].begin(), neighbor_indices[i].end(), 0);

            std::sort(neighbor_indices[i].begin(), neighbor_indices[i].end(), [&](int lhs, int rhs) {
               return euclid_2d(weights[i], weights[lhs]) < euclid_2d(weights[i], weights[rhs]);
            });

            neighbor_indices[i].resize(options.neighbor_count);
        }
    }
};

std::vector<std::vector<double>> uniform_weights(std::size_t population_size);
Population moead(const Problem& problem, const MOEADOptions& options);

#endif //TINH_TOAN_TIEN_HOA_MOEAD_H
