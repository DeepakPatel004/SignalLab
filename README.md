# SignalLab

ITT 036 Digital Signal Encoder & Decoder, Autumn 2026. C++17 implementation.

Generate digital data or process analog samples, encode the resulting bits,
visualize their waveform, and reconstruct the signal with the matching decoder.
The final integration preserves the team's generator, line-coding and PCM modules.

## Features

- Random binary data, or random data containing at least four or eight consecutive zeros.
- Expand-around-center palindrome search: O(n²) time and O(1) working space,
  excluding the returned substring. Ties return the earliest occurrence.
- Digital/analog input menu displaying the stream, longest palindrome,
  zero-based starting position and length.
- Lengths from 0 to 10000; zero-sequence modes require at least 4 or 8 bits.
  Invalid input prints an error and exits. Run again to retry.
- NRZ-L, NRZ-I, Manchester, Differential Manchester, AMI, B8ZS and HDB3
  encoding and decoding, with voltage levels displayed as `+1`, `0` and `-1`.
- Uniform PCM encoding and decoding with a configurable sample range and
  bit depth, manual samples or a generated sine, and timed staircase reconstruction.
- Labelled SVG graphs and full CSV exports for line-coded and PCM signals.
- Optional decoding immediately after encoding; PCM bits can also be line-encoded.

The generator fills the stream with random bits, then inserts the required zero
sequence at a random position. Longer zero runs can occur naturally. Each run
uses a fresh random seed.

