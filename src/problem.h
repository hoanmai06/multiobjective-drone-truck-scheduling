#ifndef TINH_TOAN_TIEN_HOA_PROBLEM_H
#define TINH_TOAN_TIEN_HOA_PROBLEM_H

#include "nlohmann/json.hpp"
#include "drone_type.h"

#include <vector>
#include <fstream>
#include <unordered_map>

using json = nlohmann::json;

class Problem;

class TruckConfig {
public:
    double v_max;
    double M_t;
    double velocity_factors[12]{0};

    explicit TruckConfig(const std::string& config_path);
};

class DroneConfig {
public:
    // General parameter
    double takeoff_speed = 0;
    double cruise_speed = 0;
    double landing_speed = 0;
    double cruise_altitude = 0;
    double capacity = 0;
    double battery_capacity = 0;

    // Linear parameter
    double beta = 0;
    double gamma = 0;
    double fixed_time = 0;
    double fixed_distance = 0;

    // Nonlinear parameter
    double k1 = 0;
    double k2 = 0;
    double c1 = 0;
    double c2 = 0;
    double c4 = 0;
    double c5 = 0;

    DroneConfig(const std::string& config_path, const std::string& name);
};

class Truck {
private:
    const Problem& _problem;
    const TruckConfig& _config;

    double _current_time = 0;
    int _current_node = 0;

    void _go(int customer);
public:
    Truck(const Problem& problem, const TruckConfig& config) : _problem(problem), _config(config) {}
    double time() const {return _current_time;}
    double time_when_go_back_depot();
    void serve(int customer);
};

class LinearDrone {
private:
    const Problem& _problem;
    const DroneConfig& _config;

    int _current_node = 0;
    double _current_time = 0;
    double _current_load = 0;
    double _energy_consumed = 0;

    void _go(int customer);
    double _power() const {return _config.beta * _current_load + _config.gamma;}
    bool _is_valid() const;
public:
    LinearDrone(const Problem& problem, const DroneConfig& config) : _problem(problem), _config(config) {}
    double time() const {return _current_time;}
    double time_when_go_back_depot();
    bool can_go(int customer) const;
    bool can_go_back_depot_after_serve(int customer) const;
    void serve(int customer);
};

// TODO: Hoàn thiện lớp này
class NonlinearDrone {
public:
    NonlinearDrone(const Problem& problem, const DroneConfig& config) {}
};

class Problem {
private:
    int _truck_count;
    int _drone_count;
    int _drone_flight_time_limitation;
    int _customer_count;

    double* _distances;
    double* _demands;
    bool* _does_customer_require_truck;
    double* _service_time_by_truck;
    double* _service_time_by_drone;

    // Always equals _customer_count + 1
    const int _node_count;

    TruckConfig _truck_config;
    DroneConfig _drone_config;

    Problem(int truck_count, int drone_count, int customer_count, int drone_flight_time_limitation, const TruckConfig& truck_config, const DroneConfig& drone_config)
            : _truck_count(truck_count)
            , _drone_count(drone_count)
            , _customer_count(customer_count)
            , _drone_flight_time_limitation(drone_flight_time_limitation)
            , _node_count(customer_count + 1)
            , _truck_config(truck_config)
            , _drone_config(drone_config)
            , _distances(new double[(customer_count + 1)*(customer_count + 1)])
            , _demands(new double[customer_count + 1])
            , _does_customer_require_truck(new bool[customer_count + 1])
            , _service_time_by_truck(new double[customer_count + 1])
            , _service_time_by_drone(new double[customer_count + 1]) {}
public:
    ~Problem() {
        delete[] _distances;
        delete[] _demands;
        delete[] _does_customer_require_truck;
        delete[] _service_time_by_truck;
        delete[] _service_time_by_drone;
    }

    int truck_count() const {return _truck_count;};
    int drone_count() const {return _drone_count;};
    int drone_flight_time_limitation() const {return _drone_flight_time_limitation;};
    int customer_count() const {return _customer_count;};
    int node_count() const {return _node_count;}

    double& distance(int customer1, int customer2) {return _distances[_node_count*customer1 + customer2];};
    const double& distance(int customer1, int customer2) const {return _distances[_node_count*customer1 + customer2];};

    double& demand(int customer) {return _demands[customer];}
    const double& demand(int customer) const {return _demands[customer];}

    bool& does_customer_require_truck(int customer) {return _does_customer_require_truck[customer];}
    const bool& does_customer_require_truck(int customer) const {return _does_customer_require_truck[customer];}

    double& service_time_by_truck(int customer) {return _service_time_by_truck[customer];}
    const double& service_time_by_truck(int customer) const {return _service_time_by_truck[customer];}

    double& service_time_by_drone(int customer) {return _service_time_by_drone[customer];}
    const double& service_time_by_drone(int customer) const {return _service_time_by_drone[customer];}

    bool can_drone_serve(int customer) const {return !does_customer_require_truck(customer);}

    Truck truck() const {return {*this, _truck_config};}
    Drone drone() const {return {*this, _drone_config};}

    static Problem from_file(const std::string& data_path, const TruckConfig& truck_config, const DroneConfig& drone_config);
};

using Route = std::vector<int>;

class Solution : public std::vector<Route> {
public:
    const Problem& problem;

    std::vector<Route> trucks;
    std::vector<std::vector<Route>> drones;

    explicit Solution(const Problem& problem) : problem(problem) {
        trucks.resize(problem.truck_count());
        drones.resize(problem.drone_count());
    }

    bool is_valid() const;
    std::pair<double, double> objectives() const;
};

#endif //TINH_TOAN_TIEN_HOA_PROBLEM_H
