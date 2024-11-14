#ifndef TINH_TOAN_TIEN_HOA_PROBLEM_H
#define TINH_TOAN_TIEN_HOA_PROBLEM_H

#include "drone_type.h"

#include <vector>
#include <fstream>
#include <unordered_map>

class Problem;

// Lớp TruckConfig chứa các thiết lập cho xe tải của bài toán
class TruckConfig {
public:
    // Vận tốc tối đa của xe
    double v_max;

    // TODO: M_t là gì tôi cũng không biết nhưng có trong file config
    double M_t;

    // Hệ số vận tốc theo giờ của truck
    double velocity_factors[12];

    explicit TruckConfig(const std::string& config_path);
};

class DroneConfig {
public:
    // Các thông số mà LinearDrone hay NonlinearDrone đều có

    // Tốc độ cất cánh
    double takeoff_speed = 0;

    // Tốc độ bay ngang
    double cruise_speed = 0;

    // Tốc độ hạ cánh
    double landing_speed = 0;

    // Độ cao bay
    double cruise_altitude = 0;

    // Trọng tải
    double capacity = 0;

    // Dung lượng pin
    double battery_capacity = 0;

    // Các tham số drone tuyến tính mới có, beta với gamma chi tiết trong file mô tả
    // TODO: fixed_time và fixed_distance là gì thì tôi cũng chịu
    double beta = 0;
    double gamma = 0;
    double fixed_time = 0;
    double fixed_distance = 0;

    // Các tham số mà drone phi tuyến mới có
    double k1 = 0;
    double k2 = 0;
    double c1 = 0;
    double c2 = 0;
    double c4 = 0;
    double c5 = 0;

    DroneConfig(const std::string& config_path, const std::string& name);
};

// Lớp Truck dùng để xử lý logic di chuyển của Truck và cung cấp nó qua một interface đơn giản
class Truck {
private:
    // Biến chứa dữ liệu bài toán và thiết lập của truck
    const Problem& _problem;
    const TruckConfig& _config;

    // Biến dùng để lưu trạng thái hiện tại của truck
    double _current_time = 0;
    int _current_node = 0;

    // Hàm này chứa logic di chuyển của truck (vận tốc đổi theo giờ), cập nhật _current_time và _current_node.
    void _go(int customer);
public:
    // Khởi tạo một Truck với Problem và TruckConfig.
    // problem.truck() sẽ trả về truck ứng với bài toán luôn nên sẽ không cần dùng constructor này
    Truck(const Problem& problem, const TruckConfig& config) : _problem(problem), _config(config) {}

    // Copy assignment
    Truck& operator=(const Truck& value) {
        _current_node = value._current_node;
        _current_time = value._current_time;
        return *this;
    }

    // Trả về thời điểm hiện tại của truck
    double time() const {return _current_time;}

    // Trả về thời điểm của truck nếu nó về depot
    double time_when_go_back_depot();

    // Đưa truck đi phục vụ customer
    // Hàm này sẽ cập nhật _current_time và _current_node của truck
    void serve(int customer);
};

// Lớp LinearDrone dùng để xử lý logic di chuyển của drone tuyến tính và cung cấp nó qua một interface đơn giản
class LinearDrone {
private:
    // Biến chứa dữ liệu bài toán và thiết lập của drone
    const Problem& _problem;
    const DroneConfig& _config;

    // Biến chứa trạng thái của drone
    int _current_node = 0;
    double _current_time = 0;
    double _current_load = 0;
    double _energy_consumed = 0;

    // Hàm chứa logic di chuyển của drone (cất cánh, bay ngang rồi hạ cánh) và thay đổi trạng thái drone tương ứng
    void _go(int customer);

    // Hàm trả về công suất hiện tại của drone (phụ thuộc vào trọng tải, beta và gamma)
    double _power() const {return _config.beta * _current_load + _config.gamma;}

    // Hàm trả về liệu trạng thái của drone hiện tại có hợp lệ không
    bool _is_valid() const;
public:
    // Khởi tạo một Drone với Problem và DroneConfig.
    // problem.drone() sẽ trả về drone ứng với bài toán luôn nên sẽ không cần dùng constructor này
    LinearDrone(const Problem& problem, const DroneConfig& config) : _problem(problem), _config(config) {}

    // Copy assignment
    LinearDrone& operator=(const LinearDrone& value) {
        _current_node = value._current_node;
        _current_time = value._current_time;
        _current_load = value._current_load;
        _energy_consumed = value._energy_consumed;
        return *this;
    }

    // Trả về thời điểm hiện tại của drone
    double time() const {return _current_time;}

    // Trả về thời điểm của drone nếu giờ nó đi về depot
    double time_when_go_back_depot();

    // Trả về lượng hàng drone đang mang
    double load() const {return _current_load;}

    // Trả về trọng tải của drone
    double capacity() const {return _config.capacity;}

    // Trả về lượng năng lượng mà drone đã tiêu thụ
    double energy_consumed() const {return _energy_consumed;}

    // Trả về năng lượng tối đa của drone
    double battery_capacity() const {return _config.battery_capacity;}

    // Trả về liệu drone có thể phục vụ được khách này và vẫn còn năng lượng để trở về depot không
    bool can_serve(int customer) const;

    // Đưa drone đi phục vụ customer
    void serve(int customer);
};

// TODO: Lớp này chưa hoàn thiện
class NonlinearDrone {
public:
    NonlinearDrone(const Problem& problem, const DroneConfig& config) {}
};

