#include "problem.h"

#include "log.h"
#include "genetic_operator.h"
#include "local_search.h"
#include "nsga2.h"

// Chỉnh loại drone tuyến tính hay phi tuyến trong drone_type.h

int main() {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "4");
    Problem problem = Problem::from_file("../data/random_data/50.10.1.txt", truck_config, drone_config);

    GeneticAlgorithmOptions options;
    options.population_size = 100;
    options.max_population_count = 1000;
    options.crossover_rate = 0.95;
    options.mutation_rate = 0.1;

    options.max_same_parent_crossover_retry_count = 10;
    options.max_create_offspring_retry_count = 10;

    options.force_mutation_on_bad_crossover = true;

    options.local_search_period = 10;

    options.initialization = create_random_population;
    options.crossover = crossover;
    options.mutation = mutation;
    options.repair = repair;
    options.postprocessing = [] (Individual&, const Problem&) {};
    options.local_search = first_improvement_permutation_swap_hill_climbing;

    Population solutions = nsga2(problem, options);

    return 0;
}
