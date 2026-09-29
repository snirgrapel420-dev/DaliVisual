#include "PresetManager.h"

namespace dali
{
bool Preset::writeTo(const juce::File& f) const
{
    juce::ValueTree root(fileRoot);
    root.setProperty("name", name, nullptr);
    root.setProperty("version", 1, nullptr);
    root.appendChild(state.createCopy(), nullptr);
    if (auto xml = root.createXml())
    {
        f.getParentDirectory().createDirectory();
        return xml->writeTo(f);
    }
    return false;
}

bool Preset::readFrom(const juce::File& f, Preset& out)
{
    auto xml = juce::XmlDocument::parse(f);
    if (xml == nullptr) return false;
    auto root = juce::ValueTree::fromXml(*xml);
    if (!root.hasType(fileRoot) || root.getNumChildren() == 0) return false;
    out.name = root.getProperty("name", f.getFileNameWithoutExtension()).toString();
    out.state = root.getChild(0).createCopy();
    return true;
}

PresetManager::PresetManager(Capture c, Apply a) : capture(std::move(c)), apply(std::move(a))
{
    installFactoryPresets(false);
    refresh();
}

juce::File PresetManager::getPresetFolder()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Dali Audio").getChildFile("Dali Visual").getChildFile("Presets");
}

juce::String PresetManager::sanitise(const juce::String& name)
{
    auto s = juce::File::createLegalFileName(name.trim());
    return s.isEmpty() ? juce::String("Untitled") : s;
}

void PresetManager::refresh()
{
    names.clear();
    auto files = getPresetFolder().findChildFiles(juce::File::findFiles, false, juce::String("*") + Preset::extension);
    files.sort();
    for (auto& f : files) names.add(f.getFileNameWithoutExtension());
    sendChangeMessage();
}

bool PresetManager::save(const juce::String& name)
{
    Preset p { sanitise(name), capture() };
    if (!p.writeTo(fileFor(p.name))) return false;
    current = p.name;
    refresh();
    return true;
}

bool PresetManager::load(const juce::String& name)
{
    Preset p;
    if (!Preset::readFrom(fileFor(name), p)) return false;
    apply(p.state);
    current = sanitise(name);
    sendChangeMessage();
    return true;
}

bool PresetManager::duplicate()
{
    const juce::String base = current.isNotEmpty() ? current : juce::String("Preset");
    juce::String n = base + " copy";
    for (int i = 2; fileFor(n).existsAsFile(); ++i) n = base + " copy " + juce::String(i);
    return save(n);
}

bool PresetManager::remove(const juce::String& name)
{
    const bool ok = fileFor(name).deleteFile();
    if (ok && name == current) current.clear();
    refresh();
    return ok;
}

void PresetManager::installFactoryPresets(bool overwrite)
{
    // Installed once per factory-set version, so presets the user deletes stay deleted.
    const auto marker = getPresetFolder().getChildFile(".factory_v1");
    if (!overwrite && marker.existsAsFile()) return;
    getPresetFolder().createDirectory();
    for (auto& p : createFactoryPresets())
    {
        auto f = fileFor(p.name);
        if (overwrite || !f.existsAsFile()) p.writeTo(f);
    }
    marker.replaceWithText("Dali Visual factory presets v1");
    refresh();
}
} // namespace dali
