#include "local_search.h"
#include "log.h"

#include <chrono>

bool is_better(const Fitness& lhs, const Fitness& rhs) {
    return (lhs[0] < rhs[0] && lhs[1] <= rhs[1]) || (lhs[0] <= rhs[0] && lhs[1] < rhs[1]);
}

void first_improvement_permutation_swap_hill_climbing(Individual& individual, const Problem& problem) {
//    auto start_time = std::chrono::high_resolution_clock::now();

    Fitness current_fitness = fitness(individual, problem);
    IndividualInformation information(individual, problem);

BEGIN:

    for (int i = 0; i < individual.permutation_gene.size(); ++i) {
        for (int j = i + 1; j < individual.permutation_gene.size(); ++j) {
            // Kiểm tra tính hợp lệ của phép tráo
            // Tráo 2 khách ở xe tải luôn là valid, chỉ kiểm tra valid nếu i hoặc j ở drone
            if (i >= information.first_drone_customer_index() || j >= information.first_drone_customer_index()) {
                if (problem.does_customer_require_truck(individual.permutation_gene[i]) && j >= information.first_drone_customer_index()) break;
                if (!information.can_swap(i, j)) continue;
            }

            // Tính fitness của phép tráo
            std::swap(individual.permutation_gene[i], individual.permutation_gene[j]);
            Fitness clone_fitness = fitness(individual, problem);
            std::swap(individual.permutation_gene[i], individual.permutation_gene[j]);

            if (is_better(clone_fitness, current_fitness)) {
                current_fitness = std::move(clone_fitness);
                std::swap(individual.permutation_gene[i], individual.permutation_gene[j]);

//                print(current_fitness);

                goto BEGIN;
            }
        }
    }

//    auto end_time = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
//    std::cout << "Time: " << duration << " ms\n";
}
