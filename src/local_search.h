#ifndef TINH_TOAN_TIEN_HOA_LOCAL_SEARCH_H
#define TINH_TOAN_TIEN_HOA_LOCAL_SEARCH_H

#include "genetic.h"

class IndividualInformation {
private:
    const Problem& _problem;
    const Individual& _individual;

    int _first_drone_customer_index;
    std::vector<int> _index_of_first_customer_in_same_route;

public:
    explicit IndividualInformation(const Individual& individual, const Problem& problem) : _individual(individual), _problem(problem) {
        // Tìm khách drone đầu tiên
        int count = 0;
        for (_first_drone_customer_index = 0; count < problem.truck_count() && _first_drone_customer_index < problem.customer_count(); ++_first_drone_customer_index) {
            if (individual.binary_gene[_first_drone_customer_index] == 1) ++count;
        }

        // Tính mảng các khách đầu trong route
        // Với các khách tiếp theo, nếu bit trước đó ngăn thì là khách đó, nếu không thì cùng giá trị với khách trước
        _index_of_first_customer_in_same_route.resize(individual.permutation_gene.size());
        _index_of_first_customer_in_same_route.front() = 0;
        for (int i = 1; i < individual.permutation_gene.size(); ++i) {
            _index_of_first_customer_in_same_route[i] = individual.binary_gene[i - 1] ? i : _index_of_first_customer_in_same_route[i - 1];
        }
    }

    [[nodiscard]] const Individual& individual() const {return _individual;}
    [[nodiscard]] int first_drone_customer_index() const {return _first_drone_customer_index;}
    [[nodiscard]] int index_of_first_customer_in_same_route(int index) const {return _index_of_first_customer_in_same_route[index];}

    bool can_swap(int i, int j) {
        // 0. Nếu cả 2 khách hàng là của truck thì luôn tráo được
        if (i < _first_drone_customer_index && j < _first_drone_customer_index) {
            return true;
        }

        // 1. Nếu khách hàng yêu cầu truck bị tráo qua drone route thì có vấn đề
        if (_problem.does_customer_require_truck(_individual.permutation_gene[i]) && j >= _first_drone_customer_index) {
            return false;
        }

        bool result = true;

        // 2. Nếu route chứa index i có vấn đề sau khi swap thì có vấn đề
        if (i >= _first_drone_customer_index) {
            Drone drone = _problem.drone();
            int customer_index = index_of_first_customer_in_same_route(i);
            int customer = (customer_index == i) ? _individual.permutation_gene[j] : _individual.permutation_gene[customer_index];
            drone.serve(customer);

            for (customer_index = index_of_first_customer_in_same_route(i) + 1;; ++customer_index) {
                if (_individual.binary_gene[customer_index - 1] == 1 || customer_index >= _individual.permutation_gene.size()) break;
                customer = (customer_index == i) ? _individual.permutation_gene[j] : _individual.permutation_gene[customer_index];
                if (customer_index == j) customer = _individual.permutation_gene[i];
                if (!drone.can_serve(customer)) return false;
                drone.serve(customer);
            }
        }

        if (j >= _first_drone_customer_index) {
            Drone drone = _problem.drone();
            int customer_index = index_of_first_customer_in_same_route(j);
            int customer = (customer_index == j) ? _individual.permutation_gene[i] : _individual.permutation_gene[customer_index];
            drone.serve(customer);

            for (customer_index = index_of_first_customer_in_same_route(j) + 1;; ++customer_index) {
                if (_individual.binary_gene[customer_index - 1] == 1 || customer_index >= _individual.permutation_gene.size()) break;
                customer = (customer_index == j) ? _individual.permutation_gene[i] : _individual.permutation_gene[customer_index];
                if (customer_index == i) customer = _individual.permutation_gene[j];
                if (!drone.can_serve(customer)) return false;
                drone.serve(customer);
            }
        }

        return result;
    }
};

void first_improvement_permutation_swap_hill_climbing(Individual& individual, const Problem& problem);
void fast_first_improvement_permutation_swap_hill_climbing(Individual& individual, const Problem& problem);

#endif //TINH_TOAN_TIEN_HOA_LOCAL_SEARCH_H
