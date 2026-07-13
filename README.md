# Chord Analyzer

Standalone JUCE desktop application for real-time MIDI chord, notation, Roman-numeral, and harmonic-function analysis.

## Build

Install CMake 3.22 or newer, then configure and build with the macOS Makefiles preset:

```sh
cmake --preset macos-make-debug
cmake --build build/macos-make-debug --parallel 4
ctest --test-dir build/macos-make-debug --output-on-failure
```

The first configure downloads the pinned JUCE 8.0.13 source through CMake FetchContent.

The Debug app bundle is created at `build/macos-make-debug/ChordAnalyzer_artefacts/Debug/Chord Analyzer.app`.

## Windows build

Install Visual Studio 2022 with the **Desktop development with C++** workload and CMake 3.22 or newer. In a Developer PowerShell, run:

```powershell
cmake --preset windows-release
cmake --build build/windows-release --config Release --target ChordAnalyzer ChordAnalysisTests --parallel
ctest --test-dir build/windows-release -C Release --output-on-failure
```

The Windows executable is created at `build/windows-release/ChordAnalyzer_artefacts/Release/Chord Analyzer.exe`. The MSVC runtime is statically linked, so the release executable does not require a separate Visual C++ runtime installer.

## Distribution note

JUCE distribution requires a compatible licence choice. This repository is intentionally marked as distribution-licence undecided until the owner chooses either the applicable open-source terms or a JUCE commercial licence.
