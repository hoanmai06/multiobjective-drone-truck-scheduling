#include "genetic_operator.h"

#include "problem.h"

#include <vector>
#include <utility>
#include <unordered_map>
#include <random>

std::mt19937 random_engine(0);

Individual create_random_individual(const Problem &problem) {
    Individual individual(problem);

    // Phần nhị phân
    int pos = std::max(problem.truck_count() - 1, problem.truck_only_customer_count() - 1);
    std::uniform_int_distribution<> first_one_position_distribution(pos, problem.customer_count() - 1 -
                                                                         problem.drone_count());
    int first_one_index = first_one_position_distribution(random_engine);
    individual.binary_gene[first_one_index] = true;

    std::fill(
            individual.binary_gene.begin(),
            individual.binary_gene.begin() + problem.truck_count() - 1,
            true);

    std::shuffle(
            individual.binary_gene.begin(),
            individual.binary_gene.begin() + pos,
            random_engine);

    std::uniform_int_distribution<> drone_trip_count_distribution(problem.drone_count(),
                                                                  problem.customer_count() - first_one_index);
    int drone_trip_count = drone_trip_count_distribution(random_engine);

    std::fill(
            individual.binary_gene.begin() + first_one_index + 1,
            individual.binary_gene.begin() + first_one_index + 1 + drone_trip_count,
            true);

    std::shuffle(
            individual.binary_gene.begin() + first_one_index + 1,
            individual.binary_gene.end(),
            random_engine);

    // Phần hoán vị
    // Điền các khách chỉ cho xe tải ở đầu
    int index = 0;
    for (int customer = 1; customer <= problem.customer_count(); ++customer) {
        if (!problem.can_drone_serve(customer)) individual.permutation_gene[index++] = customer;
    }
    for (int customer = 1; customer <= problem.customer_count(); ++customer) {
        if (problem.can_drone_serve(customer)) individual.permutation_gene[index++] = customer;
    }

    // Tráo phần cả xe tải và drone và phần xe tải
    std::shuffle(
            individual.permutation_gene.begin() + problem.truck_only_customer_count(),
            individual.permutation_gene.end(),
            random_engine);
    std::shuffle(
            individual.permutation_gene.begin(),
            individual.permutation_gene.begin() + pos,
            random_engine);

    return individual;
}

Population create_random_population(int population_size, const Problem &problem) {
    Population result;

    for (int i = 0; i < population_size; ++i) {
        result.push_back(std::move(create_random_individual(problem)));
    }

    return result;
}

void fill_remaining(const PermutationGene &parent, PermutationGene &child,
                    std::unordered_map<int, int> &map, int cut1, int cut2) {
    int n = (int) parent.size();

    for (int i = 0; i < n; ++i) {
        if (i < cut1 || i > cut2) {
            int value = parent[i];
            while (map.find(value) != map.end()) {
                value = map[value];
            }
            child[i] = value;
        }
    }
}

// Hàm đếm số bit 1 (số 1.0) trong chromosome
int countOnes(const BinaryGene &chromosome) {
    int count = 0;
    for (double gene: chromosome) {
        if (abs(gene - 1.0) < 1e-6) { // So sánh với sai số cho số thực
            count += 1;
        }
    }
    return count;
}

// Sửa lại gene nếu sai số lượng bit 1
void repairWrongBit1(BinaryGene &child1, BinaryGene &child2,
                     int ones_p1, int ones_p2) {
    // Số bit 1 child1
    int ones_c1 = accumulate(child1.begin(), child1.end(), 0);
    // Số bit 1 child2
    int ones_c2 = accumulate(child2.begin(), child2.end(), 0);

    // Sửa child1
    while (ones_c1 != ones_p1) {
        if (ones_c1 < ones_p1) { // Cần thêm bit 1
            for (size_t i = 0; i < child1.size(); i++) {
                if (abs(child1[i]) < 1e-6) { // Nếu là 0.0
                    child1[i] = 1.0;
                    ones_c1 += 1.0;
                    break;
                }
            }
        } else { // Cần bớt bit 1
            for (size_t i = 0; i < child1.size(); i++) {
                if (abs(child1[i] - 1.0) < 1e-6) { // Nếu là 1.0
                    child1[i] = 0.0;
                    ones_c1 -= 1.0;
                    break;
                }
            }
        }
    }

    // Sửa child2
    while (ones_c2 != ones_p2) {
        if (ones_c2 < ones_p2) { // Cần thêm bit 1
            for (size_t i = 0; i < child2.size(); i++) {
                if (abs(child2[i]) < 1e-6) { // Nếu là 0.0
                    child2[i] = 1.0;
                    ones_c2 += 1.0;
                    break;
                }
            }
        } else { // Cần bớt bit 1
            for (size_t i = 0; i < child2.size(); i++) {
                if (abs(child2[i] - 1.0) < 1e-6) { // Nếu là 1.0
                    child2[i] = 0.0;
                    ones_c2 -= 1.0;
                    break;
                }
            }
        }
    }
}

std::pair<Individual, Individual> crossover(const Individual &parent1,
                                            const Individual &parent2) {
    // Partially Mapped Crossover (PMX) for permutation gene
    // Chọn 2 điểm cắt ngẫu nhiên
    int n = (int) parent1.permutation_gene.size();
    Individual child1((int) parent1.permutation_gene.size());
    Individual child2((int) parent1.permutation_gene.size());
    std::uniform_int_distribution<> dis(0, n - 1);
    int cut1 = dis(random_engine);
    int cut2 = dis(random_engine);
    if (cut1 > cut2) {
        std::swap(cut1, cut2);
    }
    std::unordered_map<int, int> map1, map2;
    for (int i = cut1; i <= cut2; ++i) {
        child1.permutation_gene[i] = parent2.permutation_gene[i];
        child2.permutation_gene[i] = parent1.permutation_gene[i];
        map1[parent2.permutation_gene[i]] = parent1.permutation_gene[i];
        map2[parent1.permutation_gene[i]] = parent2.permutation_gene[i];
    }
    fill_remaining(parent1.permutation_gene, child1.permutation_gene,
                   map1, cut1, cut2);
    fill_remaining(parent2.permutation_gene, child2.permutation_gene,
                   map2, cut1, cut2);

    // Single Point Crossover for binary gene
    int cut = dis(random_engine);
    for (int i = 0; i < cut; ++i) {
        child1.binary_gene[i] = parent1.binary_gene[i];
        child2.binary_gene[i] = parent2.binary_gene[i];
    }
    for (int i = cut; i < n - 1; ++i) {
        child1.binary_gene[i] = parent2.binary_gene[i];
        child2.binary_gene[i] = parent1.binary_gene[i];
    }
    int ones_p1 = countOnes(parent1.binary_gene);
    int ones_p2 = countOnes(parent2.binary_gene);
    repairWrongBit1(child1.binary_gene, child2.binary_gene,
                    ones_p1, ones_p2);

    return {child1, child2};
}

void mutation(Individual &individual) {
    int n = (int) individual.permutation_gene.size();
    std::uniform_int_distribution<> distribution(0, n - 1);
    int idx1 = distribution(random_engine);
    int idx2 = distribution(random_engine);
    while (idx2 == idx1) {
        idx2 = distribution(random_engine);
    }

    int tmp = individual.permutation_gene[idx1];
    individual.permutation_gene[idx1] = individual.permutation_gene[idx2];
    individual.permutation_gene[idx2] = tmp;
}
