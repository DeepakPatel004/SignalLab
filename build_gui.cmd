@echo off
cd /d "%~dp0"
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -mwindows gui.cpp data_generator.cpp palindrome.cpp line_coding.cpp pcm.cpp waveform.cpp -lgdi32 -lshell32 -o SignalLabGUI.exe
if errorlevel 1 (
    echo Build failed. Ensure MinGW g++ is installed and on PATH.
    pause
    exit /b 1
)
echo Built SignalLabGUI.exe. Double-click it to open the GUI.
