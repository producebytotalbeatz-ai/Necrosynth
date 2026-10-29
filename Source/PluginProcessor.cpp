#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace Necro;

const ParamDef Necro::defs[NUM] =
{
    { "wave1",   "Osc1 Wave",  0, 3, 0, 0 },
    { "wave2",   "Osc2 Wave",  0, 3, 1, 0 },
    { "semi2",   "Semi",       -24, 24, 0, 0 },
    { "fine2",   "Rot",        -100, 100, 7, 0 },
    { "mix2",    "Osc2 Mix",   0, 1, 0.5f, 0 },
    { "sub",     "Sub",        0, 1, 0.5f, 0 },
    { "noise",   "Dust",       0, 1, 0, 0 },
    { "poss",    "Possess",    0, 1, 0, 0 },
    { "scream",  "Scream",     0, 1, 0.15f, 0 },
    { "cutoff",  "Cutoff",     20, 18000, 1200, 1200 },
    { "reso",    "Reso",       0, 1, 0.3f, 0 },
    { "envamt",  "Env Amt",    -1, 1, 0.4f, 0 },
    { "atk",     "Attack",     0.001f, 5, 0.005f, 0.2f },
    { "dec",     "Decay",      0.01f, 5, 0.4f, 0.5f },
    { "sus",     "Sustain",    0, 1, 0.6f, 0 },
    { "rel",     "Release",    0.01f, 8, 0.4f, 0.6f },
    { "lforate", "Rate",       0.05f, 20, 2, 2 },
    { "lfofilt", "To Filter",  0, 1, 0, 0 },
    { "lfopitch","To Pitch",   0, 1, 0, 0 },
    { "dtime",   "Echo Time",  0.02f, 1, 0.3f, 0 },
    { "dfb",     "Echo Fdbk",  0, 0.95f, 0.3f, 0 },
    { "dmix",    "Echo Mix",   0, 1, 0, 0 },
    { "rsize",   "Crypt Size", 0, 1, 0.5f, 0 },
    { "rmix",    "Crypt Mix",  0, 1, 0.15f, 0 },
    { "tone",    "Tone",       1000, 20000, 9000, 6000 },
    { "master",  "Master",     0, 1, 0.7f, 0 },
};

// ------------------------------------------------------------------ presets
static const char* kCats[11] = { "Bass", "Lead", "Synth", "Key", "Pluck", "String", "Organ", "Pad", "Bell", "Arp", "FX" };

static const char* kNames[11][11] =
{
    { "Crypt Keeper", "Hellhound Reese", "Trick or Trap 808", "Grave Digger Sub", "Zombie Growl", "Undertaker",
      "Coffin Nail", "Werewolf Howl", "Vampire Wobble", "Dark Ritual Sub", "Banshee Low" },
    { "Possessed Lead", "Screaming Pumpkin", "Witch Hunter", "Phantom Solo", "Black Cat Wail", "Ghoul Whistle",
      "Spectre Saw", "Nightmare Lead", "Haunted Square", "Wraith Cry", "Jack O Lantern" },
    { "Midnight Stab", "Slasher Stab", "Cursed Chord", "Terror Brass", "Bloodmoon Saw", "Hex Synth",
      "Dread Sync", "Coven Chords", "Mummy Pulse", "Dark Carnival", "Gothic Poly" },
    { "Haunted Piano", "Dracula Keys", "Cobweb EP", "Seance Keys", "Ghost Rhodes", "Abandoned Piano",
      "Moonlit Clav", "Spooky Toy Piano", "Crypt Harpsichord", "Mansion Keys", "Attic Keys" },
    { "Poltergeist Pluck", "Bone Pluck", "Skeleton Pluck", "Raven Pluck", "Cursed Harp", "Ghost Guitar",
      "Cauldron Drip", "Spider Pluck", "Thorn Pluck", "Mourning Pluck", "Dagger Pluck" },
    { "Ghost Strings", "Haunted Violins", "Phantom Orchestra", "Banshee Strings", "Dark Cello", "Graveyard Strings",
      "Widow Strings", "Moaning Strings", "Ravenwood Ensemble", "Eerie Tremolo", "Lullaby Strings" },
    { "Dracula Organ", "Cathedral Organ", "Funeral Organ", "Haunted Church", "Phantom Opera", "Carnival Organ",
      "Gargoyle Organ", "Requiem Organ", "Black Mass Organ", "Creepy Calliope", "Crypt Pipes" },
    { "Midnight Fog", "Ghost Choir", "Cursed Cathedral", "Graveyard Mist", "Haunted Manor Pad", "Soul Drift",
      "Moonless Night", "Spirit Wind", "Cold Breath", "Phantom Haze", "Asylum Pad" },
    { "Haunted Music Box", "Funeral Bell", "Ghost Chimes", "Cursed Glockenspiel", "Midnight Toll", "Crystal Skull",
      "Ice Spirit Bell", "Witching Hour", "Lullaby Box", "Graveyard Chime", "Ouija Bell" },
    { "Witch Cackle Arp", "Skeleton Dance", "Bat Swarm", "Spider Walk", "Haunted Sequence", "Zombie Shuffle",
      "Cauldron Bubble", "Trick Treat Arp", "Moth Flutter", "Nightstalker Arp", "Pumpkin Patch" },
    { "Zombie Rise", "Bone Rattle", "Poltergeist Riser", "Ghost Whisper", "Coffin Creak", "Nightmare Drone",
      "Graveyard Drone", "Banshee Scream", "Cursed Wind", "Demon Voice", "Jump Scare" },
};

