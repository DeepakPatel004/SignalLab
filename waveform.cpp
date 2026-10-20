#include "waveform.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace {
constexpr std::size_t preview_limit = 64;

std::string xml_text(const std::string& text) {
    std::string escaped;
    for (char c : text) {
        if (c == '&') escaped += "&amp;";
        else if (c == '<') escaped += "&lt;";
        else if (c == '>') escaped += "&gt;";
        else escaped += c;
    }
    return escaped;
}

void begin_svg(std::ostream& out, int panels, const std::string& title) {
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1100\" height=\""
        << 100 + panels * 230 << "\" viewBox=\"0 0 1100 " << 100 + panels * 230 << "\">\n"
        << "<title>" << xml_text(title) << "</title>\n"
        << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
        << "<g font-family=\"Arial,sans-serif\" fill=\"#172b4d\">\n"
        << "<text x=\"70\" y=\"32\" font-size=\"22\">" << xml_text(title) << "</text>\n"
        << "<text x=\"70\" y=\"56\" font-size=\"13\">"
        << "Preview: first 64 bits/samples at most. CSV contains the full signal.</text>\n";
}

void panel(std::ostream& out, const std::vector<double>& values, double interval,
           double low, double high, int index, const std::string& label,
           const std::string& time_label) {
    const double top = 100 + index * 230;
    constexpr double left = 90, width = 960, height = 130;
    const double duration = values.size() * interval;
    const auto y = [&](double value) { return top + height * (high - value) / (high - low); };
    out << "<text x=\"70\" y=\"" << top - 15 << "\" font-size=\"16\">"
        << xml_text(label) << "</text>\n";
    for (int tick = 0; tick <= 2; ++tick) {
        const double value = low + (high - low) * tick / 2;
        out << "<line x1=\"90\" x2=\"1050\" y1=\"" << y(value)
            << "\" y2=\"" << y(value) << "\" stroke=\"#dce3eb\"/>\n"
            << "<text x=\"78\" y=\"" << y(value) + 4
            << "\" text-anchor=\"end\" font-size=\"12\">" << value << "</text>\n";
    }
    for (int tick = 0; tick <= 4; ++tick) {
        const double x = left + width * tick / 4;
        out << "<line x1=\"" << x << "\" x2=\"" << x << "\" y1=\"" << top
            << "\" y2=\"" << top + height << "\" stroke=\"#edf0f5\"/>\n"
            << "<text x=\"" << x << "\" y=\"" << top + height + 20
            << "\" text-anchor=\"middle\" font-size=\"12\">" << duration * tick / 4
            << "</text>\n";
    }
    out << "<text x=\"570\" y=\"" << top + height + 45
        << "\" text-anchor=\"middle\" font-size=\"13\">" << xml_text(time_label)
        << "</text>\n<polyline fill=\"none\" stroke=\"#176bba\" stroke-width=\"2\" points=\"";
    for (std::size_t i = 0; i < values.size(); ++i) {
        const double x = left + width * i / values.size();
        out << x << ',' << y(values[i]) << ' '
            << left + width * (i + 1) / values.size() << ',' << y(values[i]) << ' ';
    }
    out << "\"/>\n";
}

void check_written(const std::ofstream& out) {
    if (!out) throw std::runtime_error("Could not write waveform file.");
}
}

void write_line_waveform(const std::string& bits, const std::vector<int>& levels,
                         int levels_per_bit, const std::string& name,
                         const std::string& file_prefix,
                         const std::vector<int>& ordinary_ami) {
    if (bits.empty() || (levels_per_bit != 1 && levels_per_bit != 2) ||
        levels.size() != bits.size() * levels_per_bit ||
        (!ordinary_ami.empty() && ordinary_ami.size() != bits.size())) {
        throw std::invalid_argument("Waveform dimensions do not match the binary stream.");
    }
    if (bits.find_first_not_of("01") != std::string::npos) {
        throw std::invalid_argument("Waveform input must be binary.");
    }
    std::ofstream csv(file_prefix + ".csv");
    if (!csv) throw std::runtime_error("Could not create waveform CSV.");
    csv << "bit_start,bit_end,input_bit,ami_level,encoded_level\n" << std::setprecision(17);
    for (std::size_t i = 0; i < levels.size(); ++i) {
        const auto bit_index = i / levels_per_bit;
        csv << static_cast<double>(i) / levels_per_bit << ','
            << static_cast<double>(i + 1) / levels_per_bit << ',' << bits[bit_index] << ',';
        if (!ordinary_ami.empty()) csv << ordinary_ami[bit_index];
        csv << ',' << levels[i] << '\n';
    }
    csv.flush();
    check_written(csv);

    const auto count = std::min(bits.size(), preview_limit);
    std::vector<double> digital;
    for (std::size_t i = 0; i < count; ++i) digital.push_back(bits[i] - '0');
    std::vector<double> encoded(levels.begin(), levels.begin() + count * levels_per_bit);
    std::ofstream svg(file_prefix + ".svg");
    if (!svg) throw std::runtime_error("Could not create waveform SVG.");
    svg << std::setprecision(6);
    begin_svg(svg, ordinary_ami.empty() ? 2 : 3, "SignalLab - " + name);
    panel(svg, digital, 1, 0, 1, 0, "Digital input (logical level)", "Time (bit periods)");
    int next_panel = 1;
    if (!ordinary_ami.empty()) {
        const std::vector<double> ami(ordinary_ami.begin(), ordinary_ami.begin() + count);
        panel(svg, ami, 1, -1, 1, next_panel++, "AMI before scrambling (V)", "Time (bit periods)");
    }
    panel(svg, encoded, 1.0 / levels_per_bit, -1, 1, next_panel,
          name + " encoded signal (V)", "Time (bit periods)");
    svg << "</g></svg>\n";
    svg.flush();
    check_written(svg);
}

void write_pcm_waveform(const std::vector<double>& samples,
                        const std::vector<double>& reconstructed,
                        double sample_interval, const std::string& filename) {
    if (samples.empty() || samples.size() != reconstructed.size() ||
        !std::isfinite(sample_interval) || sample_interval <= 0) {
        throw std::invalid_argument("PCM waveform needs matching samples and a positive interval.");
    }
    double low = samples.front(), high = low;
    for (const auto* series : {&samples, &reconstructed}) {
        for (double value : *series) {
            if (!std::isfinite(value)) throw std::invalid_argument("PCM waveform samples must be finite.");
            low = std::min(low, value);
            high = std::max(high, value);
        }
    }
    if (low == high) {
        const double padding = std::max(1.0, std::abs(low) * 0.05);
        low -= padding;
        high += padding;
    }
    if (!std::isfinite(high - low) || !(low < high)) {
        throw std::invalid_argument("PCM values are outside the waveform display range.");
    }
    const auto count = std::min(samples.size(), preview_limit);
    const std::vector<double> input(samples.begin(), samples.begin() + count);
    const std::vector<double> decoded(reconstructed.begin(), reconstructed.begin() + count);
    std::ofstream svg(filename);
    if (!svg) throw std::runtime_error("Could not create PCM waveform SVG.");
    svg << std::setprecision(6);
    begin_svg(svg, 2, "SignalLab - PCM sampling and reconstruction");
    panel(svg, input, sample_interval, low, high, 0, "Input samples (held for comparison)", "Time (s)");
    panel(svg, decoded, sample_interval, low, high, 1, "Decoded staircase (zero-order hold)", "Time (s)");
    svg << "</g></svg>\n";
    svg.flush();
    check_written(svg);
}
