#include "MainComponent.h"

#include <algorithm>
#include <array>
#include <map>

namespace
{
const juce::Colour background { 0xfff7f6f2 };
const juce::Colour surface { 0xffffffff };
const juce::Colour ink { 0xff22262c };
const juce::Colour mutedInk { 0xff69717a };
const juce::Colour divider { 0xffdddeda };
const juce::Colour accent { 0xff4d7197 };
const juce::Colour activeAccent { 0xffdce8f2 };

juce::String unicode(const uint32_t codePoint)
{
    return juce::String::charToString(static_cast<juce::juce_wchar>(codePoint));
}

const auto bullet = unicode(0x2022);
const auto emDash = unicode(0x2014);
const auto sharp = unicode(0x266f);
const auto flat = unicode(0x266d);
const auto trebleClef = unicode(0x1d11e);
const auto bassClef = unicode(0x1d122);

bool isBlackKey(const int midiNote)
{
    const auto pitchClass = ((midiNote % 12) + 12) % 12;
    return pitchClass == 1 || pitchClass == 3 || pitchClass == 6 || pitchClass == 8 || pitchClass == 10;
}

int letterIndex(const char letter)
{
    switch (letter)
    {
        case 'C': return 0;
        case 'D': return 1;
        case 'E': return 2;
        case 'F': return 3;
        case 'G': return 4;
        case 'A': return 5;
        case 'B': return 6;
        default: return 0;
    }
}

int diatonicValue(const int midiNote, const std::string& noteName)
{
    const auto octave = midiNote / 12 - 1;
    return octave * 7 + letterIndex(noteName.empty() ? 'C' : noteName.front());
}

juce::Font font(const float height, const bool bold = false)
{
    return juce::Font(juce::FontOptions(height, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font musicFont(const float height)
{
    return juce::Font(juce::FontOptions(height)
                          .withFallbacks({ "Bravura", "Noto Music", "Apple Symbols", "Segoe UI Symbol" })
                          .withFallbackEnabled());
}

void styleCaption(juce::Label& label)
{
    label.setFont(font(12.0f, true));
    label.setColour(juce::Label::textColourId, mutedInk);
    label.setJustificationType(juce::Justification::centredLeft);
}

void styleComboBox(juce::ComboBox& comboBox)
{
    comboBox.setColour(juce::ComboBox::backgroundColourId, surface);
    comboBox.setColour(juce::ComboBox::textColourId, ink);
    comboBox.setColour(juce::ComboBox::outlineColourId, divider);
    comboBox.setColour(juce::ComboBox::arrowColourId, mutedInk);
}

} // namespace

void GrandStaffComponent::setNotes(std::vector<int> midiNotes, std::vector<std::string> noteNames,
                                   const juce::String keyName)
{
    notes = std::move(midiNotes);
    names = std::move(noteNames);
    displayedKey = keyName;
    repaint();
}

void GrandStaffComponent::paint(juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    graphics.setColour(surface);
    graphics.fillRoundedRectangle(bounds, 10.0f);
    graphics.setColour(divider);
    graphics.drawRoundedRectangle(bounds, 10.0f, 1.0f);

    graphics.setColour(mutedInk);
    graphics.setFont(font(12.0f, true));
    graphics.drawText(juce::String("GRAND STAFF  ") + bullet + "  " + displayedKey, bounds.removeFromTop(24.0f).toNearestInt(),
                      juce::Justification::centredLeft);

    const auto lineGap = 11.0f;
    const auto left = bounds.getX() + 58.0f;
    const auto right = bounds.getRight() - 20.0f;
    const auto trebleTop = bounds.getY() + 38.0f;
    const auto bassTop = trebleTop + 86.0f;
    const auto drawStaff = [&](const float top)
    {
        graphics.setColour(juce::Colour(0xffa9afb5));
        for (int line = 0; line < 5; ++line)
            graphics.drawLine(left, top + line * lineGap, right, top + line * lineGap, 1.0f);
    };
    drawStaff(trebleTop);
    drawStaff(bassTop);

    graphics.setColour(ink);
    graphics.setFont(musicFont(37.0f));
    graphics.drawText(trebleClef, static_cast<int>(bounds.getX() + 16.0f), static_cast<int>(trebleTop - 13.0f), 34, 58,
                      juce::Justification::centred);
    graphics.drawText(bassClef, static_cast<int>(bounds.getX() + 17.0f), static_cast<int>(bassTop - 5.0f), 34, 48,
                      juce::Justification::centred);

    if (notes.empty())
    {
        graphics.setColour(mutedInk);
        graphics.setFont(font(15.0f));
        graphics.drawText("Play notes to see their notation", bounds.toNearestInt().withTrimmedTop(44),
                          juce::Justification::centred);
        return;
    }

    const auto drawNote = [&](const int midiNote, const std::string& name)
    {
        const auto treble = midiNote >= 60;
        const auto staffTop = treble ? trebleTop : bassTop;
        const auto staffBottom = staffTop + 4.0f * lineGap;
        const auto reference = treble ? 4 * 7 + letterIndex('E') : 2 * 7 + letterIndex('G');
        const auto y = staffBottom - (diatonicValue(midiNote, name) - reference) * lineGap * 0.5f;
        const auto noteX = bounds.getCentreX() + (midiNote % 2 == 0 ? -2.5f : 2.5f);

        graphics.setColour(ink);
        for (auto ledgerY = staffTop - lineGap; ledgerY >= y - 0.5f; ledgerY -= lineGap)
            graphics.drawLine(noteX - 10.0f, ledgerY, noteX + 10.0f, ledgerY, 1.0f);
        for (auto ledgerY = staffBottom + lineGap; ledgerY <= y + 0.5f; ledgerY += lineGap)
            graphics.drawLine(noteX - 10.0f, ledgerY, noteX + 10.0f, ledgerY, 1.0f);

        graphics.fillEllipse(noteX - 6.5f, y - 4.5f, 13.0f, 9.0f);
        graphics.drawLine(noteX + 5.5f, y, noteX + 5.5f, y - 32.0f, 1.4f);

        if (name.find('#') != std::string::npos || name.find('b') != std::string::npos)
        {
            graphics.setFont(musicFont(16.0f));
            graphics.drawText(name.find('#') != std::string::npos ? sharp : flat,
                              static_cast<int>(noteX - 26.0f), static_cast<int>(y - 10.0f), 16, 20,
                              juce::Justification::centred);
        }
    };

    for (size_t index = 0; index < notes.size(); ++index)
        drawNote(notes[index], index < names.size() ? names[index] : "C");
}

void PianoKeyboardDisplay::setActiveNotes(std::vector<int> midiNotes)
{
    activeNotes = std::set<int>(midiNotes.begin(), midiNotes.end());
    repaint();
}

void PianoKeyboardDisplay::paint(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    graphics.setColour(surface);
    graphics.fillRoundedRectangle(bounds, 10.0f);
    graphics.setColour(divider);
    graphics.drawRoundedRectangle(bounds, 10.0f, 1.0f);

    const auto keyboard = bounds.reduced(12.0f).withTrimmedTop(18.0f);
    std::map<int, int> whiteIndices;
    int whiteCount = 0;
    for (int note = 21; note <= 108; ++note)
        if (!isBlackKey(note))
            whiteIndices[note] = whiteCount++;

    const auto whiteWidth = keyboard.getWidth() / static_cast<float>(whiteCount);
    const auto blackWidth = whiteWidth * 0.62f;
    const auto blackHeight = keyboard.getHeight() * 0.62f;

    for (const auto& [note, index] : whiteIndices)
    {
        const auto key = juce::Rectangle<float>(keyboard.getX() + index * whiteWidth, keyboard.getY(), whiteWidth, keyboard.getHeight());
        graphics.setColour(activeNotes.contains(note) ? activeAccent : surface);
        graphics.fillRect(key);
        graphics.setColour(divider);
        graphics.drawRect(key, 0.75f);

        if (note == 60)
        {
            graphics.setColour(mutedInk);
            graphics.setFont(font(10.0f));
            graphics.drawText("C4", key.toNearestInt().withTrimmedTop(static_cast<int>(keyboard.getHeight() - 16.0f)),
                              juce::Justification::centred);
        }
    }

    for (int note = 21; note <= 108; ++note)
    {
        if (!isBlackKey(note))
            continue;
        auto previousWhite = note - 1;
        while (isBlackKey(previousWhite)) --previousWhite;
        const auto index = whiteIndices[previousWhite];
        const auto key = juce::Rectangle<float>(keyboard.getX() + (index + 1) * whiteWidth - blackWidth * 0.5f,
                                                keyboard.getY(), blackWidth, blackHeight);
        graphics.setColour(activeNotes.contains(note) ? accent : juce::Colour(0xff2a3036));
        graphics.fillRoundedRectangle(key, 2.0f);
    }
}

MainComponent::MainComponent(juce::ApplicationProperties& properties)
    : applicationProperties(properties)
{
    setOpaque(true);

    const auto* userSettings = applicationProperties.getUserSettings();
    keyContext.tonicPitchClass = userSettings != nullptr ? userSettings->getIntValue("keyTonic", 0) : 0;
    keyContext.mode = userSettings != nullptr && userSettings->getValue("keyMode", "major") == "minor"
        ? chord::KeyMode::minor : chord::KeyMode::major;

    for (auto* caption : { &deviceCaption, &keyCaption, &modeCaption, &noteCaption })
    {
        styleCaption(*caption);
        addAndMakeVisible(*caption);
    }

    styleComboBox(midiDeviceBox);
    styleComboBox(keyBox);
    styleComboBox(modeBox);
    addAndMakeVisible(midiDeviceBox);
    addAndMakeVisible(keyBox);
    addAndMakeVisible(modeBox);

    const std::array<juce::String, 12> keyLabels {
        "C", juce::String("C") + sharp + " / D" + flat, "D", juce::String("D") + sharp + " / E" + flat,
        "E", "F", juce::String("F") + sharp + " / G" + flat, "G", juce::String("G") + sharp + " / A" + flat,
        "A", juce::String("A") + sharp + " / B" + flat, "B"
    };
    for (int index = 0; index < static_cast<int>(keyLabels.size()); ++index)
        keyBox.addItem(keyLabels[static_cast<size_t>(index)], index + 1);
    modeBox.addItem("Major", 1);
    modeBox.addItem("Minor", 2);
    keyBox.setSelectedId(keyContext.tonicPitchClass + 1, juce::dontSendNotification);
    modeBox.setSelectedId(keyContext.mode == chord::KeyMode::major ? 1 : 2, juce::dontSendNotification);

    settingsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffeef0ed));
    settingsButton.setColour(juce::TextButton::textColourOffId, ink);
    addAndMakeVisible(settingsButton);

    const auto styleAnalysisLabel = [this](juce::Label& label, const float height, const juce::Colour colour)
    {
        label.setFont(font(height, height > 20.0f));
        label.setColour(juce::Label::textColourId, colour);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };
    styleAnalysisLabel(chordNameLabel, 51.0f, ink);
    styleAnalysisLabel(romanLabel, 19.0f, mutedInk);
    styleAnalysisLabel(functionLabel, 20.0f, accent);
    styleAnalysisLabel(possibleLabel, 14.0f, mutedInk);
    styleAnalysisLabel(activeNotesLabel, 19.0f, ink);
    chordNameLabel.setText(emDash, juce::dontSendNotification);
    romanLabel.setText(juce::String("Roman numeral  ") + emDash, juce::dontSendNotification);
    functionLabel.setText(juce::String("Harmonic function  ") + emDash, juce::dontSendNotification);

    addAndMakeVisible(grandStaff);
    addAndMakeVisible(pianoKeyboard);

    midiDeviceBox.onChange = [this]
    {
        selectMidiDevice(midiDeviceBox.getSelectedId() - 1);
    };
    keyBox.onChange = [this]
    {
        keyContext.tonicPitchClass = keyBox.getSelectedId() - 1;
        saveKeyContext();
        updateAnalysis();
    };
    modeBox.onChange = [this]
    {
        keyContext.mode = modeBox.getSelectedId() == 2 ? chord::KeyMode::minor : chord::KeyMode::major;
        saveKeyContext();
        updateAnalysis();
    };
    settingsButton.onClick = [this] { showSettingsMenu(); };

    refreshMidiDevices(true);
    updateAnalysis();
}

MainComponent::~MainComponent()
{
    cancelPendingUpdate();
    midiInput.reset();
}

void MainComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(background);
    graphics.setColour(divider);
    graphics.drawLine(0.0f, 75.0f, static_cast<float>(getWidth()), 75.0f, 1.0f);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(30);
    auto topBar = bounds.removeFromTop(42);

