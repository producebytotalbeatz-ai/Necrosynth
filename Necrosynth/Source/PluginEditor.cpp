#include "PluginEditor.h"

using namespace juce;
using namespace Necro;

static const Colour kOrange (0xffff7a18), kBone (0xffe8dcc0), kPurple (0xff7b2cbf), kGreen (0xff9dff3b);

static Font fnt (float h, int style = Font::plain) { return Font (FontOptions (h, style)); }

// ---------------------------------------------------------------- Look & feel
SpookyLAF::SpookyLAF()
{
    setColour (ComboBox::backgroundColourId, Colour (0xff1a1024));
    setColour (ComboBox::textColourId, kBone);
    setColour (ComboBox::outlineColourId, kOrange.withAlpha (0.6f));
    setColour (ComboBox::arrowColourId, kOrange);
    setColour (PopupMenu::backgroundColourId, Colour (0xff140b1c));
    setColour (PopupMenu::textColourId, kBone);
    setColour (PopupMenu::highlightedBackgroundColourId, kOrange.withAlpha (0.8f));
    setColour (PopupMenu::highlightedTextColourId, Colours::black);
    setColour (TextButton::buttonColourId, Colour (0xff1a1024));
    setColour (TextButton::buttonOnColourId, kOrange);
    setColour (TextButton::textColourOffId, kOrange);
    setColour (TextButton::textColourOnId, Colours::black);
    setColour (ScrollBar::thumbColourId, kOrange.withAlpha (0.55f));
    setColour (ListBox::backgroundColourId, Colours::transparentBlack);
    setColour (ListBox::outlineColourId, Colours::transparentBlack);
    setColour (TooltipWindow::backgroundColourId, Colour (0xff140b1c));
    setColour (TooltipWindow::textColourId, kBone);
}

void SpookyLAF::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                  float a0, float a1, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    auto c = b.getCentre();
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float ang = a0 + pos * (a1 - a0);
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

    // glow
    g.setGradientFill (ColourGradient (kOrange.withAlpha (0.10f + 0.40f * pos), c.x, c.y,
                                       Colours::transparentBlack, c.x, c.y - r * 1.7f, true));
    g.fillEllipse (b.expanded (11.0f));

    // drop shadow
    g.setColour (Colour (0x88000000));
    g.fillEllipse (b.reduced (5.0f).translated (0.0f, 3.0f));

    // tick marks
    g.setColour (Colour (0x55e8dcc0));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.0f;
        const float dx = std::sin (a), dy = -std::cos (a);
        g.drawLine (c.x + dx * (r + 1.0f), c.y + dy * (r + 1.0f), c.x + dx * (r + 4.0f), c.y + dy * (r + 4.0f), 1.0f);
    }

    // metallic rim
    auto body = b.reduced (5.0f);
    g.setGradientFill (ColourGradient (Colour (0xff8a5a2a), c.x - r, c.y - r, Colour (0xff2a1408), c.x + r, c.y + r, false));
    g.fillEllipse (body);
    // dome
    auto dome = body.reduced (3.0f);
    g.setGradientFill (ColourGradient (Colour (0xff4a2c5c), c.x - r * 0.4f, c.y - r * 0.6f,
                                       Colour (0xff0e0714), c.x + r * 0.5f, c.y + r * 0.7f, true));
    g.fillEllipse (dome);
    // specular highlight
    g.setGradientFill (ColourGradient (Colour (0x55ffffff), c.x - r * 0.25f, c.y - r * 0.5f,
                                       Colours::transparentWhite, c.x - r * 0.25f, c.y - r * 0.05f, true));
    g.fillEllipse (dome.reduced (r * 0.25f).translated (-r * 0.08f, -r * 0.22f));

    // arcs
    Path track; track.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
    g.setColour (Colour (0xff2a1a33));
    g.strokePath (track, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    const float zero = bipolar ? a0 + (float) s.valueToProportionOfLength (0.0) * (a1 - a0) : a0;
    Path val; val.addCentredArc (c.x, c.y, r, r, 0.0f, jmin (zero, ang), jmax (zero, ang), true);
    g.setColour (kOrange.withAlpha (0.35f));
    g.strokePath (val, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (kOrange);
    g.strokePath (val, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));

    // bone pointer
    const float dx = std::sin (ang), dy = -std::cos (ang);
    g.setColour (kBone);
    g.drawLine (c.x + dx * r * 0.25f, c.y + dy * r * 0.25f, c.x + dx * r * 0.66f, c.y + dy * r * 0.66f, 3.0f);
    g.fillEllipse (c.x + dx * r * 0.66f - 2.5f, c.y + dy * r * 0.66f - 2.5f, 5.0f, 5.0f);
}

