#pragma once

#include <cstddef>
#include <string>

// A practical CLI limit keeps display and later palindrome searches manageable.
constexpr std::size_t max_stream_length = 10000;

std::string generate_binary(std::size_t length);
