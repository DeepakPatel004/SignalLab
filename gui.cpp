#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include "data_generator.hpp"
#include "line_coding.hpp"
#include "palindrome.hpp"
#include "pcm.hpp"
#include "waveform.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
enum ControlId { Input = 101, Source, Bits, Length, Samples, Count, Rate, Depth,
                 Minimum, Maximum, Scheme, Scrambler, Encode, Decode, Export, Folder };
struct Encoding {
    const char* name;
    std::vector<int> (*encode)(const std::string&);
    std::string (*decode)(const std::vector<int>&);
    int levels_per_bit;
};
const Encoding encodings[] = {
    {"NRZ-L", encode_nrz_l, decode_nrz_l, 1},
    {"NRZ-I", encode_nrz_i, decode_nrz_i, 1},
    {"Manchester", encode_manchester, decode_manchester, 2},
    {"Differential Manchester", encode_differential_manchester, decode_differential_manchester, 2},
    {"AMI", encode_ami, decode_ami, 1},
    {"B8ZS", encode_b8zs, decode_b8zs, 1},
    {"HDB3", encode_hdb3, decode_hdb3, 1}
};
struct Plot {
    std::string title;
    std::string axis;
    std::vector<double> levels;
    std::vector<double> samples;
    double interval = 1;
    double low = -1, high = 1;
};

HWND window, graph, results, status;
HFONT font, heading_font;
HBRUSH background;
std::vector<Plot> plots;
std::string bits, summary;
std::vector<int> signal, ami;
std::vector<double> samples, reconstructed;
double sample_interval = 1;
int selected_encoding = 0;
bool encoded_pcm = false;

