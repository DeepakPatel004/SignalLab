#include "data_generator.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

int read_number(const std::string& prompt, int minimum, int maximum) {
    std::cout << prompt;
    std::string line;
    int value = 0;
    char extra = '\0';
    if (!std::getline(std::cin, line)) {
        throw std::invalid_argument("Input ended unexpectedly.");
    }
    std::istringstream input(line);
    if (!(input >> value) || (input >> extra) || value < minimum || value > maximum) {
        throw std::invalid_argument("Enter a whole number in the displayed range.");
    }
    return value;
}

int main() {
    try {
        std::cout << "SignalLab - ITT 036\n";
        std::cout << "1. Random binary\n2. Contains 0000\n3. Contains 00000000\n";
        const int mode = read_number("Mode (1-3): ", 1, 3);
        const int length = read_number("Stream length (0-10000): ", 0, 10000);
        const std::size_t zero_runs[] = {0, 4, 8};
        const auto data = generate_binary(static_cast<std::size_t>(length), zero_runs[mode - 1]);
        std::cout << "Binary stream: " << data << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
