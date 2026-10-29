#pragma once
#include <JuceHeader.h>

namespace Necro
{
    enum P { W1, W2, SEMI2, FINE2, MIX2, SUB, NOISE, POSS, SCREAM, CUT, RESO, ENVAMT,
             ATK, DEC, SUS, REL, LFORATE, LFOFILT, LFOPITCH,
             DTIME, DFB, DMIX, RSIZE, RMIX, TONE, MASTER, NUM };

    struct ParamDef { const char* id; const char* name; float mn, mx, def, centre; };
    extern const ParamDef defs[NUM];

    struct Params
    {
        std::atomic<float>* p[NUM] {};
        float get (int i) const noexcept { return p[i]->load(); }
    };

    struct SpookySound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override    { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    inline float polyBlep (float t, float dt)
    {
        if (t < dt)         { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt)  { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    inline float oscSample (int wave, float ph, float dt)
    {
        switch (wave)
        {
            case 0: return (2.0f * ph - 1.0f) - polyBlep (ph, dt);
            case 1: { float s = ph < 0.5f ? 1.0f : -1.0f;
                      s += polyBlep (ph, dt);
                      float p2 = ph + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
                      s -= polyBlep (p2, dt); return s; }
            case 2: return std::sin (juce::MathConstants<float>::twoPi * ph);
            default: return 4.0f * std::abs (ph - 0.5f) - 1.0f;
        }
    }

    class SpookyVoice : public juce::SynthesiserVoice
    {
    public:
        explicit SpookyVoice (const Params& pr) : P (pr) {}

        bool canPlaySound (juce::SynthesiserSound* s) override
        { return dynamic_cast<SpookySound*> (s) != nullptr; }

        void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override
        {
            freq  = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
            level = 0.25f + 0.75f * velocity;
            bend  = (pitchWheel - 8192) / 8192.0f * 2.0f;
            ic1 = ic2 = 0.0f;
            ph1 = ph2 = phSub = 0.0f;

            juce::ADSR::Parameters ap { juce::jmax (0.003f, P.get (ATK)), P.get (DEC), P.get (SUS), juce::jmax (0.02f, P.get (REL)) };
            ampEnv.setSampleRate (getSampleRate()); ampEnv.setParameters (ap); ampEnv.noteOn();
            fEnv.setSampleRate (getSampleRate());   fEnv.setParameters (ap);   fEnv.noteOn();
        }

        void stopNote (float, bool allowTailOff) override
        {
            if (allowTailOff) { ampEnv.noteOff(); fEnv.noteOff(); }
            else              { ampEnv.reset(); fEnv.reset(); clearCurrentNote(); }
        }

        void pitchWheelMoved (int v) override { bend = (v - 8192) / 8192.0f * 2.0f; }
        void controllerMoved (int, int) override {}

        void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
        {
            if (! isVoiceActive()) return;

            const float sr = (float) getSampleRate();
            const int w1 = (int) P.get (W1), w2 = (int) P.get (W2);
            const float semi2 = P.get (SEMI2), fine2 = P.get (FINE2), mix2 = P.get (MIX2);
            const float sub = P.get (SUB), noise = P.get (NOISE), poss = P.get (POSS), scream = P.get (SCREAM);
            const float cutoff = P.get (CUT), reso = P.get (RESO), envAmt = P.get (ENVAMT);
            const float lfoRate = P.get (LFORATE), lfoFilt = P.get (LFOFILT), lfoPitch = P.get (LFOPITCH);
            const float k = 1.0f / (0.6f + reso * 10.0f);      // softer resonance than v1
            const float twoPi = juce::MathConstants<float>::twoPi;

            for (int i = 0; i < num; ++i)
            {
                if (! ampEnv.isActive()) { clearCurrentNote(); break; }

                const float lfo = std::sin (twoPi * lfoPh);
                lfoPh += lfoRate / sr; if (lfoPh >= 1.0f) lfoPh -= 1.0f;

                const float pm = bend + lfo * lfoPitch * 2.0f;
                const float f1 = freq * std::exp2 (pm / 12.0f);
                const float f2 = f1 * std::exp2 ((semi2 + fine2 / 100.0f) / 12.0f);
                const float d1 = f1 / sr, d2 = f2 / sr;

                const float o1 = oscSample (w1, ph1, d1);
                const float o2 = oscSample (w2, ph2, d2);
                ph1 += d1; if (ph1 >= 1.0f) ph1 -= 1.0f;
                ph2 += d2; if (ph2 >= 1.0f) ph2 -= 1.0f;
                phSub += 0.5f * d1; if (phSub >= 1.0f) phSub -= 1.0f;

                float s = o1 + o2 * mix2;
                s += sub * std::sin (twoPi * phSub);
                s += noise * (rng.nextFloat() * 2.0f - 1.0f) * 0.4f;

                // POSSESS: ring mod
                s = s * (1.0f - poss) + (o1 * o2) * poss * 1.2f;
                s *= 0.5f;

                // SCREAM: gentler fold + saturation (less gritty than v1)
                s = std::tanh (std::sin (s * (1.0f + scream * 4.0f)) * (1.0f + scream));

                const float eF  = fEnv.getNextSample();
                const float mod = envAmt * eF * 4.0f + lfo * lfoFilt * 2.5f;
                const float fc  = juce::jlimit (20.0f, juce::jmin (16000.0f, sr * 0.45f), cutoff * std::exp2 (mod));
                const float g   = std::tan (juce::MathConstants<float>::pi * fc / sr);
                const float a1  = 1.0f / (1.0f + g * (g + k));
                const float a2  = g * a1, a3 = g * a2;
                const float v3  = s - ic2;
                const float v1  = a1 * ic1 + a2 * v3;
                const float v2  = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1;
                ic2 = 2.0f * v2 - ic2;

                const float y = v2 * ampEnv.getNextSample() * level;
                for (int ch = 0; ch < out.getNumChannels(); ++ch)
                    out.addSample (ch, start + i, y);
            }
        }

    private:
        const Params& P;
        juce::ADSR ampEnv, fEnv;
        juce::Random rng;
        float freq = 440.0f, level = 1.0f, bend = 0.0f;
        float ph1 = 0, ph2 = 0, phSub = 0, lfoPh = 0, ic1 = 0, ic2 = 0;
    };
}

// ----------------------------------------------------------------------
class NecroAudioProcessor : public juce::AudioProcessor
{
public:
    NecroAudioProcessor();
    ~NecroAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Necrosynth"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return numPresets(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override { return presetName (index); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void randomizeAll();

    static int numPresets();
    static juce::String presetName (int i);
    static juce::String presetCategory (int i);
    static juce::StringArray categoryNames();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> outLevel { 0.0f };
    float scope[512] {};
    std::atomic<int> scopePos { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    Necro::Params params;
    juce::Synthesiser synth;
    juce::Reverb reverb;
    juce::AudioBuffer<float> delayBuf;
    int writePos = 0;
    float delaySmoothed = 0.0f;
    float z1[2] {}, z2[2] {};
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NecroAudioProcessor)
};
