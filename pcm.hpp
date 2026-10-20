#pragma once

#include <string>
#include <vector>

std::string encode_pcm(const std::vector<double>& samples, unsigned bit_depth,
                       double minimum, double maximum);
std::vector<double> decode_pcm(const std::string& bitstream, unsigned bit_depth,
                               double minimum, double maximum);
void write_pcm_staircase_csv(const std::vector<double>& samples,
                             const std::vector<double>& reconstructed,
                             const std::string& filename,
                             double sample_interval = 1.0);
