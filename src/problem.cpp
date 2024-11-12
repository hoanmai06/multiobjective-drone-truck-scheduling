#include "problem.h"

#include "nlohmann/json.hpp"

using json = nlohmann::json;

Problem Problem::from_file(const std::string &data_path, const TruckConfig& truck_config, const DroneConfig& drone_config) {
    std::ifstream file(data_path);
    if (!file.is_open()) throw std::runtime_error("Can't open file " + data_path);

    std::string key;

    int staff_count;
    int drone_count;
    int drone_flight_time_limitation;
    int customer_count;

    file >> key >> staff_count;
    file >> key >> drone_count;
    file >> key >> drone_flight_time_limitation;
    file >> key >> customer_count;

    std::getline(file, key);
    std::getline(file, key);

    Problem problem(staff_count, drone_count, customer_count, drone_flight_time_limitation, truck_config, drone_config);

    std::vector<double> x(customer_count + 1);
    std::vector<double> y(customer_count + 1);

    for (int i = 1; i <= problem.customer_count(); ++i) {
        file >> x[i]
             >> y[i]
             >> problem.demand(i)
             >> problem.does_customer_require_truck(i)
             >> problem.service_time_by_truck(i)
             >> problem.service_time_by_drone(i);
    }

    for (int i = 0; i <= problem.customer_count(); ++i) {
        for (int j = 0; j <= problem.customer_count(); ++j) {
            problem.distance(i, j) = std::sqrt((x[i] - x[j])*(x[i] - x[j]) + (y[i] - y[j])*(y[i] - y[j]));
        }
    }

    return problem;
}

TruckConfig::TruckConfig(const std::string &config_path) {
    std::ifstream config(config_path);
    if (!config.is_open()) throw std::runtime_error("Can't open file " + config_path);

    json parsed_config = json::parse(config);

    v_max = parsed_config["V_max (m/s)"];
    M_t = parsed_config["M_t (kg)"];

    velocity_factors[0] = parsed_config["T (hour)"]["0-1"];
    velocity_factors[1] = parsed_config["T (hour)"]["1-2"];
    velocity_factors[2] = parsed_config["T (hour)"]["2-3"];
    velocity_factors[3] = parsed_config["T (hour)"]["3-4"];
    velocity_factors[4] = parsed_config["T (hour)"]["4-5"];
    velocity_factors[5] = parsed_config["T (hour)"]["5-6"];
    velocity_factors[6] = parsed_config["T (hour)"]["6-7"];
    velocity_factors[7] = parsed_config["T (hour)"]["7-8"];
    velocity_factors[8] = parsed_config["T (hour)"]["8-9"];
    velocity_factors[9] = parsed_config["T (hour)"]["9-10"];
    velocity_factors[10] = parsed_config["T (hour)"]["10-11"];
    velocity_factors[11] = parsed_config["T (hour)"]["11-12"];
}

DroneConfig::DroneConfig(const std::string &config_path, const std::string &name) {
    std::ifstream file(config_path);
    if (!file.is_open()) throw std::runtime_error("Can't open file " + config_path);

    json parsed_config = json::parse(file);

    bool isNonlinear = parsed_config.contains("k1");

    takeoff_speed = parsed_config[name]["takeoffSpeed [m/s]"];
    cruise_speed = parsed_config[name]["cruiseSpeed [m/s]"];
    landing_speed = parsed_config[name]["landingSpeed [m/s]"];
    cruise_altitude = parsed_config[name]["cruiseAlt [m]"];
    capacity = parsed_config[name]["capacity [kg]"];
    battery_capacity = parsed_config[name]["batteryPower [Joule]"];

    if (!isNonlinear) {
        beta = parsed_config[name]["beta(w/kg)"];
        gamma = parsed_config[name]["gama(w)"];
        fixed_time = parsed_config[name]["FixedTime (s)"];
        fixed_distance = parsed_config[name]["FixedDistance (m)"];
    }

    if (isNonlinear) {
        k1 = parsed_config["k1"];
        k2 = parsed_config["k2 (sqrt(kg/m)"];
        c1 = parsed_config["c1 (sqrt(m/kg)"];
        c2 = parsed_config["c2 (sqrt(m/kg)"];
        c4 = parsed_config["c4 (kg/m)"];
        c5 = parsed_config["c5 (Ns/m)"];
    }
}

void Truck::_go(int customer) {
    double distance = _problem.distance(_current_node, customer);
    _current_node = customer;

    int hour = (int) (_current_time / 3600);
    double seconds_till_next_hour = 3600 * (hour + 1) - _current_time;
    double distance_travelled_till_next_hour = seconds_till_next_hour * _config.v_max * _config.velocity_factors[hour % 12];

    // Nếu quãng đường đi được hết trong giờ hiện tại thì đi như thường
    if (distance_travelled_till_next_hour > distance) {
        _current_time += distance / (_config.v_max * _config.velocity_factors[hour % 12]);
        return;
    }

    // Nếu không
    while (true) {
        // Đi tới hết giờ, tăng giờ, kiểm tra lại có đi hết được trong cùng giờ thì trả về
        distance -= distance_travelled_till_next_hour;
        ++hour;
        distance_travelled_till_next_hour = 3600 * _config.v_max * _config.velocity_factors[hour % 12];

        if (distance_travelled_till_next_hour > distance) {
            _current_time = 3600 * hour + distance / (_config.v_max * _config.velocity_factors[hour % 12]);
            return;
        }
    }
}

