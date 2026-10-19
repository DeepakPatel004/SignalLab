# SignalLab

ITT 036 Digital Signal Encoder & Decoder, Autumn 2026. C++17 implementation.

Day 1: generate a digital binary stream and find its longest contiguous palindrome.
Day 2: encode binary data using common line-coding schemes and AMI-based scrambling.

## Features

- Random binary data, or random data containing at least four or eight consecutive zeros.
- Expand-around-center palindrome search: O(n²) time and O(1) working space,
  excluding the returned substring. Ties return the earliest occurrence.
- Simple menu displaying the stream, longest palindrome and its length.
- Lengths from 0 to 10000; zero-sequence modes require at least 4 or 8 bits.
  Invalid input prints an error and exits. Run again to retry.
- NRZ-L, NRZ-I, Manchester, Differential Manchester, AMI, B8ZS and HDB3
  encoding and decoding, with voltage levels displayed as `+1`, `0` and `-1`.
- Uniform PCM encoding and decoding with a configurable sample range and
  bit depth, plus CSV export of the reconstructed staircase.

The generator fills the stream with random bits, then inserts the required zero
sequence at a random position. Longer zero runs can occur naturally. Each run
uses a fresh random seed.

## Build

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp data_generator.cpp palindrome.cpp line_coding.cpp pcm.cpp -o signallab
```

Run `./signallab` on Linux/macOS or `.\signallab.exe` in Windows PowerShell.

Requires a C++17 compiler; no external libraries or CMake are needed.

## Line coding

At the main menu, select `2` and either enter a non-empty binary sequence or
generate one using the existing random/zero-sequence generator. Select options
`1` through `5` for NRZ-L, NRZ-I, Manchester, Differential Manchester or AMI.
For AMI, choose no scrambling, B8ZS or HDB3. The Day 1 generator and palindrome
feature remain available under main-menu option `1`.

Voltage and transition conventions:

- NRZ-L: `0` is `+1`; `1` is `-1`.
- NRZ-I: starts at `+1`; each `1` toggles the level at the bit boundary and
  each `0` leaves it unchanged.
- Manchester: `0` transitions `+1` to `-1` mid-bit; `1` transitions `-1` to
  `+1` mid-bit.
- Differential Manchester: starts at `+1` before the first bit; every bit has
  a mid-bit transition, and `0` (not `1`) also transitions at the bit boundary.
- AMI starts with a `+1` mark; successive `1` bits alternate `+1`, `-1`.
  B8ZS and HDB3 preserve that AMI polarity convention and insert violation
  pulses according to their substitution rules.
- NRZ and AMI output one level per bit. Manchester schemes output two
  half-bit levels per bit.

## Line decoding

At the main menu, select `4`, enter the encoded voltage levels on one
space-separated line, and select the matching scheme. The decoder derives bits
from the levels and transitions, and rejects invalid voltage values, signal
lengths or bipolar violations. NRZ-I and Differential Manchester assume the
previous level was `+1`; AMI-based decoding assumes the previous mark was `-1`,
so the first ordinary mark is `+1`. The Manchester decoder uses the same
mid-bit convention as the encoder.

## PCM

At the main menu, select `3`. Enter the number of already-sampled input values,
the bit depth (1-8), the quantizer's minimum and maximum, and each sample.
Samples must be finite and lie within the selected range. The range is divided
into `2^bit_depth` uniform intervals; each interval is encoded with its
zero-based binary index, most-significant bit first. The decoder reconstructs
the midpoint of each interval, so the displayed values are quantized
approximations rather than the original samples.

For example, with range `-1` to `1`, bit depth `2`, and samples `-1`, `0` and
`1`, the bitstream is `00 10 11`; the decoded midpoint values are `-0.75`,
`0.25` and `0.75`. The endpoint `1` maps to the final interval.

The application prints original and decoded samples and writes
`pcm_staircase.csv` in the current directory. It contains time, input sample,
and decoded staircase columns; open it in a spreadsheet or plotting program,
plot both value columns against time, and render the decoded series as a step
plot to see the zero-order-hold reconstruction. A new PCM run overwrites this
CSV file. No graphical plotting library is required.

Examples (voltage levels are listed in bit order; Manchester levels are listed
in half-bit order):

```text
Input 10110010, NRZ-L:
-1 +1 -1 -1 +1 +1 -1 +1

Input 10110010, AMI:
+1 0 -1 +1 0 0 -1 0

Input 100000000, B8ZS:
+1 0 0 0 +1 -1 0 -1 +1

Input 10000, HDB3 (odd preceding mark count):
+1 0 0 0 +1

Input 110000, HDB3 (even preceding mark count):
+1 -1 +1 0 0 +1
```

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
- `line_coding.hpp` / `line_coding.cpp`: basic line codes and AMI scrambling.
- `pcm.hpp` / `pcm.cpp`: uniform PCM encoder, decoder and staircase CSV export.

## Checks performed

Compiled with the C++17 command above. Checked random length/binary characters,
both zero modes, too-short inputs, empty/single-bit strings, and known palindromes
including `101101`, `0000`, `11111` and `10101`.

## Next stages

- Day 4: add a graphical waveform viewer only if required; the PCM staircase
  can already be plotted from the exported CSV.
- No additional Day 4 requirements have been provided yet.
