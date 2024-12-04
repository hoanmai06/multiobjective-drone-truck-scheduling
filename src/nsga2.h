#ifndef TINH_TOAN_TIEN_HOA_NSGA2_H
#define TINH_TOAN_TIEN_HOA_NSGA2_H

#include "problem.h"
#include "genetic.h"

#include <pagmo/pagmo.hpp>

struct GeneticAlgorithmOptions
{
    int population_size = 100;
    int max_population_count = 500;

    double crossover_rate = 0.95;
    double mutation_rate = 0.1;

    int max_same_parent_crossover_retry_count = 10;
    int max_create_offspring_retry_count = 10;

    bool force_mutation_on_bad_crossover = true;

    int local_search_period = 10;

    int num_subproblems = 100; // Số lượng bài toán con cho MOEAD

    PopulationInitializationAlgorithm initialization = nullptr;
    CrossoverAlgorithm crossover = nullptr;
    MutationAlgorithm mutation = nullptr;
    RepairAlgorithm repair = nullptr;
    IndividualPostprocessingAlgorithm postprocessing = [](Individual &, const Problem &) {};
    IndividualPostprocessingAlgorithm local_search = [](Individual &, const Problem &) {};
    SurvivorSelectionAlgorithm survivor_selection = nullptr;
};

class NSGA2Population
{
public:
    Population individual_list;
    std::vector<Fitness> fitness_list;
    std::vector<int> ranks;

    [[nodiscard]] std::size_t size() const
    {
        return individual_list.size();
    }

    void reserve(std::size_t n)
    {
        individual_list.reserve(n);
        fitness_list.reserve(n);
        ranks.reserve(n);
    }

    void resize(std::size_t n)
    {
        individual_list.resize(n);
        fitness_list.resize(n);
        ranks.resize(n);
    }

    void recalculate_rank()
    {
        auto pagmo_result = pagmo::fast_non_dominated_sorting(fitness_list);
        std::vector<std::vector<std::size_t>> fronts_indices = std::get<0>(pagmo_result);
        for (int front = 0; front < fronts_indices.size(); ++front)
        {
            for (int index = 0; index < fronts_indices[front].size(); ++index)
            {
                ranks[index] = front;
            }
        }
    }

    NSGA2Population() = default;

    explicit NSGA2Population(Population &&pop, const Problem &problem) : individual_list(pop)
    {
        fitness_list.reserve(individual_list.size());
        ranks.resize(individual_list.size());

        for (const Individual &individual : individual_list)
        {
            fitness_list.push_back(std::move(fitness(individual, problem)));
        }

        recalculate_rank();
    }
};

Population nsga2(const Problem &problem, const GeneticAlgorithmOptions &options);
class MOEADPopulation
{
public:
    Population individual_list;

    std::vector<Fitness> fitness_list;
    double lambda; // weights: lambda, 1 - lambda
    Fitness best_fitness;

    [[nodiscard]] std::size_t size() const
    {
        return individual_list.size();
    }

    void reserve(std::size_t n)
    {
        individual_list.reserve(n);
        fitness_list.reserve(n);
    }

    void resize(std::size_t n)
    {
        individual_list.resize(n);
        fitness_list.resize(n);
    }

    void sort()
    {
        std::vector<std::size_t> indices(individual_list.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [this](std::size_t i, std::size_t j)
                  { double f1 = fitness_list[i][0] * lambda + fitness_list[i][1] * (1 - lambda);
                    double f2 = fitness_list[j][0] * lambda + fitness_list[j][1] * (1 - lambda);
                    return f1 < f2; });
        Population new_individual_list;
        std::vector<Fitness> new_fitness_list;
        new_individual_list.reserve(individual_list.size());
        new_fitness_list.reserve(fitness_list.size());
        for (std::size_t index : indices)
        {
            new_individual_list.push_back(std::move(individual_list[index]));
            new_fitness_list.push_back(std::move(fitness_list[index]));
        }

        individual_list = std::move(new_individual_list);
        fitness_list = std::move(new_fitness_list);
        best_fitness = fitness_list.front();
    }

    MOEADPopulation() = default;

    explicit MOEADPopulation(Population &&pop, const Problem &problem) : individual_list(pop)
    {
        fitness_list.reserve(individual_list.size());
        best_fitness = fitness(individual_list[0], problem);
        for (const Individual &individual : individual_list)
        {
            fitness_list.push_back(std::move(fitness(individual, problem)));
        }
    }
    // Copy constructor to clone MOEADPopulation and add new weights
    MOEADPopulation(const MOEADPopulation &other, double lambda)
        : individual_list(other.individual_list), fitness_list(other.fitness_list), best_fitness(other.best_fitness), lambda(lambda)
    {
    }
};

Population moead(const Problem &problem, const GeneticAlgorithmOptions &options);

#endif // TINH_TOAN_TIEN_HOA_NSGA2_H