// w1,w2,semi,fine,mix2,sub,noise,poss,scream,cut,reso,envAmt,A,D,S,R,lfoRate,lfoFilt,lfoPitch,dTime,dFb,dMix,rSize,rMix,tone,master
static const float kTmpl[11][NUM] =
{
    { 0,1,-12,8,  .5f,.7f,0,.05f,.2f,   500,.3f,.5f,    .003f,.35f,.6f,.25f,  .4f,0,0,     .25f,.2f,0,     .3f,.1f,  7000,.7f },
    { 0,1,7,8,    .6f,.1f,0,.2f,.25f,   2800,.3f,.4f,   .01f,.3f,.7f,.4f,     5.5f,0,.2f,  .375f,.35f,.2f, .4f,.2f,  8000,.6f },
    { 0,0,0,12,   .7f,.2f,0,.05f,.15f,  2500,.25f,.5f,  .01f,.4f,.5f,.5f,     .3f,.1f,0,   .375f,.3f,.15f, .5f,.2f,  8000,.6f },
    { 2,3,12,4,   .4f,.2f,0,.1f,0,      4000,.15f,.4f,  .003f,.8f,.15f,.6f,   5,0,.05f,    .33f,.3f,.2f,   .5f,.25f, 9000,.6f },
    { 0,1,5,6,    .5f,.2f,0,.1f,.1f,    1500,.3f,.7f,   .001f,.2f,0,.25f,     5,.1f,0,     .3f,.35f,.25f,  .4f,.2f,  8000,.65f },
    { 0,0,0,12,   .8f,.1f,.05f,0,0,     2500,.15f,.2f,  .6f,.5f,.85f,1.2f,    5,.1f,.12f,  .5f,.3f,.2f,   .7f,.4f,  7500,.55f },
    { 1,1,12,2,   .6f,.5f,0,0,.05f,     3000,.1f,0,     .01f,.1f,1,.2f,       6,0,.07f,    .4f,.2f,.1f,   .6f,.3f,  8500,.55f },
    { 3,0,-12,9,  .7f,.3f,.1f,0,0,      1400,.2f,.3f,   2.5f,1.2f,.8f,4,      .15f,.3f,.05f,.6f,.45f,.3f, .95f,.55f,7000,.55f },
    { 2,3,19,3,   .5f,0,0,.2f,0,        6000,.1f,.1f,   .001f,1.2f,0,2,       .3f,0,.1f,   .33f,.4f,.3f,  .7f,.4f,  10000,.6f },
    { 1,0,12,-6,  .5f,.1f,0,.1f,.1f,    1800,.35f,.6f,  .001f,.15f,0,.2f,     8,.1f,0,     .1875f,.4f,.25f,.4f,.2f, 8000,.65f },
    { 0,0,-12,18, .7f,.4f,.2f,.2f,.2f,  600,.4f,.6f,    2,1,.7f,2,            .3f,.4f,.1f, .5f,.4f,.2f,   .8f,.4f,  6000,.6f },
};

static float rr (juce::Random& r, float a, float b) { return a + (b - a) * r.nextFloat(); }
static float lim (float v, float a, float b)        { return juce::jlimit (a, b, v); }

