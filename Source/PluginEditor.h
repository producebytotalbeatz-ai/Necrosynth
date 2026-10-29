#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct SpookyLAF : public juce::LookAndFeel_V4
{
    SpookyLAF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

struct Knob : public juce::Component
{
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& text);
    void resized() override;

    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class NecroEditor : public juce::AudioProcessorEditor,
                    private juce::Timer,
                    private juce::ListBoxModel
{
public:
    explicit NecroEditor (NecroAudioProcessor&);
    ~NecroEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // ListBoxModel
    int getNumRows() override { return (int) filtered.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    void rebuildFilter();
    void choosePreset (int globalIndex);
    void step (int dir);
    void drawPumpkin (juce::Graphics&, juce::Rectangle<float>);
    void drawBat (juce::Graphics&, float x, float y, float flap, float scale);
    void drawPanel (juce::Graphics&, juce::Rectangle<float>, const juce::String& title);

    NecroAudioProcessor& proc;
    SpookyLAF laf;

    juce::OwnedArray<Knob> knobs;
    juce::ComboBox wave1Box, wave2Box;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> w1Att, w2Att;

    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, randBtn { "RANDOMIZE" };
    juce::Label nameLabel;
    juce::OwnedArray<juce::TextButton> catBtns;
    juce::ListBox list { "presets", this };
    std::vector<int> filtered;
    juce::StringArray cats;
    int catFilter = -1;
    bool randomised = false;

    struct Panel { juce::String title; juce::Rectangle<int> r; };
    std::vector<Panel> panels;
    juce::Rectangle<int> browserRect, scopeRect;

    float level = 0.0f, t = 0.0f;
    juce::String lastName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NecroEditor)
};
