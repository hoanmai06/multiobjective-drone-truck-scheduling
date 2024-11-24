#include "log.h"

std::ofstream log_file("../log/log.txt");

void log(const NSGA2Population& population) {
    for (std::size_t i = 0; i < population.size(); ++i) {
        log_file << population.individual_list[i].binary_gene << '\n';
        log_file << population.individual_list[i].permutation_gene << '\n';
        log_file << population.fitness_list[i] << '\n';
        log_file << "I\n";
    }

    log_file << "P\n";
    log_file.flush();
}