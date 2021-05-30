#include "./utils.h"

#include <iostream>
#include <iomanip>

int max_int(int a, int b) {
    return a > b ? a : b;
}

double my_pow(double a, uint8_t e) {
    double res = 1;
    for (int i = 0; i < e; i++) {
        res *= a;
    }
    return res;
}

void print_hex(char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
            << (int)data[i];
    }
    std::cout << '\n';
    std::cout << std::dec;
}