// ---------------------------------------------------------------- Knob
Knob::Knob (AudioProcessorValueTreeState& apvts, const String& id, const String& text)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (slider);

    label.setText (text.toUpperCase(), dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setColour (Label::textColourId, kBone);
    label.setFont (fnt (10.0f, Font::bold));
    addAndMakeVisible (label);

    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (apvts, id, slider);
}

void Knob::resized()
{
    slider.setBounds (0, 0, getWidth(), getWidth());
    label.setBounds (0, getWidth() - 3, getWidth(), 16);
}

// ---------------------------------------------------------------- Editor
NecroEditor::NecroEditor (NecroAudioProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (1220, 640);

    for (int i = 0; i < NUM; ++i)
    {
        if (i < 2) { knobs.add (nullptr); continue; }
        auto* k = knobs.add (new Knob (proc.apvts, defs[i].id, defs[i].name));
        addAndMakeVisible (k);
    }

    for (auto* box : { &wave1Box, &wave2Box })
    {
        box->addItemList ({ "Saw", "Square", "Sine", "Tri" }, 1);
        addAndMakeVisible (box);
    }
    w1Att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, defs[W1].id, wave1Box);
    w2Att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, defs[W2].id, wave2Box);

    // preset browser
    cats = NecroAudioProcessor::categoryNames();
    StringArray labels { "ALL" };
    labels.addArray (cats);
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = catBtns.add (new TextButton (labels[i]));
        b->setClickingTogglesState (true);
        b->setRadioGroupId (42);
        b->setToggleState (i == 0, dontSendNotification);
        b->onClick = [this, i] { catFilter = i - 1; rebuildFilter(); };
        addAndMakeVisible (b);
    }
    list.setRowHeight (26);
    addAndMakeVisible (list);
    rebuildFilter();

    nameLabel.setJustificationType (Justification::centred);
    nameLabel.setColour (Label::textColourId, kOrange);
    nameLabel.setFont (fnt (15.0f, Font::bold));
    addAndMakeVisible (nameLabel);

    prevBtn.onClick = [this] { step (-1); };
    nextBtn.onClick = [this] { step (1); };
    randBtn.onClick = [this]
    {
        proc.randomizeAll();
        randomised = true;
        nameLabel.setText ("~ RANDOM ~", dontSendNotification);
        list.repaint();
    };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);
    addAndMakeVisible (randBtn);

    startTimerHz (30);
}

NecroEditor::~NecroEditor() { setLookAndFeel (nullptr); }

void NecroEditor::rebuildFilter()
{
    filtered.clear();
    for (int i = 0; i < NecroAudioProcessor::numPresets(); ++i)
        if (catFilter < 0 || NecroAudioProcessor::presetCategory (i) == cats[catFilter])
            filtered.push_back (i);
    list.updateContent();
    list.repaint();
    repaint();
}

void NecroEditor::choosePreset (int idx)
{
    proc.setCurrentProgram (idx);
    randomised = false;
    list.repaint();
}

void NecroEditor::step (int dir)
{
    if (filtered.empty()) return;
    const int cur = proc.getCurrentProgram();
    int pos = -1;
    for (size_t i = 0; i < filtered.size(); ++i) if (filtered[i] == cur) pos = (int) i;
    pos = pos < 0 ? (dir > 0 ? 0 : (int) filtered.size() - 1)
                  : (pos + dir + (int) filtered.size()) % (int) filtered.size();
    choosePreset (filtered[(size_t) pos]);
    list.scrollToEnsureRowIsOnscreen (pos);
}

void NecroEditor::listBoxItemClicked (int row, const MouseEvent&)
{
    if (isPositiveAndBelow (row, (int) filtered.size())) choosePreset (filtered[(size_t) row]);
}

