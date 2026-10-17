# SignalLab

ITT 036 Digital Signal Encoder & Decoder, Autumn 2026. C++17 implementation.

Day 1: generate a digital binary stream and find its longest contiguous palindrome.

## Features

- Random binary data, or random data containing at least four or eight consecutive zeros.
- Expand-around-center palindrome search: O(n²) time and O(1) working space,
  excluding the returned substring. Ties return the earliest occurrence.
- Simple menu displaying the stream, longest palindrome and its length.
- Lengths from 0 to 10000; zero-sequence modes require at least 4 or 8 bits.
  Invalid input prints an error and exits. Run again to retry.

The generator fills the stream with random bits, then inserts the required zero
sequence at a random position. Longer zero runs can occur naturally. Each run
uses a fresh random seed.

## Build

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp data_generator.cpp palindrome.cpp -o signallab
```

Run `./signallab` on Linux/macOS or `.\signallab.exe` in Windows PowerShell.

Requires a C++17 compiler; no external libraries or CMake are needed.

## Example

Choose mode `3` and length `8`:

```text
SignalLab - ITT 036
1. Random binary
2. Contains 0000
3. Contains 00000000
Mode (1-3): 3
Stream length (0-10000): 8
Binary stream: 00000000
Longest palindrome: 00000000
Palindrome length: 8
```

## Files

- `main.cpp`: menu, input validation and output.
- `data_generator.hpp` / `data_generator.cpp`: binary data generation.
- `palindrome.hpp` / `palindrome.cpp`: longest palindrome search.

## Checks performed

Compiled with the C++17 command above. Checked random length/binary characters,
both zero modes, too-short inputs, empty/single-bit strings, and known palindromes
including `101101`, `0000`, `11111` and `10101`.

## Next stages

- Day 2: NRZ-L, NRZ-I, Manchester, Differential Manchester, AMI,
  and B8ZS/HDB3 scrambling.
- Later: PCM or delta modulation, visualization, decoding and the specification report.
