#include "line_coding.hpp"

#include <stdexcept>
#include <string>

namespace {
void validate_voltage_levels(const std::vector<int>& signal, bool allow_zero) {
    if (signal.empty()) {
        throw std::invalid_argument("Encoded signal must not be empty.");
    }
    for (const int level : signal) {
        if (level != -1 && level != 1 && !(allow_zero && level == 0)) {
            throw std::invalid_argument("Voltage levels must be -1, 0 or +1.");
        }
    }
}

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

bool is_b8zs_substitution(const std::vector<int>& signal, std::size_t i,
                          int last_pulse) {
    return i + 8 <= signal.size() &&
           signal[i] == 0 && signal[i + 1] == 0 && signal[i + 2] == 0 &&
           signal[i + 3] == last_pulse && signal[i + 4] == -last_pulse &&
           signal[i + 5] == 0 && signal[i + 6] == -last_pulse &&
           signal[i + 7] == last_pulse;
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

std::string decode_nrz_l(const std::vector<int>& signal) {
    validate_voltage_levels(signal, false);
    std::string data;
    data.reserve(signal.size());
    for (const int level : signal) {
        data.push_back(level == 1 ? '0' : '1');
    }
    return data;
}

std::string decode_nrz_i(const std::vector<int>& signal) {
    validate_voltage_levels(signal, false);
    std::string data;
    data.reserve(signal.size());
    int previous_level = 1;
    for (const int level : signal) {
        data.push_back(level != previous_level ? '1' : '0');
        previous_level = level;
    }
    return data;
}

std::string decode_manchester(const std::vector<int>& signal) {
    validate_voltage_levels(signal, false);
    if (signal.size() % 2 != 0) {
        throw std::invalid_argument("Manchester signal must contain two levels per bit.");
    }

    std::string data;
    data.reserve(signal.size() / 2);
    for (std::size_t i = 0; i < signal.size(); i += 2) {
        if (signal[i] == signal[i + 1]) {
            throw std::invalid_argument("Each Manchester bit must have a mid-bit transition.");
        }
        data.push_back(signal[i] == 1 ? '0' : '1');
    }
    return data;
}

std::string decode_differential_manchester(const std::vector<int>& signal) {
    validate_voltage_levels(signal, false);
    if (signal.size() % 2 != 0) {
        throw std::invalid_argument(
            "Differential Manchester signal must contain two levels per bit.");
    }

    std::string data;
    data.reserve(signal.size() / 2);
    int previous_level = 1;
    for (std::size_t i = 0; i < signal.size(); i += 2) {
        if (signal[i] == signal[i + 1]) {
            throw std::invalid_argument("Each Differential Manchester bit must transition mid-bit.");
        }
        data.push_back(signal[i] == previous_level ? '1' : '0');
        previous_level = signal[i + 1];
    }
    return data;
}

std::string decode_ami(const std::vector<int>& signal) {
    validate_voltage_levels(signal, true);
    std::string data;
    data.reserve(signal.size());
    int last_pulse = -1;
    for (const int level : signal) {
        if (level == 0) {
            data.push_back('0');
        } else {
            if (level == last_pulse) {
                throw std::invalid_argument("AMI mark polarities must alternate, starting with +1.");
            }
            data.push_back('1');
            last_pulse = level;
        }
    }
    return data;
}

std::string decode_b8zs(const std::vector<int>& signal) {
    validate_voltage_levels(signal, true);
    std::string data;
    data.reserve(signal.size());
    int last_pulse = -1;
    for (std::size_t i = 0; i < signal.size();) {
        if (is_b8zs_substitution(signal, i, last_pulse)) {
            data.append(8, '0');
            i += 8;
            continue;
        }
        const int level = signal[i++];
        if (level == 0) {
            data.push_back('0');
        } else {
            if (level == last_pulse) {
                throw std::invalid_argument("Invalid AMI polarity or B8ZS violation pattern.");
            }
            data.push_back('1');
            last_pulse = level;
        }
    }
    return data;
}

std::string decode_hdb3(const std::vector<int>& signal) {
    validate_voltage_levels(signal, true);
    std::string data;
    data.reserve(signal.size());
    int last_pulse = -1;
    std::size_t marks_since_violation = 0;

    for (std::size_t i = 0; i < signal.size();) {
        if (i + 4 <= signal.size()) {
            const bool even_parity_substitution =
                marks_since_violation % 2 == 0 &&
                signal[i] == -last_pulse && signal[i + 1] == 0 &&
                signal[i + 2] == 0 && signal[i + 3] == signal[i];
            const bool odd_parity_substitution =
                marks_since_violation % 2 == 1 &&
                signal[i] == 0 && signal[i + 1] == 0 &&
                signal[i + 2] == 0 && signal[i + 3] == last_pulse;

            if (even_parity_substitution || odd_parity_substitution) {
                data.append(4, '0');
                if (even_parity_substitution) {
                    last_pulse = signal[i];
                }
                marks_since_violation = 0;
                i += 4;
                continue;
            }
        }

        const int level = signal[i++];
        if (level == 0) {
            data.push_back('0');
        } else {
            if (level == last_pulse) {
                throw std::invalid_argument("Invalid AMI polarity or HDB3 violation pattern.");
            }
            data.push_back('1');
            last_pulse = level;
            ++marks_since_violation;
        }
    }
    return data;
}
