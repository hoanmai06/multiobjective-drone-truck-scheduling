cmake_minimum_required(VERSION 3.29)
project(tinh_toan_tien_hoa)

set(CMAKE_CXX_STANDARD 14)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

enable_testing()
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(
        tinh_toan_tien_hoa
        src/main.cpp
        src/problem.cpp
        src/genetic.cpp
)

add_executable(truck_tests
        tests/tests.cpp
        src/genetic.cpp
        src/problem.cpp
)

target_link_libraries(tinh_toan_tien_hoa PRIVATE nlohmann_json::nlohmann_json)

target_link_libraries(truck_tests GTest::gtest_main)
target_link_libraries(truck_tests nlohmann_json::nlohmann_json)

include(GoogleTest)
gtest_discover_tests(truck_tests)