static void vary (float* v, juce::Random r)
{
    if (r.nextFloat() < 0.35f) v[W1] = (float) r.nextInt (4);
    if (r.nextFloat() < 0.35f) v[W2] = (float) r.nextInt (4);
    if (r.nextFloat() < 0.5f) { static const float s[] = { -12, -7, 0, 5, 7, 12, 19 }; v[SEMI2] = s[r.nextInt (7)]; }
    v[FINE2]   = lim (v[FINE2] + rr (r, -8, 14), -100, 100);
    v[MIX2]    = lim (v[MIX2] + rr (r, -.2f, .2f), .1f, 1);
    v[SUB]     = lim (v[SUB] + rr (r, -.2f, .2f), 0, .9f);
    v[NOISE]   = lim (v[NOISE] + rr (r, 0, .08f), 0, .15f);
    v[POSS]    = lim (v[POSS] + rr (r, -.05f, .25f), 0, .5f);
    v[SCREAM]  = lim (v[SCREAM] + rr (r, -.05f, .25f), 0, .5f);
    v[CUT]     = lim (v[CUT] * std::exp2 (rr (r, -1, 1)), 120, 12000);
    v[RESO]    = lim (v[RESO] + rr (r, -.15f, .15f), 0, .65f);
    v[ENVAMT]  = lim (v[ENVAMT] + rr (r, -.3f, .3f), -.5f, 1);
    v[ATK]     = lim (v[ATK] * std::exp2 (rr (r, -1.2f, 1.2f)), .001f, 4);
    v[DEC]     = lim (v[DEC] * std::exp2 (rr (r, -1, 1)), .05f, 4);
    v[SUS]     = lim (v[SUS] + rr (r, -.2f, .2f), 0, 1);
    v[REL]     = lim (v[REL] * std::exp2 (rr (r, -1, 1)), .05f, 6);
    v[LFORATE] = lim (v[LFORATE] * std::exp2 (rr (r, -1, 1)), .1f, 16);
    v[LFOFILT] = lim (v[LFOFILT] + rr (r, -.1f, .3f), 0, .6f);
    v[LFOPITCH]= lim (v[LFOPITCH] + rr (r, -.05f, .15f), 0, .4f);
    { static const float dt[] = { .1875f, .25f, .33f, .375f, .5f }; v[DTIME] = dt[r.nextInt (5)]; }
    v[DFB]     = lim (v[DFB] + rr (r, -.15f, .15f), 0, .7f);
    v[DMIX]    = lim (v[DMIX] + rr (r, -.12f, .12f), 0, .45f);
    v[RSIZE]   = lim (v[RSIZE] + rr (r, -.2f, .2f), .1f, 1);
    v[RMIX]    = lim (v[RMIX] + rr (r, -.1f, .15f), 0, .55f);
    v[TONE]    = lim (v[TONE] + rr (r, -1500, 1500), 3000, 14000);
}

struct Preset { juce::String name, cat; float v[NUM] {}; };

static const std::vector<Preset>& presets()
{
    static const std::vector<Preset> list = []
    {
        std::vector<Preset> out;
        for (int c = 0; c < 11; ++c)
            for (int k = 0; k < 11; ++k)
            {
                Preset p; p.name = kNames[c][k]; p.cat = kCats[c];
                for (int j = 0; j < NUM; ++j) p.v[j] = kTmpl[c][j];
                if (k > 0) vary (p.v, juce::Random (1000 * c + 7 * k + 13));
                out.push_back (p);
            }
        return out;
    }();
    return list;
}

int NecroAudioProcessor::numPresets() { return (int) presets().size(); }
juce::String NecroAudioProcessor::presetName (int i)     { return juce::isPositiveAndBelow (i, numPresets()) ? presets()[(size_t) i].name : ""; }
juce::String NecroAudioProcessor::presetCategory (int i) { return juce::isPositiveAndBelow (i, numPresets()) ? presets()[(size_t) i].cat  : ""; }
juce::StringArray NecroAudioProcessor::categoryNames()   { juce::StringArray a; for (auto* c : kCats) a.add (c); return a; }

// ------------------------------------------------------------------ processor
juce::AudioProcessorValueTreeState::ParameterLayout NecroAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < NUM; ++i)
    {
        const auto& d = defs[i];
        if (i < 2)
            layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { d.id, 1 }, d.name,
                        juce::StringArray { "Saw", "Square", "Sine", "Tri" }, (int) d.def));
        else
        {
            juce::NormalisableRange<float> r (d.mn, d.mx);
            if (d.centre > 0.0f) r.setSkewForCentre (d.centre);
            layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { d.id, 1 }, d.name, r, d.def));
        }
    }
    return layout;
}

NecroAudioProcessor::NecroAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < NUM; ++i)
        params.p[i] = apvts.getRawParameterValue (defs[i].id);

    synth.addSound (new SpookySound());
    for (int i = 0; i < 8; ++i)
        synth.addVoice (new SpookyVoice (params));

    setCurrentProgram (0);
}

static void setParamValue (juce::AudioProcessorValueTreeState& apvts, int j, float value)
{
    if (auto* pr = apvts.getParameter (defs[j].id))
        pr->setValueNotifyingHost (pr->convertTo0to1 (value));
}

void NecroAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, numPresets())) return;
    currentProgram = index;
    for (int j = 0; j < NUM; ++j)
        setParamValue (apvts, j, presets()[(size_t) index].v[j]);
}