## Build

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp data_generator.cpp palindrome.cpp line_coding.cpp pcm.cpp waveform.cpp -o signallab
```

Run `./signallab` on Linux/macOS or `.\signallab.exe` in Windows PowerShell.

Run the command from the repository root. Requires a C++17 compiler (tested with
MinGW g++ on Windows); no external libraries or CMake are needed by the application.
An SVG-capable browser displays the exported graphs. Python 3 is optional for CLI
checks; Python ReportLab is needed only to regenerate the already-provided report.

## Line coding

At the main menu, select `1` for digital input and either enter a binary sequence or
generate one using the existing random/zero-sequence generator. Select options
`1` through `5` for NRZ-L, NRZ-I, Manchester, Differential Manchester or AMI.
For AMI, answer whether scrambling is required, then choose B8ZS or HDB3 if yes.
The stream and its longest palindrome are displayed before the scheme menu.
An empty stream displays a zero-length palindrome and skips line encoding.

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
- B8ZS replaces eight zeros with `000VB0VB`. `V` has the polarity of the
  preceding nonzero pulse; `B` alternates from it. Before the first mark,
  the virtual preceding pulse is `-1`.
- HDB3 replaces four zeros with `000V` after an odd number of ordinary marks
  since the last substitution; otherwise `B00V`. The count starts at zero and
  resets after each substitution. In `B00V`, `B` opposes the preceding pulse
  and `V` equals `B`. In `000V`, `V` equals the preceding pulse.

## Line decoding

After encoding, answer `2` to decode the displayed signal and show a round-trip
comparison. Alternatively, at the main menu select `3`, enter encoded levels on one
space-separated line, and select the matching scheme. The decoder derives bits
from the levels and transitions, and rejects invalid voltage values, signal
lengths or bipolar violations. NRZ-I and Differential Manchester assume the
previous level was `+1`; AMI-based decoding assumes the previous mark was `-1`,
so the first ordinary mark is `+1`. The Manchester decoder uses the same
mid-bit convention as the encoder. Decoders take only the encoded levels;
the original bits are used solely for the final comparison. This is ideal,
clock-aligned level decoding; raster-image recognition is not implemented.

## PCM

At the main menu, select `2`. Choose manually entered values or a generated sine.
Enter the sample count (1-1000), sampling rate (1-1000000 Hz), bit depth (1-8),
and quantizer minimum and maximum, then the individual samples in manual mode.
Samples must be finite and lie within the selected range. The range is divided
into `2^bit_depth` uniform intervals; each interval is encoded with its
zero-based binary index, most-significant bit first. The decoder reconstructs
the midpoint of each interval, so the displayed values are quantized
approximations rather than the original samples. The upper endpoint maps to the
final interval. The uniform step is `(maximum-minimum)/2^bit_depth`.

Samples occur at `t=i/Fs` seconds. Manual values are assumed already sampled at
that rate; there is no hardware ADC or resampling. The generated sine has frequency
`Fs/8`, midpoint `(minimum+maximum)/2`, and amplitude `(maximum-minimum)/2`.
The decoder holds each reconstructed value for one sample interval (zero-order hold).

For example, with range `-1` to `1`, bit depth `2`, and samples `-1`, `0` and
`1`, the bitstream is `00 10 11`; the decoded midpoint values are `-0.75`,
`0.25` and `0.75`. The endpoint `1` maps to the final interval.

The application prints original and decoded samples and writes
`output/pcm_staircase.csv` and `output/pcm_staircase.svg`. For the example above,
choose a sampling rate of `4` Hz to obtain sample times `0`, `0.25`, `0.5` seconds.
After reconstruction, choose whether to line-encode the PCM bitstream.

## Graphs and output files

Open the generated SVG in any modern browser; the program prints its path.
Graphs are written directly in C++, without a graphics dependency or embedded GUI.

- `output/line_waveform.svg`: input bits and encoded waveform; when scrambling
  is selected, ordinary AMI is shown in a separate panel for comparison.
- `output/line_waveform.csv`: full encoded intervals (`bit_start`, `bit_end`,
  `input_bit`, optional `ami_level`, `encoded_level`). Manchester intervals are half bits.
- `output/pcm_staircase.svg`: input samples and reconstructed staircase, in seconds.
- `output/pcm_staircase.csv`: sample times, input samples and staircase values.
  Duplicate time coordinates create vertical steps; blank input cells mark interval ends.

SVG previews show the first 64 bits/samples for readability; CSVs contain all values.
The matching files are overwritten on each new run. Output and build files are ignored by Git.

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

Choose digital input, enter `101101`, select NRZ-L, then request decoding:

```text
SignalLab - ITT 036
Binary stream: 101101
Longest palindrome: 101101
Starting position (zero-based): 0
Palindrome length: 6
NRZ-L voltage levels: -1 +1 -1 -1 +1 -1
Decoded stream: 101101
Round trip: MATCH
```

## Files

- `main.cpp`: menu, input validation and output.
- `data_generator.hpp` / `data_generator.cpp`: binary data generation.
- `palindrome.hpp` / `palindrome.cpp`: longest palindrome search.
- `line_coding.hpp` / `line_coding.cpp`: basic line codes and AMI scrambling.
- `pcm.hpp` / `pcm.cpp`: uniform PCM encoder, decoder and staircase CSV export.
- `waveform.hpp` / `waveform.cpp`: SVG graphs and line-waveform CSV export.
- `tests/core_checks.cpp`: focused C++ checks without a test framework.
- `tests/cli_checks.py`: end-to-end menu, invalid-input and SVG checks.
- `report/specification.pdf`: one-page submission report.
- `report/make_specification.py`: optional report-generation source.

## Checks performed

Compile the application above, then run these from the repository root:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -I. tests/core_checks.cpp data_generator.cpp palindrome.cpp line_coding.cpp pcm.cpp waveform.cpp -o core_checks
```

Run `./core_checks` (Windows: `.\core_checks.exe`). With Python 3 available:

```sh
python tests/cli_checks.py ./signallab
```

On Windows use `python tests/cli_checks.py .\signallab.exe`.
The focused checks cover generation, zero-run guarantees, empty/single-bit and known
palindromes, exact voltage vectors for all five schemes, B8ZS polarities, HDB3 parity,
all seven decoder round trips, PCM midpoint reconstruction, timed CSV and SVG output,
and invalid input. Compile checks without `-DNDEBUG`, which disables C++ assertions.

## Limitations and submission

- Software simulation with ideal timing and normalized voltage levels; no hardware
  acquisition, noise recovery, analog low-pass reconstruction, or raster-image decoding.
- Level-based decoding is implemented. The assignment's stricter image-analysis
  extra-credit target is not claimed as complete.
- Palindrome search is O(n²) time and O(1) working space; ties return the earliest
  occurrence. Line encoding/scrambling and decoding take O(n) time.
- Invalid input prints an error and exits with status 1; restart to retry.
- PCM is the selected modulation option; delta modulation is not implemented.

Upload the source code and `report/specification.pdf` separately to Gradescope.
The local prepared code archive is `submission/SignalLab-source.zip` (generated
for submission, not tracked in Git). It excludes the report PDF and build outputs.

To regenerate the report when Python ReportLab is installed, run
`python report/make_specification.py` from the repository root.
