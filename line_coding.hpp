#pragma once

#include <string>
#include <vector>

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
