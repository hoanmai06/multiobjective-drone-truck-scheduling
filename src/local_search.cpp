#include "local_search.h"
#include "log.h"

#include <chrono>

bool is_better(const Fitness& lhs, const Fitness& rhs) {
    return (lhs[0] < rhs[0] && lhs[1] <= rhs[1]) || (lhs[0] <= rhs[0] && lhs[1] < rhs[1]);
}

bool IndividualInformation::can_swap(int i, int j) {
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
        int customer_index = index_of_first_customer_in_same_trip(i);
        int customer = (customer_index == i) ? _individual.permutation_gene[j] : _individual.permutation_gene[customer_index];
        if (customer_index == j) customer = _individual.permutation_gene[i];
        drone.serve(customer);

        for (customer_index = index_of_first_customer_in_same_trip(i) + 1;; ++customer_index) {
            if (_individual.binary_gene[customer_index - 1] == 1 || customer_index >= _individual.permutation_gene.size()) break;
            customer = (customer_index == i) ? _individual.permutation_gene[j] : _individual.permutation_gene[customer_index];
            if (customer_index == j) customer = _individual.permutation_gene[i];
            if (!drone.can_serve(customer)) return false;
            drone.serve(customer);
        }
    }

    if (j >= _first_drone_customer_index) {
        Drone drone = _problem.drone();
        int customer_index = index_of_first_customer_in_same_trip(j);
        int customer = (customer_index == j) ? _individual.permutation_gene[i] : _individual.permutation_gene[customer_index];
        if (customer_index == i) customer = _individual.permutation_gene[j];
        drone.serve(customer);

        for (customer_index = index_of_first_customer_in_same_trip(j) + 1;; ++customer_index) {
            if (_individual.binary_gene[customer_index - 1] == 1 || customer_index >= _individual.permutation_gene.size()) break;
            customer = (customer_index == j) ? _individual.permutation_gene[i] : _individual.permutation_gene[customer_index];
            if (customer_index == i) customer = _individual.permutation_gene[j];
            if (!drone.can_serve(customer)) return false;
            drone.serve(customer);
        }
    }

    return result;
}

