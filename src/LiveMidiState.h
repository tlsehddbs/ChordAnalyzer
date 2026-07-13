#pragma once

#include <array>
#include <mutex>
#include <vector>

#include <juce_audio_devices/juce_audio_devices.h>

class LiveMidiState
{
public:
    void process(const juce::MidiMessage& message);
    void clear();
    [[nodiscard]] std::vector<int> activeNotes() const;

private:
    mutable std::mutex mutex;
    std::array<int, 128> heldCounts {};
    std::array<bool, 128> sustainedNotes {};
    bool sustainDown = false;
};
