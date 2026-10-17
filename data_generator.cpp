#include "data_generator.hpp"

#include <random>
#include <stdexcept>

std::string generate_binary(std::size_t length) {
    if (length > max_stream_length) {
        throw std::invalid_argument("Length must be at most 10000.");
    }

    std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> bit(0, 1);
    std::string data(length, '0');
    for (char& value : data) {
        value = static_cast<char>('0' + bit(generator));
    }
    return data;
}