    const auto layoutControl = [](juce::Rectangle<int> area, juce::Label& caption, juce::Component& control)
    {
        caption.setBounds(area.removeFromTop(14));
        control.setBounds(area);
    };

    auto deviceArea = topBar.removeFromLeft(250);
    layoutControl(deviceArea, deviceCaption, midiDeviceBox);
    topBar.removeFromLeft(14);
    auto keyArea = topBar.removeFromLeft(145);
    layoutControl(keyArea, keyCaption, keyBox);
    topBar.removeFromLeft(14);
    auto modeArea = topBar.removeFromLeft(120);
    layoutControl(modeArea, modeCaption, modeBox);
    settingsButton.setBounds(topBar.removeFromRight(100).reduced(0, 7));

    bounds.removeFromTop(25);
    chordNameLabel.setBounds(bounds.removeFromTop(66));
    romanLabel.setBounds(bounds.removeFromTop(28));
    functionLabel.setBounds(bounds.removeFromTop(28));
    possibleLabel.setBounds(bounds.removeFromTop(26));
    bounds.removeFromTop(8);
    noteCaption.setBounds(bounds.removeFromTop(18));
    activeNotesLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);

    const auto keyboardHeight = juce::jlimit(104, 150, bounds.getHeight() / 4);
    pianoKeyboard.setBounds(bounds.removeFromBottom(keyboardHeight));
    bounds.removeFromBottom(12);
    grandStaff.setBounds(bounds);
}

