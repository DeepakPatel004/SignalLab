#include "line_coding.hpp"

#include <stdexcept>
#include <string>

namespace {
void validate_binary(const std::string& data) {
    for (const char bit : data) {
        if (bit != '0' && bit != '1') {
            throw std::invalid_argument("Input data must contain only 0 and 1.");
        }
    }
}

std::vector<int> encode_ami_based(const std::string& data, bool b8zs) {
    validate_binary(data);
    std::vector<int> signal;
    int last_pulse = -1;
    std::size_t marks_since_violation = 0;

    for (std::size_t i = 0; i < data.size();) {
        std::size_t zero_count = 0;
        while (i + zero_count < data.size() && data[i + zero_count] == '0') {
            ++zero_count;
        }

        if (b8zs && zero_count >= 8) {
            const int first_violation = last_pulse;
            const int first_bipolar = -first_violation;
            signal.insert(signal.end(), {0, 0, 0, first_violation, first_bipolar,
                                         0, first_bipolar, first_violation});
            last_pulse = first_violation;
            marks_since_violation = 0;
            i += 8;
        } else if (!b8zs && zero_count >= 4) {
            if (marks_since_violation % 2 == 0) {
                const int balancing_pulse = -last_pulse;
                signal.insert(signal.end(), {balancing_pulse, 0, 0, balancing_pulse});
                last_pulse = balancing_pulse;
            } else {
                signal.insert(signal.end(), {0, 0, 0, last_pulse});
            }
            marks_since_violation = 0;
            i += 4;
        } else if (data[i] == '0') {
            signal.push_back(0);
            ++i;
        } else {
            last_pulse = -last_pulse;
            signal.push_back(last_pulse);
            ++marks_since_violation;
            ++i;
        }
    }
    return signal;
}
}

std::vector<int> encode_nrz_l(const std::string& data) {
    validate_binary(data);
    std::vector<int> signal;
    for (const char bit : data) {
        signal.push_back(bit == '0' ? 1 : -1);
    }
    return signal;
}

std::vector<int> encode_nrz_i(const std::string& data) {
    validate_binary(data);
    std::vector<int> signal;
    int level = 1;
    for (const char bit : data) {
        if (bit == '1') {
            level = -level;
        }
        signal.push_back(level);
    }
    return signal;
}

std::vector<int> encode_manchester(const std::string& data) {
    validate_binary(data);
    std::vector<int> signal;
    for (const char bit : data) {
        if (bit == '0') {
            signal.insert(signal.end(), {1, -1});
        } else {
            signal.insert(signal.end(), {-1, 1});
        }
    }
    return signal;
}

std::vector<int> encode_differential_manchester(const std::string& data) {
    validate_binary(data);
    std::vector<int> signal;
    int level = 1;
    for (const char bit : data) {
        if (bit == '0') {
            level = -level;
        }
        signal.push_back(level);
        level = -level;
        signal.push_back(level);
    }
    return signal;
}

std::vector<int> encode_ami(const std::string& data) {
    validate_binary(data);
    std::vector<int> signal;
    int last_pulse = -1;
    for (const char bit : data) {
        if (bit == '0') {
            signal.push_back(0);
        } else {
            last_pulse = -last_pulse;
            signal.push_back(last_pulse);
        }
    }
    return signal;
}

std::vector<int> encode_b8zs(const std::string& data) {
    return encode_ami_based(data, true);
}

std::vector<int> encode_hdb3(const std::string& data) {
    return encode_ami_based(data, false);
}
