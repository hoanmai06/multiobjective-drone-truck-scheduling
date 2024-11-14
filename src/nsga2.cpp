#include "nsga2.h"

#include <pagmo/pagmo.hpp>

#include <algorithm>
#include <random>

// Khởi tạo random_engine với seed cố định để khi debug còn mô phỏng lại được lỗi
// Khi chạy thuật toán lấy số liệu cần seed với std::random_device
static std::mt19937 random_engine(0);

class NSGA2Population {
public:
    Population data;
    std::vector<Fitness> fitness_list;
    std::vector<int> ranks;

    [[nodiscard]] std::size_t size() const {
        return data.size();
    }

    void reserve(std::size_t n) {
        data.reserve(n);
        fitness_list.reserve(n);
        ranks.reserve(n);
    }

    void resize(std::size_t n) {
        data.resize(n);
        fitness_list.resize(n);
        ranks.resize(n);
    }

    void recalculate_rank() {
        auto pagmo_result = pagmo::fast_non_dominated_sorting(fitness_list);
        std::vector<std::vector<std::size_t>> fronts_indices = std::get<0>(pagmo_result);
        for (int front = 0; front < fronts_indices.size(); ++front) {
            for (int index = 0; index < fronts_indices[front].size(); ++index) {
                ranks[index] = front;
            }
        }
    }

    NSGA2Population() = default;

    explicit NSGA2Population(Population&& pop, const Problem& problem) : data(pop) {
        fitness_list.reserve(data.size());
        ranks.resize(data.size());

        for (const Individual& individual : data) {
            fitness_list.push_back(std::move(fitness(individual, problem)));
        }

        recalculate_rank();
    }
};

// Hàm trả về non-dominated front của một quần thể
Population non_dominated_front(const NSGA2Population& population) {
    // Gọi hàm của pagmo
    std::vector<std::size_t> pareto_indices = pagmo::non_dominated_front_2d(population.fitness_list);

    // Tạo quần thể từ kết quả của pagmo
    Population result;
    result.reserve(pareto_indices.size());
    for (std::size_t index : pareto_indices) {
        result.push_back(population.data[index]);
    }

    return result;
}

const Individual& tournament_selection(const NSGA2Population& population) {
    std::uniform_int_distribution<std::size_t> distribution(0, population.size() - 1);

    std::size_t first = distribution(random_engine);
    std::size_t second = distribution(random_engine);
    while (first == second) second = distribution(random_engine);

    return population.ranks[first] < population.ranks[second] ? population.data[first] : population.data[second];
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

        Individual result = options.crossover(first, second);

        // Nếu quay số vào ô đột biến thì đột biến
        chance = distribution(random_engine);
        if (chance < options.mutation_rate) result = options.mutation(result);

        return result;
    }
}

NSGA2Population evolve_population(NSGA2Population population, const Problem& problem, const GeneticAlgorithmOptions& options) {
    population.data.reserve(2*population.data.size());

    // Sinh con và thêm vào quần thể để tạo quần thể P+Q có độ lớn gấp đôi
    for (int i = 0; i < population.data.size(); ++i) {
        population.data.push_back(std::move(create_offspring(population, problem, options)));
        population.fitness_list.push_back(fitness(population.data.back(), problem));
    }

    // Chia front
    // Tạo vector fitness và vào hàm của pagmo
    pagmo::fnds_return_type pagmo_result = pagmo::fast_non_dominated_sorting(population.fitness_list);
    std::vector<std::vector<std::size_t>> fronts_indices = std::get<0>(pagmo_result);

    // Tạo population và đổ đầy population này
    NSGA2Population result;
    result.reserve(options.population_size);

    for (std::vector<std::size_t>& front : fronts_indices) {
        // Nếu front này là front cuối được lấy vào thì phải lấy theo crowding distance
        if (result.size() + front.size() > options.population_size) {
            // Tính crowding distance bằng pagmo
            std::vector<Fitness> front_fitness;
            front_fitness.reserve(front.size());
            for (std::size_t index : front) front_fitness.push_back(population.fitness_list[index]);
            std::vector<double> crowding_distances = pagmo::crowding_distance(front_fitness);

            // Sắp xếp toàn bộ front theo thứ tự giảm dần crowding distance (dùng heap có thể lấy k phần tử lớn nhất
            // trong O(klogn) nhưng tạm thời xếp toàn bộ cho dễ)
            std::sort(front.begin(), front.end(), [&crowding_distances](int lhs, int rhs) {
               return crowding_distances[lhs] > crowding_distances[rhs];
            });

            std::size_t missing_individual_count = options.population_size - result.size();
            for (int index = 0; index < missing_individual_count; ++index) {
                result.data.push_back(std::move(population.data[index]));
                result.fitness_list.push_back(std::move(population.fitness_list[index]));
            }

            break;
        }

        // Nếu không thì lấy toàn bộ front
        for (std::size_t index : front) {
            result.data.push_back(std::move(population.data[index]));
            result.fitness_list.push_back(std::move(population.fitness_list[index]));
        }
    }

    result.recalculate_rank();
    return result;
}

Population nsga2(const Problem& problem, const GeneticAlgorithmOptions& options) {
    NSGA2Population population(options.initialization(options.population_size, problem), problem);

    for (int generation = 1; generation <= options.max_population_count; ++generation) {
        evolve_population(population, problem, options);
    }

    return non_dominated_front(population);
}
