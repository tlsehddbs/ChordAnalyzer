#pragma once

#include <string>
#include <vector>

namespace chord {

enum class KeyMode
{
    major,
    minor
};

struct KeyContext
{
    int tonicPitchClass = 0;
    KeyMode mode = KeyMode::major;
};

enum class CandidateKind
{
    exact,
    omission,
    rootless,
    upperStructure,
    polychord
};

struct ChordCandidate
{
    int rootPitchClass = -1;
    std::string name;
    CandidateKind kind = CandidateKind::exact;
    int score = 0;
};

struct AnalysisResult
{
    std::string chordName = "—";
    std::string romanNumeral = "—";
    std::string harmonicFunction = "—";
    std::vector<std::string> activeNoteNames;
    std::vector<ChordCandidate> strictInterpretations;
    std::vector<ChordCandidate> possibleInterpretations;
};

class ChordAnalyzer
{
public:
    [[nodiscard]] static AnalysisResult analyze(const std::vector<int>& midiNotes,
                                                KeyContext key);
};

[[nodiscard]] std::string keyDisplayName(KeyContext key);
[[nodiscard]] std::string pitchClassName(int pitchClass, KeyContext key);

} // namespace chord