void MainComponent::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    liveMidiState.process(message);
    triggerAsyncUpdate();
}

void MainComponent::handleAsyncUpdate()
{
    updateAnalysis();
}

void MainComponent::refreshMidiDevices(const bool restoreSavedDevice)
{
    midiInput.reset();
    liveMidiState.clear();
    const auto availableDevices = juce::MidiInput::getAvailableDevices();
    midiDevices.assign(availableDevices.begin(), availableDevices.end());
    midiDeviceBox.clear(juce::dontSendNotification);

    if (midiDevices.empty())
    {
        midiDeviceBox.addItem("No MIDI Input", 1);
        midiDeviceBox.setSelectedId(1, juce::dontSendNotification);
        midiDeviceBox.setEnabled(false);
        updateAnalysis();
        return;
    }

    midiDeviceBox.setEnabled(true);
    for (int index = 0; index < static_cast<int>(midiDevices.size()); ++index)
        midiDeviceBox.addItem(midiDevices[static_cast<size_t>(index)].name, index + 1);

    auto selectedIndex = 0;
    const auto* userSettings = applicationProperties.getUserSettings();
    const auto savedIdentifier = userSettings != nullptr ? userSettings->getValue("midiDeviceIdentifier") : juce::String();
    if (restoreSavedDevice && savedIdentifier.isNotEmpty())
    {
        for (int index = 0; index < static_cast<int>(midiDevices.size()); ++index)
            if (midiDevices[static_cast<size_t>(index)].identifier == savedIdentifier)
                selectedIndex = index;
    }

    midiDeviceBox.setSelectedId(selectedIndex + 1, juce::dontSendNotification);
    selectMidiDevice(selectedIndex);
}