void NecroAudioProcessor::randomizeAll()
{
    juce::Random r;
    float v[NUM];
    for (int j = 0; j < NUM; ++j) v[j] = defs[j].def;

    static const float semis[] = { -12, -7, 0, 5, 7, 12, 19 };
    static const float dts[]   = { .1875f, .25f, .33f, .375f, .5f };

    v[W1] = (float) r.nextInt (4);            v[W2] = (float) r.nextInt (4);
    v[SEMI2] = semis[r.nextInt (7)];          v[FINE2] = rr (r, -20, 20);
    v[MIX2] = rr (r, .2f, 1);                 v[SUB] = rr (r, 0, .8f);
    v[NOISE] = rr (r, 0, .15f);               v[POSS] = rr (r, 0, .5f);
    v[SCREAM] = rr (r, 0, .5f);
    v[CUT] = 200.0f * std::exp2 (rr (r, 0, 5.3f));
    v[RESO] = rr (r, 0, .6f);                 v[ENVAMT] = rr (r, -.3f, 1);
    v[ATK] = .001f * std::exp2 (rr (r, 0, 11));
    v[DEC] = rr (r, .05f, 2);                 v[SUS] = rr (r, 0, 1);
    v[REL] = rr (r, .05f, 4);
    v[LFORATE] = .1f * std::exp2 (rr (r, 0, 7));
    v[LFOFILT] = rr (r, 0, .5f);              v[LFOPITCH] = rr (r, 0, .3f);
    v[DTIME] = dts[r.nextInt (5)];            v[DFB] = rr (r, 0, .6f);
    v[DMIX] = rr (r, 0, .4f);                 v[RSIZE] = rr (r, .2f, 1);
    v[RMIX] = rr (r, 0, .5f);                 v[TONE] = rr (r, 3000, 12000);
    v[MASTER] = 0.7f;

    for (int j = 0; j < NUM; ++j) setParamValue (apvts, j, v[j]);
}

void NecroAudioProcessor::prepareToPlay (double sr, int)
{
    synth.setCurrentPlaybackSampleRate (sr);
    reverb.setSampleRate (sr);
    reverb.reset();
    delayBuf.setSize (2, (int) (sr * 1.2) + 4);
    delayBuf.clear();
    writePos = 0;
    delaySmoothed = params.get (DTIME) * (float) sr;
    for (int c = 0; c < 2; ++c) z1[c] = z2[c] = 0.0f;
}

bool NecroAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NecroAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    synth.renderNextBlock (buffer, midi, 0, n);

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);

    // ping-pong echo
    const float sr = (float) getSampleRate();
    const float target = params.get (DTIME) * sr;
    const float fb = params.get (DFB), mix = params.get (DMIX);
    const int size = delayBuf.getNumSamples();
    auto* dL = delayBuf.getWritePointer (0);
    auto* dR = delayBuf.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        delaySmoothed += 0.0008f * (target - delaySmoothed);
        float rp = (float) writePos - delaySmoothed;
        if (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp, i1 = (i0 + 1) % size;
        const float fr = rp - (float) i0;
        const float yl = dL[i0] + fr * (dL[i1] - dL[i0]);
        const float yr = dR[i0] + fr * (dR[i1] - dR[i0]);

        dL[writePos] = L[i] + yr * fb;
        dR[writePos] = R[i] + yl * fb;
        L[i] += yl * mix;
        R[i] += yr * mix;
        writePos = (writePos + 1) % size;
    }

    // reverb
    juce::Reverb::Parameters rp;
    rp.roomSize = params.get (RSIZE);
    rp.damping = 0.5f;
    rp.wetLevel = params.get (RMIX) * 0.5f;
    rp.dryLevel = 1.0f;
    rp.width = 1.0f;
    reverb.setParameters (rp);
    reverb.processStereo (L, R, n);

    // TONE: 2-pole smoothing lowpass, takes the "scratchy" edge off
    const float a = std::exp (-juce::MathConstants<float>::twoPi * juce::jlimit (1000.0f, 20000.0f, params.get (TONE)) / sr);
    float* ch[2] = { L, R };
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < n; ++i)
        {
            z1[c] = a * z1[c] + (1.0f - a) * ch[c][i];
            z2[c] = a * z2[c] + (1.0f - a) * z1[c];
            ch[c][i] = z2[c];
        }

    buffer.applyGain (params.get (MASTER));
    outLevel.store (buffer.getMagnitude (0, n));

    int pos = scopePos.load();
    for (int i = 0; i < n; ++i) { scope[pos] = 0.5f * (L[i] + R[i]); pos = (pos + 1) & 511; }
    scopePos.store (pos);
}

juce::AudioProcessorEditor* NecroAudioProcessor::createEditor() { return new NecroEditor (*this); }

void NecroAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void NecroAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NecroAudioProcessor(); }
