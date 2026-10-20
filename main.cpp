#include "data_generator.hpp"
#include "line_coding.hpp"
#include "palindrome.hpp"
#include "pcm.hpp"
#include "waveform.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
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
    double value = 0;
    char extra = '\0';
    if (!std::getline(std::cin, line)) throw std::invalid_argument("Input ended unexpectedly.");
    std::istringstream input(line);
    if (!(input >> value) || (input >> extra) || !std::isfinite(value)) {
        throw std::invalid_argument("Enter one finite numeric value.");
    }
    return value;
}

// Each menu entry connects the existing encoder to its matching decoder.
struct LineScheme {
    const char* name;
    std::vector<int> (*encode)(const std::string&);
    std::string (*decode)(const std::vector<int>&);
    int levels_per_bit;
};

const LineScheme schemes[] = {
    {"NRZ-L", encode_nrz_l, decode_nrz_l, 1},
    {"NRZ-I", encode_nrz_i, decode_nrz_i, 1},
    {"Manchester", encode_manchester, decode_manchester, 2},
    {"Differential Manchester", encode_differential_manchester, decode_differential_manchester, 2},
    {"AMI", encode_ami, decode_ami, 1},
    {"B8ZS", encode_b8zs, decode_b8zs, 1},
    {"HDB3", encode_hdb3, decode_hdb3, 1}
};

const LineScheme& choose_encoding() {
    for (int i = 0; i < 5; ++i) std::cout << i + 1 << ". " << schemes[i].name << '\n';
    const int choice = read_number("Scheme (1-5): ", 1, 5);
    if (choice == 5 && read_number("Scrambling required? 1. No  2. Yes: ", 1, 2) == 2) {
        return schemes[4 + read_number("Scrambler: 1. B8ZS  2. HDB3: ", 1, 2)];
    }
    return schemes[choice - 1];
}

void print_levels(const std::vector<int>& signal) {
    for (int level : signal) std::cout << ' ' << (level > 0 ? "+1" : level < 0 ? "-1" : "0");
    std::cout << '\n';
}

void display_and_encode(const std::string& data) {
    std::cout << "Binary stream: " << data << '\n';
    const auto palindrome = find_longest_palindrome(data);
    std::cout << "Longest palindrome: " << data.substr(palindrome.start, palindrome.length)
              << "\nStarting position (zero-based): " << palindrome.start
              << "\nPalindrome length: " << palindrome.length << '\n';
    if (data.empty()) {
        std::cout << "Empty stream: no line signal to encode.\n";
        return;
    }
    const auto& scheme = choose_encoding();
    const auto signal = scheme.encode(data);
    std::vector<int> ordinary_ami;
    if (&scheme == &schemes[5] || &scheme == &schemes[6]) {
        ordinary_ami = encode_ami(data);
        std::cout << "AMI before scrambling:";
        print_levels(ordinary_ami);
    }
    std::cout << scheme.name << " voltage levels:";
    print_levels(signal);
    std::filesystem::create_directories("output");
    write_line_waveform(data, signal, scheme.levels_per_bit, scheme.name,
                        "output/line_waveform", ordinary_ami);
    std::cout << "Graph: output/line_waveform.svg (open in a browser)\n"
                 "Full waveform data: output/line_waveform.csv\n";
    if (read_number("Decode this signal? 1. No  2. Yes: ", 1, 2) == 2) {
        const auto decoded = scheme.decode(signal);
        std::cout << "Decoded stream: " << decoded
                  << "\nRound trip: " << (decoded == data ? "MATCH" : "MISMATCH") << '\n';
    }
}

void run_digital() {
    std::cout << "1. Enter binary sequence\n2. Generate binary sequence\n";
    const int source = read_number("Data source (1-2): ", 1, 2);
    std::string data;
    if (source == 1) {
        std::cout << "Binary sequence (0-10000 bits): ";
        if (!std::getline(std::cin, data)) throw std::invalid_argument("Input ended unexpectedly.");
    } else {
        std::cout << "1. Random binary\n2. Contains 0000\n3. Contains 00000000\n";
        const int mode = read_number("Mode (1-3): ", 1, 3);
        const int length = read_number("Stream length (0-10000): ", 0, 10000);
        const std::size_t zero_runs[] = {0, 4, 8};
        data = generate_binary(static_cast<std::size_t>(length), zero_runs[mode - 1]);
    }
    if (data.size() > max_stream_length || data.find_first_not_of("01") != std::string::npos) {
        throw std::invalid_argument("Enter at most 10000 bits containing only 0 and 1.");
    }
    display_and_encode(data);
}

