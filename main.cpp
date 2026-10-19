#include "data_generator.hpp"
#include "line_coding.hpp"
#include "palindrome.hpp"
#include "pcm.hpp"

#include <cmath>
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

double read_sample_value(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    double value = 0.0;
    char extra = '\0';
    if (!std::getline(std::cin, line)) {
        throw std::invalid_argument("Input ended unexpectedly.");
    }
    std::istringstream input(line);
    if (!(input >> value) || (input >> extra) || !std::isfinite(value)) {
        throw std::invalid_argument("Enter one finite numeric sample value.");
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

void run_pcm() {
    const int sample_count = read_number("Number of samples (1-1000): ", 1, 1000);
    const int bit_depth = read_number("PCM bit depth (1-8): ", 1, 8);
    const double minimum = read_sample_value("Quantization minimum: ");
    const double maximum = read_sample_value("Quantization maximum: ");
    if (!(minimum < maximum)) {
        throw std::invalid_argument("Quantization minimum must be less than maximum.");
    }

    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(sample_count));
    for (int i = 0; i < sample_count; ++i) {
        const double sample = read_sample_value("Sample " + std::to_string(i + 1) + ": ");
        if (sample < minimum || sample > maximum) {
            throw std::invalid_argument("Each sample must be within the selected quantization range.");
        }
        samples.push_back(sample);
    }

    const auto bitstream = encode_pcm(samples, static_cast<unsigned>(bit_depth),
                                      minimum, maximum);
    const auto reconstructed = decode_pcm(bitstream, static_cast<unsigned>(bit_depth),
                                          minimum, maximum);
    std::cout << "Input samples:";
    for (const double sample : samples) {
        std::cout << ' ' << sample;
    }
    std::cout << "\nPCM bitstream (MSB first, grouped by sample): ";
    for (std::size_t i = 0; i < bitstream.size(); i += static_cast<std::size_t>(bit_depth)) {
        if (i != 0) {
            std::cout << ' ';
        }
        std::cout << bitstream.substr(i, static_cast<std::size_t>(bit_depth));
    }
    std::cout << "\nReconstructed samples:\n";
    for (std::size_t i = 0; i < samples.size(); ++i) {
        std::cout << "  " << i << ": input " << samples[i]
                  << ", decoded " << reconstructed[i] << '\n';
    }

    const std::string csv_file = "pcm_staircase.csv";
    write_pcm_staircase_csv(samples, reconstructed, csv_file);
    std::cout << "Staircase data written to " << csv_file << '\n';
}

int main() {
    try {
        std::cout << "SignalLab - ITT 036\n"
                     "1. Generate data and find longest palindrome\n"
                     "2. Line coding\n"
                     "3. PCM sampling and reconstruction\n";
        const int action = read_number("Option (1-3): ", 1, 3);
        if (action == 1) {
            run_day_one_features();
        } else if (action == 2) {
            run_line_coding();
        } else {
            run_pcm();
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
