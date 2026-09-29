#pragma once
// ============================================================================
//  Preset / PresetManager — *.dvpreset files (XML) in
//     <user app data>/Dali Audio/Dali Visual/Presets
//  A preset stores: scene, every parameter (macros, audio reaction,
//  sensitivity, palette/colour, effects on/amount/P2, template params),
//  the modulation matrix, the effects order, the template image and the
//  output settings. MIDI mappings are global and not part of presets.
//  Factory presets are generated on first run (see FactoryPresets.cpp).
// ============================================================================
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <functional>

namespace dali
{
struct Preset
{
    juce::String name;
    juce::ValueTree state;                               // processor state tree (see captureState)
    static constexpr const char* extension = ".dvpreset";
    static inline const juce::Identifier fileRoot { "DaliVisualPreset" };

    bool writeTo(const juce::File& f) const;
    static bool readFrom(const juce::File& f, Preset& out);
};

class PresetManager : public juce::ChangeBroadcaster
{
public:
    using Capture = std::function<juce::ValueTree()>;
    using Apply   = std::function<void(const juce::ValueTree&)>;

    PresetManager(Capture capture, Apply apply);

    static juce::File getPresetFolder();

    juce::StringArray getNames() const { return names; }
    juce::String getCurrentName() const { return current; }
    int getCurrentIndex() const { return names.indexOf(current); }

    bool save(const juce::String& name);                 // create or overwrite
    bool load(const juce::String& name);
    bool loadIndex(int i) { return juce::isPositiveAndBelow(i, names.size()) && load(names[i]); }
    bool duplicate();                                    // "<name> copy"
    bool remove(const juce::String& name);
    void next()     { if (!names.isEmpty()) loadIndex((getCurrentIndex() + 1) % names.size()); }
    void previous() { if (!names.isEmpty()) loadIndex((getCurrentIndex() - 1 + names.size()) % names.size()); }
    void refresh();
    void setCurrentName(const juce::String& n) { current = n; sendChangeMessage(); }

    /** Writes factory presets that are missing from the folder. */
    void installFactoryPresets(bool overwrite = false);

    static juce::String sanitise(const juce::String& name);

private:
    juce::File fileFor(const juce::String& name) const { return getPresetFolder().getChildFile(sanitise(name) + Preset::extension); }
    Capture capture;
    Apply apply;
    juce::StringArray names;
    juce::String current;
};

/** Factory presets (state trees contain only overrides; missing values = defaults). */
std::vector<Preset> createFactoryPresets();
} // namespace dali