void run_line_decoding() {
    std::cout << "Encoded levels separated by spaces (-1, 0, +1): ";
    std::string line;
    if (!std::getline(std::cin, line)) throw std::invalid_argument("Input ended unexpectedly.");
    std::istringstream input(line);
    std::vector<int> signal;
    // Parse complete tokens so a trailing token such as '1e' cannot be accepted.
    std::string token;
    while (input >> token) {
        if (token != "-1" && token != "0" && token != "1" && token != "+1") {
            throw std::invalid_argument("Voltage levels must be -1, 0 or +1.");
        }
        signal.push_back(std::stoi(token));
        if (signal.size() > max_stream_length * 2) throw std::invalid_argument("Signal is too long.");
    }
    if (signal.empty()) throw std::invalid_argument("Enter one or more voltage levels.");
    for (int i = 0; i < 7; ++i) std::cout << i + 1 << ". " << schemes[i].name << '\n';
    const int choice = read_number("Decode scheme (1-7): ", 1, 7);
    const auto data = schemes[choice - 1].decode(signal);
    std::cout << "Decoded stream: " << data << '\n';
}

void run_pcm() {
    const int source = read_number("Analog samples: 1. Enter values  2. Generate sine: ", 1, 2);
    const int count = read_number("Number of samples (1-1000): ", 1, 1000);
    const int sample_rate = read_number("Sampling rate in Hz (1-1000000): ", 1, 1000000);
    const int depth = read_number("PCM bit depth (1-8): ", 1, 8);
    const double minimum = read_sample_value("Quantization minimum: ");
    const double maximum = read_sample_value("Quantization maximum: ");
    if (!(minimum < maximum) || !std::isfinite(maximum - minimum)) {
        throw std::invalid_argument("Quantization range must have finite minimum < maximum.");
    }
    std::vector<double> samples;
    for (int i = 0; i < count; ++i) {
        double sample = 0;
        if (source == 1) {
            sample = read_sample_value("Sample " + std::to_string(i + 1) + ": ");
        } else {
            // Sine frequency Fs/8: eight equally spaced samples per period.
            const double amplitude = (maximum - minimum) / 2;
            sample = std::clamp(minimum + amplitude + amplitude * std::sin(2 * std::acos(-1.0) * i / 8),
                                minimum, maximum);
        }
        samples.push_back(sample);
    }
    const auto bits = encode_pcm(samples, static_cast<unsigned>(depth), minimum, maximum);
    const auto reconstructed = decode_pcm(bits, static_cast<unsigned>(depth), minimum, maximum);
    std::cout << "Input samples:";
    for (double sample : samples) std::cout << ' ' << sample;
    std::cout << "\nPCM bitstream (MSB first, grouped by sample): ";
    for (std::size_t i = 0; i < bits.size(); i += depth) std::cout << bits.substr(i, depth) << ' ';
    std::cout << "\nReconstructed samples:\n";
    for (std::size_t i = 0; i < reconstructed.size(); ++i) {
        std::cout << "  t=" << static_cast<double>(i) / sample_rate << " s: input " << samples[i]
                  << ", decoded " << reconstructed[i] << '\n';
    }
    std::filesystem::create_directories("output");
    const double interval = 1.0 / sample_rate;
    write_pcm_staircase_csv(samples, reconstructed, "output/pcm_staircase.csv", interval);
    write_pcm_waveform(samples, reconstructed, interval, "output/pcm_staircase.svg");
    std::cout << "Staircase: output/pcm_staircase.svg (open in a browser)\n"
                 "Full sample data: output/pcm_staircase.csv\n";
    if (read_number("Line-encode the PCM bits? 1. No  2. Yes: ", 1, 2) == 2) {
        display_and_encode(bits);
    }
}

int main() {
    try {
        std::cout << "SignalLab - ITT 036\n1. Digital input\n2. Analog input (PCM)\n"
                     "3. Decode entered voltage levels\n";
        const int action = read_number("Input (1-3): ", 1, 3);
        void (*actions[])() = {run_digital, run_pcm, run_line_decoding};
        actions[action - 1]();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
