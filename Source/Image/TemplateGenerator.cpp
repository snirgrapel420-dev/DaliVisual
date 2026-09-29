#include "TemplateGenerator.h"
#include "../Core/Parameters.h"
#include "../Modulation/ModulationMatrix.h"

namespace dali
{
namespace
{
juce::StringArray allTemplateIds()
{
    juce::StringArray ids { params::id::tplEnable, params::id::tplMode, params::id::tplBlend, "tplMirror", "tplKaleido" };
    ids.addArray(params::templateParamIds());
    return ids;
}
}

juce::ValueTree TemplateGenerator::createTree(bool includeImage) const
{
    juce::ValueTree t(treeId);
    t.setProperty("version", 1, nullptr);
    juce::ValueTree p("Params");
    for (auto& id : allTemplateIds())
        if (auto* prm = apvts.getParameter(id))
            p.setProperty(id, prm->convertFrom0to1(prm->getValue()), nullptr);
    t.appendChild(p, nullptr);

    if (includeImage && image.hasImage())
    {
        const auto png = image.getEncodedPNG();
        juce::ValueTree img("Image");
        img.setProperty("name", image.getName(), nullptr);
        img.setProperty("png", juce::Base64::toBase64(png.getData(), png.getSize()), nullptr);
        t.appendChild(img, nullptr);
    }
    return t;
}

bool TemplateGenerator::applyTree(const juce::ValueTree& t)
{
    if (!t.hasType(treeId)) return false;
    const auto p = t.getChildWithName("Params");
    for (auto& id : allTemplateIds())
        if (auto* prm = apvts.getParameter(id))
            if (p.hasProperty(id))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost(prm->convertTo0to1(float(p.getProperty(id))));
                prm->endChangeGesture();
            }

    const auto img = t.getChildWithName("Image");
    if (img.isValid())
    {
        juce::MemoryOutputStream out;
        if (juce::Base64::convertFromBase64(out, img.getProperty("png").toString()))
            image.loadEncoded(out.getMemoryBlock(), img.getProperty("name", "Template image").toString());
    }
    return true;
}

bool TemplateGenerator::saveToFile(const juce::File& f) const
{
    if (auto xml = createTree(true).createXml())
        return xml->writeTo(f.withFileExtension(fileExtension));
    return false;
}

bool TemplateGenerator::loadFromFile(const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse(f))
        return applyTree(juce::ValueTree::fromXml(*xml));
    return false;
}

void TemplateGenerator::resetParameters()
{
    for (auto& id : allTemplateIds())
        if (id != params::id::tplEnable)
            if (auto* prm = apvts.getParameter(id))
                prm->setValueNotifyingHost(prm->getDefaultValue());
}

void TemplateGenerator::addReactiveRoutes(ModulationMatrix& m)
{
    auto route = [&](ModSource s, const char* id, float amt, float attack, float release)
    {
        const int t = ModulationTarget::fromParamId(id);
        if (t < 0) return;
        const int slot = m.addRoute(s, t, amt);
        if (slot < 0) return;
        auto sl = m.getSlot(slot);
        sl.attackMs = attack; sl.releaseMs = release;
        m.setSlot(slot, sl);
    };
    route(ModSource::Bass,      "tplScale",    0.12f, 5.0f, 180.0f);
    route(ModSource::Kick,      "tplSymCount", 0.10f, 0.0f, 120.0f);
    route(ModSource::Mid,       "tplWarp",     0.30f, 20.0f, 200.0f);
    route(ModSource::High,      "tplDetail",   0.25f, 5.0f, 90.0f);
    route(ModSource::Transient, "tplFeedback", 0.25f, 0.0f, 250.0f);
    route(ModSource::SyncLFO,   "tplRotation", 0.06f, 0.0f, 0.0f);
}
} // namespace dali
