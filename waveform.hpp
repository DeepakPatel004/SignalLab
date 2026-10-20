#pragma once

#include <string>
#include <vector>

// SVG previews show at most 64 bits/samples; CSV exports retain the full signal.
// Manchester uses two levels per bit. Other schemes use one.
void write_line_waveform(const std::string& bits, const std::vector<int>& levels,
                         int levels_per_bit, const std::string& name,
                         const std::string& file_prefix,
                         const std::vector<int>& ordinary_ami = {});
void write_pcm_waveform(const std::vector<double>& samples,
                        const std::vector<double>& reconstructed,
                        double sample_interval, const std::string& filename);
