#include "../src/problem.h"

#include "gtest/gtest.h"

TEST(TRUCK_TEST, TRUCK_NO_CROSSHOUR_TEST) {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "3");
    Problem problem = Problem::from_file("../data/random_data/6.5.1.txt", truck_config, drone_config);

    Truck truck = problem.truck();
    EXPECT_EQ(truck.time(), 0);

    EXPECT_NEAR(problem.distance(0, 1), 1993.739492438524, 0.0000001);
    truck.serve(1);
    double time = problem.distance(0, 1)/0.7/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, 0.0000001);

    EXPECT_NEAR(problem.distance(1, 2), 3768.216922821885, 0.0000001);
    truck.serve(2);
    time += + problem.distance(1, 2)/0.7/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, 0.0000001);

    EXPECT_NEAR(problem.distance(2, 3), 7493.041215108023, 0.0000001);
    truck.serve(3);
    time += problem.distance(2, 3)/0.7/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, 0.0000001);

    EXPECT_NEAR(problem.distance(3, 5), 6691.202948816279, 0.0000001);
    truck.serve(5);
    time += problem.distance(3, 5)/0.7/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, 0.0000001);

    EXPECT_NEAR(problem.distance(5, 0), 3047.0239066824514, 0.0000001);
    time += problem.distance(5, 0)/0.7/15.557;
    EXPECT_NEAR(truck.time_when_go_back_depot(), time, 0.0000001);
}

TEST(TRUCK_TEST, TRUCK_CROSSHOUR_TEST) {
    TruckConfig truck_config("../config_parameter/truck_config.json");
    DroneConfig drone_config("../config_parameter/drone_linear_config.json", "3");
    Problem problem = Problem::from_file("../data/test_data/3_far_customer.txt", truck_config, drone_config);

    Truck truck = problem.truck();
    EXPECT_EQ(truck.time(), 0);

    EXPECT_NEAR(problem.distance(0, 1), 50000, 0.0000001);
    truck.serve(1);
    double time = 3600 + (problem.distance(0, 1) - 39203.64)/0.4/15.557 + 60;
    EXPECT_NEAR(truck.time(), time, 0.0000001);
}
