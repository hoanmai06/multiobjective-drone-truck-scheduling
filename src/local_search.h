#ifndef TINH_TOAN_TIEN_HOA_LOCAL_SEARCH_H
#define TINH_TOAN_TIEN_HOA_LOCAL_SEARCH_H

#include "genetic.h"

class IndividualInformation {
private:
    const Problem& _problem;
    const Individual& _individual;

    int _first_drone_customer_index;
    std::vector<int> _trip_of_index;
    std::vector<int> _index_of_first_customer_of_trip;
    std::vector<double> _trip_finish_times;
    std::vector<double> _trip_wait_times;
    std::vector<double> _route_finish_times;

public:
    explicit IndividualInformation(const Individual& individual, const Problem& problem) : _individual(individual), _problem(problem) {
        // Tìm khách drone đầu tiên
        int count = 0;
        for (_first_drone_customer_index = 0; count < problem.truck_count() && _first_drone_customer_index < problem.customer_count(); ++_first_drone_customer_index) {
            if (individual.binary_gene[_first_drone_customer_index] == 1) ++count;
        }

        _trip_finish_times = trip_finish_times(individual, problem);
        _trip_wait_times = trip_wait_times(individual, problem);
        _route_finish_times = route_finish_times(_trip_finish_times, problem);

        // Tính mảng các khách đầu trong trip
        _index_of_first_customer_of_trip.resize(_trip_finish_times.size() + 1);
        _trip_of_index.resize(individual.permutation_gene.size());
        _trip_of_index[0] = 0;
        for (int customer_index = 1; customer_index < individual.permutation_gene.size(); ++customer_index) {
            // Nếu khách này của trip mới
            if (individual.binary_gene[customer_index - 1] == 1) {
                _trip_of_index[customer_index] = _trip_of_index[customer_index - 1] + 1;
                _index_of_first_customer_of_trip[_trip_of_index[customer_index]] = customer_index;
            }
            // Nếu khách này cùng trip với khách trước
            else {
                _trip_of_index[customer_index] = _trip_of_index[customer_index - 1];
            }
        }

        // Lính canh
        _index_of_first_customer_of_trip[_trip_finish_times.size()] = (int) individual.permutation_gene.size();
    }

    [[nodiscard]] const Individual& individual() const {return _individual;}
    [[nodiscard]] int first_drone_customer_index() const {return _first_drone_customer_index;}

    [[nodiscard]] int index_of_first_customer_in_same_trip(int customer_index) const {
        return _index_of_first_customer_of_trip[_trip_of_index[customer_index]];
    }

    bool can_swap(int i, int j);
    Fitness fitness_after_swap(int i, int j);
};

void first_improvement_permutation_swap_hill_climbing(Individual& individual, const Problem& problem);

#endif //TINH_TOAN_TIEN_HOA_LOCAL_SEARCH_H
