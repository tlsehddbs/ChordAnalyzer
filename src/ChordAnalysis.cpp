#include "ChordAnalysis.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <tuple>

namespace chord
{
namespace
{
constexpr std::array<const char*, 12> defaultNames { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
constexpr std::array<const char*, 12> sharpNames   { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
constexpr std::array<const char*, 12> majorKeys    { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
constexpr std::array<const char*, 12> minorKeys    { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B" };
constexpr std::array<int, 7> letterPitchClasses { 0, 2, 4, 5, 7, 9, 11 };
constexpr std::array<const char*, 7> letters { "C", "D", "E", "F", "G", "A", "B" };

int normalise(const int value)
{
    const auto result = value % 12;
    return result < 0 ? result + 12 : result;
}

bool contains(const std::set<int>& values, const int value)
{
    return values.contains(normalise(value));
}

enum class Quality
{
    major,
    minor,
    diminished,
    augmented,
    sus2,
    sus4,
    power
};

struct ToneSpec
{
    int interval;
    int degree;
};

struct CoreDefinition
{
    Quality quality;
    std::string suffix;
    std::vector<ToneSpec> tones;
    int third = -1;
    int fifth = -1;
};

const std::array<CoreDefinition, 7> coreDefinitions {
    CoreDefinition { Quality::major, "",     {{ 0, 1 }, { 4, 3 }, { 7, 5 }}, 4, 7 },
    CoreDefinition { Quality::minor, "m",    {{ 0, 1 }, { 3, 3 }, { 7, 5 }}, 3, 7 },
    CoreDefinition { Quality::diminished, "dim", {{ 0, 1 }, { 3, 3 }, { 6, 5 }}, 3, 6 },
    CoreDefinition { Quality::augmented, "aug", {{ 0, 1 }, { 4, 3 }, { 8, 5 }}, 4, 8 },
    CoreDefinition { Quality::sus2, "sus2", {{ 0, 1 }, { 2, 2 }, { 7, 5 }}, -1, 7 },
    CoreDefinition { Quality::sus4, "sus4", {{ 0, 1 }, { 5, 4 }, { 7, 5 }}, -1, 7 },
    CoreDefinition { Quality::power, "5",   {{ 0, 1 }, { 7, 5 }}, -1, 7 }
};

struct DetailedCandidate
{
    ChordCandidate publicCandidate;
    Quality quality = Quality::major;
    std::string symbol;
    std::map<int, ToneSpec> toneSpecs;
    bool dominantSeventh = false;
    bool majorSeventh = false;
    bool altered = false;
    bool exact = true;
};

bool usesSharpNames(const KeyContext key)
{
    const auto keyName = keyDisplayName(key);
    return keyName.find('#') != std::string::npos;
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

int accidentalValue(const std::string& name)
{
    int value = 0;
    for (const auto character : name)
    {
        if (character == '#') ++value;
        if (character == 'b') --value;
    }
    return value;
}

std::string accidentalText(int accidental)
{
    if (accidental > 0) return std::string(static_cast<size_t>(accidental), '#');
    if (accidental < 0) return std::string(static_cast<size_t>(-accidental), 'b');
    return {};
}

std::string spellTone(const int root, const std::string& rootName, const ToneSpec tone)
{
    const auto rootLetter = letterIndex(rootName.front());
    const auto targetLetter = (rootLetter + (tone.degree - 1) % 7) % 7;
    const auto targetPitch = normalise(root + tone.interval);
    const auto naturalPitch = letterPitchClasses[static_cast<size_t>(targetLetter)];

    auto accidental = normalise(targetPitch - naturalPitch);
    if (accidental > 6) accidental -= 12;

    return std::string(letters[static_cast<size_t>(targetLetter)]) + accidentalText(accidental);
}

std::string makeChordName(const int root, const std::string& symbol, const int bass,
                          const KeyContext key, const bool rootPresent)
{
    auto name = pitchClassName(root, key) + symbol;
    if (rootPresent && bass != root)
        name += "/" + pitchClassName(bass, key);
    return name;
}

std::string romanSuffix(const DetailedCandidate& candidate)
{
    auto suffix = candidate.symbol;

    if (candidate.quality == Quality::minor && suffix.starts_with("m"))
        suffix.erase(0, 1);
    else if (candidate.quality == Quality::diminished)
    {
        if (suffix.starts_with("m7b5"))
            return "ø7" + suffix.substr(4);
        if (suffix.starts_with("dim"))
            return "°" + suffix.substr(3);
    }
    else if (candidate.quality == Quality::augmented && suffix.starts_with("aug"))
        suffix.replace(0, 3, "+");

    return suffix;
}

bool isMinorLike(const Quality quality)
{
    return quality == Quality::minor || quality == Quality::diminished;
}

std::string numeralForDegree(const int degree, const Quality quality)
{
    static constexpr std::array<const char*, 7> uppercase { "I", "II", "III", "IV", "V", "VI", "VII" };
    auto numeral = std::string(uppercase[static_cast<size_t>(degree - 1)]);
    if (isMinorLike(quality))
        std::transform(numeral.begin(), numeral.end(), numeral.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return numeral;
}

int diatonicDegree(const int root, const KeyContext key)
{
    const auto difference = normalise(root - key.tonicPitchClass);
    const std::array<int, 7> scale = key.mode == KeyMode::major
        ? std::array<int, 7> { 0, 2, 4, 5, 7, 9, 11 }
        : std::array<int, 7> { 0, 2, 3, 5, 7, 8, 10 };

    for (int index = 0; index < static_cast<int>(scale.size()); ++index)
        if (scale[static_cast<size_t>(index)] == difference)
            return index + 1;
    return 0;
}

std::string chromaticNumeral(const int difference, const Quality quality)
{
    static constexpr std::array<const char*, 12> names {
        "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII"
    };
    auto numeral = std::string(names[static_cast<size_t>(normalise(difference))]);
    if (isMinorLike(quality))
        std::transform(numeral.begin(), numeral.end(), numeral.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return numeral;
}

std::pair<std::string, std::string> harmonicAnalysis(const DetailedCandidate& candidate,
                                                     const KeyContext key)
{
    const auto degree = diatonicDegree(candidate.publicCandidate.rootPitchClass, key);
    const auto suffix = romanSuffix(candidate);
    const auto difference = normalise(candidate.publicCandidate.rootPitchClass - key.tonicPitchClass);

    if (candidate.dominantSeventh)
    {
        const auto targetPitch = normalise(candidate.publicCandidate.rootPitchClass - 7);
        const auto targetDegree = diatonicDegree(targetPitch, key);
        if (targetDegree > 0 && targetPitch != key.tonicPitchClass)
        {
            auto target = numeralForDegree(targetDegree, Quality::major);
            return { "V/" + target + suffix,
                     candidate.altered ? "Secondary Dominant (Altered)" : "Secondary Dominant" };
        }
    }

    if (candidate.quality == Quality::diminished)
    {
        const auto targetPitch = normalise(candidate.publicCandidate.rootPitchClass + 1);
        const auto targetDegree = diatonicDegree(targetPitch, key);
        if (targetDegree > 0 && targetPitch != key.tonicPitchClass)
            return { "vii°/" + numeralForDegree(targetDegree, Quality::major), "Leading-tone dominant" };
    }

    if (degree > 0)
    {
        auto function = std::string("Tonic");
        if (degree == 2 || degree == 4) function = "Predominant";
        if (degree == 5 || degree == 7) function = candidate.altered ? "Dominant (Altered)" : "Dominant";
        return { numeralForDegree(degree, candidate.quality) + suffix, function };
    }

    const auto borrowed = difference == 1 || difference == 3 || difference == 5
                       || difference == 8 || difference == 10;
    return { chromaticNumeral(difference, candidate.quality) + suffix,
             borrowed ? "Borrowed / Modal Interchange" : "Chromatic" };
}

std::optional<DetailedCandidate> buildCandidate(const int root, const std::set<int>& pitchClasses,
                                                const int bass, const KeyContext key,
                                                const bool rootPresent, const bool inferred)
{
    const auto relative = [&]()
    {
        std::set<int> result;
        for (const auto pitchClass : pitchClasses)
            result.insert(normalise(pitchClass - root));
        return result;
    }();

    for (const auto& core : coreDefinitions)
    {
        if (rootPresent && !contains(relative, 0))
            continue;

        auto missingFifth = false;
        auto coreValid = true;
        for (const auto tone : core.tones)
        {
            if (tone.interval == 0 && !rootPresent)
                continue;
            if (contains(relative, tone.interval))
                continue;

            if (tone.interval == core.fifth)
            {
                missingFifth = true;
                continue;
            }

            coreValid = false;
            break;
        }

        if (!coreValid)
            continue;

        if (inferred && (core.third < 0 || core.quality == Quality::power))
            continue;

        auto remaining = relative;
        std::map<int, ToneSpec> toneSpecs;
        for (const auto tone : core.tones)
        {
            if (contains(remaining, tone.interval))
                remaining.erase(tone.interval);
            toneSpecs[tone.interval] = tone;
        }

        const auto hasMinorSeven = remaining.erase(10) > 0;
        const auto hasMajorSeven = remaining.erase(11) > 0;
        const auto hasSixthOrDimSeven = remaining.erase(9) > 0;

        if ((hasMinorSeven && hasMajorSeven) || (inferred && !hasMinorSeven && !hasMajorSeven && !(core.quality == Quality::diminished && hasSixthOrDimSeven)))
            continue;

        const auto hasNine = remaining.erase(2) > 0;
        const auto hasEleven = remaining.erase(5) > 0;
        const auto hasThirteen = hasSixthOrDimSeven
                              && !(core.quality == Quality::diminished && !hasMinorSeven && !hasMajorSeven);
        const auto hasFlatNine = remaining.erase(1) > 0;
        const auto hasSharpNine = remaining.erase(3) > 0;
        const auto hasSharpEleven = remaining.erase(6) > 0;
        const auto hasFlatThirteen = remaining.erase(8) > 0;

        if (!remaining.empty())
            continue;

        if (core.quality == Quality::diminished && hasSixthOrDimSeven && !hasMinorSeven && !hasMajorSeven)
        {
            toneSpecs[9] = { 9, 7 };
        }
        else if (hasSixthOrDimSeven && !hasMinorSeven && !hasMajorSeven && !hasNine && !hasEleven && !hasThirteen)
        {
            toneSpecs[9] = { 9, 6 };
        }
        else if (hasSixthOrDimSeven)
        {
            toneSpecs[9] = { 9, 6 };
        }

        if (hasMinorSeven) toneSpecs[10] = { 10, 7 };
        if (hasMajorSeven) toneSpecs[11] = { 11, 7 };
        if (hasNine) toneSpecs[2] = { 2, 9 };
        if (hasEleven) toneSpecs[5] = { 5, 11 };
        if (hasThirteen) toneSpecs[9] = { 9, 13 };
        if (hasFlatNine) toneSpecs[1] = { 1, 9 };
        if (hasSharpNine) toneSpecs[3] = { 3, 9 };
        if (hasSharpEleven) toneSpecs[6] = { 6, 11 };
        if (hasFlatThirteen) toneSpecs[8] = { 8, 13 };

        const auto hasAnySeven = hasMinorSeven || hasMajorSeven
                              || (core.quality == Quality::diminished && hasSixthOrDimSeven);
        std::string symbol;

        if (core.quality == Quality::power && hasMinorSeven)
            symbol = "7(no3)";
        else if (core.quality == Quality::power && hasMajorSeven)
            symbol = "maj7(no3)";
        else if (core.quality == Quality::diminished && hasSixthOrDimSeven && !hasMinorSeven && !hasMajorSeven)
            symbol = "dim7";
        else if (hasAnySeven)
        {
            const auto topExtension = hasThirteen ? 13 : hasEleven ? 11 : hasNine ? 9 : 0;
            if (core.quality == Quality::major && hasMinorSeven)
                symbol = topExtension == 0 ? "7" : std::to_string(topExtension);
            else if (core.quality == Quality::major && hasMajorSeven)
                symbol = topExtension == 0 ? "maj7" : "maj" + std::to_string(topExtension);
            else if (core.quality == Quality::minor && hasMinorSeven)
                symbol = topExtension == 0 ? "m7" : "m" + std::to_string(topExtension);
            else if (core.quality == Quality::diminished && hasMinorSeven)
                symbol = "m7b5";
            else
                symbol = core.suffix + (hasMajorSeven ? "maj7" : "7");
        }
        else if (hasSixthOrDimSeven && (core.quality == Quality::major || core.quality == Quality::minor))
        {
            symbol = core.suffix + "6";
        }
        else
        {
            symbol = core.suffix;
            std::vector<std::string> additions;
            if (hasNine) additions.push_back("add9");
            if (hasEleven) additions.push_back("add11");
            if (hasThirteen || (hasSixthOrDimSeven && core.quality != Quality::diminished)) additions.push_back("add13");
            if (!additions.empty())
            {
                symbol += additions.front();
                for (size_t index = 1; index < additions.size(); ++index)
                    symbol += "(" + additions[index] + ")";
            }
        }

        std::vector<std::string> modifiers;
        if (missingFifth) modifiers.push_back("no5");
        if (hasFlatNine) modifiers.push_back("b9");
        if (hasSharpNine && core.quality != Quality::minor && core.quality != Quality::diminished) modifiers.push_back("#9");
        if (hasSharpEleven && core.quality != Quality::diminished) modifiers.push_back("#11");
        if (hasFlatThirteen && core.quality != Quality::augmented) modifiers.push_back("b13");

        if (!modifiers.empty())
        {
            if (!symbol.empty() && symbol.back() == ')')
                symbol.pop_back(), symbol += "," + modifiers.front() + ")", modifiers.erase(modifiers.begin());
            if (!modifiers.empty())
            {
                symbol += "(";
                for (size_t index = 0; index < modifiers.size(); ++index)
                    symbol += (index == 0 ? "" : ",") + modifiers[index];
                symbol += ")";
            }
        }

        auto publicCandidate = ChordCandidate {};
        publicCandidate.rootPitchClass = root;
        publicCandidate.kind = inferred ? CandidateKind::rootless
                             : missingFifth || (core.quality == Quality::power && hasAnySeven) ? CandidateKind::omission
                             : CandidateKind::exact;
        publicCandidate.name = makeChordName(root, symbol, bass, key, rootPresent);
        if (inferred) publicCandidate.name += " (rootless)";
        publicCandidate.score = inferred ? 55 : 100;
        if (missingFifth) publicCandidate.score -= 12;
        if (core.quality == Quality::power && hasAnySeven) publicCandidate.score -= 8;

        const auto degree = diatonicDegree(root, key);
        if (degree > 0) publicCandidate.score += 4;
        if (root == key.tonicPitchClass) publicCandidate.score += 3;
        if (root == bass && rootPresent) publicCandidate.score += 2;

        return DetailedCandidate {
            std::move(publicCandidate), core.quality, std::move(symbol), std::move(toneSpecs),
            hasMinorSeven && (core.quality == Quality::major || core.quality == Quality::augmented
                               || core.quality == Quality::sus2 || core.quality == Quality::sus4
                               || core.quality == Quality::power), hasMajorSeven,
            hasFlatNine || hasSharpNine || hasSharpEleven || hasFlatThirteen,
            !inferred
        };
    }

    return std::nullopt;
}

std::vector<DetailedCandidate> buildPolychordCandidates(const std::set<int>& pitchClasses,
                                                        const int bass, const KeyContext key)
{
    std::vector<DetailedCandidate> candidates;
    if (pitchClasses.size() < 6)
        return candidates;

    const auto triads = std::array<CoreDefinition, 4> {
        coreDefinitions[0], coreDefinitions[1], coreDefinitions[2], coreDefinitions[3]
    };

    for (int lowerRoot = 0; lowerRoot < 12; ++lowerRoot)
    {
        for (const auto& lower : triads)
        {
            std::set<int> lowerNotes;
            for (const auto tone : lower.tones)
                lowerNotes.insert(normalise(lowerRoot + tone.interval));
            if (!std::includes(pitchClasses.begin(), pitchClasses.end(), lowerNotes.begin(), lowerNotes.end()))
                continue;

            std::set<int> upperNotes;
            std::set_difference(pitchClasses.begin(), pitchClasses.end(), lowerNotes.begin(), lowerNotes.end(),
                                std::inserter(upperNotes, upperNotes.begin()));
            if (upperNotes.size() != 3)
                continue;

            for (int upperRoot = 0; upperRoot < 12; ++upperRoot)
            {
                for (const auto& upper : triads)
                {
                    std::set<int> expected;
                    for (const auto tone : upper.tones)
                        expected.insert(normalise(upperRoot + tone.interval));
                    if (expected != upperNotes)
                        continue;

                    auto publicCandidate = ChordCandidate {};
                    publicCandidate.rootPitchClass = lowerRoot;
                    publicCandidate.kind = CandidateKind::polychord;
                    publicCandidate.score = 42 + (lowerRoot == bass ? 3 : 0);
                    publicCandidate.name = makeChordName(upperRoot, upper.suffix, upperRoot, key, false)
                                         + "/" + makeChordName(lowerRoot, lower.suffix, bass, key, false);
                    candidates.push_back({ std::move(publicCandidate), lower.quality, lower.suffix, {}, false, false, false, false });
                }
            }
        }
    }

    return candidates;
}

std::vector<DetailedCandidate> buildUpperStructureCandidates(const std::set<int>& pitchClasses,
                                                            const int bass, const KeyContext key)
{
    std::vector<DetailedCandidate> candidates;
    if (pitchClasses.size() < 7)
        return candidates;

    const auto triads = std::array<CoreDefinition, 4> {
        coreDefinitions[0], coreDefinitions[1], coreDefinitions[2], coreDefinitions[3]
    };
    struct LowerStructure { Quality quality; std::string suffix; std::array<int, 4> intervals; };
    const std::array<LowerStructure, 4> lowerStructures {
        LowerStructure { Quality::major, "7",    { 0, 4, 7, 10 } },
        LowerStructure { Quality::major, "maj7", { 0, 4, 7, 11 } },
        LowerStructure { Quality::minor, "m7",   { 0, 3, 7, 10 } },
        LowerStructure { Quality::diminished, "m7b5", { 0, 3, 6, 10 } }
    };

    for (int lowerRoot = 0; lowerRoot < 12; ++lowerRoot)
    {
        for (const auto& lower : lowerStructures)
        {
            std::set<int> lowerNotes;
            for (const auto interval : lower.intervals)
                lowerNotes.insert(normalise(lowerRoot + interval));
            if (!std::includes(pitchClasses.begin(), pitchClasses.end(), lowerNotes.begin(), lowerNotes.end()))
                continue;

            std::set<int> upperNotes;
            std::set_difference(pitchClasses.begin(), pitchClasses.end(), lowerNotes.begin(), lowerNotes.end(),
                                std::inserter(upperNotes, upperNotes.begin()));
            if (upperNotes.size() != 3)
                continue;

            for (int upperRoot = 0; upperRoot < 12; ++upperRoot)
            {
                for (const auto& upper : triads)
                {
                    std::set<int> expected;
                    for (const auto tone : upper.tones)
                        expected.insert(normalise(upperRoot + tone.interval));
                    if (expected != upperNotes)
                        continue;

                    auto publicCandidate = ChordCandidate {};
                    publicCandidate.rootPitchClass = lowerRoot;
                    publicCandidate.kind = CandidateKind::upperStructure;
                    publicCandidate.score = 48 + (lowerRoot == bass ? 3 : 0);
                    publicCandidate.name = makeChordName(upperRoot, upper.suffix, upperRoot, key, false)
                                         + "/" + makeChordName(lowerRoot, lower.suffix, bass, key, false)
                                         + " (upper structure)";
                    candidates.push_back({ std::move(publicCandidate), lower.quality, lower.suffix, {},
                                           lower.suffix == "7", lower.suffix == "maj7", false, false });
                }
            }
        }
    }

    return candidates;
}

std::vector<std::string> spellActiveNotes(const std::vector<int>& midiNotes, const DetailedCandidate* candidate,
                                          const KeyContext key)
{
    std::vector<std::string> names;
    names.reserve(midiNotes.size());

    for (const auto midiNote : midiNotes)
    {
        const auto pitchClass = normalise(midiNote);
        if (candidate != nullptr)
        {
            const auto interval = normalise(pitchClass - candidate->publicCandidate.rootPitchClass);
            if (const auto found = candidate->toneSpecs.find(interval); found != candidate->toneSpecs.end())
            {
                names.push_back(spellTone(candidate->publicCandidate.rootPitchClass,
                                          pitchClassName(candidate->publicCandidate.rootPitchClass, key),
                                          found->second));
                continue;
            }
        }
        names.push_back(pitchClassName(pitchClass, key));
    }

    return names;
}

} // namespace

std::string keyDisplayName(const KeyContext key)
{
    return key.mode == KeyMode::major
        ? majorKeys[static_cast<size_t>(normalise(key.tonicPitchClass))]
        : minorKeys[static_cast<size_t>(normalise(key.tonicPitchClass))];
}

std::string pitchClassName(const int pitchClass, const KeyContext key)
{
    const auto index = static_cast<size_t>(normalise(pitchClass));
    if (usesSharpNames(key))
        return sharpNames[index];
    return defaultNames[index];
}

AnalysisResult ChordAnalyzer::analyze(const std::vector<int>& midiNotes, const KeyContext key)
{
    AnalysisResult result;
    if (midiNotes.empty())
        return result;

    auto sortedNotes = midiNotes;
    std::sort(sortedNotes.begin(), sortedNotes.end());

    std::set<int> pitchClasses;
    for (const auto note : sortedNotes)
        pitchClasses.insert(normalise(note));

    const auto bass = normalise(sortedNotes.front());
    std::vector<DetailedCandidate> exactCandidates;
    std::vector<DetailedCandidate> inferredCandidates;

    for (int root = 0; root < 12; ++root)
    {
        if (contains(pitchClasses, root))
        {
            if (auto candidate = buildCandidate(root, pitchClasses, bass, key, true, false))
                exactCandidates.push_back(std::move(*candidate));
        }
        else if (auto candidate = buildCandidate(root, pitchClasses, bass, key, false, true))
        {
            inferredCandidates.push_back(std::move(*candidate));
        }
    }

    auto polychords = buildPolychordCandidates(pitchClasses, bass, key);
    inferredCandidates.insert(inferredCandidates.end(), std::make_move_iterator(polychords.begin()), std::make_move_iterator(polychords.end()));
    auto upperStructures = buildUpperStructureCandidates(pitchClasses, bass, key);
    inferredCandidates.insert(inferredCandidates.end(), std::make_move_iterator(upperStructures.begin()), std::make_move_iterator(upperStructures.end()));

    const auto sortCandidates = [](std::vector<DetailedCandidate>& candidates)
    {
        std::sort(candidates.begin(), candidates.end(), [](const auto& left, const auto& right)
        {
            return std::tie(left.publicCandidate.score, left.publicCandidate.name)
                 > std::tie(right.publicCandidate.score, right.publicCandidate.name);
        });
    };
    sortCandidates(exactCandidates);
    sortCandidates(inferredCandidates);

    // Strict representative policy: buildCandidate accepts a candidate only after every
    // played pitch class has been accounted for as a chord tone, tension, or explicit
    // omission. The highest-ranked non-inferred candidate is the concise main name.
    const DetailedCandidate* primary = exactCandidates.empty() ? nullptr : &exactCandidates.front();
    if (primary != nullptr)
    {
        result.chordName = primary->publicCandidate.name;
        std::tie(result.romanNumeral, result.harmonicFunction) = harmonicAnalysis(*primary, key);
    }
    else if (pitchClasses.size() == 1)
    {
        result.chordName = "Single note";
    }
    else
    {
        result.chordName = "Unrecognized set";
    }

    result.activeNoteNames = spellActiveNotes(sortedNotes, primary, key);

    // Equivalent strict readings retain a sounded root and account for every pitch
    // class. They are kept separate from the broader, inferred-root suggestions.
    std::set<std::string> strictSeen;
    if (primary != nullptr) strictSeen.insert(primary->publicCandidate.name);
    for (const auto& candidate : exactCandidates)
    {
        if (result.strictInterpretations.size() == 2)
            break;
        if (strictSeen.insert(candidate.publicCandidate.name).second)
            result.strictInterpretations.push_back(candidate.publicCandidate);
    }

    std::set<std::string> seen;
    if (primary != nullptr) seen.insert(primary->publicCandidate.name);
    const auto appendCandidates = [&](const std::vector<DetailedCandidate>& candidates)
    {
        for (const auto& candidate : candidates)
        {
            if (result.possibleInterpretations.size() == 2)
                break;
            if (seen.insert(candidate.publicCandidate.name).second)
                result.possibleInterpretations.push_back(candidate.publicCandidate);
        }
    };

    const auto appendKind = [&](const CandidateKind kind)
    {
        for (const auto& candidate : inferredCandidates)
            if (candidate.publicCandidate.kind == kind)
            {
                if (result.possibleInterpretations.size() < 2
                    && seen.insert(candidate.publicCandidate.name).second)
                    result.possibleInterpretations.push_back(candidate.publicCandidate);
                return;
            }
    };

    appendKind(CandidateKind::rootless);
    appendKind(CandidateKind::polychord);
    appendKind(CandidateKind::upperStructure);
    appendCandidates(inferredCandidates);
    appendCandidates(exactCandidates);

    return result;
}

} // namespace chord
