# SignalLab

ITT 036 Digital Signal Encoder & Decoder, implemented in C++17.

## Open the GUI

On Windows, double-click `SignalLabGUI.exe`. No terminal or browser is needed.

To build it from source, install MinGW g++ and run `build_gui.cmd`, or use:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -mwindows gui.cpp data_generator.cpp palindrome.cpp line_coding.cpp pcm.cpp waveform.cpp -lgdi32 -lshell32 -o SignalLabGUI.exe
```

The GUI uses Windows controls and GDI drawing. No additional graphical framework
is required. The static build includes the C++ runtime; the executable uses Windows
system libraries. Generated files are stored beside the executable in `output/`.

## Use the application

1. Choose **Digital** or **Analog / PCM** under Input.
2. Choose the source and fill in the enabled fields.
3. Choose a line encoding; AMI enables the scrambling selector.
4. Click **Encode / Draw (F5)**. Results appear on the left and waveforms on the right.
5. Click **Decode (F6)** to recover the bits from the encoded levels and compare them.
6. Optionally click **Save SVG / CSV** to export graphs and complete signal data.

Digital sources: typed binary input, random bits, or random streams containing
four/eight consecutive zeros. The requested zero sequence is guaranteed; the stream
must be long enough to contain it. Up to 10000 bits are accepted. The longest
contiguous palindrome is displayed with its zero-based position and length.

PCM sources: comma/space-separated samples or a generated sine. Set the sample
rate, bit depth, minimum and maximum. Sine sample count applies to generated input;
manual input uses however many values you enter (up to 1000). The app displays
input samples, reconstructed values, the PCM staircase and line-coded PCM bits.

Invalid values are shown in the status area; correct them and click Encode again.
Graphs show the first 64 bits/samples. Exported CSV files retain the full data.

## Signal conventions

- NRZ-L: `0` is `+1`; `1` is `-1`.
- NRZ-I: starts at `+1`; `1` toggles the level and `0` leaves it unchanged.
- Manchester: `0` is `(+1,-1)`; `1` is `(-1,+1)` within each bit.
- Differential Manchester: starts at `+1`; every bit transitions at the midpoint,
  and `0` also transitions at the bit boundary.
- AMI: `0` is zero voltage; `1` marks alternate polarity, starting at `+1`.
- B8ZS: replaces eight zeros with `000VB0VB`. A violation `V` repeats the preceding
  nonzero polarity; a balancing `B` alternates it. The virtual previous mark is `-1`.
- HDB3: replaces four zeros with `000V` after an odd ordinary-mark count since the
  last substitution; otherwise `B00V`. The count starts at zero and resets after
  each substitution. In `B00V`, `B` opposes the preceding mark and `V` equals `B`.

NRZ and AMI use one level per bit; Manchester schemes use two half-bit levels.
The decoder receives only encoded levels. Original bits are used only for the
round-trip comparison. Initial polarities match the encoder conventions above.

## PCM encoding

Manual samples are assumed equally spaced at the chosen rate `Fs`. Generated sine
frequency is `Fs/8`, with midpoint offset and half-range amplitude. Sampling times
are `i/Fs` seconds.

The quantizer uses `2^b` equal intervals for `b=1..8`. Step size is
`(maximum-minimum)/2^b`; sample codes are interval indices, most-significant bit
first. The maximum endpoint maps to the last interval. Decoding uses interval
midpoints and holds each value for one sample interval (zero-order hold).

For range `[-1,1]`, depth `2`, and samples `-1,0,1`, the codes are `00 10 11`
and reconstructed values are `-0.75,0.25,0.75`.

## Files

- `gui.cpp`: Windows GUI, input fields, results and waveform drawing.
- `build_gui.cmd`: builds the Windows GUI executable.
- `main.cpp`: optional command-line interface.
- `data_generator.cpp/.hpp`: random and constrained binary generation.
- `palindrome.cpp/.hpp`: expand-around-center palindrome search.
- `line_coding.cpp/.hpp`: line encoders, scramblers and level decoders.
- `pcm.cpp/.hpp`: PCM encoding, decoding and staircase CSV export.
- `waveform.cpp/.hpp`: optional SVG and CSV export.

## Optional command-line build

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp data_generator.cpp palindrome.cpp line_coding.cpp pcm.cpp waveform.cpp -o signallab
```

Run `./signallab` or `.\signallab.exe`. Compile the GUI and CLI entry points
separately; do not compile every `.cpp` into one executable.