void NecroEditor::paintListBoxItem (int row, Graphics& g, int w, int h, bool)
{
    if (! isPositiveAndBelow (row, (int) filtered.size())) return;
    const int idx = filtered[(size_t) row];
    const bool cur = (idx == proc.getCurrentProgram()) && ! randomised;

    if (cur)
    {
        g.setGradientFill (ColourGradient (kOrange.withAlpha (0.9f), 0, 0, kOrange.withAlpha (0.55f), 0, (float) h, false));
        g.fillRoundedRectangle (1.0f, 1.0f, (float) w - 2.0f, (float) h - 2.0f, 5.0f);
    }
    else if (row % 2 == 0)
    {
        g.setColour (Colour (0x14ffffff));
        g.fillRoundedRectangle (1.0f, 1.0f, (float) w - 2.0f, (float) h - 2.0f, 5.0f);
    }
    g.setColour (cur ? Colours::black : kBone);
    g.setFont (fnt (13.0f, cur ? Font::bold : Font::plain));
    g.drawText (NecroAudioProcessor::presetName (idx), 10, 0, w - 70, h, Justification::centredLeft);
    g.setColour (cur ? Colours::black.withAlpha (0.7f) : kOrange.withAlpha (0.75f));
    g.setFont (fnt (9.0f, Font::bold));
    g.drawText (NecroAudioProcessor::presetCategory (idx).toUpperCase(), w - 62, 0, 54, h, Justification::centredRight);
}

void NecroEditor::timerCallback()
{
    t += 1.0f / 30.0f;
    const float target = jlimit (0.0f, 1.0f, proc.outLevel.load() * 1.5f);
    level += (target > level ? 0.5f : 0.12f) * (target - level);

    if (! randomised)
    {
        const int cur = proc.getCurrentProgram();
        const String n = NecroAudioProcessor::presetName (cur);
        if (n != lastName) { lastName = n; nameLabel.setText (n, dontSendNotification); }
    }
    repaint();
}

void NecroEditor::resized()
{
    panels.clear();

    prevBtn.setBounds (500, 38, 30, 30);
    nameLabel.setBounds (536, 38, 250, 30);
    nextBtn.setBounds (792, 38, 30, 30);
    randBtn.setBounds (850, 34, 140, 38);

    // preset graveyard (left column)
    browserRect = { 10, 115, 250, 515 };
    const int bw = 55;
    for (int i = 0; i < catBtns.size(); ++i)
        catBtns[i]->setBounds (18 + (i % 4) * (bw + 4), 147 + (i / 4) * 26, bw, 22);
    list.setBounds (16, 147 + 3 * 26 + 8, 238, 630 - (147 + 86) - 12);

    // synth panels
    const int cw = 70, pad = 8, gap = 10;
    auto place = [&] (const String& title, std::vector<int> idx, int x, int y, int h) -> int
    {
        const int w = (int) idx.size() * cw + 2 * pad;
        panels.push_back ({ title, { x, y, w, h } });
        for (size_t k = 0; k < idx.size(); ++k)
            knobs[idx[k]]->setBounds (x + pad + (int) k * cw, y + h - 96, cw, 90);
        return x + w + gap;
    };

    const int y1 = 115, h1 = 200, y2 = 330, h2 = 130;
    int x = 270;
    wave1Box.setBounds (x + pad, y1 + 34, 2 * cw + 30, 26);
    wave2Box.setBounds (x + pad, y1 + 66, 2 * cw + 30, 26);
    x = place ("OSCILLATORS", { SEMI2, FINE2, MIX2, SUB, NOISE }, x, y1, h1);
    x = place ("MUTATE", { POSS, SCREAM }, x, y1, h1);
    x = place ("FILTER", { CUT, RESO, ENVAMT }, x, y1, h1);
    place ("OUTPUT", { TONE, MASTER }, x, y1, h1);

    x = 270;
    x = place ("ENVELOPE", { ATK, DEC, SUS, REL }, x, y2, h2);
    x = place ("CREEP", { LFORATE, LFOFILT, LFOPITCH }, x, y2, h2);
    place ("HAUNT", { DTIME, DFB, DMIX, RSIZE, RMIX }, x, y2, h2);

    scopeRect = { 270, 475, 940, 155 };
}

