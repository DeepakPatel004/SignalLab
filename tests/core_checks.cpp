#include "data_generator.hpp"
#include "palindrome.hpp"
#include "line_coding.hpp"
#include "pcm.hpp"
#include "waveform.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    using Levels = std::vector<int>;
    assert(encode_nrz_l("101101") == Levels({-1,1,-1,-1,1,-1}));
    assert(encode_nrz_i("101") == Levels({-1,-1,1}));
    assert(encode_manchester("01") == Levels({1,-1,-1,1}));
    assert(encode_differential_manchester("01") == Levels({-1,1,1,-1}));
    assert(encode_ami("1011") == Levels({1,0,-1,1}));
    assert(encode_b8zs("00000000") == Levels({0,0,0,-1,1,0,1,-1}));
    assert(encode_b8zs("100000000") == Levels({1,0,0,0,1,-1,0,-1,1}));
    assert(encode_hdb3("0000") == Levels({1,0,0,1}));
    assert(encode_hdb3("10000") == Levels({1,0,0,0,1}));
    assert(encode_hdb3("110000") == Levels({1,-1,1,0,0,1}));
    assert(encode_hdb3("00000000") == Levels({1,0,0,1,-1,0,0,-1}));
    const auto p = find_longest_palindrome("0010");
    assert(p.start == 1 && p.length == 3);
    assert(longest_palindrome("101101") == "101101");
    assert(longest_palindrome("").empty());
    assert(longest_palindrome("0") == "0");
    assert(longest_palindrome("0000") == "0000");
    assert(longest_palindrome("11111") == "11111");
    assert(longest_palindrome("10101") == "10101");
    assert(longest_palindrome("01") == "0");
    assert(find_longest_palindrome("").start == 0);
    const auto random = generate_binary(32);
    assert(random.size() == 32 && random.find_first_not_of("01") == std::string::npos);
    assert(generate_binary(0).empty());
    for (std::size_t run : {4u, 8u}) {
        const auto data = generate_binary(24, run);
        assert(data.size() == 24 && data.find(std::string(run, '0')) != std::string::npos);
        assert(generate_binary(run, run) == std::string(run, '0'));
    }
    const std::vector<Levels(*)(const std::string&)> encoders = {
        encode_nrz_l, encode_nrz_i, encode_manchester,
        encode_differential_manchester, encode_ami, encode_b8zs, encode_hdb3};
    const std::vector<std::string(*)(const Levels&)> decoders = {
        decode_nrz_l, decode_nrz_i, decode_manchester,
        decode_differential_manchester, decode_ami, decode_b8zs, decode_hdb3};
    for (const std::string data : {"0", "1", "101101", "00000000000000000",
                                   "100001100000000100000000", "1100001000010000"}) {
        for (std::size_t i = 0; i < encoders.size(); ++i) {
            assert(decoders[i](encoders[i](data)) == data);
        }
    }
    bool rejected = false;
    try { decode_hdb3({0,0,0,0}); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { generate_binary(7, 8); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { decode_b8zs(Levels(8, 0)); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { decode_manchester({1, 1}); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);

    const std::vector<double> input = {-1, 0, 1};
    const auto bits = encode_pcm(input, 2, -1, 1);
    assert(bits == "001011");
    const auto decoded = decode_pcm(bits, 2, -1, 1);
    assert(decoded == std::vector<double>({-0.75, 0.25, 0.75}));
    for (std::size_t i = 0; i < input.size(); ++i) assert(std::abs(input[i] - decoded[i]) <= 0.25);
    rejected = false;
    try { decode_pcm("001", 2, -1, 1); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    std::filesystem::create_directories("build/test-output");
    write_pcm_staircase_csv(input, decoded, "build/test-output/pcm.csv", 0.25);
    write_pcm_waveform(input, decoded, 0.25, "build/test-output/pcm.svg");
    std::ifstream csv("build/test-output/pcm.csv");
    const std::string text((std::istreambuf_iterator<char>(csv)), {});
    assert(text.find("0.25,0,0.25") != std::string::npos);
    assert(text.find("0.75,,0.75") != std::string::npos);
    write_line_waveform("100000000", encode_b8zs("100000000"), 1,
                        "B8ZS", "build/test-output/b8zs", encode_ami("100000000"));
    write_line_waveform("01", encode_manchester("01"), 2,
                        "Manchester", "build/test-output/manchester");
    std::cout << "All focused core checks passed (seven line-code round trips,\n"
                 "polarity/parity vectors, generation, palindrome, PCM and exports).\n";
}