void MainComponent::selectMidiDevice(const int deviceIndex)
{
    midiInput.reset();
    liveMidiState.clear();
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(midiDevices.size()))
    {
        updateAnalysis();
        return;
    }

    midiInput = juce::MidiInput::openDevice(midiDevices[static_cast<size_t>(deviceIndex)].identifier, this);
    if (midiInput != nullptr)
    {
        midiInput->start();
        if (auto* settings = applicationProperties.getUserSettings())
        {
            settings->setValue("midiDeviceIdentifier", midiDevices[static_cast<size_t>(deviceIndex)].identifier);
            settings->saveIfNeeded();
        }
    }
    updateAnalysis();
}

void MainComponent::updateAnalysis()
{
    const auto notes = liveMidiState.activeNotes();
    const auto analysis = chord::ChordAnalyzer::analyze(notes, keyContext);
    chordNameLabel.setText(analysis.chordName, juce::dontSendNotification);
    romanLabel.setText("Roman numeral  " + analysis.romanNumeral, juce::dontSendNotification);
    functionLabel.setText("Harmonic function  " + analysis.harmonicFunction, juce::dontSendNotification);

    juce::StringArray names;
    for (const auto& name : analysis.activeNoteNames)
        names.add(juce::String(name));
    activeNotesLabel.setText(names.isEmpty() ? emDash : names.joinIntoString(juce::String("  ") + bullet + "  "),
                             juce::dontSendNotification);

    juce::StringArray interpretations;
    for (const auto& candidate : analysis.possibleInterpretations)
        interpretations.add(juce::String(candidate.name));
    possibleLabel.setText(interpretations.isEmpty() ? "" : "Possible:  " + interpretations.joinIntoString("   |   "),
                          juce::dontSendNotification);

    grandStaff.setNotes(notes, analysis.activeNoteNames,
                        juce::String(chord::keyDisplayName(keyContext))
                            + (keyContext.mode == chord::KeyMode::major ? " Major" : " Minor"));
    pianoKeyboard.setActiveNotes(notes);
}

