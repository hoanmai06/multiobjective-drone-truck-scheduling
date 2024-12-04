#include "nsga2.h"
#include "log.h"

#include <algorithm>
#include <random>

// Khởi tạo random_engine với seed cố định để khi debug còn mô phỏng lại được lỗi
// Khi chạy thuật toán lấy số liệu cần seed với std::random_device
static std::mt19937 random_engine(0);

const double EPSILON = 0.0000001;

// Hàm trả về non-dominated front của một quần thể
Population non_dominated_front(const NSGA2Population &population)
{
    // Gọi hàm của pagmo
    std::vector<std::size_t> pareto_indices = pagmo::non_dominated_front_2d(population.fitness_list);

    // Tạo quần thể từ kết quả của pagmo
    Population result;
    result.reserve(pareto_indices.size());
    for (std::size_t index : pareto_indices)
    {
        result.push_back(population.individual_list[index]);
    }

    return result;
}

const Individual &tournament_selection(const NSGA2Population &population)
{
    std::uniform_int_distribution<std::size_t> distribution(0, population.size() - 1);

    std::size_t first = distribution(random_engine);
    std::size_t second = distribution(random_engine);
    while (first == second)
        second = distribution(random_engine);

    return population.ranks[first] < population.ranks[second] ? population.individual_list[first] : population.individual_list[second];
}

bool operator==(const Individual &lhs, const Individual &rhs)
{
    return lhs.binary_gene == rhs.binary_gene && lhs.permutation_gene == rhs.permutation_gene;
}

bool operator!=(const Individual &lhs, const Individual &rhs)
{
    return !(lhs == rhs);
}

Individual create_offspring(const NSGA2Population &population, const Problem &problem, const GeneticAlgorithmOptions &options)
{
    std::uniform_real_distribution<double> distribution(0, 1);
    double chance;

    while (true)
    {
        // Chọn ngẫu nhiên 2 cá thể, nếu 2 cá thể như nhau thì chọn lại
        const Individual &first = tournament_selection(population);
        const Individual &second = tournament_selection(population);
        if (&first == &second)
            continue;

        // Nếu quay số vào ô mất lượt thì mất lượt
        chance = distribution(random_engine);
        if (chance > options.crossover_rate)
            continue;

        // Chọn ngẫu nhiên một trong 2 con của crossover
        bool get_first_child = std::uniform_int_distribution(0, 1)(random_engine);
        Individual result = get_first_child
                                ? options.crossover(first, second).first
                                : options.crossover(first, second).second;

        // Nếu con sinh ra giống bố mẹ thì cố sinh lại
        for (int i = 0; i < options.max_same_parent_crossover_retry_count; ++i)
        {
            if (result != first && result != second)
                break;

            result = get_first_child
                         ? options.crossover(first, second).first
                         : options.crossover(first, second).second;
        }

        // Nếu quay số vào ô đột biến hoặc nếu phải đột biến trên con tệ và con đang tệ thì đột biến
        chance = distribution(random_engine);
        if (options.force_mutation_on_bad_crossover && (result == first || result == second))
            chance = 0;
        if (chance < options.mutation_rate)
            options.mutation(result);

        if (!is_valid(result, problem))
        {
            result = options.repair(result, problem);
        }

        return result;
    }
}

bool is_fitness_in_list(const Fitness &fitness, const std::vector<Fitness> &fitness_list)
{
    for (const Fitness &fitness_in_list : fitness_list)
    {
        if (std::fabs(fitness[0] - fitness_in_list[0]) < EPSILON && std::fabs(fitness[1] - fitness_in_list[1]) < EPSILON)
            return true;
    }
    return false;
}

