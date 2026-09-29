#include "OutputManager.h"
#include "../UI/VisualView.h"

namespace dali
{
class OutputManager::OutputWindow : public juce::Component
{
public:
    OutputWindow(EngineState& s, const juce::Rectangle<int>& area, bool useKiosk, std::function<void()> onClose)
        : view(s, RenderEngine::Role::Output), closeCallback(std::move(onClose)), kiosk(useKiosk)
    {
        setOpaque(true);
        addAndMakeVisible(view);
        view.setInterceptsMouseClicks(false, false);
        setMouseCursor(juce::MouseCursor::NoCursor);
        setWantsKeyboardFocus(true);
        setBounds(area);
        addToDesktop(0);                     // no title bar, no border
        setAlwaysOnTop(true);
        setVisible(true);
        if (kiosk) juce::Desktop::getInstance().setKioskModeComponent(this, false);
        setBounds(area);
        toFront(true);
        grabKeyboardFocus();
    }

    ~OutputWindow() override
    {
        if (kiosk && juce::Desktop::getInstance().getKioskModeComponent() == this)
            juce::Desktop::getInstance().setKioskModeComponent(nullptr, false);
    }

    void resized() override { view.setBounds(getLocalBounds()); }
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }

    bool keyPressed(const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey) { requestClose(); return true; }
        return false;
    }
    void mouseDoubleClick(const juce::MouseEvent&) override { requestClose(); }

private:
    void requestClose()
    {
        auto cb = closeCallback;
        juce::MessageManager::callAsync([cb] { if (cb) cb(); });   // never delete ourselves inside our own callback
    }

    VisualView view;
    std::function<void()> closeCallback;
    bool kiosk;
};

OutputManager::OutputManager(EngineState& s) : state(s) {}

OutputManager::~OutputManager()
{
    window.reset();
    state.telemetry.outputActive = false;
}

juce::Array<OutputManager::DisplayInfo> OutputManager::getDisplays()
{
    juce::Array<DisplayInfo> result;
    const auto& displays = juce::Desktop::getInstance().getDisplays().displays;
    for (int i = 0; i < displays.size(); ++i)
    {
        const auto& d = displays.getReference(i);
        const auto r = d.totalArea;
        result.add({ i, "Display " + juce::String(i + 1) + (d.isMain ? " (main)" : "") + "  "
                        + juce::String(juce::roundToInt(r.getWidth() * d.scale)) + " x "
                        + juce::String(juce::roundToInt(r.getHeight() * d.scale)),
                     r, d.isMain });
    }
    return result;
}

void OutputManager::open(int displayIndex)
{
    const auto displays = getDisplays();
    if (displays.isEmpty()) return;
    if (displayIndex < 0) displayIndex = state.output.displayIndex.load();
    if (displayIndex < 0 || displayIndex >= displays.size()) displayIndex = displays.size() - 1;
    state.output.displayIndex = displayIndex;

    window.reset();
    const auto& d = displays.getReference(displayIndex);
    // The main display needs kiosk mode to cover the menu bar / task bar.
    window = std::make_unique<OutputWindow>(state, d.area, d.isMain,
                                            [this] { close(); });
    state.telemetry.outputActive = true;
    sendChangeMessage();
}

void OutputManager::close()
{
    if (window == nullptr) return;
    window.reset();
    state.telemetry.outputActive = false;
    sendChangeMessage();
}

void OutputManager::addFrameSink(FrameSink* s)
{
    const juce::SpinLock::ScopedLockType sl(state.sinkLock);
    state.sinks.addIfNotAlreadyThere(s);
}

void OutputManager::removeFrameSink(FrameSink* s)
{
    const juce::SpinLock::ScopedLockType sl(state.sinkLock);
    state.sinks.removeFirstMatchingValue(s);
}
} // namespace dali
