cmake_minimum_required(VERSION 3.29)
project(tinh_toan_tien_hoa)

set(CMAKE_CXX_STANDARD 14)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(
        tinh_toan_tien_hoa
        src/main.cpp
        src/problem.cpp
        src/genetic.cpp
)

target_link_libraries(tinh_toan_tien_hoa PRIVATE nlohmann_json::nlohmann_json)
