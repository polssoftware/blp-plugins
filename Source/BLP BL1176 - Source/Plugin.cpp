#include "BlpEditorTheme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

namespace {
using namespace juce;
using State = AudioProcessorValueTreeState;
const StringArray ratios { "4:1", "8:1", "12:1", "20:1", "All" };

State::ParameterLayout parameters()
{
    State::ParameterLayout p;
    auto knob = [&p](const char* id, const char* name, float lo, float hi, float initial, bool db)
    {
        p.add(std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name,
            NormalisableRange<float> { lo, hi, 0.1f }, initial,
            AudioParameterFloatAttributes().withLabel(db ? "dB" : "")
                .withStringFromValueFunction([](float v, int) { return String(v, 1); })));
    };
    knob("input", "Input", 0, 20, 10, true);
    knob("output", "Output", -15, 15, 0, true);
    knob("attack", "Attack", 0, 10, 1.2f, false);
    knob("release", "Release", 0, 10, 5, false);
    p.add(std::make_unique<AudioParameterChoice>(ParameterID { "ratio", 1 }, "Ratio", ratios, 2));
    p.add(std::make_unique<AudioParameterBool>(ParameterID { "bypass", 1 }, "Bypass", false));
    return p;
}

constexpr std::array ids { "input", "output", "attack", "release", "ratio", "bypass" };
struct Preset { const char* name; std::array<float, 6> values; };
constexpr Preset presets[] {
    { "Default", { 10, 0, 1.2f, 5, 2, 0 } },
    { "Vocal Control", { 8, 1, 2, 4, 0, 0 } },
    { "Rap Forward", { 13, 2, 4, 7, 1, 0 } },
    { "Peak Tamer", { 7, 0, 9, 5, 3, 0 } },
    { "All In", { 15, 4, 6, 8, 4, 0 } }
};

class Processor final : public AudioProcessor
{
public:
    Processor() : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo(), true)
                                    .withOutput("Output", AudioChannelSet::stereo(), true)),
                  state(*this, nullptr, "BLP1176", parameters())
    {
        for (size_t i = 0; i < ids.size(); ++i)
            raw[i] = state.getRawParameterValue(ids[i]);
    }

    const String getName() const override { return "BLP BL1176"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.25; }
    bool isBusesLayoutSupported(const BusesLayout& layout) const override
    {
        const auto out = layout.getMainOutputChannelSet();
        return out == layout.getMainInputChannelSet()
            && (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo());
    }

    int getNumPrograms() override { return static_cast<int>(std::size(presets)); }
    int getCurrentProgram() override { return program.load(); }
    const String getProgramName(int i) override { return isPositiveAndBelow(i, getNumPrograms()) ? presets[i].name : ""; }
    void changeProgramName(int, const String&) override {}
    void setCurrentProgram(int i) override
    {
        if (!isPositiveAndBelow(i, getNumPrograms()))
            return;
        program.store(i);
        for (size_t n = 0; n < ids.size(); ++n)
        {
            auto* param = state.getParameter(ids[n]);
            param->setValueNotifyingHost(param->convertTo0to1(presets[i].values[n]));
        }
    }

    AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    void getStateInformation(MemoryBlock& data) override
    {
        auto snapshot = state.copyState();
        snapshot.setProperty("program", program.load(), nullptr);
        if (auto xml = snapshot.createXml())
            copyXmlToBinary(*xml, data);
    }
    void setStateInformation(const void* data, int size) override
    {
        auto xml = getXmlFromBinary(data, size);
        if (xml && xml->hasTagName(state.state.getType()))
        {
            auto snapshot = ValueTree::fromXml(*xml);
            program.store(jlimit(0, getNumPrograms() - 1, static_cast<int>(snapshot.getProperty("program", 0))));
            state.replaceState(snapshot);
        }
    }

    void prepareToPlay(double rate, int) override
    {
        sampleRate = std::isfinite(rate) && rate >= 8000 ? rate : 44100;
        reduction = 0;
        colourState = {};
        const auto kh = std::tan(MathConstants<double>::pi * jmin(6.97116726008586, sampleRate * 0.45) / sampleRate);
        const auto kl = std::tan(MathConstants<double>::pi * jmin(13685.11931106457, sampleRate * 0.45) / sampleRate);
        hpB = 1.0 / (1.0 + kh); hpA = (kh - 1.0) / (kh + 1.0);
        lpB = kl / (1.0 + kl); lpA = (kl - 1.0) / (kl + 1.0);
        const auto values = targets(false);
        for (size_t i = 0; i < smooth.size(); ++i)
        {
            smooth[i].reset(sampleRate, 0.005);
            smooth[i].setCurrentAndTargetValue(values[i]);
        }
    }
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& audio, MidiBuffer&) override { process(audio, false); }
    void processBlockBypassed(AudioBuffer<float>& audio, MidiBuffer&) override { process(audio, true); }
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
    State state;

