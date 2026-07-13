#pragma once

#include "ChordAnalysis.h"
#include "LiveMidiState.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <juce_gui_extra/juce_gui_extra.h>

class GrandStaffComponent final : public juce::Component
{
public:
    void setNotes(std::vector<int> midiNotes, std::vector<std::string> noteNames, juce::String keyName);
    void paint(juce::Graphics& graphics) override;

private:
    std::vector<int> notes;
    std::vector<std::string> names;
    juce::String displayedKey;
};

class PianoKeyboardDisplay final : public juce::Component
{
public:
    void setActiveNotes(std::vector<int> midiNotes);
    void paint(juce::Graphics& graphics) override;

private:
    std::set<int> activeNotes;
};

class MainComponent final : public juce::Component,
                            private juce::AsyncUpdater,
                            private juce::MidiInputCallback
{
public:
    explicit MainComponent(juce::ApplicationProperties& properties);
    ~MainComponent() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) override;
    void handleAsyncUpdate() override;

    void refreshMidiDevices(bool restoreSavedDevice);
    void selectMidiDevice(int deviceIndex);
    void updateAnalysis();
    void saveKeyContext();
    void showSettingsMenu();
    void resetSavedSelections();

    juce::ApplicationProperties& applicationProperties;
    LiveMidiState liveMidiState;
    std::unique_ptr<juce::MidiInput> midiInput;
    std::vector<juce::MidiDeviceInfo> midiDevices;

    chord::KeyContext keyContext;

    juce::Label deviceCaption { {}, "MIDI" };
    juce::Label keyCaption { {}, "Key" };
    juce::Label modeCaption { {}, "Mode" };
    juce::ComboBox midiDeviceBox;
    juce::ComboBox keyBox;
    juce::ComboBox modeBox;
    juce::TextButton settingsButton { "Settings" };

    juce::Label chordNameLabel;
    juce::Label romanLabel;
    juce::Label functionLabel;
    juce::Label possibleLabel;
    juce::Label noteCaption { {}, "Active Notes" };
    juce::Label activeNotesLabel;
    GrandStaffComponent grandStaff;
    PianoKeyboardDisplay pianoKeyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
