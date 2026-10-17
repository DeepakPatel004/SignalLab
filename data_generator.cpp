#include "data_generator.hpp"

#include <random>
#include <stdexcept>

std::string generate_binary(std::size_t length, std::size_t zero_run) {
    if (length > max_stream_length) {
        throw std::invalid_argument("Length must be at most 10000.");
    }
    if (zero_run != 0 && zero_run != 4 && zero_run != 8) {
        throw std::invalid_argument("Zero run must be 0, 4 or 8.");
    }
    if (length < zero_run) {
        throw std::invalid_argument("Stream length is too short for the requested zero sequence.");
    }

    std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> bit(0, 1);
    std::string data(length, '0');
    for (char& value : data) {
        value = static_cast<char>('0' + bit(generator));
    }
    if (zero_run != 0) {
        std::uniform_int_distribution<std::size_t> position(0, length - zero_run);
        data.replace(position(generator), zero_run, zero_run, '0');
    }
    return data;
}