// ---------------------------------------------------------------- drawing helpers
static void drawWeb (Graphics& g, Point<float> o, float sx, float sy)
{
    Path p;
    const int spokes = 6;
    const float len = 150.0f, hp = MathConstants<float>::halfPi;
    for (int i = 0; i <= spokes; ++i)
    {
        const float a = hp * (float) i / (float) spokes;
        p.startNewSubPath (o);
        p.lineTo (o.x + sx * std::cos (a) * len, o.y + sy * std::sin (a) * len);
    }
    for (int r = 1; r <= 5; ++r)
    {
        const float rad = len * (float) r / 5.0f;
        for (int i = 0; i < spokes; ++i)
        {
            const float a0 = hp * (float) i / (float) spokes, a1 = hp * (float) (i + 1) / (float) spokes;
            Point<float> p0 (o.x + sx * std::cos (a0) * rad, o.y + sy * std::sin (a0) * rad);
            Point<float> p1 (o.x + sx * std::cos (a1) * rad, o.y + sy * std::sin (a1) * rad);
            Point<float> mid = o + ((p0 + p1) * 0.5f - o) * 0.86f;
            p.startNewSubPath (p0);
            p.quadraticTo (mid, p1);
        }
    }
    g.setColour (Colour (0x33ffffff));
    g.strokePath (p, PathStrokeType (1.0f));
}

void NecroEditor::drawBat (Graphics& g, float x, float y, float flap, float s)
{
    Path w;
    w.startNewSubPath (0, 0);
    w.quadraticTo (-18 * s, (-10 * flap - 6) * s, -32 * s, 4 * flap * s);
    w.quadraticTo (-22 * s, 6 * s, -16 * s, 2 * s);
    w.quadraticTo (-9 * s, 8 * s, 0, 4 * s);
    w.closeSubPath();
    Path wr = w; wr.applyTransform (AffineTransform::scale (-1.0f, 1.0f));
    g.setColour (Colour (0xff05020a));
    g.fillPath (w, AffineTransform::translation (x, y));
    g.fillPath (wr, AffineTransform::translation (x, y));
    g.fillEllipse (x - 4 * s, y - 4 * s, 8 * s, 11 * s);
    g.setColour (kPurple.withAlpha (0.5f));
    g.strokePath (w, PathStrokeType (1.0f), AffineTransform::translation (x, y));
    g.strokePath (wr, PathStrokeType (1.0f), AffineTransform::translation (x, y));
    g.setColour (Colour (0xffff3030).withAlpha (0.9f));
    g.fillEllipse (x - 2.5f * s, y - 1.5f * s, 1.5f * s, 1.5f * s);
    g.fillEllipse (x + 1.0f * s, y - 1.5f * s, 1.5f * s, 1.5f * s);
}

void NecroEditor::drawPumpkin (Graphics& g, Rectangle<float> r)
{
    auto c = r.getCentre();
    g.setGradientFill (ColourGradient (kOrange.withAlpha (0.15f + 0.5f * level), c.x, c.y,
                                       Colours::transparentBlack, c.x, c.y - r.getHeight() * 1.3f, true));
    g.fillEllipse (r.expanded (36.0f));

    // shadow
    g.setColour (Colour (0x66000000));
    g.fillEllipse (r.getX() + 6, r.getBottom() - 6, r.getWidth() - 12, 10);

    Path stem; stem.addRoundedRectangle (c.x - 5, r.getY() - 8, 10, 16, 3.0f);
    g.setGradientFill (ColourGradient (Colour (0xff5a7a2a), c.x - 5, 0, Colour (0xff2a3d10), c.x + 5, 0, false));
    g.fillPath (stem);

    auto lobe = [&] (Rectangle<float> b)
    {
        g.setGradientFill (ColourGradient (Colour (0xffffa040), b.getX() + b.getWidth() * 0.35f, b.getY() + b.getHeight() * 0.25f,
                                           Colour (0xff8a2f00), b.getRight(), b.getBottom(), true));
        g.fillEllipse (b);
    };
    lobe (r.withSizeKeepingCentre (r.getWidth() * 0.62f, r.getHeight()));
    lobe (r.withSizeKeepingCentre (r.getWidth() * 0.82f, r.getHeight() * 0.94f).translated (-r.getWidth() * 0.07f, 0));
    lobe (r.withSizeKeepingCentre (r.getWidth() * 0.82f, r.getHeight() * 0.94f).translated (r.getWidth() * 0.07f, 0));

    const float flicker = 0.75f + 0.25f * std::sin (t * 17.0f) * std::sin (t * 5.3f);
    const float glow = jlimit (0.0f, 1.0f, (0.35f + level) * flicker);
    g.setColour (Colour (0xfffff06a).withAlpha (glow * 0.35f));
    g.fillEllipse (c.x - 30, c.y - 22, 60, 50);
    g.setColour (Colour (0xfffff06a).withAlpha (glow));

    Path face;
    face.addTriangle (c.x - 22, c.y - 6, c.x - 8, c.y - 6, c.x - 15, c.y - 20);
    face.addTriangle (c.x + 8, c.y - 6, c.x + 22, c.y - 6, c.x + 15, c.y - 20);
    face.startNewSubPath (c.x - 24, c.y + 8);
    for (int i = 0; i <= 6; ++i) face.lineTo (c.x - 24 + 8.0f * (float) i, c.y + 8 + (i % 2 == 0 ? 0.0f : 9.0f));
    for (int i = 6; i >= 0; --i) face.lineTo (c.x - 24 + 8.0f * (float) i, c.y + 20 + (i % 2 == 0 ? 0.0f : 6.0f));
    face.closeSubPath();
    g.fillPath (face);
}