NSGA2Population evolve_population(const NSGA2Population &population, const Problem &problem, const GeneticAlgorithmOptions &options)
{
    NSGA2Population new_population;
    new_population.individual_list.reserve(2 * new_population.individual_list.size());

    for (int i = 0; i < population.size(); ++i)
    {
        if (!is_fitness_in_list(population.fitness_list[i], new_population.fitness_list))
        {
            new_population.individual_list.push_back(population.individual_list[i]);
            new_population.fitness_list.push_back(population.fitness_list[i]);
        }
    }

    // Sinh con và thêm vào quần thể để tạo quần thể P+Q có độ lớn gấp đôi
    while (new_population.size() < 2 * options.population_size)
    {
        Individual offspring = create_offspring(population, problem, options);
        Fitness offspring_fitness = fitness(offspring, problem);

        for (int attempt = 0; attempt < options.max_create_offspring_retry_count; ++attempt)
        {
            if (!is_fitness_in_list(offspring_fitness, new_population.fitness_list))
                break;
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

    for (std::vector<std::size_t> &front : fronts_indices)
    {
        if (result.size() == options.population_size)
            break;
        // Nếu front này là front cuối được lấy vào thì phải lấy theo crowding distance
        if (result.size() + front.size() > options.population_size)
        {
            // Tính crowding distance bằng pagmo
            std::vector<Fitness> front_fitness;
            front_fitness.reserve(front.size());
            for (std::size_t index : front)
                front_fitness.push_back(new_population.fitness_list[index]);
            std::vector<double> crowding_distances = pagmo::crowding_distance(front_fitness);

            // Sinh ra một mảng index với giá trị crowding distance tại index giảm dần
            std::vector<std::size_t> indices(front.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(), [&crowding_distances](int lhs, int rhs)
                      { return crowding_distances[lhs] > crowding_distances[rhs]; });

            std::size_t missing_individual_count = options.population_size - result.size();
            for (int index = 0; index < missing_individual_count; ++index)
            {
                std::size_t individual_index = front[indices[index]];
                result.individual_list.push_back(std::move(new_population.individual_list[individual_index]));
                result.fitness_list.push_back(std::move(new_population.fitness_list[individual_index]));
            }

            break;
        }

        // Nếu không thì lấy toàn bộ front
        for (std::size_t index : front)
        {
            result.individual_list.push_back(std::move(new_population.individual_list[index]));
            result.fitness_list.push_back(std::move(new_population.fitness_list[index]));
        }
    }

    result.recalculate_rank();
    return result;
}

Population nsga2(const Problem &problem, const GeneticAlgorithmOptions &options)
{
    NSGA2Population population(options.initialization(options.population_size, problem), problem);

    for (std::size_t i = 0; i < population.size(); ++i)
    {
        if (!is_valid(population.individual_list[i], problem))
        {
            population.individual_list[i] = options.repair(population.individual_list[i], problem);
        }

        population.fitness_list[i] = fitness(population.individual_list[i], problem);
    }

    log(population);

    for (int generation = 1; generation <= options.max_population_count; ++generation)
    {
        population = evolve_population(population, problem, options);

        if (options.local_search_period != 0 && generation % options.local_search_period == 0)
        {
            for (int i = 0; i < population.size(); ++i)
            {
                options.local_search(population.individual_list[i], problem);
                population.fitness_list[i] = fitness(population.individual_list[i], problem);
            }
        }

        if (generation == options.max_population_count)
        {
            for (int i = 0; i < population.size(); ++i)
            {
                options.postprocessing(population.individual_list[i], problem);
                population.fitness_list[i] = fitness(population.individual_list[i], problem);
            }
        }

        log(population);
        for (const Fitness &fitness : population.fitness_list)
        {
            std::cout << fitness << '\n';
        }
        std::cout << "Generation " << generation << '\n';
    }

    return non_dominated_front(population);
}

std::vector<double> createWeights(int numWeights)
{
    std::vector<double> lambdas;
    lambdas.reserve(numWeights + 1); // Tối ưu bộ nhớ

    for (int i = 0; i <= numWeights; ++i)
    {
        lambdas.push_back(static_cast<double>(i) / numWeights);
    }

    return lambdas;
}
const Individual &tournament_selection_moead(const MOEADPopulation &population)
{
    std::uniform_int_distribution<std::size_t> distribution(0, population.size() - 1);

    std::size_t first = distribution(random_engine);
    std::size_t second = distribution(random_engine);
    while (first == second)
        second = distribution(random_engine);
    double f1 = population.fitness_list[first][0] * population.lambda + population.fitness_list[first][1] * (1 - population.lambda);
    double f2 = population.fitness_list[second][0] * population.lambda + population.fitness_list[second][1] * (1 - population.lambda);
    return f1 < f2 ? population.individual_list[first] : population.individual_list[second];
}

Individual create_offspring_moead(const MOEADPopulation &population, const Problem &problem, const GeneticAlgorithmOptions &options)
{
    std::uniform_real_distribution<double> distribution(0, 1);
    double chance;

    while (true)
    {
        // Chọn ngẫu nhiên 2 cá thể, nếu 2 cá thể như nhau thì chọn lại
        const Individual &first = tournament_selection_moead(population);
        const Individual &second = tournament_selection_moead(population);
        if (&first == &second)
            continue;
        // Nếu quay số vào ô mất lượt thì mất lượt
        chance = distribution(random_engine);
        if (chance > options.crossover_rate)
            continue;

        // Chọn ngẫu nhiên một trong 2 con của crossover
        bool get_first_child = std::uniform_int_distribution(0, 1)(random_engine);
        Individual result = get_first_child
                                ? options.crossover(first, second).first
                                : options.crossover(first, second).second;
        // Nếu quay số vào ô đột biến thì đột biến
        chance = distribution(random_engine);
        if (chance < options.mutation_rate)
            options.mutation(result);

        if (!is_valid(result, problem))
        {
            result = options.repair(result, problem);
        }

        bool is_duplicated = false;
        for (const Individual &individual : population.individual_list)
        {
            if (individual.permutation_gene == result.permutation_gene && individual.binary_gene == result.binary_gene)
            {
                is_duplicated = true;
                break;
            }
        }

        if (is_duplicated)
            continue;

        return result;
    }
}

MOEADPopulation evolve_population_moead(const MOEADPopulation &population, const Problem &problem, const GeneticAlgorithmOptions &options)
{
    MOEADPopulation new_population = population;
    new_population.individual_list.reserve(2 * new_population.individual_list.size());
    for (int i = 0; i < options.population_size; ++i)
    {
        new_population.individual_list.push_back(std::move(create_offspring_moead(population, problem, options)));
        new_population.fitness_list.push_back(fitness(new_population.individual_list.back(), problem));
    }
    new_population.sort();
    new_population.individual_list = options.survivor_selection(new_population.individual_list,
                                                                new_population.fitness_list,
                                                                new_population.lambda,
                                                                options.population_size);
    new_population.sort();
    return new_population;
}

Population moead(const Problem &problem, const GeneticAlgorithmOptions &options)
{
    // 1. Khởi tạo
    MOEADPopulation population(options.initialization(options.population_size, problem), problem);

    for (std::size_t i = 0; i < population.size(); ++i)
    {
        if (!is_valid(population.individual_list[i], problem))
        {
            population.individual_list[i] = options.repair(population.individual_list[i], problem);
            population.fitness_list[i] = fitness(population.individual_list[i], problem);
        }
    }
    Population best_solutions;
    std::vector<Fitness> best_fitness;
    std::vector<double> lambdas = createWeights(options.num_subproblems);
    std::vector<MOEADPopulation> subproblem_populations;
    for (std::size_t i = 0; i < options.num_subproblems; ++i)
    {
        subproblem_populations.push_back(MOEADPopulation(population, lambdas[i]));
        subproblem_populations.back().sort();
        best_fitness.push_back(subproblem_populations.back().best_fitness);
        best_solutions.push_back(subproblem_populations.back().individual_list[0]);
    }

    logMOEAD(best_solutions, best_fitness);
    for (const Fitness &fitness : best_fitness)
    {
        std::cout << fitness << '\n';
    }
    std::cout << "Generation " << 0 << '\n';
    for (int generation = 1; generation <= options.max_population_count; ++generation)
    {
        for (int i = 0; i < options.num_subproblems; ++i)
        {
            subproblem_populations[i] = evolve_population_moead(subproblem_populations[i], problem, options);
            best_solutions.push_back(subproblem_populations[i].individual_list[0]);
            best_fitness.push_back(subproblem_populations[i].best_fitness);
        }
        for (const Fitness &fitness : best_fitness)
        {
            std::cout << fitness << '\n';
        }
        std::cout << "Generation " << generation << '\n';
        logMOEAD(best_solutions, best_fitness);
    }
    return population.individual_list;
}