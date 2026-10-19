#include "pcm.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace {
unsigned quantization_levels(unsigned bit_depth) {
    if (bit_depth == 0 || bit_depth > 8) {
        throw std::invalid_argument("PCM bit depth must be between 1 and 8.");
    }
    return 1U << bit_depth;
}

double quantization_step(double minimum, double maximum, unsigned levels) {
    if (!std::isfinite(minimum) || !std::isfinite(maximum) ||
        !(minimum < maximum) || !std::isfinite(maximum - minimum)) {
        throw std::invalid_argument("PCM range must have finite minimum < maximum.");
    }
    const double step = (maximum - minimum) / levels;
    if (!std::isfinite(step) || step == 0.0) {
        throw std::invalid_argument("PCM range is too small or large for the selected bit depth.");
    }
    return step;
}
}

std::string encode_pcm(const std::vector<double>& samples, unsigned bit_depth,
                       double minimum, double maximum) {
    const unsigned levels = quantization_levels(bit_depth);
    const double step = quantization_step(minimum, maximum, levels);
    if (samples.empty()) {
        throw std::invalid_argument("PCM requires at least one sample.");
    }

    std::string bitstream;
    bitstream.reserve(samples.size() * bit_depth);
    for (const double sample : samples) {
        if (!std::isfinite(sample) || sample < minimum || sample > maximum) {
            throw std::invalid_argument("PCM samples must be finite and within the selected range.");
        }

        unsigned code = levels - 1;
        if (sample < maximum) {
            code = static_cast<unsigned>((sample - minimum) / step);
            if (code >= levels) {
                code = levels - 1;
            }
        }
        for (unsigned bit = bit_depth; bit > 0; --bit) {
            bitstream.push_back((code & (1U << (bit - 1))) != 0 ? '1' : '0');
        }
    }
    return bitstream;
}

std::vector<double> decode_pcm(const std::string& bitstream, unsigned bit_depth,
                               double minimum, double maximum) {
    const unsigned levels = quantization_levels(bit_depth);
    const double step = quantization_step(minimum, maximum, levels);
    if (bitstream.empty() || bitstream.size() % bit_depth != 0) {
        throw std::invalid_argument("PCM bitstream must contain complete, non-empty sample codes.");
    }

    std::vector<double> reconstructed;
    reconstructed.reserve(bitstream.size() / bit_depth);
    for (std::size_t offset = 0; offset < bitstream.size(); offset += bit_depth) {
        unsigned code = 0;
        for (unsigned bit = 0; bit < bit_depth; ++bit) {
            const char value = bitstream[offset + bit];
            if (value != '0' && value != '1') {
                throw std::invalid_argument("PCM bitstream must contain only 0 and 1.");
            }
            code = code * 2U + static_cast<unsigned>(value - '0');
        }
        reconstructed.push_back(minimum + (static_cast<double>(code) + 0.5) * step);
    }
    return reconstructed;
}

void write_pcm_staircase_csv(const std::vector<double>& samples,
                             const std::vector<double>& reconstructed,
                             const std::string& filename) {
    if (samples.empty() || samples.size() != reconstructed.size()) {
        throw std::invalid_argument("PCM input and reconstructed samples must have equal non-zero lengths.");
    }

    std::ofstream output(filename);
    if (!output) {
        throw std::runtime_error("Could not create PCM CSV file: " + filename);
    }
    output << "time,input_sample,staircase_sample\n"
           << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        output << i << ',' << samples[i] << ',' << reconstructed[i] << '\n'
               << i + 1 << ",," << reconstructed[i] << '\n';
    }
    if (!output) {
        throw std::runtime_error("Could not write PCM CSV file: " + filename);
    }
}
