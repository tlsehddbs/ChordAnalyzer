cmake_minimum_required(VERSION 3.22)

project(ChordAnalyzer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.13
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(ChordAnalyzer
    PRODUCT_NAME "Chord Analyzer"
    COMPANY_NAME "Chord Analyzer"
    BUNDLE_ID "com.chordanalyzer.app"
)

target_sources(ChordAnalyzer PRIVATE
    src/Main.cpp
    src/ChordAnalysis.cpp
    src/LiveMidiState.cpp
    src/MainComponent.cpp
)

target_include_directories(ChordAnalyzer PRIVATE src)

target_compile_definitions(ChordAnalyzer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(ChordAnalyzer PRIVATE
    juce::juce_audio_devices
    juce::juce_gui_extra
)

enable_testing()

juce_add_console_app(ChordAnalysisTests PRODUCT_NAME "Chord Analysis Tests")
target_sources(ChordAnalysisTests PRIVATE
    tests/ChordAnalysisTests.cpp
    src/ChordAnalysis.cpp
    src/LiveMidiState.cpp
)
target_include_directories(ChordAnalysisTests PRIVATE src)
target_link_libraries(ChordAnalysisTests PRIVATE juce::juce_audio_devices)
add_test(NAME ChordAnalysisTests COMMAND ChordAnalysisTests)
