#pragma once

#include <string>
#include <vector>

// Levels are +/-1 V (0 V for AMI space). NRZ: one level/bit;
// Manchester: two half-bit levels. Initial level +1; previous AMI mark -1.
// NRZ-L: 0=+1. Manchester: 0=(+1,-1). Differential Manchester: 0 toggles
// at the boundary. B8ZS=000VB0VB; HDB3=000V (odd marks) or B00V (even).
std::vector<int> encode_nrz_l(const std::string& data);
std::vector<int> encode_nrz_i(const std::string& data);
std::vector<int> encode_manchester(const std::string& data);
std::vector<int> encode_differential_manchester(const std::string& data);
std::vector<int> encode_ami(const std::string& data);
std::vector<int> encode_b8zs(const std::string& data);
std::vector<int> encode_hdb3(const std::string& data);

std::string decode_nrz_l(const std::vector<int>& signal);
std::string decode_nrz_i(const std::vector<int>& signal);
std::string decode_manchester(const std::vector<int>& signal);
std::string decode_differential_manchester(const std::vector<int>& signal);
std::string decode_ami(const std::vector<int>& signal);
std::string decode_b8zs(const std::vector<int>& signal);
std::string decode_hdb3(const std::vector<int>& signal);
