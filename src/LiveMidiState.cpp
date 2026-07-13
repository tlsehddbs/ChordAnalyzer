#include "LiveMidiState.h"

#include <algorithm>

void LiveMidiState::process(const juce::MidiMessage& message)
{
    std::scoped_lock lock(mutex);

    if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        heldCounts.fill(0);
        sustainedNotes.fill(false);
        return;
    }

    if (message.isController() && message.getControllerNumber() == 64)
    {
        sustainDown = message.getControllerValue() >= 64;
        if (!sustainDown)
        {
            for (int note = 0; note < static_cast<int>(sustainedNotes.size()); ++note)
                if (heldCounts[static_cast<size_t>(note)] == 0)
                    sustainedNotes[static_cast<size_t>(note)] = false;
        }
        return;
    }

    const auto note = message.getNoteNumber();
    if (note < 0 || note >= static_cast<int>(heldCounts.size()))
        return;

    if (message.isNoteOn())
    {
        ++heldCounts[static_cast<size_t>(note)];
        sustainedNotes[static_cast<size_t>(note)] = false;
    }
    else if (message.isNoteOff())
    {
        auto& held = heldCounts[static_cast<size_t>(note)];
        held = std::max(0, held - 1);
        if (held == 0 && sustainDown)
            sustainedNotes[static_cast<size_t>(note)] = true;
    }
}

void LiveMidiState::clear()
{
    std::scoped_lock lock(mutex);
    heldCounts.fill(0);
    sustainedNotes.fill(false);
    sustainDown = false;
}

std::vector<int> LiveMidiState::activeNotes() const
{
    std::scoped_lock lock(mutex);
    std::vector<int> notes;
    for (int note = 0; note < static_cast<int>(heldCounts.size()); ++note)
        if (heldCounts[static_cast<size_t>(note)] > 0 || sustainedNotes[static_cast<size_t>(note)])
            notes.push_back(note);
    return notes;
}
