#include "data_generator.hpp"
#include "line_coding.hpp"
#include "palindrome.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

void run_day_one_features() {
    std::cout << "1. Random binary\n2. Contains 0000\n3. Contains 00000000\n";
    const int mode = read_number("Mode (1-3): ", 1, 3);
    const int length = read_number("Stream length (0-10000): ", 0, 10000);
    const std::size_t zero_runs[] = {0, 4, 8};
    const auto data = generate_binary(static_cast<std::size_t>(length), zero_runs[mode - 1]);
    std::cout << "Binary stream: " << data << '\n';
    const auto palindrome = longest_palindrome(data);
    std::cout << "Longest palindrome: " << palindrome << '\n'
              << "Palindrome length: " << palindrome.size() << '\n';
}

std::vector<int> choose_encoding(const std::string& data, std::string& name) {
    std::cout << "1. NRZ-L\n2. NRZ-I\n3. Manchester\n"
                 "4. Differential Manchester\n5. AMI\n";
    const int scheme = read_number("Scheme (1-5): ", 1, 5);
    switch (scheme) {
        case 1:
            name = "NRZ-L";
            return encode_nrz_l(data);
        case 2:
            name = "NRZ-I";
            return encode_nrz_i(data);
        case 3:
            name = "Manchester";
            return encode_manchester(data);
        case 4:
            name = "Differential Manchester";
            return encode_differential_manchester(data);
        default: {
            const int scrambling = read_number(
                "Scramble AMI? 1. No  2. B8ZS  3. HDB3: ", 1, 3);
            if (scrambling == 2) {
                name = "B8ZS";
                return encode_b8zs(data);
            }
            if (scrambling == 3) {
                name = "HDB3";
                return encode_hdb3(data);
            }
            name = "AMI";
            return encode_ami(data);
        }
    }
}

void run_line_coding() {
    std::string data;
    std::cout << "1. Enter binary sequence\n2. Generate binary sequence\n";
    const int source = read_number("Data source (1-2): ", 1, 2);
    if (source == 1) {
        std::cout << "Binary sequence (1-10000 bits): ";
        if (!std::getline(std::cin, data)) {
            throw std::invalid_argument("Input ended unexpectedly.");
        }
    } else {
        std::cout << "1. Random binary\n2. Contains 0000\n3. Contains 00000000\n";
        const int mode = read_number("Mode (1-3): ", 1, 3);
        const int length = read_number("Stream length (1-10000): ", 1, 10000);
        const std::size_t zero_runs[] = {0, 4, 8};
        data = generate_binary(static_cast<std::size_t>(length), zero_runs[mode - 1]);
    }
    if (data.empty() || data.size() > max_stream_length) {
        throw std::invalid_argument("Enter between 1 and 10000 bits.");
    }

    std::string name;
    const auto signal = choose_encoding(data, name);
    std::cout << name << " voltage levels:";
    for (const int level : signal) {
        std::cout << ' ' << (level > 0 ? "+1" : level < 0 ? "-1" : "0");
    }
    std::cout << '\n';
}

int main() {
    try {
        std::cout << "SignalLab - ITT 036\n"
                     "1. Generate data and find longest palindrome\n"
                     "2. Line coding\n";
        const int action = read_number("Option (1-2): ", 1, 2);
        if (action == 1) {
            run_day_one_features();
        } else {
            run_line_coding();
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
