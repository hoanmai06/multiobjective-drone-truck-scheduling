#ifndef TINH_TOAN_TIEN_HOA_PRINT_H
#define TINH_TOAN_TIEN_HOA_PRINT_H

#include <iostream>

template <typename T>
void print(const T& value, const std::string& end = "\n") {
    std::cout << value << end;
}

template <typename T>
void print(
        const std::vector<T>& value,
        const std::string& delim = ", ",
        const std::string& after_start = "",
        const std::string& before_end = "",
        const std::string& end = "\n") {
    if (value.empty()) {
        std::cout << "[]" << end;
        return;
    }

    std::cout << "[" << after_start << value.front();
    for (std::size_t i = 1; i < value.size(); ++i) {
        std::cout << delim << value[i];
    }
    std::cout << before_end <<
              "]" << end;
}

#endif //TINH_TOAN_TIEN_HOA_PRINT_H