Fitness IndividualInformation::fitness_after_swap(int i, int j) {
    int trip_i = _trip_of_index[i];
    int trip_j = _trip_of_index[j];

    // Lưu lại thời điểm kết thúc và thời gian chờ ở những trip bị tráo
    double finish_time_i = _trip_finish_times[trip_i];
    double finish_time_j = _trip_finish_times[trip_j];
    double wait_time_i = _trip_wait_times[trip_i];
    double wait_time_j = _trip_wait_times[trip_j];

    double route_finish_i;
    double route_finish_j;

    if (i < _first_drone_customer_index) route_finish_i = _route_finish_times[trip_i];
    if (j < _first_drone_customer_index) route_finish_j = _route_finish_times[trip_j];

    // Tính lại thời điểm kết thúc và thời gian chờ chỉ ở những trip bị tráo

    // Nếu i ở trip truck
    if (i < _first_drone_customer_index) {
        Truck truck = _problem.truck();
        int customer_count = 0;
        double previous_time = 0;
        _trip_wait_times[trip_i] = 0;
        for (int customer_index = _index_of_first_customer_of_trip[trip_i]; customer_index < _index_of_first_customer_of_trip[trip_i + 1]; ++customer_index) {
            int customer = (customer_index == i) ? _individual.permutation_gene[j] : _individual.permutation_gene[customer_index];
            if (customer_index == j) customer = _individual.permutation_gene[i];

            truck.serve(customer);
            _trip_wait_times[trip_i] += customer_count * (truck.time() - previous_time);
            ++customer_count;
            previous_time = truck.time();
        }
        _trip_wait_times[trip_i] += customer_count * (truck.time_when_go_back_depot() - truck.time());
        _trip_finish_times[trip_i] = truck.time_when_go_back_depot();
    }
    // Nếu i ở trip drone
    else {
        Drone drone = _problem.drone();
        int customer_count = 0;
        double previous_time = 0;
        _trip_wait_times[trip_i] = 0;
        for (int customer_index = _index_of_first_customer_of_trip[trip_i]; customer_index < _index_of_first_customer_of_trip[trip_i + 1]; ++customer_index) {
            int customer = (customer_index == i) ? _individual.permutation_gene[j] : _individual.permutation_gene[customer_index];
            if (customer_index == j) customer = _individual.permutation_gene[i];

            drone.serve(customer);
            _trip_wait_times[trip_i] += customer_count * (drone.time() - previous_time);
            ++customer_count;
            previous_time = drone.time();
        }
        _trip_wait_times[trip_i] += customer_count * (drone.time_when_go_back_depot() - drone.time());
        _trip_finish_times[trip_i] = drone.time_when_go_back_depot();
    }

    // Nếu j ở trip truck
    if (j < _first_drone_customer_index) {
        Truck truck = _problem.truck();
        int customer_count = 0;
        double previous_time = 0;
        _trip_wait_times[trip_j] = 0;
        for (int customer_index = _index_of_first_customer_of_trip[trip_j]; customer_index < _index_of_first_customer_of_trip[trip_j + 1]; ++customer_index) {
            int customer = (customer_index == j) ? _individual.permutation_gene[i] : _individual.permutation_gene[customer_index];
            if (customer_index == i) customer = _individual.permutation_gene[j];

            truck.serve(customer);
            _trip_wait_times[trip_j] += customer_count * (truck.time() - previous_time);
            ++customer_count;
            previous_time = truck.time();
        }
        _trip_wait_times[trip_j] += customer_count * (truck.time_when_go_back_depot() - truck.time());
        _trip_finish_times[trip_j] = truck.time_when_go_back_depot();
    }
    // Nếu j ở trip drone
    else {
        Drone drone = _problem.drone();
        int customer_count = 0;
        double previous_time = 0;
        _trip_wait_times[trip_j] = 0;
        for (int customer_index = _index_of_first_customer_of_trip[trip_j]; customer_index < _index_of_first_customer_of_trip[trip_j + 1]; ++customer_index) {
            int customer = (customer_index == j) ? _individual.permutation_gene[i] : _individual.permutation_gene[customer_index];
            if (customer_index == i) customer = _individual.permutation_gene[j];

            drone.serve(customer);
            _trip_wait_times[trip_j] += customer_count * (drone.time() - previous_time);
            ++customer_count;
            previous_time = drone.time();
        }
        _trip_wait_times[trip_j] += customer_count * (drone.time_when_go_back_depot() - drone.time());
        _trip_finish_times[trip_j] = drone.time_when_go_back_depot();
    }

    // Sinh ra kết quả

    if (i < _first_drone_customer_index) _route_finish_times[trip_i] = _trip_finish_times[trip_i];
    if (j < _first_drone_customer_index) _route_finish_times[trip_j] = _trip_finish_times[trip_j];

    // Tối ưu: Nếu cả i và j đều ở trip truck thì finish_time là max của _trip_finish_times luôn
    double finish_time = (i < _first_drone_customer_index && j < _first_drone_customer_index)
            ? *std::max_element(_route_finish_times.begin(), _route_finish_times.end())
            : latest_finish_time(_trip_finish_times, _problem);

    double total_wait_time = std::accumulate(_trip_wait_times.begin(), _trip_wait_times.end(), 0.0);

    // Trả lại giá trị thời điểm kết thúc và thời gian chờ của các trip về ban đầu
    _trip_finish_times[trip_i] = finish_time_i;
    _trip_finish_times[trip_j] = finish_time_j;
    _trip_wait_times[trip_i] = wait_time_i;
    _trip_wait_times[trip_j] = wait_time_j;

    if (i < _first_drone_customer_index) _route_finish_times[trip_i] = route_finish_i;
    if (j < _first_drone_customer_index) _route_finish_times[trip_j] = route_finish_j;

    return {finish_time, total_wait_time};
}

void first_improvement_permutation_swap_hill_climbing(Individual& individual, const Problem& problem) {
//    auto start_time = std::chrono::high_resolution_clock::now();

    Fitness current_fitness = fitness(individual, problem);

BEGIN:
    IndividualInformation information(individual, problem);

    for (int i = 0; i < individual.permutation_gene.size(); ++i) {
        for (int j = i + 1; j < individual.permutation_gene.size(); ++j) {
            // Kiểm tra tính hợp lệ của phép tráo
            // Tráo 2 khách ở xe tải luôn là valid, chỉ kiểm tra valid nếu i hoặc j ở drone
            if (i >= information.first_drone_customer_index() || j >= information.first_drone_customer_index()) {
                if (problem.does_customer_require_truck(individual.permutation_gene[i]) && j >= information.first_drone_customer_index()) break;
                if (!information.can_swap(i, j)) continue;
            }

            // Tính fitness của phép tráo
            Fitness fitness = information.fitness_after_swap(i, j);

            if (is_better(fitness, current_fitness)) {
                current_fitness = std::move(fitness);
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
