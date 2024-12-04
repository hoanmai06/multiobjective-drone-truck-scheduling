#include "nsga2.h"
#include "log.h"

#include <algorithm>
#include <random>

// Khởi tạo random_engine với seed cố định để khi debug còn mô phỏng lại được lỗi
// Khi chạy thuật toán lấy số liệu cần seed với std::random_device
static std::mt19937 random_engine(0);

const double EPSILON = 0.0000001;

// Hàm trả về non-dominated front của một quần thể
Population non_dominated_front(const NSGA2Population& population) {
    // Gọi hàm của pagmo
    std::vector<std::size_t> pareto_indices = pagmo::non_dominated_front_2d(population.fitness_list);

    // Tạo quần thể từ kết quả của pagmo
    Population result;
    result.reserve(pareto_indices.size());
    for (std::size_t index : pareto_indices) {
        result.push_back(population.individual_list[index]);
    }

    return result;
}

const Individual& tournament_selection(const NSGA2Population& population) {
    std::uniform_int_distribution<std::size_t> distribution(0, population.size() - 1);

    std::size_t first = distribution(random_engine);
    std::size_t second = distribution(random_engine);
    while (first == second) second = distribution(random_engine);

    return population.ranks[first] < population.ranks[second] ? population.individual_list[first] : population.individual_list[second];
}

bool operator==(const Individual& lhs, const Individual& rhs) {
    return lhs.binary_gene == rhs.binary_gene && lhs.permutation_gene == rhs.permutation_gene;
}

bool operator!=(const Individual& lhs, const Individual& rhs) {
    return !(lhs == rhs);
}

Individual create_offspring(const NSGA2Population& population, const Problem& problem, const GeneticAlgorithmOptions& options) {
    std::uniform_real_distribution<double> distribution(0, 1);
    double chance;

    while (true) {
        // Chọn ngẫu nhiên 2 cá thể, nếu 2 cá thể như nhau thì chọn lại
        const Individual& first = tournament_selection(population);
        const Individual& second = tournament_selection(population);
        if (&first == &second) continue;

        // Nếu quay số vào ô mất lượt thì mất lượt
        chance = distribution(random_engine);
        if (chance > options.crossover_rate) continue;

        // Chọn ngẫu nhiên một trong 2 con của crossover
        bool get_first_child = std::uniform_int_distribution(0, 1)(random_engine);
        Individual result = get_first_child
                            ? options.crossover(first, second).first
                            : options.crossover(first, second).second;

        // Nếu con sinh ra giống bố mẹ thì cố sinh lại
        for (int i = 0; i < options.max_same_parent_crossover_retry_count; ++i) {
            if (result != first && result != second) break;

            result = get_first_child
                     ? options.crossover(first, second).first
                     : options.crossover(first, second).second;
        }

        // Nếu quay số vào ô đột biến hoặc nếu phải đột biến trên con tệ và con đang tệ thì đột biến
        chance = distribution(random_engine);
        if (options.force_mutation_on_bad_crossover && (result == first || result == second)) chance = 0;
        if (chance < options.mutation_rate) options.mutation(result);

        if (!is_valid(result, problem)) {
            result = options.repair(result, problem);
        }

        options.postprocessing(result, problem);

        return result;
    }
}

bool is_fitness_in_list(const Fitness& fitness, const std::vector<Fitness>& fitness_list) {
    for (const Fitness& fitness_in_list : fitness_list) {
        if (std::fabs(fitness[0] - fitness_in_list[0]) < EPSILON && std::fabs(fitness[1] - fitness_in_list[1]) < EPSILON) return true;
    }
    return false;
}

NSGA2Population evolve_population(const NSGA2Population& population, const Problem& problem, const GeneticAlgorithmOptions& options) {
    NSGA2Population new_population = population;
    new_population.individual_list.reserve(2 * new_population.individual_list.size());

    // Sinh con và thêm vào quần thể để tạo quần thể P+Q có độ lớn gấp đôi
    for (int i = 0; i < options.population_size; ++i) {
        Individual offspring = create_offspring(population, problem, options);
        Fitness offspring_fitness = fitness(offspring, problem);

        for (int attempt = 0; attempt < options.max_create_offspring_retry_count; ++attempt) {
            if (!is_fitness_in_list(offspring_fitness, new_population.fitness_list)) break;
            offspring = create_offspring(population, problem, options);
            offspring_fitness = fitness(offspring, problem);
        }

        new_population.individual_list.push_back(std::move(offspring));
        new_population.fitness_list.push_back(std::move(offspring_fitness));
    }

    // Chia front bằng cách gọi hàm của pagmo
    pagmo::fnds_return_type pagmo_result = pagmo::fast_non_dominated_sorting(new_population.fitness_list);
    std::vector<std::vector<std::size_t>> fronts_indices = std::get<0>(pagmo_result);

    // Tạo population chứa thế hệ tiếp theo
    NSGA2Population result;
    result.reserve(options.population_size);

    for (std::vector<std::size_t>& front : fronts_indices) {
        if (result.size() == options.population_size) break;
        // Nếu front này là front cuối được lấy vào thì phải lấy theo crowding distance
        if (result.size() + front.size() > options.population_size) {
            // Tính crowding distance bằng pagmo
            std::vector<Fitness> front_fitness;
            front_fitness.reserve(front.size());
            for (std::size_t index : front) front_fitness.push_back(new_population.fitness_list[index]);
            std::vector<double> crowding_distances = pagmo::crowding_distance(front_fitness);

            // Sinh ra một mảng index với giá trị crowding distance tại index giảm dần
            std::vector<std::size_t> indices(front.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(), [&crowding_distances](int lhs, int rhs) {
                return crowding_distances[lhs] > crowding_distances[rhs];
            });

            std::size_t missing_individual_count = options.population_size - result.size();
            for (int index = 0; index < missing_individual_count; ++index) {
                std::size_t individual_index = front[indices[index]];
                result.individual_list.push_back(std::move(new_population.individual_list[individual_index]));
                result.fitness_list.push_back(std::move(new_population.fitness_list[individual_index]));
            }

            break;
        }

        // Nếu không thì lấy toàn bộ front
        for (std::size_t index : front) {
            result.individual_list.push_back(std::move(new_population.individual_list[index]));
            result.fitness_list.push_back(std::move(new_population.fitness_list[index]));
        }
    }

    result.recalculate_rank();
    return result;
}

Population nsga2(const Problem& problem, const GeneticAlgorithmOptions& options) {
    NSGA2Population population(options.initialization(options.population_size, problem), problem);

    for (std::size_t i = 0; i < population.size(); ++i) {
        if (!is_valid(population.individual_list[i], problem)) {
            population.individual_list[i] = options.repair(population.individual_list[i], problem);
        }

        options.postprocessing(population.individual_list[i], problem);
        population.fitness_list[i] = fitness(population.individual_list[i], problem);
    }

    log(population);

    for (int generation = 1; generation <= options.max_population_count; ++generation) {
        population = evolve_population(population, problem, options);
        log(population);
        for (const Fitness& fitness : population.fitness_list) {
            std::cout << fitness << '\n';
        }
        std::cout << "Generation " << generation << '\n';
    }

    return non_dominated_front(population);
}
