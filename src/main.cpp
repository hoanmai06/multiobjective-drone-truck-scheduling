#include "problem.h"

#include "genetic.h"

#include <iostream>
#include <fstream>

// Chỉnh loại drone tuyến tính hay phi tuyến trong drone_type.h

int main() {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "3");
    Problem problem = Problem::from_file("../data/random_data/6.5.1.txt", truck_config, drone_config);

    Individual individual(problem);
    individual.permutation_gene = {1, 2, 3, 5, 4, 6};
    individual.binary_gene = {false, false, false, true, true};

    Solution solution = decode(individual);
    solution.objectives();
    fitness(individual);

    return 0;
}