void MainComponent::saveKeyContext()
{
    if (auto* settings = applicationProperties.getUserSettings())
    {
        settings->setValue("keyTonic", keyContext.tonicPitchClass);
        settings->setValue("keyMode", keyContext.mode == chord::KeyMode::major ? "major" : "minor");
        settings->saveIfNeeded();
    }
}

void MainComponent::showSettingsMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Rescan MIDI Inputs");
    menu.addItem(2, "Reset Saved Selections");
    menu.addSeparator();
    menu.addItem(3, "Chord Analyzer 0.1.0", false);
    menu.addItem(4, "Distribution license: undecided", false);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settingsButton),
                       [safeThis = juce::Component::SafePointer<MainComponent>(this)](const int selection)
    {
        if (safeThis == nullptr) return;
        if (selection == 1) safeThis->refreshMidiDevices(true);
        if (selection == 2) safeThis->resetSavedSelections();
    });
}

void MainComponent::resetSavedSelections()
{
    if (auto* settings = applicationProperties.getUserSettings())
    {
        settings->removeValue("midiDeviceIdentifier");
        settings->removeValue("keyTonic");
        settings->removeValue("keyMode");
        settings->saveIfNeeded();
    }

    keyContext = {};
    keyBox.setSelectedId(1, juce::dontSendNotification);
    modeBox.setSelectedId(1, juce::dontSendNotification);
    refreshMidiDevices(false);
    updateAnalysis();
}
