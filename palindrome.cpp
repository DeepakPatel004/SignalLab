#include "palindrome.hpp"

PalindromeRange find_longest_palindrome(const std::string& data) {
    std::size_t best_start = 0;
    std::size_t best_length = 0;

    const auto expand = [&](std::size_t left, std::size_t right) {
        while (right < data.size() && data[left] == data[right]) {
            const auto length = right - left + 1;
            if (length > best_length) {
                best_start = left;
                best_length = length;
            }
            // Stop before subtracting from an unsigned zero index.
            if (left == 0) {
                break;
            }
            --left;
            ++right;
        }
    };

    for (std::size_t center = 0; center < data.size(); ++center) {
        expand(center, center);      // Odd-length palindromes.
        expand(center, center + 1);  // Even-length palindromes.
    }
    return {best_start, best_length};
}

std::string longest_palindrome(const std::string& data) {
    const auto result = find_longest_palindrome(data);
    return data.substr(result.start, result.length);
}