void Truck::serve(int customer) {
    _go(customer);
    _current_time += _problem.service_time_by_truck(customer);
}

double Truck::time_when_go_back_depot() {
    Truck temp = *this;
    temp._go(0);
    return temp._current_time;
}

void LinearDrone::_go(int customer) {
    double time;

    // Cất cánh
    time = _config.cruise_altitude / _config.takeoff_speed;
    _current_time += time;
    _energy_consumed += time*_power();

    // Bay tới đích
    double distance = _problem.distance(_current_node, customer);
    time = distance / _config.cruise_speed;
    _current_time += time;
    _energy_consumed += time*_power();

    // Hạ cánh
    time = _config.cruise_altitude / _config.landing_speed;
    _current_time += time;
    _energy_consumed += time*_power();

    // Thay đổi vị trí hiện tại
    _current_node = customer;
}

bool LinearDrone::_is_valid() const {
    // 1. Ràng buộc trọng tải
    if (_current_load > _config.capacity) return false;

    // 2. Ràng buộc năng lượng
    if (_energy_consumed > _config.battery_capacity) return false;

    // TODO: Hỏi cô về ràng buộc fix_distance, fix_time trong config và droneLimitationFightTime trong data

    return true;
}

double LinearDrone::time_when_go_back_depot() {
    LinearDrone temp = *this;
    temp._go(0);
    return temp.time();
}

bool LinearDrone::can_serve(int customer) const {
    // Drone không được tới những khách mà yêu cầu xe tải
    if (_problem.does_customer_require_truck(customer)) return false;

    // Thử đi tới đó và kiểm tra xem drone còn valid không
    LinearDrone temp = *this;
    temp.serve(customer);
    if (!temp._is_valid()) return false;

    temp._go(0);
    return temp._is_valid();
}

void LinearDrone::serve(int customer) {
    _go(customer);
    _current_time += _problem.service_time_by_drone(customer);
    _current_load += _problem.demand(customer);
}

bool Solution::is_valid() const {
    // 1. Drone chỉ đi những đơn drone đi được
    // 2. Drone đảm bảo năng lượng
    // 3. Drone còn đủ năng lượng quay về depot
    // Các logic này được xử lý bởi hàm Drone::can_go

    for (const std::vector<Route>& drone_routes : drones) {
        for (const Route& route : drone_routes) {
            Drone drone = problem.drone();
            for (int customer : route) {
                if (!drone.can_serve(customer)) return false;
                drone.serve(customer);
            }

            if (!drone.can_serve(0)) return false;
        }
    }

    return true;
}

std::pair<double, double> truck_objectives(const Route& route, const Problem& problem) {
    double finish_time;
    double total_waiting_time = 0;
    double previous_time = 0;

    Truck truck = problem.truck();

    // Duyệt qua các khách hàng trong route
    for (int i = 0; i < route.size(); ++i) {
        truck.serve(route[i]);

        // Thời gian phục vụ khách hàng i được chờ bởi i khách hàng trước đó trong xe
        total_waiting_time += (truck.time() - previous_time) * i;
        previous_time = truck.time();
    }

    finish_time = truck.time_when_go_back_depot();

    // Thời gian về depot được chờ bởi tất cả đơn trong xe
    total_waiting_time += (finish_time - previous_time) * (double) route.size();
    return {finish_time, total_waiting_time};
}

std::pair<double, double> drone_objectives(const std::vector<Route>& routes, const Problem& problem) {
    double finish_time = 0;
    double total_waiting_time = 0;

    // Duyệt qua các chuyến của drone
    for (const Route& route : routes) {
        double previous_time = 0;
        Drone drone = problem.drone();

        // Duyệt qua các khách hàng trong route và phục vụ khách đó
        for (int i = 0; i < route.size(); ++i) {
            drone.serve(route[i]);

            // Thời gian phục vụ khách hàng i được chờ bởi i khách hàng trước đó trong drone
            total_waiting_time += (drone.time() - previous_time) * i;
            previous_time = drone.time();
        }

        double route_finish_time = drone.time_when_go_back_depot();
        finish_time += route_finish_time;

        // Thời gian về depot được chờ bởi mọi khách trước
        total_waiting_time += (route_finish_time - previous_time) * (double) route.size();
    }

    return {finish_time, total_waiting_time};
}

std::pair<double, double> Solution::objectives() const {
    double latest_finish_time = 0;
    double total_waiting_time = 0;

    // Xử lý truck
    for (const Route& truck_route : trucks) {
        std::pair<double, double> objectives = truck_objectives(truck_route, problem);

        if (objectives.first > latest_finish_time) latest_finish_time = objectives.first;
        total_waiting_time += objectives.second;
    }

    // Xử lý drone
    for (const std::vector<Route>& drone_routes : drones) {
        std::pair<double, double> objectives = drone_objectives(drone_routes, problem);

        if (objectives.first > latest_finish_time) latest_finish_time = objectives.first;
        total_waiting_time += objectives.second;
    }

    return {latest_finish_time, total_waiting_time};
}
