#include "genetic.h"

#include <numeric>
#include <algorithm>

// Hàm để giải mã từ Individual về Solution
// Hàm hơi dài :(
Solution decode(const Individual& individual, const Problem& problem) {
    Solution solution(problem);

    // Đếm số route trong individual bằng số số 1 trong binary cộng 1
    int current_truck = 0;
    int index;

    // Điền các route vào trong truck
    // Khách hàng đầu tiên luôn cho vào route đầu
    solution.trucks[current_truck].push_back(individual.permutation_gene.front());

    // Duyệt qua các khách còn lại, nếu trước khách đó là số 1 thì ngắt route rồi thêm khách vào route
    for (index = 1; index < individual.permutation_gene.size(); ++index) {
        if (individual.binary_gene[index - 1] == 1) {
            ++current_truck;
            // Nếu số truck đã quá rồi thì khách này là của drone
            if (current_truck == problem.truck_count()) break;
        }
        solution.trucks[current_truck].push_back(individual.permutation_gene[index]);
    }

    // Nếu individual không chứa drone thì trả về luôn
    if (index == individual.permutation_gene.size()) return solution;

    // Nếu có drone thì index hiện đang trỏ tới khách hàng đầu của drone

    // Tiếp theo cần tính thời gian thực hiện các trip của drone này để thực hiện Longest Processing Time First
    std::vector<double> trip_finish_times;
    trip_finish_times.reserve(problem.node_count());

    // Khởi tạo drone và cho drone đi giao khách đầu tiên
    Drone drone = problem.drone();
    drone.serve(individual.permutation_gene[index]);

    // Duyệt qua các khách của drone
    for (int i = index + 1; i < individual.permutation_gene.size(); ++i) {
        // Nếu trước đó là số 1 thì ngắt route, lưu thời gian và reset drone
        if (individual.binary_gene[i - 1] == 1) {
            trip_finish_times.emplace_back(drone.time_when_go_back_depot());
            drone = problem.drone();
        }

        drone.serve(individual.permutation_gene[i]);
    }

    // Đơn cuối vẫn chưa lưu vào mảng nên lưu thủ công đơn cuối (hiện thì đơn chỉ lưu khi binary trước đó là số 1)
    trip_finish_times.emplace_back(drone.time_when_go_back_depot());

    // Sau khi có danh sách các finish_time thì tạo một mảng xem route thứ i trong gene sẽ vào drone thứ bao nhiêu
    std::vector<int> sorted_trip_indices(trip_finish_times.size());
    std::iota(sorted_trip_indices.begin(), sorted_trip_indices.end(), 0);
    // Sắp xếp index sao cho index có thời gian lớn nhất xếp trước
    std::sort(sorted_trip_indices.begin(), sorted_trip_indices.end(), [&trip_finish_times](int lhs, int rhs) {
        return trip_finish_times[lhs] > trip_finish_times[rhs];
    });

    // Mảng chứa xem thật sự là route i sẽ nằm ở drone nào
    std::vector<int> drone_of_trip(trip_finish_times.size());
    // Mảng chứa thời gian kết thúc của drone
    std::vector<double> drone_finish_times(problem.drone_count());

    // Duyệt qua tất cả các trip theo thứ tự giảm dần về finish_time
    for (int trip_index : sorted_trip_indices) {
        // Chọn drone hiện có finish_times bé nhất
        int drone_index = (int) (std::min_element(drone_finish_times.begin(), drone_finish_times.end()) - drone_finish_times.begin());

        // Thêm trip hiện tại vào drone đó
        drone_of_trip[trip_index] = drone_index;
        drone_finish_times[drone_index] += trip_finish_times[trip_index];
    }

    // Thật sự thêm các trip của drone vào solution
    int trip_index = 0;

    // Khách hàng đầu cho vào trip đầu của drone tương ứng
    solution.drones[drone_of_trip[trip_index]].emplace_back();
    solution.drones[drone_of_trip[trip_index]].back().push_back(individual.permutation_gene[index]);

    // Duyệt qua các khách của drone
    for (int i = index + 1; i < individual.permutation_gene.size(); ++i) {
        // Nếu trước đó là số 1 thì ngắt route
        if (individual.binary_gene[i - 1] == 1) {
            ++trip_index;
            solution.drones[drone_of_trip[trip_index]].emplace_back();
        }

        solution.drones[drone_of_trip[trip_index]].back().push_back(individual.permutation_gene[i]);
    }

    return solution;
}

// Kiểm tra xem các drone route trong individual có hợp lệ không
bool is_drone_routes_valid(const Individual& individual, const Problem& problem) {
    // Tìm vị trí của khách drone đầu tiên
    int index, count = 0;
    for (index = 0; count < problem.truck_count(); ++index) {
        if (individual.binary_gene[index] == 1) ++count;
    }

    // index hiện chỉ tới khách drone đầu tiên
    // Duyệt qua các khách trong các drone route một để xử lý
    Drone drone = problem.drone();
    if (!drone.can_serve(individual.permutation_gene[index])) return false;
    drone.serve(individual.permutation_gene[index]);

    for (int i = index + 1; i < individual.permutation_gene.size(); ++i) {
        if (individual.binary_gene[i - 1] == 1) {
            drone = problem.drone();
        }
        if (!drone.can_serve(individual.permutation_gene[index])) return false;
        drone.serve(individual.permutation_gene[index]);
    }

    return true;
}