void NecroEditor::drawPanel (Graphics& g, Rectangle<float> r, const String& title)
{
    Path pp; pp.addRoundedRectangle (r, 12.0f);
    DropShadow (Colour (0xaa000000), 10, { 0, 4 }).drawForPath (g, pp);

    g.setGradientFill (ColourGradient (Colour (0xee231433), r.getX(), r.getY(), Colour (0xee0e0714), r.getX(), r.getBottom(), false));
    g.fillPath (pp);

    // inner highlight + pulsing border
    g.setColour (Colour (0x18ffffff));
    g.drawLine (r.getX() + 12, r.getY() + 1.5f, r.getRight() - 12, r.getY() + 1.5f, 1.0f);
    g.setColour (kOrange.withAlpha (0.25f + 0.45f * level));
    g.strokePath (pp, PathStrokeType (1.3f));

    g.setColour (kOrange);
    g.setFont (fnt (11.0f, Font::bold));
    g.drawText (title, r.toNearestInt().withTrimmedLeft (14).withHeight (24), Justification::centredLeft);
    g.setColour (kOrange.withAlpha (0.35f));
    g.fillRect (r.getX() + 14.0f, r.getY() + 23.0f, 28.0f, 1.5f);
}

void NecroEditor::paint (Graphics& g)
{
    const float W = (float) getWidth(), H = (float) getHeight();

    g.setGradientFill (ColourGradient (Colour (0xff0b0510), 0, 0, Colour (0xff2a0f34), 0, H, false));
    g.fillAll();

    // twinkling stars
    Random rs (7);
    for (int i = 0; i < 60; ++i)
    {
        const float sx = rs.nextFloat() * W, sy = rs.nextFloat() * 110.0f;
        const float tw = 0.3f + 0.7f * std::abs (std::sin (t * (0.6f + rs.nextFloat()) + (float) i));
        g.setColour (kBone.withAlpha (0.35f * tw));
        g.fillEllipse (sx, sy, 1.8f, 1.8f);
    }

    // moon
    g.setGradientFill (ColourGradient (Colour (0x55e8dcc0), W - 60, 50, Colours::transparentBlack, W - 60, -30, true));
    g.fillEllipse (W - 140, -30, 160, 160);
    g.setGradientFill (ColourGradient (Colour (0xfffff4d6), W - 90, 20, Colour (0xffc8b890), W - 40, 80, false));
    g.fillEllipse (W - 95, 12, 70, 70);
    g.setColour (Colour (0x26000000));
    g.fillEllipse (W - 80, 25, 16, 16);
    g.fillEllipse (W - 56, 46, 10, 10);
    g.fillEllipse (W - 70, 55, 7, 7);

    drawWeb (g, { 0, 0 }, 1, 1);
    drawWeb (g, { W, H }, -1, -1);

    // drifting fog
    for (int i = 0; i < 7; ++i)
    {
        const float fx = std::fmod (t * (6.0f + (float) i * 3.0f) + (float) i * 230.0f, W + 500.0f) - 250.0f;
        const float fy = H - 60.0f + 25.0f * std::sin (t * 0.4f + (float) i);
        g.setGradientFill (ColourGradient (kPurple.withAlpha (0.16f), fx, fy, Colours::transparentBlack, fx + 220, fy + 90, true));
        g.fillEllipse (fx - 220, fy - 70, 440, 140);
    }

    // bats
    for (int i = 0; i < 4; ++i)
    {
        const float bx = std::fmod (t * (38.0f + 9.0f * (float) i) + (float) i * 310.0f, W + 160.0f) - 80.0f;
        const float by = 28.0f + 17.0f * (float) i + 14.0f * std::sin (t * 1.7f + (float) i * 2.0f);
        drawBat (g, bx, by, std::sin (t * 14.0f + (float) i), 0.55f + 0.1f * (float) (i % 2));
    }

    // title with glow
    const String title = "NECROSYNTH";
    for (int i = 6; i >= 1; --i)
    {
        g.setColour (kOrange.withAlpha (0.05f + 0.03f * level * (float) (7 - i)));
        g.setFont (fnt (46.0f, Font::bold));
        g.drawText (title, Rectangle<int> (130 - i, 20 - i, 300 + 2 * i, 64 + 2 * i), Justification::centredLeft);
    }
    g.setColour (Colour (0xff000000).withAlpha (0.6f));
    g.setFont (fnt (46.0f, Font::bold));
    g.drawText (title, 132, 23, 300, 64, Justification::centredLeft);
    g.setColour (kOrange);
    g.drawText (title, 130, 20, 300, 64, Justification::centredLeft);
    g.setColour (kBone.withAlpha (0.55f));
    g.setFont (fnt (11.0f, Font::bold));
    g.drawText ("H A L L O W E E N   H O R R O R   S Y N T H", 132, 78, 340, 16, Justification::centredLeft);

    drawPumpkin (g, { 30, 28, 80, 62 });

    // preset name plate
    auto np = nameLabel.getBounds().toFloat().expanded (4, 2);
    g.setGradientFill (ColourGradient (Colour (0xff1f1230), np.getX(), np.getY(), Colour (0xff0a050f), np.getX(), np.getBottom(), false));
    g.fillRoundedRectangle (np, 8.0f);
    g.setColour (kOrange.withAlpha (0.6f));
    g.drawRoundedRectangle (np.reduced (0.5f), 8.0f, 1.2f);

    // preset graveyard
    drawPanel (g, browserRect.toFloat(), "PRESET GRAVEYARD");
    g.setColour (kBone.withAlpha (0.5f));
    g.setFont (fnt (10.0f, Font::bold));
    g.drawText (String ((int) filtered.size()) + " / " + String (NecroAudioProcessor::numPresets()),
                browserRect.withTrimmedRight (14).withHeight (24), Justification::centredRight);

    // synth panels
    for (auto& pn : panels)
    {
        drawPanel (g, pn.r.toFloat(), pn.title);
        if (pn.title == "OSCILLATORS")
        {
            g.setColour (kBone.withAlpha (0.6f));
            g.setFont (fnt (10.0f, Font::bold));
            g.drawText ("OSC 1", wave1Box.getRight() + 8, wave1Box.getY(), 50, 26, Justification::centredLeft);
            g.drawText ("OSC 2", wave2Box.getRight() + 8, wave2Box.getY(), 50, 26, Justification::centredLeft);
        }
    }

    // crystal ball scope
    auto sr = scopeRect.toFloat();
    drawPanel (g, sr, "SPIRIT SCOPE");
    auto sc = sr.reduced (14.0f).withTrimmedTop (16.0f);
    g.setGradientFill (ColourGradient (Colour (0xff06140a), sc.getX(), sc.getY(), Colour (0xff020805), sc.getX(), sc.getBottom(), false));
    g.fillRoundedRectangle (sc, 8.0f);
    g.setColour (Colour (0x229dff3b));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (sc.getY() + sc.getHeight() * (float) i / 4.0f), sc.getX() + 4, sc.getRight() - 4);
    for (int i = 1; i < 12; ++i) g.drawVerticalLine ((int) (sc.getX() + sc.getWidth() * (float) i / 12.0f), sc.getY() + 4, sc.getBottom() - 4);

    Path wave;
    const int pos = proc.scopePos.load();
    const int pts = 256;
    for (int i = 0; i < pts; ++i)
    {
        const float s = proc.scope[(pos + 512 - pts * 2 + i * 2) & 511];
        const float px = sc.getX() + sc.getWidth() * (float) i / (float) (pts - 1);
        const float py = sc.getCentreY() - jlimit (-1.0f, 1.0f, s * 2.2f) * sc.getHeight() * 0.45f;
        if (i == 0) wave.startNewSubPath (px, py); else wave.lineTo (px, py);
    }
    g.setColour (kGreen.withAlpha (0.18f));
    g.strokePath (wave, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (kGreen);
    g.strokePath (wave, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
}

void NecroEditor::paintOverChildren (Graphics& g)
{
    const float W = (float) getWidth(), H = (float) getHeight();
    g.setGradientFill (ColourGradient (Colours::transparentBlack, W * 0.5f, H * 0.5f,
                                       Colour (0x66000000), 0, 0, true));
    g.fillRect (getLocalBounds());
}