// Lớp Problem chứa các thông tin về bài toán
class Problem {
private:
    // Các tham số cơ bản: số xe, số drone, giới hạn thời gian bay của drone, số khách hàng
    int _truck_count;
    int _drone_count;
    int _drone_flight_time_limitation;
    int _customer_count;

    // Mảng chứa khoảng cách giữa các node trong bài
    // Không cần quan tâm, truy cập được qua hàm distance(int i, int j)
    double* _distances;

    // Danh sách các demand của khách
    double* _demands;

    // _does_customer_require_truck[i] chứa liệu khách i có yêu cầu xe tải không
    bool* _does_customer_require_truck;

    // _service_time_by_truck[i] chứa thời gian phục vụ bằng truck của khách i
    double* _service_time_by_truck;

    // _service_time_by_drone[i] chứa thời gian phục vụ bằng drone của khách i
    double* _service_time_by_drone;

    // Cái này tôi cũng không nhớ tại sao tôi cho vào
    const int _node_count;

    // Danh sách các khách yêu cầu xe tải
    std::vector<int> _truck_only_customers;

    // Mảng chứa liệu drone có thể phục vụ khách không
    std::vector<bool> _can_drone_serve;

    // Problem lưu lại bản sao config của truck và drone
    TruckConfig _truck_config;
    DroneConfig _drone_config;

    // Constructor khởi tạo các thành phần nêu trên
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
            , _service_time_by_drone(new double[customer_count + 1])
            , _can_drone_serve(customer_count + 1) {}
public:
    // Destructor hủy các con trỏ được khai báo động, nếu không sẽ dò bộ nhớ
    ~Problem() {
        delete[] _distances;
        delete[] _demands;
        delete[] _does_customer_require_truck;
        delete[] _service_time_by_truck;
        delete[] _service_time_by_drone;
    }

    // Trả về số lượng xe tải trong bài toán
    int truck_count() const {return _truck_count;};

    // Trả về số lượng drone trong bài toán
    int drone_count() const {return _drone_count;};

    // Trả về giới hạn thời gian bay của drone
    int drone_flight_time_limitation() const {return _drone_flight_time_limitation;};

    // Trả về số khách hàng trong bài toán
    int customer_count() const {return _customer_count;};

    // Trả về số nút trong bài toán (số khách hàng + 1)
    int node_count() const {return _node_count;}

    const std::vector<int>& truck_only_customers() const {return _truck_only_customers;}
    int truck_only_customer_count() const {return (int) _truck_only_customers.size();}

    // Các hàm tiếp theo có 2 bản, một bản trả về reference (có thể chỉnh sửa qua lời gọi hàm kiểu problem.distance(1, 2) = 10)
    // và một bản là const reference (chỉ đọc)

    // Hàm trả về khoảng cách
    double& distance(int customer1, int customer2) {return _distances[_node_count*customer1 + customer2];};
    const double& distance(int customer1, int customer2) const {return _distances[_node_count*customer1 + customer2];};

    // Trả về demand của khách customer
    double& demand(int customer) {return _demands[customer];}
    const double& demand(int customer) const {return _demands[customer];}

    // Trả về liệu khách customer có yêu cầu truck không
    bool& does_customer_require_truck(int customer) {return _does_customer_require_truck[customer];}
    const bool& does_customer_require_truck(int customer) const {return _does_customer_require_truck[customer];}

    // Trả về thời gian phục vụ khách customer bằng xe tải
    double& service_time_by_truck(int customer) {return _service_time_by_truck[customer];}
    const double& service_time_by_truck(int customer) const {return _service_time_by_truck[customer];}

    // Trả về thời gian phục vụ khách customer bằng drone
    double& service_time_by_drone(int customer) {return _service_time_by_drone[customer];}
    const double& service_time_by_drone(int customer) const {return _service_time_by_drone[customer];}

    // Hàm trả về liệu drone có phục vụ được khách customer hay không
    bool can_drone_serve(int customer) const {return _can_drone_serve[customer];}

    // Hàm trả về một bản truck/drone của bài toán
    Truck truck() const {return {*this, _truck_config};}
    Drone drone() const {return {*this, _drone_config};}

    // Hàm để đọc bài toán từ file
    static Problem from_file(const std::string& data_path, const TruckConfig& truck_config, const DroneConfig& drone_config);
};

// Định nghĩa một sộ trình là một vector kiểu int
using Route = std::vector<int>;

// Lớp chứa nghiệm bài toán
class Solution {
public:
    // Biến chứa thông tin về bài toán
    const Problem& problem;

    // Danh sách lộ trình xe tải
    std::vector<Route> trucks;

    // Danh sách lộ trình của drone
    std::vector<std::vector<Route>> drones;

    // Hàm khởi tạo một solution rỗng với problem
    // Danh sách lộ trình xe tải có truck_count lộ trình rỗng
    // Danh sách lộ trình drone có drone_count lộ trình rỗng
    explicit Solution(const Problem& problem) : problem(problem) {
        trucks.resize(problem.truck_count());
        drones.resize(problem.drone_count());
    }

    // Hàm kiểm tra nghiệm hiện tại có hợp lệ không
    bool is_valid() const;

    // Hàm tính mục tiêu của nghiệm
    std::pair<double, double> objectives() const;
};

#endif //TINH_TOAN_TIEN_HOA_PROBLEM_H