HWND control(int id) { return GetDlgItem(window, id); }
int selection(int id) { return static_cast<int>(SendMessageA(control(id), CB_GETCURSEL, 0, 0)); }
std::string text(int id) {
    const int size = GetWindowTextLengthA(control(id));
    std::string value(static_cast<std::size_t>(size) + 1, '\0');
    GetWindowTextA(control(id), value.data(), size + 1);
    value.resize(size);
    return value;
}
double number(int id, const char* label) {
    std::istringstream input(text(id));
    double value;
    char extra;
    if (!(input >> value) || (input >> extra) || !std::isfinite(value)) {
        throw std::invalid_argument(std::string(label) + ": enter a finite number.");
    }
    return value;
}
int integer(int id, int low, int high, const char* label) {
    const double value = number(id, label);
    if (value < low || value > high || value != std::floor(value)) {
        throw std::invalid_argument(std::string(label) + ": enter a whole number from " +
                                    std::to_string(low) + " to " + std::to_string(high) + ".");
    }
    return static_cast<int>(value);
}
HWND add(const char* type, const char* value, int x, int y, int width, int height,
         int id = 0, DWORD style = 0) {
    const DWORD extended = std::string(type) == "EDIT" ? WS_EX_CLIENTEDGE : 0;
    HWND child = CreateWindowExA(extended, type, value, WS_CHILD | WS_VISIBLE | style,
                                x, y, width, height, window,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
    SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return child;
}
void label(const char* value, int x, int y, int width) { add("STATIC", value, x, y, width, 22); }
void edit(int id, const char* value, int x, int y, int width) {
    add("EDIT", value, x, y, width, 27, id, ES_AUTOHSCROLL | WS_TABSTOP);
}
void combo(int id, const std::vector<const char*>& values, int x, int y, int width) {
    HWND child = add("COMBOBOX", "", x, y, width, 220, id,
                     CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
    for (const char* value : values) SendMessageA(child, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(value));
    SendMessageA(child, CB_SETCURSEL, 0, 0);
}
void availability() {
    const bool pcm = selection(Input) == 1;
    const int source = selection(Source);
    EnableWindow(control(Bits), !pcm && source == 0);
    EnableWindow(control(Length), !pcm && source != 0);
    EnableWindow(control(Samples), pcm && source == 0);
    for (int id : {Rate, Depth, Minimum, Maximum}) EnableWindow(control(id), pcm);
    EnableWindow(control(Count), pcm && source != 0);
    EnableWindow(control(Scrambler), selection(Scheme) == 4);
}
void change_input() {
    SendMessageA(control(Source), CB_RESETCONTENT, 0, 0);
    const std::vector<const char*> choices = selection(Input) == 0
        ? std::vector<const char*>{"Enter bits", "Random bits", "Contains 0000", "Contains 00000000"}
        : std::vector<const char*>{"Enter samples", "Generate sine"};
    for (const char* item : choices) SendMessageA(control(Source), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
    SendMessageA(control(Source), CB_SETCURSEL, 0, 0);
    availability();
}
void append_levels(std::ostringstream& out, const std::vector<int>& levels) {
    for (int value : levels) out << (value > 0 ? "+1 " : value < 0 ? "-1 " : "0 ");
    out << "\r\n";
}
std::vector<double> preview(const std::vector<int>& levels, std::size_t count) {
    return {levels.begin(), levels.begin() + std::min(levels.size(), count)};
}
void generate() {
    encoded_pcm = selection(Input) == 1;
    samples.clear();
    reconstructed.clear();
    plots.clear();
    if (encoded_pcm) {
        const int rate = integer(Rate, 1, 1000000, "Sampling rate");
        const int depth = integer(Depth, 1, 8, "Bit depth");
        const double low = number(Minimum, "Minimum"), high = number(Maximum, "Maximum");
        if (!(low < high) || !std::isfinite(high - low)) {
            throw std::invalid_argument("Quantization range must have finite minimum < maximum.");
        }
        sample_interval = 1.0 / rate;
        if (selection(Source) == 0) {
            std::string values = text(Samples);
            std::replace(values.begin(), values.end(), ',', ' ');
            std::istringstream input(values);
            std::string token;
            while (input >> token) {
                std::istringstream sample_input(token);
                double value;
                char extra;
                if (!(sample_input >> value) || (sample_input >> extra) || !std::isfinite(value)) {
                    throw std::invalid_argument("Samples must be finite numbers separated by spaces or commas.");
                }
                samples.push_back(value);
                if (samples.size() > 1000) throw std::invalid_argument("Use at most 1000 PCM samples.");
            }
        } else {
            const int count = integer(Count, 1, 1000, "Sample count");
            for (int i = 0; i < count; ++i) {
                const double amplitude = (high - low) / 2;
                samples.push_back(std::clamp(low + amplitude + amplitude *
                    std::sin(2 * std::acos(-1.0) * i / 8), low, high));
            }
        }
        bits = encode_pcm(samples, depth, low, high);
        reconstructed = decode_pcm(bits, depth, low, high);
        const auto count = std::min<std::size_t>(64, samples.size());
        plots.push_back({"PCM staircase - blue: reconstructed; orange dots: input samples",
                         "Time (seconds)", {reconstructed.begin(), reconstructed.begin() + count},
                         {samples.begin(), samples.begin() + count}, sample_interval, low, high});
    } else if (selection(Source) == 0) {
        bits = text(Bits);
    } else {
        const std::size_t runs[] = {0, 0, 4, 8};
        bits = generate_binary(integer(Length, 0, 10000, "Bit count"), runs[selection(Source)]);
        SetWindowTextA(control(Bits), bits.c_str());
    }
    if (bits.size() > max_stream_length || bits.find_first_not_of("01") != std::string::npos) {
        throw std::invalid_argument("Use up to 10000 bits containing only 0 and 1.");
    }
    selected_encoding = selection(Scheme);
    if (selected_encoding == 4) selected_encoding += selection(Scrambler);
    const auto& coding = encodings[selected_encoding];
    signal = coding.encode(bits);
    ami = selected_encoding >= 5 ? encode_ami(bits) : std::vector<int>{};
    std::vector<double> digital;
    for (std::size_t i = 0; i < std::min<std::size_t>(64, bits.size()); ++i) digital.push_back(bits[i] - '0');
    if (!bits.empty()) {
        plots.push_back({"Digital input", "Time (bit periods)", digital, {}, 1, 0, 1});
        if (!ami.empty()) plots.push_back({"AMI before scrambling (V)", "Time (bit periods)",
                                          preview(ami, 64), {}, 1, -1, 1});
        plots.push_back({std::string(coding.name) + " encoded waveform (V)", "Time (bit periods)",
                         preview(signal, 64 * coding.levels_per_bit), {}, 1.0 / coding.levels_per_bit, -1, 1});
    }
    const auto palindrome = find_longest_palindrome(bits);
    std::ostringstream out;
    out << "BINARY STREAM (" << bits.size() << " bits)\r\n" << bits
        << "\r\n\r\nLONGEST PALINDROME\r\n" << bits.substr(palindrome.start, palindrome.length)
        << "\r\nStart (zero-based): " << palindrome.start << "\r\nLength: " << palindrome.length;
    if (encoded_pcm) {
        out << "\r\n\r\nPCM RECONSTRUCTION\r\nTime (s) | Input | Decoded\r\n";
        for (std::size_t i = 0; i < samples.size(); ++i) {
            out << std::setprecision(6) << i * sample_interval << " | "
                << samples[i] << " | " << reconstructed[i] << "\r\n";
        }
    }
    out << "\r\n\r\n" << coding.name << " VOLTAGE LEVELS\r\n";
    append_levels(out, signal);
    if (bits.empty()) out << "Empty stream: no line signal to encode.\r\n";
    summary = out.str();
    SetWindowTextA(results, summary.c_str());
    SetWindowTextA(status, "Signal ready. Click Decode to recover the bits, or Save SVG / CSV to export.");
    EnableWindow(control(Decode), !bits.empty());
    EnableWindow(control(Export), !bits.empty());
    InvalidateRect(graph, nullptr, TRUE);
}
void decode() {
    const auto recovered = encodings[selected_encoding].decode(signal);
    const std::string report = summary + "\r\nDECODED STREAM\r\n" + recovered +
        "\r\nRound trip: " + (recovered == bits ? "MATCH" : "MISMATCH") + "\r\n";
    SetWindowTextA(results, report.c_str());
    SetWindowTextA(status, recovered == bits ? "Decoded successfully. Recovered bits match the input." :
                                            "Decoded stream differs from the input.");
}
void export_files() {
    std::filesystem::create_directories("output");
    const auto& coding = encodings[selected_encoding];
    write_line_waveform(bits, signal, coding.levels_per_bit, coding.name, "output/line_waveform", ami);
    if (encoded_pcm) {
        write_pcm_staircase_csv(samples, reconstructed, "output/pcm_staircase.csv", sample_interval);
        write_pcm_waveform(samples, reconstructed, sample_interval, "output/pcm_staircase.svg");
    }
    SetWindowTextA(status, "Saved graphs and complete CSV data in the output folder.");
}
void draw_text(HDC dc, int x, int y, const std::string& value) {
    TextOutA(dc, x, y, value.c_str(), static_cast<int>(value.size()));
}
std::string short_number(double value) {
    std::ostringstream out;
    out << std::setprecision(4) << value;
    return out.str();
}
LRESULT CALLBACK graph_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message != WM_PAINT) return DefWindowProcA(hwnd, message, wparam, lparam);
    PAINTSTRUCT paint;
    HDC dc = BeginPaint(hwnd, &paint);
    RECT bounds;
    GetClientRect(hwnd, &bounds);
    FillRect(dc, &bounds, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(30, 48, 70));
    draw_text(dc, 16, 12, "WAVEFORMS  |  First 64 bits / samples");
    if (plots.empty()) draw_text(dc, 16, 55, "Choose your input and click Encode && Draw.");
    const int panel_height = (bounds.bottom - 48) / static_cast<int>(std::max<std::size_t>(1, plots.size()));
    HPEN grid_pen = CreatePen(PS_SOLID, 1, RGB(220, 229, 238));
    HPEN wave_pen = CreatePen(PS_SOLID, 2, RGB(24, 103, 184));
    HPEN dot_pen = CreatePen(PS_SOLID, 1, RGB(215, 107, 30));
    HBRUSH dot_brush = CreateSolidBrush(RGB(215, 107, 30));
    HGDIOBJ old_pen = SelectObject(dc, grid_pen);
    HGDIOBJ old_brush = SelectObject(dc, dot_brush);
    for (std::size_t index = 0; index < plots.size(); ++index) {
        const auto& plot = plots[index];
        const int top = 66 + static_cast<int>(index) * panel_height;
        const int left = 64, width = bounds.right - 88, height = panel_height - 72;
        const auto y = [&](double value) { return top + static_cast<int>(
            (plot.high - value) / (plot.high - plot.low) * height); };
        draw_text(dc, 16, top - 26, plot.title);
        SelectObject(dc, grid_pen);
        for (int tick = 0; tick <= 2; ++tick) {
            const double value = plot.low + (plot.high - plot.low) * tick / 2;
            MoveToEx(dc, left, y(value), nullptr); LineTo(dc, left + width, y(value));
            draw_text(dc, 4, y(value) - 8, short_number(value));
        }
        for (int tick = 0; tick <= 4; ++tick) {
            const int x = left + width * tick / 4;
            MoveToEx(dc, x, top, nullptr); LineTo(dc, x, top + height);
            draw_text(dc, x - 10, top + height + 4,
                      short_number(plot.levels.size() * plot.interval * tick / 4));
        }
        draw_text(dc, left + width / 2 - 65, top + height + 24, plot.axis);
        SelectObject(dc, wave_pen);
        for (std::size_t i = 0; i < plot.levels.size(); ++i) {
            const int x = left + static_cast<int>(width * i / plot.levels.size());
            if (i == 0) MoveToEx(dc, x, y(plot.levels[i]), nullptr);
            else LineTo(dc, x, y(plot.levels[i]));
            LineTo(dc, left + static_cast<int>(width * (i + 1) / plot.levels.size()), y(plot.levels[i]));
        }
        SelectObject(dc, dot_pen);
        for (std::size_t i = 0; i < plot.samples.size(); ++i) {
            const int x = left + static_cast<int>(width * i / plot.samples.size());
            const int center = y(plot.samples[i]);
            Ellipse(dc, x - 3, center - 3, x + 4, center + 4);
        }
    }
    SelectObject(dc, old_pen); SelectObject(dc, old_brush);
    DeleteObject(grid_pen); DeleteObject(wave_pen); DeleteObject(dot_pen); DeleteObject(dot_brush);
    EndPaint(hwnd, &paint);
    return 0;
}
void layout() {
    RECT rect;
    GetClientRect(window, &rect);
    MoveWindow(results, 20, 286, 350, rect.bottom - 328, TRUE);
    MoveWindow(graph, 386, 286, rect.right - 406, rect.bottom - 328, TRUE);
    MoveWindow(status, 20, rect.bottom - 32, rect.right - 40, 25, TRUE);
}
LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_CREATE) {
        window = hwnd;
        HWND title = add("STATIC", "SignalLab  |  Digital Signal Encoder && Decoder", 20, 14, 900, 28);
        SendMessageA(title, WM_SETFONT, reinterpret_cast<WPARAM>(heading_font), TRUE);
        label("&Input", 20, 54, 90); combo(Input, {"Digital", "Analog / PCM"}, 20, 77, 170);
        label("&Source", 210, 54, 100); combo(Source, {"Enter bits", "Random bits", "Contains 0000", "Contains 00000000"}, 210, 77, 210);
        label("Line &encoding", 440, 54, 170); combo(Scheme, {"NRZ-L", "NRZ-I", "Manchester", "Differential Manchester", "AMI"}, 440, 77, 230);
        label("AMI sc&rambling", 690, 54, 170); combo(Scrambler, {"None", "B8ZS", "HDB3"}, 690, 77, 170);
        label("Bit count", 880, 54, 100); edit(Length, "24", 880, 77, 100);
        label("Binary input", 20, 116, 120); edit(Bits, "101101", 145, 112, 835);
        label("PCM samples", 20, 151, 120); edit(Samples, "-1, -0.5, 0, 0.5, 1", 145, 147, 835);
        label("Sine sample count", 20, 187, 155); edit(Count, "16", 20, 210, 150);
        label("Sampling rate (Hz)", 190, 187, 160); edit(Rate, "8", 190, 210, 150);
        label("PCM bits (1-8)", 360, 187, 130); edit(Depth, "3", 360, 210, 130);
        label("Minimum", 510, 187, 100); edit(Minimum, "-1", 510, 210, 100);
        label("Maximum", 630, 187, 100); edit(Maximum, "1", 630, 210, 100);
        add("BUTTON", "Encode / Draw (F5)", 20, 248, 160, 30, Encode, BS_DEFPUSHBUTTON | WS_TABSTOP);
        add("BUTTON", "Decode (F6)", 192, 248, 110, 30, Decode, WS_TABSTOP);
        add("BUTTON", "Save SVG / CSV", 314, 248, 155, 30, Export, WS_TABSTOP);
        add("BUTTON", "Open output folder", 481, 248, 180, 30, Folder, WS_TABSTOP);
        results = add("EDIT", "Choose your input, then click Encode & Draw.", 20, 286, 350, 400, 0,
                      ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP);
        SendMessageA(results, EM_SETLIMITTEXT, 2000000, 0);
        graph = add("SignalLabPlot", "", 386, 286, 700, 400);
        status = add("STATIC", "Ready. Graphs are displayed here; no browser is needed.", 20, 730, 1100, 25);
        EnableWindow(control(Decode), FALSE); EnableWindow(control(Export), FALSE);
        availability();
        return 0;
    }
    if (message == WM_SIZE) { if (results) layout(); return 0; }
    if (message == WM_GETMINMAXINFO) {
        auto* limits = reinterpret_cast<MINMAXINFO*>(lparam);
        limits->ptMinTrackSize = {1120, 800};
        return 0;
    }
    if (message == WM_COMMAND) {
        const int id = LOWORD(wparam);
        try {
            if (id == Input && HIWORD(wparam) == CBN_SELCHANGE) change_input();
            if ((id == Source || id == Scheme) && HIWORD(wparam) == CBN_SELCHANGE) availability();
            if (HIWORD(wparam) == BN_CLICKED) {
                if (id == Encode) generate();
                if (id == Decode) decode();
                if (id == Export) export_files();
                if (id == Folder) {
                    std::filesystem::create_directories("output");
                    ShellExecuteA(hwnd, "open", std::filesystem::absolute("output").string().c_str(),
                                  nullptr, nullptr, SW_SHOWNORMAL);
                }
            }
        } catch (const std::exception& error) {
            SetWindowTextA(status, error.what());
            if (id == Encode) {
                plots.clear(); signal.clear();
                SetWindowTextA(results, error.what());
                EnableWindow(control(Decode), FALSE); EnableWindow(control(Export), FALSE);
                InvalidateRect(graph, nullptr, TRUE);
            }
        }
        return 0;
    }
    if (message == WM_CTLCOLORSTATIC) {
        HDC dc = reinterpret_cast<HDC>(wparam);
        SetBkColor(dc, RGB(243, 246, 250)); SetTextColor(dc, RGB(30, 48, 70));
        return reinterpret_cast<LRESULT>(background);
    }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    SetProcessDPIAware();
    // Double-click launches use the executable's folder for exports.
    char executable[MAX_PATH];
    if (GetModuleFileNameA(nullptr, executable, MAX_PATH)) {
        std::filesystem::current_path(std::filesystem::path(executable).parent_path());
    }
    font = CreateFontA(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       DEFAULT_PITCH, "Segoe UI");
    heading_font = CreateFontA(-24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH, "Segoe UI");
    background = CreateSolidBrush(RGB(243, 246, 250));
    WNDCLASSA plot_class{};
    plot_class.lpfnWndProc = graph_proc; plot_class.hInstance = instance;
    plot_class.lpszClassName = "SignalLabPlot"; plot_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&plot_class);
    WNDCLASSA main_class{};
    main_class.lpfnWndProc = window_proc; main_class.hInstance = instance;
    main_class.lpszClassName = "SignalLabWindow"; main_class.hbrBackground = background;
    main_class.hCursor = LoadCursor(nullptr, IDC_ARROW); main_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassA(&main_class);
    HWND app = CreateWindowExA(0, "SignalLabWindow", "SignalLab", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                               CW_USEDEFAULT, CW_USEDEFAULT, 1240, 900, nullptr, nullptr, instance, nullptr);
    if (!app) return 1;
    ShowWindow(app, show); UpdateWindow(app);
    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        if (message.message == WM_KEYDOWN && (message.wParam == VK_F5 || message.wParam == VK_F6)) {
            const int action = message.wParam == VK_F5 ? Encode : Decode;
            if (IsWindowEnabled(control(action))) SendMessageA(app, WM_COMMAND, action, 0);
            continue;
        }
        if (!IsDialogMessageA(app, &message)) {
            TranslateMessage(&message); DispatchMessageA(&message);
        }
    }
    DeleteObject(font); DeleteObject(heading_font); DeleteObject(background);
    return static_cast<int>(message.wParam);
}
