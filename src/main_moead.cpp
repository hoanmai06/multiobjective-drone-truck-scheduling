#include "problem.h"

#include "log.h"
#include "genetic_operator.h"
#include "local_search.h"
// Chỉnh loại drone tuyến tính hay phi tuyến trong drone_type.h

int main()
{
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "4");
    Problem problem = Problem::from_file("../data/random_data/50.10.1.txt", truck_config, drone_config);
    GeneticAlgorithmOptions options;

    options.population_size = 100;
    options.max_population_count = 500;
    options.crossover_rate = 0.95;
    options.mutation_rate = 0.2;
    options.num_subproblems = 100;

    options.initialization = create_random_population;
    options.crossover = crossover;
    options.mutation = mutation;
    options.repair = repair;
    options.survivor_selection = hybirdSelection;
    options.postprocessing = first_improvement_permutation_swap_hill_climbing;

    Population solutions = moead(problem, options);

    return 0;
}