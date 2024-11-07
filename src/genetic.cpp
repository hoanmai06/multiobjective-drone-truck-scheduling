#include "genetic.h"

Solution decode(const Individual& individual) {
    Solution result(individual.problem);
//
//    int index, current_truck = 0;
//
//    // Điền tất cả đơn cho truck
//    for (index = 0; current_truck < individual.problem.truck_count(); ++index) {
//        result.trucks[current_truck].push_back(individual.permutation_gene[index]);
//
//        if (individual.binary_gene[index] == 1) {
//            ++current_truck;
//        }
//    }
//
//    // index hiện đang trỏ tới khách hàng đầu của drone
//    // Tiếp theo tách các đơn của drone vào drone route để lần lượt tính thời gian hoàn thành
//    // Sau đó phân bổ vào các drone bằng Longest Processing Time First
//    std::vector<Route> drone_routes;
//    drone_routes.emplace_back();
//
//    for (; index < individual.binary_gene.size(); ++index) {
//        drone_routes.back().push_back(individual.permutation_gene[index]);
//        if (individual.binary_gene[index] == 1) {
//            result.emplace_back();
//        }
//    }
//    // Khách cuối luôn thêm vào sau route cuối, không xét ở trên nên phải tự thêm ở dưới
//    result.back().push_back(individual.permutation_gene.back());

    return result;
}