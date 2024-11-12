#include "../src/problem.h"
#include "../src/genetic.h"

#include "gtest/gtest.h"

const double EPSILON = 0.0000001;

TEST(PROBLEM_TEST, PROBLEM_DISTANCE_TEST) {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "3");
    Problem problem = Problem::from_file("../data/random_data/6.5.1.txt", truck_config, drone_config);

    EXPECT_NEAR(problem.distance(0, 1), 1993.739492438524, EPSILON);
    EXPECT_NEAR(problem.distance(1, 2), 3768.216922821885, EPSILON);
    EXPECT_NEAR(problem.distance(2, 3), 7493.041215108023, EPSILON);
    EXPECT_NEAR(problem.distance(3, 5), 6691.202948816279, EPSILON);
    EXPECT_NEAR(problem.distance(5, 0), 3047.0239066824514, EPSILON);
}

TEST(TRUCK_TEST, TRUCK_NO_CROSSHOUR_TEST) {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "3");
    Problem problem = Problem::from_file("../data/random_data/6.5.1.txt", truck_config, drone_config);

    Truck truck = problem.truck();
    EXPECT_EQ(truck.time(), 0);

    truck.serve(1);
    double time = problem.distance(0, 1)/0.7/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, EPSILON);

    truck.serve(2);
    time += + problem.distance(1, 2)/0.7/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, EPSILON);

    truck.serve(3);
    time += problem.distance(2, 3)/0.7/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, EPSILON);

    truck.serve(5);
    time += problem.distance(3, 5)/0.7/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, EPSILON);

    time += problem.distance(5, 0)/0.7/15.557;
    EXPECT_NEAR(truck.time_when_go_back_depot(), time, EPSILON);
}

TEST(TRUCK_TEST, TRUCK_CROSSHOUR_TEST) {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "3");
    Problem problem = Problem::from_file("../data/test_data/3_far_customer.txt", truck_config, drone_config);

    Truck truck = problem.truck();
    EXPECT_EQ(truck.time(), 0);

    truck.serve(1);
    double time = 3600 + (50000 - 3600*0.7*15.557)/0.4/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, EPSILON);
}

TEST(LINEAR_DRONE_TEST, LINEAR_DRONE_TEST) {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "3");
    Problem problem = Problem::from_file("../data/random_data/6.5.1.txt", truck_config, drone_config);

    LinearDrone drone = problem.drone();

    // Drone không tới được những khách yêu cầu truck
    EXPECT_EQ(problem.can_drone_serve(1), false);
    EXPECT_EQ(problem.can_drone_serve(2), false);
    EXPECT_EQ(problem.can_drone_serve(3), false);
    EXPECT_EQ(problem.can_drone_serve(4), true);
    EXPECT_EQ(problem.can_drone_serve(5), false);
    EXPECT_EQ(problem.can_drone_serve(6), true);

    // Thời gian bay, load và năng lượng của drone
    drone.serve(4);
    double time = 50/7.8232 + 1657.9864510121794/15.6464 + 50/3.9116 + 30;
    double energy_consumed = 181.2*(50/7.8232 + 1657.9864510121794/15.6464 + 50/3.9116);
    EXPECT_NEAR(problem.distance(0, 4), 1657.9864510121794, EPSILON);
    EXPECT_NEAR(drone.time(), time, EPSILON);
    EXPECT_EQ(drone.load(), 0.09);
    EXPECT_NEAR(drone.energy_consumed(), energy_consumed, EPSILON);

    drone.serve(6);
    time += 50/7.8232 + 4023.0511190511925/15.6464 + 50/3.9116 + 30;
    energy_consumed += (0.09*210.8 + 181.2)*(50/7.8232 + 4023.0511190511925/15.6464 + 50/3.9116);
    EXPECT_NEAR(problem.distance(4, 6), 4023.0511190511925, EPSILON);
    EXPECT_NEAR(drone.time(), time, EPSILON);
    EXPECT_EQ(drone.load(), 0.18);
    EXPECT_NEAR(drone.energy_consumed(), energy_consumed, EPSILON);
}

TEST(OBJECTIVE_TEST, OBJECTIVE_TEST) {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "3");
    Problem problem = Problem::from_file("../data/random_data/6.5.1.txt", truck_config, drone_config);

    // Objective khi chỉ có 1 truck
    Solution solution(problem);
    solution.trucks[0] = {1, 2, 3, 5};

    EXPECT_NEAR(solution.objectives().first, 2351.426595824311, EPSILON);
    EXPECT_NEAR(solution.objectives().second, 5044.70819991153, EPSILON);

    // Objective khi chỉ có 1 drone, 1 chuyến
    solution.trucks[0] = {};
    solution.drones[0] = {{4, 6}};
    EXPECT_NEAR(problem.distance(6, 0), 2914.2983382522502, EPSILON);
    EXPECT_NEAR(solution.objectives().first, 666.8703285302448, EPSILON);
    EXPECT_NEAR(solution.objectives().second, 717.1643186647211, EPSILON);

    // Objective khi có 1 drone, 2 chuyến
    solution.trucks[0] = {};
    solution.drones[0] = {{4}, {6}};
    EXPECT_NEAR(problem.distance(4, 0), 1657.9864510121793, EPSILON);
    EXPECT_NEAR(solution.objectives().first, 721.1469461683748, EPSILON);
    EXPECT_NEAR(solution.objectives().second, 330.5734730841874, EPSILON);

    // Objective khi 1 truck, 1 drone đi 2 chuyến
    solution.trucks[0] = {1, 2, 3, 5};
    solution.drones[0] = {{4}, {6}};
    EXPECT_NEAR(solution.objectives().first, 2351.426595824311, EPSILON);
    EXPECT_NEAR(solution.objectives().second, 5044.70819991153 + 330.5734730841874, EPSILON);
}