bool is_valid(const Individual& individual, const Problem& problem) {
    // Phần hoán vị phải thật sự là hoán vị
    std::vector<bool> has_occurred(individual.permutation_gene.size());
    for (int i : individual.permutation_gene) {
        if (has_occurred[i]) return false;
        has_occurred[i] = true;
    }

    // Các drone route phải hợp lệ
    return is_drone_routes_valid(individual, problem);
}

Fitness fitness(const Individual &individual, const Problem& problem) {
    std::vector<double> all_finish_times(problem.truck_count() + problem.drone_count());
    double total_wait_time = 0;
    int route_index = 0;

    // Tính chi phí phần truck
    // Khởi tạo truck và cho truck đi đơn đầu
    Truck truck = problem.truck();
    truck.serve(individual.permutation_gene.front());

    int index_in_gene, customer_count_in_route = 1;
    double previous_time = truck.time();

    // Duyệt qua các đơn còn lại
    for (index_in_gene = 1; index_in_gene < individual.permutation_gene.size(); ++index_in_gene) {
        // Nếu khách này nằm trong route mới
        if (individual.binary_gene[index_in_gene - 1] == 1) {
            // Lưu lại thời gian về route cũ, thời gian về này được chờ bởi tất cả đơn trước đó
            all_finish_times[route_index] = truck.time_when_go_back_depot();
            total_wait_time += customer_count_in_route * (all_finish_times[route_index] - previous_time);

            // Nếu đã hết route của truck thì khách này là của drone nên break
            if (++route_index == problem.truck_count()) break;

            // Không thì khởi tạo lại truck, thời gian và vị trí khách trong route
            truck = problem.truck();
            previous_time = 0;
            customer_count_in_route = 0;
        }
        truck.serve(individual.permutation_gene[index_in_gene]);
        // Thời gian xe đi này được chờ bởi số khách trước đó trong xe
        total_wait_time += customer_count_in_route * (truck.time() - previous_time);
        previous_time = truck.time();
        ++customer_count_in_route;
    }

    // Nếu individual không chứa drone thì trả về luôn
    if (index_in_gene == individual.permutation_gene.size())
        return {*std::max_element(all_finish_times.begin(), all_finish_times.end()), total_wait_time};

    // Nếu có drone thì index hiện đang trỏ tới khách hàng đầu của drone
    // Tiếp theo cần tính thời gian thực hiện các trip của drone này để thực hiện Longest Processing Time First
    std::vector<double> trip_finish_times;
    trip_finish_times.reserve(problem.node_count());

    // Khởi tạo drone và cho drone đi giao khách đầu tiên
    Drone drone = problem.drone();
    drone.serve(individual.permutation_gene[index_in_gene]);
    customer_count_in_route = 1;
    previous_time = 0;

    // Duyệt qua các khách của drone
    for (int i = index_in_gene + 1; i < individual.permutation_gene.size(); ++i) {
        // Nếu trước đó là số 1 thì ngắt route, lưu thời gian và reset drone
        if (individual.binary_gene[i - 1] == 1) {
            // Thời gian đi về depot được chờ bởi mọi khách trong drone
            total_wait_time += customer_count_in_route * (drone.time_when_go_back_depot() - drone.time());
            customer_count_in_route = 0;
            previous_time = 0;

            trip_finish_times.emplace_back(drone.time_when_go_back_depot());
            drone = problem.drone();
        }

        drone.serve(individual.permutation_gene[i]);
        // Thời gian drone di chuyển này được chờ bởi các khách trước đó trong drone
        total_wait_time += customer_count_in_route * (drone.time() - previous_time);
        previous_time = drone.time();
        ++customer_count_in_route;
    }

    // Đơn cuối vẫn chưa lưu vào mảng nên lưu thủ công đơn cuối (hiện thì đơn chỉ lưu khi binary trước đó là số 1)
    trip_finish_times.emplace_back(drone.time_when_go_back_depot());
    total_wait_time += customer_count_in_route * (drone.time_when_go_back_depot() - drone.time());

    // Sau khi có danh sách các finish_time thì tạo một mảng xem route thứ i trong gene sẽ vào drone thứ bao nhiêu
    std::vector<int> sorted_trip_indices(trip_finish_times.size());
    std::iota(sorted_trip_indices.begin(), sorted_trip_indices.end(), 0);
    // Sắp xếp index sao cho index có thời gian lớn nhất xếp trước
    std::sort(sorted_trip_indices.begin(), sorted_trip_indices.end(), [&trip_finish_times](int lhs, int rhs) {
        return trip_finish_times[lhs] > trip_finish_times[rhs];
    });

    // Mảng chứa thời gian kết thúc của drone
    std::vector<double> drone_finish_times(problem.drone_count());

    // Duyệt qua tất cả các trip theo thứ tự giảm dần về finish_time
    for (int trip_index : sorted_trip_indices) {
        // Chọn drone hiện có finish_times bé nhất
        int drone_index = (int) (std::min_element(drone_finish_times.begin(), drone_finish_times.end()) - drone_finish_times.begin());

        // Thêm trip hiện tại vào drone đó
        drone_finish_times[drone_index] += trip_finish_times[trip_index];
    }

    for (int i = 0; i < drone_finish_times.size(); ++i) {
        all_finish_times[problem.truck_count() + i] = drone_finish_times[i];
    }

    return {*std::max_element(all_finish_times.begin(), all_finish_times.end()), total_wait_time};
}