private:
    std::array<float, 8> targets(bool bypass) const
    {
        // Independent fitted endpoints from five default-setting WAV renders.
        // These coefficients are provisional outside the measured control settings.
        constexpr double baseAttack[] { 5.330442784078851e-05, 4.031027143211599e-05, 1.720194646725556e-05, 1.40872640849921e-05, 1.000013420222141e-05 };
        constexpr double baseRelease[] { 0.2055064684842672, 0.2100429862866149, 0.2199693789250136, 0.2182936061616323, 0.230261067078304 };
        constexpr double thresholds[] { -8.541374910645118, -8.158610866640711, -7.51594512216405, -6.969759159682843, -6.859030111056949 };
        constexpr double slopes[] { 0.8556670297904844, 0.8918012324646883, 0.9493099708445254, 0.9545949639680891, 0.9769617136316602 };
        constexpr double knees[] { 0.02604494703323767, 0.130074243628927, 0.05815043072281258, 0.411515438690739, 0.03308126052662265 };
        constexpr double makeup[] { 0.2250982121194411, 0.3042885038041554, 0.5353411445307863, 0.4555727904727025, 0.8441722769018494 };
        const auto ratio = jlimit(0, 4, roundToInt(raw[4]->load()));
        const auto attackSeconds = baseAttack[ratio] * std::pow(0.025, (raw[2]->load() - 1.2) / 10.0);
        const auto releaseSeconds = baseRelease[ratio] * std::pow(0.05 / 1.1, (raw[3]->load() - 5.0) / 10.0);
        return { Decibels::decibelsToGain(raw[0]->load()),
                 Decibels::decibelsToGain(raw[1]->load() + static_cast<float>(makeup[ratio])),
                 static_cast<float>(-std::expm1(-1.0 / (sampleRate * attackSeconds))),
                 static_cast<float>(-std::expm1(-1.0 / (sampleRate * releaseSeconds))),
                 static_cast<float>(slopes[ratio]), static_cast<float>(thresholds[ratio]),
                 bypass || raw[5]->load() >= 0.5f ? 0.0f : 1.0f,
                 static_cast<float>(knees[ratio]) };
    }

    void process(AudioBuffer<float>& audio, bool bypass)
    {
        ScopedNoDenormals noDenormals;
        const auto values = targets(bypass);
        for (size_t i = 0; i < smooth.size(); ++i)
            smooth[i].setTargetValue(values[i]);

        auto channels = audio.getArrayOfWritePointers();
        const auto count = audio.getNumChannels();
        for (int i = 0; i < audio.getNumSamples(); ++i)
        {
            const auto input = smooth[0].getNextValue();
            const auto output = smooth[1].getNextValue();
            const auto attack = smooth[2].getNextValue();
            const auto release = smooth[3].getNextValue();
            const auto slope = smooth[4].getNextValue();
            const auto threshold = smooth[5].getNextValue();
            const auto mix = smooth[6].getNextValue();

            float peak = 0;
            for (int c = 0; c < count; ++c)
                peak = jmax(peak, std::abs(channels[c][i]));

            const auto over = Decibels::gainToDecibels(peak * input, -120.0f) - threshold;
            const auto width = smooth[7].getNextValue();
            const auto knee = jlimit(0.0f, width, over + width * 0.5f);
            const auto target = slope * (knee * knee / (2.0f * width) + jmax(0.0f, over - width * 0.5f));
            reduction += (target - reduction) * (target > reduction ? attack : release);
            const auto wetGain = input * output * Decibels::decibelsToGain(-reduction, -160.0f);
            for (int c = 0; c < count; ++c)
            {
                const auto dry = channels[c][i];
                auto& colour = colourState[static_cast<size_t>(c)];
                const double driven = dry * wetGain;
                const double high = hpB * driven + colour.hp;
                colour.hp = -hpB * driven - hpA * high;
                const double low = lpB * high + colour.lp;
                colour.lp = lpB * high - lpA * low;
                channels[c][i] = dry + mix * (static_cast<float>(low) - dry);
            }
        }
    }

    std::array<std::atomic<float>*, 6> raw {};
    std::array<SmoothedValue<float>, 8> smooth;
    std::atomic<int> program { 0 };
    double sampleRate = 44100;
    float reduction = 0;
    struct ColourState { double hp = 0, lp = 0; };
    std::array<ColourState, 2> colourState {};
    double hpB = 1, hpA = 0, lpB = 1, lpA = 0;
};

