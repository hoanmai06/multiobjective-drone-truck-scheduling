#include "problem.h"

#include <iostream>
#include <fstream>

// Chỉnh loại drone tuyến tính hay phi tuyến trong drone_type.h

int main() {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "3");
    Problem problem = Problem::from_file("../data/random_data/6.5.1.txt", truck_config, drone_config);

    return 0;
}
