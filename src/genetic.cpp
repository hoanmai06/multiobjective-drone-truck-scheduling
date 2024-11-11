#include "genetic.h"

#include <numeric>
#include <algorithm>

// Hàm để giải mã từ Individual về Solution
// Hàm hơi dài :(
Solution decode(const Individual& individual) {
    const Problem& problem = individual.problem;
    Solution solution(individual.problem);

    // Đếm số route trong individual bằng số số 1 trong binary cộng 1
    int route_count = (int) std::count(individual.binary_gene.begin(), individual.binary_gene.end(), 1) + 1;
    int current_truck = 0;
    int index;

    // Điền các route vào trong truck

    // Khách hàng đầu tiên luôn cho vào route đầu
    solution.trucks[current_truck].push_back(individual.permutation_gene.front());

    // Duyệt qua các khách còn lại, nếu trước khách đó là số 1 thì ngắt route rồi thêm khách vào route
    for (index = 1; index < individual.permutation_gene.size(); ++index) {
        if (individual.binary_gene[index - 1] == 1) {
            ++current_truck;
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