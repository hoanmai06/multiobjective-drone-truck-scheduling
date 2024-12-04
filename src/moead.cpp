#include "moead.h"
#include "log.h"

#include <random>

static std::mt19937 random_engine(0);

double tchebycheff(const Fitness& fitness, const std::vector<double>& reference_point, const std::vector<double>& weight) {
    return std::max(weight[0]*std::abs(fitness[0] - reference_point[0]), weight[1]*std::abs(fitness[1] - reference_point[1]));
}

std::vector<std::vector<double>> uniform_weights(std::size_t population_size) {
    if (population_size <= 0) return {};
    if (population_size == 1) return {{0.5, 0.5}};

    std::vector<std::vector<double>> weights;
    weights.reserve(population_size);

    double step = 1.0/((double) population_size - 1);
    for (std::size_t i = 0; i < population_size; ++i) {
        weights.push_back({(double) i * step, (double) (population_size - i - 1) * step});
    }

    return weights;
}

Individual create_offspring(std::size_t i, MOEADPopulation& population, const Problem& problem, const MOEADOptions& options) {
    // Chọn ngẫu nhiên 2 cá thể trong tập hàng xóm và sinh con
    std::uniform_int_distribution<std::size_t> distribution(0, options.neighbor_count - 1);
    std::uniform_real_distribution<> chance_distribution(0, 1);
    double chance;

    std::size_t first = distribution(random_engine);
    std::size_t second = distribution(random_engine);
    while (first == second) second = distribution(random_engine);

    first = population.neighbor_indices[i][first];
    second = population.neighbor_indices[i][second];

    // Lai ghép
    bool get_first_child = std::uniform_int_distribution(0, 1)(random_engine);
    Individual result = get_first_child
                        ? options.crossover(population.individual_list[first], population.individual_list[second]).first
                        : options.crossover(population.individual_list[first], population.individual_list[second]).second;

    // Nếu quay số vào ô đột biến hoặc nếu phải đột biến trên con tệ và con đang tệ thì đột biến
    chance = chance_distribution(random_engine);
    if (options.force_mutation_on_bad_crossover && (result == population.individual_list[first] || result == population.individual_list[second])) chance = 0;
    if (chance < options.mutation_rate) options.mutation(result);

    if (!is_valid(result, problem)) {
        result = options.repair(result, problem);
    }

    return result;
}

void evolve_population(MOEADPopulation& population, const Problem& problem, const MOEADOptions& options) {
    // Duyệt qua toàn bộ cá thể trong quần thể
    for (std::size_t i = 0; i < population.size(); ++i) {
        // 1. Sinh con ứng với cá thể hiện tại
        Individual offspring = create_offspring(i, population, problem, options);
        Fitness offspring_fitness = fitness(offspring, problem);

        // 2. Cập nhật z
        if (offspring_fitness[0] < population.reference_point[0]) population.reference_point[0] = offspring_fitness[0];
        if (offspring_fitness[1] < population.reference_point[1]) population.reference_point[1] = offspring_fitness[1];

        // 3. Cập nhật hàng xóm: Nếu nghiệm này tốt hơn hàng xóm cho bài toán của hàng xóm thì đổi hàng xóm
        for (std::size_t index : population.neighbor_indices[i]) {
            double fitness1 = tchebycheff(offspring_fitness, population.reference_point, population.weights[i]);
            double fitness2 = tchebycheff(population.fitness_list[index], population.reference_point, population.weights[i]);
            if (fitness1 < fitness2) {
                population.individual_list[index] = offspring;
                population.fitness_list[index] = offspring_fitness;
            }
        }

        // 4. Cập nhật external population
        // 4.1. Loại bỏ các nghiệm bị trội bởi offspring khỏi EP
        std::size_t end = 0;
        for (std::size_t index = 0; index < population.external_population.size(); ++index) {
            if (is_better(offspring_fitness, population.external_population_fitness[index])) {
                continue;
            }
            if (end != index) {
                population.external_population[end] = population.external_population[index];
                population.external_population_fitness[end] = population.external_population_fitness[index];
            }
            ++end;
        }
        population.external_population.resize(end);
        population.external_population_fitness.resize(end);

        // 4.2. Thêm offspring vào EP nếu không có nghiệm nào trong EP trội nó
        bool do_insert = true;
        for (const Fitness& fitness : population.external_population_fitness) {
            if (is_better(fitness, offspring_fitness) || fitness == offspring_fitness) {
                do_insert = false;
                break;
            }
        }
        if (do_insert) {
            population.external_population.push_back(offspring);
            population.external_population_fitness.push_back(offspring_fitness);
        }
    }
}

Population moead(const Problem& problem, const MOEADOptions& options) {
    // 1. Initialization
    MOEADPopulation population(problem, options);
    log(population);

    // 2. Update
    for (int i = 1; i <= options.max_population_count; ++i) {
        evolve_population(population, problem, options);
        log(population);

        for (const Fitness& fitness : population.external_population_fitness) {
            print(fitness);
        }
        print("Generation " + std::to_string(i));
    }

    return population.external_population;
}
