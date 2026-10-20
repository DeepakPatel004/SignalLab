#pragma once

#include <string>

struct PalindromeRange {
    std::size_t start = 0;  // Zero-based; empty input returns {0, 0}.
    std::size_t length = 0;
};

PalindromeRange find_longest_palindrome(const std::string& data);

// Returns the earliest longest contiguous palindrome; empty input returns "".
std::string longest_palindrome(const std::string& data);
