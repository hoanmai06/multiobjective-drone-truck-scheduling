#include "nsga2.h"

#include <pagmo/pagmo.hpp>

#include <algorithm>
#include <random>

std::random_device rd;
//std::mt19937 random_engine(rd());

// Khởi tạo random_engine với seed cố định để khi debug còn mô phỏng lại được lỗi
// Khi chạy thuật toán lấy số liệu cần seed với std::random_device
std::mt19937 random_engine(0);

// Hàm trả về mảng fitness của một Population
std::vector<Fitness> fitness(const Population& population) {
    std::vector<Fitness> fitness;

    fitness.reserve(population.size());
    for (const Individual& individual : population) {
        fitness.push_back(individual.fitness);
    }

    return fitness;
}

// Hàm trả về non-dominated front của một quần thể
Population non_dominated_front(const Population& population) {
    // Tạo vector fitness để truyền vào hàm của pagmo
    std::vector<Fitness> fitness_list = fitness(population);

    // Gọi hàm của pagmo
    std::vector<std::size_t> pareto_indices = pagmo::non_dominated_front_2d(fitness_list);

    // Tạo quần thể từ kết quả của pagmo
    Population result;
    result.reserve(pareto_indices.size());
    for (std::size_t index : pareto_indices) {
        result.push_back(population[index]);
    }

    return result;
}

Individual create_offspring(const Population& population, const Problem& problem, const GeneticAlgorithmOptions& options) {
    std::uniform_real_distribution<double> distribution(0, 1);
    double chance;

    while (true) {
        // Chọn ngẫu nhiên 2 cá thể, nếu 2 cá thể như nhau thì chọn lại
        const Individual& first = options.selection(population);
        const Individual& second = options.selection(population);
        if (first == second) continue;

        // Nếu quay số vào ô mất lượt thì mất lượt
        chance = distribution(random_engine);
        if (chance > options.crossover_rate) continue;

        Individual result = options.crossover(first, second);

        // Nếu quay số vào ô đột biến thì đột biến
        chance = distribution(random_engine);
        if (chance < options.mutation_rate) result = options.mutation(result);

        return result;
    }
}

Population evolve_population(Population population, const Problem& problem, const GeneticAlgorithmOptions& options) {
    population.reserve(2*population.size());

    // Sinh con và thêm vào quần thể để tạo quần thể P+Q có độ lớn gấp đôi
    for (int i = 0; i < population.size(); ++i) {
        population.push_back(std::move(create_offspring(population, problem, options)));
    }

    // Chia front
    // Tạo vector fitness và vào hàm của pagmo (có thể tối ưu cách biểu diễn quần thể để
    // chứa fitness ở dạng vector luôn)
    std::vector<Fitness> fitness_list = fitness(population);
    pagmo::fnds_return_type pagmo_result = pagmo::fast_non_dominated_sorting(fitness_list);
    std::vector<std::vector<std::size_t>> fronts_indices = std::get<0>(pagmo_result);

    // Tạo population và đổ đầy population này
    Population result;
    result.reserve(options.population_size);

    for (std::vector<std::size_t>& front : fronts_indices) {
        // Nếu front này là front cuối được lấy vào thì phải lấy theo crowding distance
        if (result.size() + front.size() > options.population_size) {
            // Tính crowding distance bằng pagmo
            std::vector<Fitness> front_fitness;
            front_fitness.reserve(front.size());
            for (std::size_t index : front) front_fitness.push_back(population[index].fitness);
            std::vector<double> crowding_distances = pagmo::crowding_distance(front_fitness);

            // Sắp xếp toàn bộ front theo thứ tự giảm dần crowding distance (dùng heap có thể lấy k phần tử lớn nhất
            // trong O(klogn) nhưng tạm thời xếp toàn bộ cho dễ)
            std::sort(front.begin(), front.end(), [&crowding_distances](int lhs, int rhs) {
               return crowding_distances[lhs] > crowding_distances[rhs];
            });

            std::size_t missing_individual_count = options.population_size - result.size();
            for (int index = 0; index < missing_individual_count; ++index) {
                result.push_back(std::move(population[index]));
            }

            break;
        }

        // Nếu không thì lấy toàn bộ front
        for (std::size_t index : front) {
            result.push_back(std::move(population[index]));
        }
    }

    return result;
}

Population nsga2(const Problem& problem, const GeneticAlgorithmOptions& options) {
    Population population = options.initialization(options.population_size, problem);

    for (int generation = 1; generation <= options.max_population_count; ++generation) {
        evolve_population(population, problem, options);
    }

    return non_dominated_front(population);
}