class KnobStyle final : public blp::EditorLookAndFeel
{
public:
    void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float position,
                          float start, float end, Slider&) override
    {
        const auto radius = 23.0f;
        const auto centre = Rectangle<int>(x, y, w, h).toFloat().getCentre();
        const auto angle = jmap(position, start, end);
        auto arc = [&](float finish, Colour colour)
        {
            if (finish <= start)
                return;
            Path p;
            p.addCentredArc(centre.x, centre.y, radius + 5, radius + 5, 0, start, finish, true);
            g.setColour(colour);
            g.strokePath(p, PathStrokeType(2.5f, PathStrokeType::curved, PathStrokeType::rounded));
        };
        arc(end, Colour(0xff47474f));
        arc(angle, Colour(0xffee1c25));
        g.setColour(Colour(0xfff6f5f7));
        g.fillEllipse(Rectangle<float>(radius * 2, radius * 2).withCentre(centre));
        Path mark;
        mark.addRoundedRectangle(-1.4f, -radius + 5, 2.8f, 11, 1.3f);
        g.setColour(Colour(0xff323238));
        g.fillPath(mark, AffineTransform::rotation(angle).translated(centre.x, centre.y));
    }
};

class Editor final : public blp::ThemedEditor
{
public:
    explicit Editor(Processor& p) : blp::ThemedEditor(p)
    {
        setLookAndFeel(&style);
        constexpr std::array names { "Input", "Output", "Attack", "Release" };
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            auto& knob = knobs[i];
            knob.setName(names[i]);
            knob.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(Slider::TextBoxBelow, false, 70, 19);
            knob.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            knob.setColour(Slider::textBoxTextColourId, blp::textWhite);
            addAndMakeVisible(knob);
            blp::showValueOnHover(knob, *this);
            attachments[i] = std::make_unique<State::SliderAttachment>(p.state, ids[i], knob);
            labels[i].setText(names[i], dontSendNotification);
            labels[i].setJustificationType(Justification::centred);
            labels[i].setColour(Label::textColourId, blp::textWhite);
            addAndMakeVisible(labels[i]);
        }
        ratio.addItemList(ratios, 1);
        ratio.setName("Ratio");
        addAndMakeVisible(ratio);
        addAndMakeVisible(bypass);
        ratioAttachment = std::make_unique<State::ComboBoxAttachment>(p.state, "ratio", ratio);
        bypassAttachment = std::make_unique<State::ButtonAttachment>(p.state, "bypass", bypass);
        setSize(430, 270);
    }

    ~Editor() override { setLookAndFeel(nullptr); }
    void paint(Graphics& g) override
    {
        g.fillAll(blp::canvas);
        g.setColour(Colour(0xfff6f5f7));
        g.setFont(FontOptions(18.0f));
        
        g.setColour(Colour(0xffee1c25));
        g.setFont(FontOptions(12.0f));
        blp::paintHeader(g, getWidth(), "BLP BL1176");
}

    void resized() override
    {
        bypass.setBounds(blp::bypassBounds(getWidth()));
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            const auto x = 21 + 100 * static_cast<int>(i);
            knobs[i].setBounds(x, 65, 88, 87);
            labels[i].setBounds(x, 135, 88, 19);
        }
        ratio.setBounds(127, 210, 176, 34);
    }

private:
    KnobStyle style;
    std::array<Slider, 4> knobs;
    std::array<Label, 4> labels;
    blp::MenuBox ratio { "Ratio" };
    blp::BypassButton bypass;
    std::array<std::unique_ptr<State::SliderAttachment>, 4> attachments;
    std::unique_ptr<State::ComboBoxAttachment> ratioAttachment;
    std::unique_ptr<State::ButtonAttachment> bypassAttachment;
};

AudioProcessorEditor* Processor::createEditor() { return new Editor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Processor; }
