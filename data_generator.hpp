#pragma once

#include <cstddef>
#include <string>

// A practical CLI limit keeps display and later palindrome searches manageable.
constexpr std::size_t max_stream_length = 10000;

// zero_run must be 0 (random), 4 or 8; length must accommodate the run.
std::string generate_binary(std::size_t length, std::size_t zero_run = 0);
