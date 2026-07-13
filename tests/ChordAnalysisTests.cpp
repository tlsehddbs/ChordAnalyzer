#include "ChordAnalysis.h"
#include "LiveMidiState.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
int failures = 0;

void expect(const bool condition, const std::string& description)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << description << '\n';
        ++failures;
    }
}

chord::AnalysisResult analyze(const std::vector<int>& notes, const chord::KeyContext key = {})
{
    return chord::ChordAnalyzer::analyze(notes, key);
}

bool hasCandidate(const chord::AnalysisResult& result, const chord::CandidateKind kind)
{
    return std::any_of(result.possibleInterpretations.begin(), result.possibleInterpretations.end(),
                       [kind](const chord::ChordCandidate& candidate) { return candidate.kind == kind; });
}

} // namespace

int main()
{
    expect(analyze({ 50, 53, 57, 60 }).chordName == "Dm7", "recognises Dm7");
    expect(analyze({ 50, 53, 57, 60 }).romanNumeral == "ii7", "labels Dm7 as ii7 in C major");
    expect(analyze({ 50, 53, 57, 60 }).harmonicFunction == "Predominant", "labels Dm7 as predominant");

    expect(analyze({ 55, 59, 62, 65 }).chordName == "G7", "recognises G7");
    expect(analyze({ 55, 59, 62, 65 }).romanNumeral == "V7", "labels G7 as V7");
    expect(analyze({ 48, 52, 55, 59, 62 }).chordName == "Cmaj9", "recognises Cmaj9");
    expect(analyze({ 48, 52, 55, 57, 58 }).chordName == "C13", "recognises C13 voicing with omitted ninth and eleventh");

    expect(analyze({ 52, 55, 60 }).chordName == "C/E", "recognises slash-chord inversion");
    expect(analyze({ 48, 52, 58 }).chordName == "C7(no5)", "recognises omitted fifth");

    const auto altered = analyze({ 43, 47, 50, 53, 56, 61, 63 });
    expect(altered.chordName == "G7(b9,#11,b13)", "recognises compound altered dominant");
    expect(altered.harmonicFunction == "Dominant (Altered)", "labels altered V chord");

    const auto secondary = analyze({ 50, 54, 57, 60 });
    expect(secondary.romanNumeral == "V/V7", "labels D7 as V/V7 in C major");
    expect(secondary.harmonicFunction == "Secondary Dominant", "labels D7 as secondary dominant");

    const auto cMajor = analyze({ 48, 52, 55 });
    expect(cMajor.chordName == "C", "recognises C major triad");
    expect(!cMajor.possibleInterpretations.empty(), "offers alternate interpretations when available");

    const auto rootless = analyze({ 52, 58, 62 });
    expect(hasCandidate(rootless, chord::CandidateKind::rootless), "offers rootless jazz voicing as a possible interpretation");

    const auto polychord = analyze({ 48, 52, 55, 50, 54, 57 });
    expect(hasCandidate(polychord, chord::CandidateKind::polychord), "offers a polychord interpretation");

    const auto upperStructure = analyze({ 48, 52, 55, 58, 50, 54, 57 });
    expect(hasCandidate(upperStructure, chord::CandidateKind::upperStructure), "offers an upper-structure interpretation");

    const auto allAlterations = analyze({ 48, 52, 55, 58, 61, 63, 66, 68 });
    expect(allAlterations.chordName == "C7(b9,#9,#11,b13)", "recognises simultaneous altered tensions");
    expect(chord::keyDisplayName({ 1, chord::KeyMode::minor }) == "C#", "uses conventional minor-key spelling");

    LiveMidiState midiState;
    midiState.process(juce::MidiMessage::noteOn(1, 60, 1.0f));
    midiState.process(juce::MidiMessage::controllerEvent(1, 64, 127));
    midiState.process(juce::MidiMessage::noteOff(1, 60));
    expect(midiState.activeNotes() == std::vector<int> { 60 }, "sustain holds released note");
    midiState.process(juce::MidiMessage::controllerEvent(1, 64, 0));
    expect(midiState.activeNotes().empty(), "pedal release clears sustained note");

    if (failures != 0)
        return EXIT_FAILURE;

    std::cout << "Chord analysis tests passed\n";
    return EXIT_SUCCESS;
}
