#include "PluginEditor.h"
#include "Presets.h"

using namespace kr;
using APVTS = juce::AudioProcessorValueTreeState;

namespace
{
constexpr int kW = 1200, kH = 800;
const juce::Identifier idOn ("on"), idLen ("len"), idDir ("dir"), idDepth ("depth"), idVals ("vals");

juce::Font font (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Point<float> polar (juce::Point<float> c, float r, float a) { return { c.x + r * std::cos (a), c.y + r * std::sin (a) }; }
float stepAngle (int j, int n) { return -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi * (float) j / (float) juce::jmax (1, n); }
} // namespace

// ============================================================================================ KLook
void KLook::setTheme (const Theme& t)
{
    th = t;
    setColour (juce::ResizableWindow::backgroundColourId, t.bg);
    setColour (juce::Label::textColourId, t.dim);
    setColour (juce::Slider::textBoxTextColourId, t.text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, t.accent.withAlpha (0.4f));
    setColour (juce::ComboBox::backgroundColourId, t.panel2);
    setColour (juce::ComboBox::textColourId, t.text);
    setColour (juce::ComboBox::outlineColourId, t.panel2);
    setColour (juce::ComboBox::arrowColourId, t.dim);
    setColour (juce::PopupMenu::backgroundColourId, t.panel);
    setColour (juce::PopupMenu::textColourId, t.text);
    setColour (juce::PopupMenu::headerTextColourId, t.accent);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, t.accent.withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, t.text);
    setColour (juce::TextButton::textColourOffId, t.text);
    setColour (juce::TextButton::textColourOnId, t.bg);
    setColour (juce::TextButton::buttonColourId, t.panel2);
    setColour (juce::TextButton::buttonOnColourId, t.accent);
    setColour (juce::ToggleButton::textColourId, t.text);
    setColour (juce::ToggleButton::tickColourId, t.accent);
    setColour (juce::TextEditor::backgroundColourId, t.panel);
    setColour (juce::TextEditor::textColourId, t.text);
    setColour (juce::TextEditor::outlineColourId, t.panel2);
    setColour (juce::TextEditor::focusedOutlineColourId, t.accent);
    setColour (juce::ScrollBar::thumbColourId, t.dim.withAlpha (0.5f));
    setColour (juce::AlertWindow::backgroundColourId, t.panel);
    setColour (juce::AlertWindow::textColourId, t.text);
    setColour (juce::CaretComponent::caretColourId, t.accent);
}

juce::Label* KLook::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (font (12.5f));
    l->setColour (juce::Label::textColourId, th.text);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

void KLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const auto col = s.findColour (juce::Slider::rotarySliderFillColourId);
    auto b = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float lw = juce::jmax (3.0f, r * 0.16f);
    juce::Path track, val;
    track.addCentredArc (c.x, c.y, r - lw, r - lw, 0, a0, a1, true);
    g.setColour (th.panel2);
    g.strokePath (track, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float an = a0 + pos * (a1 - a0);
    val.addCentredArc (c.x, c.y, r - lw, r - lw, 0, a0, an, true);
    g.setColour (s.isEnabled() ? col : th.dim);
    g.strokePath (val, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (th.panel);
    g.fillEllipse (juce::Rectangle<float> (r * 1.1f, r * 1.1f).withCentre (c));
    g.setColour (th.text);
    g.drawLine (juce::Line<float> (polar (c, r * 0.18f, an - juce::MathConstants<float>::halfPi),
                                   polar (c, r * 0.52f, an - juce::MathConstants<float>::halfPi)), 2.0f);
}

void KLook::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                              juce::Slider::SliderStyle style, juce::Slider& s)
{
    const auto col = s.findColour (juce::Slider::trackColourId);
    auto b = juce::Rectangle<int> (x, y, w, h).toFloat();
    if (style == juce::Slider::LinearBar)
    {
        g.setColour (th.panel2);
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (col.withAlpha (0.55f));
        g.fillRoundedRectangle (b.withRight (juce::jmax (b.getX() + 2.0f, pos)), 3.0f);
        return;
    }
    if (style == juce::Slider::TwoValueHorizontal)
    {
        const float cy = b.getCentreY();
        g.setColour (th.panel2);
        g.fillRoundedRectangle (b.getX(), cy - 3, b.getWidth(), 6, 3);
        g.setColour (col);
        g.fillRoundedRectangle (minPos, cy - 3, juce::jmax (2.0f, maxPos - minPos), 6, 3);
        for (float p : { minPos, maxPos })
        {
            g.setColour (th.text);
            g.fillEllipse (p - 6, cy - 6, 12, 12);
        }
        return;
    }
    const float cy = b.getCentreY();
    g.setColour (th.panel2);
    g.fillRoundedRectangle (b.getX(), cy - 3, b.getWidth(), 6, 3);
    g.setColour (col);
    g.fillRoundedRectangle (b.getX(), cy - 3, juce::jmax (2.0f, pos - b.getX()), 6, 3);
    g.setColour (th.text);
    g.fillEllipse (pos - 6, cy - 6, 12, 12);
}

void KLook::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto base = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId) : b.findColour (juce::TextButton::buttonColourId);
    if (over) base = base.brighter (0.12f);
    if (down) base = base.darker (0.15f);
    g.setColour (base);
    g.fillRoundedRectangle (r, 5.0f);
}

void KLook::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                     .withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.5f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (3, 1), juce::Justification::centred, 1);
}

void KLook::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    auto box = r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.22f);
    g.setColour (b.getToggleState() ? th.accent : th.panel2.brighter (over ? 0.15f : 0.0f));
    g.fillRoundedRectangle (box, 4.0f);
    if (b.getToggleState())
    {
        g.setColour (th.bg);
        juce::Path tick;
        tick.startNewSubPath (box.getX() + box.getWidth() * 0.22f, box.getCentreY());
        tick.lineTo (box.getX() + box.getWidth() * 0.43f, box.getBottom() - box.getHeight() * 0.25f);
        tick.lineTo (box.getRight() - box.getWidth() * 0.2f, box.getY() + box.getHeight() * 0.25f);
        g.strokePath (tick, juce::PathStrokeType (2.0f));
    }
    g.setColour (th.text);
    g.setFont (font (13.0f));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().withTrimmedLeft (4), juce::Justification::centredLeft, 1);
}

void KLook::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& cb)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h);
    g.setColour (cb.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 5.0f);
    juce::Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (th.dim);
    g.fillPath (arrow);
}

// ============================================================================================ Visualizer
std::array<Visualizer::TrackView, kTracks> Visualizer::gather() const
{
    std::array<TrackView, kTracks> v;
    const bool running = proc.transportRunning;
    for (int t = 0; t < kTracks; ++t)
    {
        auto& tv = v[(size_t) t];
        tv.active = proc.getParam (pid (t, "active")) > 0.5f;
        tv.muted = proc.getParam (pid (t, "mute")) > 0.5f;
        const auto& vs = proc.vis[(size_t) t];
        if (running && vs.pos >= 0)
        {
            tv.steps = vs.steps; tv.beats = vs.beats; tv.rotate = vs.rotate; tv.pos = vs.pos;
        }
        else
        {
            tv.steps = (int) proc.getParam (pid (t, "steps"));
            tv.beats = juce::jmin (tv.steps, (int) proc.getParam (pid (t, "beats")));
            tv.rotate = (int) proc.getParam (pid (t, "rotate"));
            tv.pos = -1;
        }
        tv.flash = flash[(size_t) t];
        tv.col = ed.theme().track[(size_t) t];
        tv.stepQ = divisions[juce::jlimit (0, kNumDivisions - 1, (int) proc.getParam (pid (t, "clock")))].quarters;
    }
    return v;
}

void Visualizer::tick()
{
    bool any = false;
    for (int t = 0; t < kTracks; ++t)
    {
        const auto h = proc.vis[(size_t) t].hits.load();
        if (h != lastHits[(size_t) t]) { lastHits[(size_t) t] = h; flash[(size_t) t] = 1.0f; }
        else flash[(size_t) t] *= 0.82f;
        any |= flash[(size_t) t] > 0.01f;
    }
    juce::ignoreUnused (any);
    repaint();
}

void Visualizer::mouseDown (const juce::MouseEvent& e)
{
    // click a ring / row to select its track
    auto b = getLocalBounds().toFloat();
    if (mode == 0 || mode == 4)
    {
        const int t = juce::jlimit (0, kTracks - 1, (int) ((e.position.y - b.getY()) / (b.getHeight() / kTracks)));
        ed.selectTrack (t);
        return;
    }
    if (mode == 5)
    {
        const int col = (int) (e.position.x / (b.getWidth() / 4)), row = (int) (e.position.y / (b.getHeight() / 2));
        ed.selectTrack (juce::jlimit (0, kTracks - 1, row * 4 + col));
        return;
    }
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 14.0f, R0 = R * 0.24f;
    const float d = e.position.getDistanceFrom (c);
    const int t = (int) std::floor ((d - R0) / ((R - R0) / kTracks));
    if (juce::isPositiveAndBelow (t, kTracks)) ed.selectTrack (t);
}

void Visualizer::paint (juce::Graphics& g)
{
    const auto& th = ed.theme();
    auto b = getLocalBounds().toFloat();
    g.setColour (th.panel);
    g.fillRoundedRectangle (b, 10.0f);
    b = b.reduced (12.0f);

    const auto tv = gather();
    const int sel = ed.selectedTrack();
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 2.0f;
    const float R0 = R * 0.24f;
    auto alphaFor = [&] (const TrackView& t) { return ! t.active ? 0.12f : (t.muted ? 0.35f : 1.0f); };

    auto centreText = [&] {
        g.setColour (th.dim);
        g.setFont (font (13.0f));
        if (proc.transportRunning)
        {
            const double ppq = proc.currentPpq.load();
            const int bar = (int) std::floor (ppq / 4.0) + 1, beat = (int) std::floor (std::fmod (ppq, 4.0)) + 1;
            g.setColour (th.text);
            g.setFont (font (20.0f, true));
            g.drawText (juce::String (bar) + "." + juce::String (beat), juce::Rectangle<float> (120, 26).withCentre (c.translated (0, -8)), juce::Justification::centred);
            g.setFont (font (12.0f));
            g.setColour (th.dim);
            g.drawText (juce::String (proc.currentBpm.load(), 1) + " BPM", juce::Rectangle<float> (120, 18).withCentre (c.translated (0, 14)), juce::Justification::centred);
        }
        else
        {
            g.setFont (font (16.0f, true));
            g.setColour (th.text.withAlpha (0.8f));
            g.drawText ("KREISEL", juce::Rectangle<float> (140, 22).withCentre (c.translated (0, -6)), juce::Justification::centred);
            g.setFont (font (11.0f));
            g.setColour (th.dim);
            g.drawText ("gestoppt", juce::Rectangle<float> (120, 16).withCentre (c.translated (0, 13)), juce::Justification::centred);
        }
    };

    switch (mode)
    {
        case 0: // Band – each track stretched over the full width
        {
            const float rh = b.getHeight() / kTracks;
            for (int t = 0; t < kTracks; ++t)
            {
                const auto& v = tv[(size_t) t];
                auto row = juce::Rectangle<float> (b.getX(), b.getY() + rh * t, b.getWidth(), rh).reduced (0, 4);
                if (t == sel) { g.setColour (th.panel2); g.fillRoundedRectangle (row.expanded (4, 2), 6); }
                g.setColour (v.col.withAlpha (alphaFor (v)));
                g.setFont (font (13.0f, true));
                g.drawText (juce::String (t + 1), row.removeFromLeft (22), juce::Justification::centred);
                const float cw = row.getWidth() / v.steps;
                for (int j = 0; j < v.steps; ++j)
                {
                    auto cell = juce::Rectangle<float> (row.getX() + cw * j, row.getY(), cw, row.getHeight()).reduced (juce::jmin (2.0f, cw * 0.15f), 2);
                    const bool hit = patternHit (j, v.steps, v.beats, v.rotate);
                    const bool cur = j == v.pos;
                    g.setColour (hit ? v.col.withAlpha (alphaFor (v) * (cur ? 1.0f : 0.75f)) : th.panel2.withAlpha (alphaFor (v) > 0.5f ? 1.0f : 0.5f));
                    g.fillRoundedRectangle (hit ? cell : cell.reduced (0, cell.getHeight() * 0.3f), 3);
                    if (cur)
                    {
                        g.setColour (th.text.withAlpha (0.4f + 0.6f * v.flash));
                        g.drawRoundedRectangle (cell.expanded (1), 3, 2.0f);
                    }
                }
            }
            return;
        }
        case 4: // Raster – horizontal position = real time, so tracks of different clocks line up
        {
            double window = 0;
            for (auto& v : tv) if (v.active) window = juce::jmax (window, v.steps * v.stepQ);
            if (window <= 0) window = 4;
            const float rh = b.getHeight() / kTracks;
            auto area = b.withTrimmedLeft (24);
            for (int beat = 0; beat <= (int) std::ceil (window); ++beat)
            {
                const float x = area.getX() + (float) (beat / window) * area.getWidth();
                g.setColour (th.panel2.withAlpha (beat % 4 == 0 ? 1.0f : 0.45f));
                g.drawVerticalLine ((int) x, b.getY(), b.getBottom());
            }
            for (int t = 0; t < kTracks; ++t)
            {
                const auto& v = tv[(size_t) t];
                auto row = juce::Rectangle<float> (b.getX(), b.getY() + rh * t, b.getWidth(), rh).reduced (0, 5);
                g.setColour (v.col.withAlpha (alphaFor (v)));
                g.setFont (font (13.0f, true));
                g.drawText (juce::String (t + 1), row.removeFromLeft (22), juce::Justification::centred);
                if (t == sel) { g.setColour (v.col.withAlpha (0.25f)); g.drawRoundedRectangle (row.expanded (2), 4, 1.5f); }
                const float cw = (float) (v.stepQ / window) * area.getWidth();
                const int reps = (int) std::ceil (window / (v.steps * v.stepQ));
                for (int rep = 0; rep < reps; ++rep)
                    for (int j = 0; j < v.steps; ++j)
                    {
                        const float x = area.getX() + cw * (rep * v.steps + j);
                        if (x >= area.getRight() - 0.5f) break;
                        auto cell = juce::Rectangle<float> (x, row.getY(), cw, row.getHeight()).reduced (juce::jmin (1.5f, cw * 0.12f), 0);
                        const bool hit = patternHit (j, v.steps, v.beats, v.rotate);
                        const float a = alphaFor (v) * (rep == 0 ? 1.0f : 0.45f);
                        g.setColour (hit ? v.col.withAlpha (a) : th.panel2.withAlpha (a));
                        g.fillRect (hit ? cell : cell.reduced (0, cell.getHeight() * 0.38f));
                        if (j == v.pos && rep == 0)
                        {
                            g.setColour (th.text.withAlpha (0.5f + 0.5f * v.flash));
                            g.drawRect (cell.expanded (1), 2.0f);
                        }
                    }
            }
            return;
        }
        case 5: // Puls – one breathing circle per track
        {
            const float cw = b.getWidth() / 4, chh = b.getHeight() / 2;
            for (int t = 0; t < kTracks; ++t)
            {
                const auto& v = tv[(size_t) t];
                auto cell = juce::Rectangle<float> (b.getX() + cw * (t % 4), b.getY() + chh * (t / 4), cw, chh).reduced (8);
                const auto cc = cell.getCentre();
                const float rr = juce::jmin (cell.getWidth(), cell.getHeight()) * 0.5f;
                if (t == sel) { g.setColour (th.panel2); g.fillRoundedRectangle (cell, 10); }
                const float a = alphaFor (v);
                const float core = rr * (0.32f + 0.22f * v.flash);
                g.setColour (v.col.withAlpha (a * (0.25f + 0.6f * v.flash)));
                g.fillEllipse (juce::Rectangle<float> (core * 2, core * 2).withCentre (cc));
                g.setColour (v.col.withAlpha (a * 0.15f));
                g.fillEllipse (juce::Rectangle<float> (core * 2.8f, core * 2.8f).withCentre (cc));
                for (int j = 0; j < v.steps; ++j)
                {
                    const bool hit = patternHit (j, v.steps, v.beats, v.rotate);
                    const auto p = polar (cc, rr * 0.82f, stepAngle (j, v.steps));
                    const float d = hit ? juce::jmax (3.0f, rr * 0.07f) : 2.0f;
                    g.setColour (j == v.pos ? th.text.withAlpha (a) : (hit ? v.col.withAlpha (a) : th.dim.withAlpha (a * 0.5f)));
                    g.fillEllipse (juce::Rectangle<float> (d * 2, d * 2).withCentre (p));
                }
                g.setColour (th.text.withAlpha (a));
                g.setFont (font (16.0f, true));
                g.drawText (juce::String (t + 1), juce::Rectangle<float> (40, 20).withCentre (cc), juce::Justification::centred);
            }
            return;
        }
        default: break;
    }

    // ---- circular modes
    g.setColour (th.panel2.withAlpha (0.35f));
    g.fillEllipse (juce::Rectangle<float> (R0 * 2, R0 * 2).withCentre (c));

    for (int t = kTracks - 1; t >= 0; --t)
    {
        const auto& v = tv[(size_t) t];
        const float a = alphaFor (v);
        const float r = radiusFor (t, R0, R);
        const float band = (R - R0) / kTracks;
        const int n = juce::jmax (1, v.steps);

        if (t == sel && mode != 8)
        {
            g.setColour (th.panel2.withAlpha (0.6f));
            g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), band * 0.9f);
        }

        switch (mode)
        {
            case 1: // Ringe
            {
                g.setColour (v.col.withAlpha (a * 0.25f));
                g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), 1.2f);
                const float dotMax = juce::jmin (band * 0.36f, juce::MathConstants<float>::pi * r / n * 0.8f);
                for (int j = 0; j < n; ++j)
                {
                    const bool hit = patternHit (j, n, v.beats, v.rotate);
                    const auto p = polar (c, r, stepAngle (j, n));
                    const bool cur = j == v.pos;
                    float d = hit ? dotMax : juce::jmax (1.5f, dotMax * 0.35f);
                    if (cur && hit) d *= 1.0f + 0.45f * v.flash;
                    if (cur && hit && v.flash > 0.05f)
                    {
                        g.setColour (v.col.withAlpha (a * 0.35f * v.flash));
                        g.fillEllipse (juce::Rectangle<float> (d * 4.2f, d * 4.2f).withCentre (p));
                    }
                    g.setColour (hit ? v.col.withAlpha (a) : th.dim.withAlpha (a * 0.55f));
                    g.fillEllipse (juce::Rectangle<float> (d * 2, d * 2).withCentre (p));
                    if (cur)
                    {
                        g.setColour (th.text.withAlpha (a));
                        g.drawEllipse (juce::Rectangle<float> (d * 2 + 5, d * 2 + 5).withCentre (p), 1.5f);
                    }
                }
                break;
            }
            case 2: // Polygon
            {
                juce::Path poly;
                int count = 0;
                for (int j = 0; j < n; ++j)
                    if (patternHit (j, n, v.beats, v.rotate))
                    {
                        const auto p = polar (c, r, stepAngle (j, n));
                        if (count++ == 0) poly.startNewSubPath (p); else poly.lineTo (p);
                    }
                if (count > 1) poly.closeSubPath();
                g.setColour (v.col.withAlpha (a * (0.06f + 0.12f * v.flash)));
                g.fillPath (poly);
                g.setColour (v.col.withAlpha (a * 0.9f));
                g.strokePath (poly, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));
                for (int j = 0; j < n; ++j)
                {
                    const bool hit = patternHit (j, n, v.beats, v.rotate);
                    const auto p = polar (c, r, stepAngle (j, n));
                    const float d = hit ? 4.0f + (j == v.pos ? 4.0f * v.flash : 0.0f) : 1.5f;
                    g.setColour (j == v.pos ? th.text.withAlpha (a) : (hit ? v.col.withAlpha (a) : th.dim.withAlpha (a * 0.4f)));
                    g.fillEllipse (juce::Rectangle<float> (d * 2, d * 2).withCentre (p));
                }
                break;
            }
            case 3: // Speichen
            {
                const float r1 = r - band * 0.42f, r2 = r + band * 0.42f;
                for (int j = 0; j < n; ++j)
                {
                    const bool hit = patternHit (j, n, v.beats, v.rotate);
                    const float an = stepAngle (j, n);
                    if (hit)
                    {
                        g.setColour (v.col.withAlpha (a * (j == v.pos ? 1.0f : 0.8f)));
                        g.drawLine (juce::Line<float> (polar (c, r1, an), polar (c, r2, an)), j == v.pos ? 3.0f + 3.0f * v.flash : 3.0f);
                    }
                    else
                    {
                        g.setColour (th.dim.withAlpha (a * 0.35f));
                        g.drawLine (juce::Line<float> (polar (c, r - 2, an), polar (c, r + 2, an)), 1.0f);
                    }
                }
                if (v.pos >= 0 && v.active)
                {
                    const float an = stepAngle (v.pos, n);
                    g.setColour (th.text.withAlpha (0.25f * a));
                    g.drawLine (juce::Line<float> (polar (c, R0, an), polar (c, r2, an)), 1.0f);
                }
                break;
            }
            case 6: // Spirale
            {
                const float rIn = R0 * 0.9f, rOut = R;
                const float off = juce::MathConstants<float>::twoPi * (float) t / kTracks;
                auto spiralPt = [&] (float f) {
                    const float an = -juce::MathConstants<float>::halfPi + off + f * juce::MathConstants<float>::pi * 1.4f;
                    return polar (c, rIn + (rOut - rIn) * f, an);
                };
                juce::Path arm;
                for (int i = 0; i <= 60; ++i)
                {
                    const auto p = spiralPt ((float) i / 60.0f);
                    if (i == 0) arm.startNewSubPath (p); else arm.lineTo (p);
                }
                g.setColour (v.col.withAlpha (a * (t == sel ? 0.5f : 0.22f)));
                g.strokePath (arm, juce::PathStrokeType (t == sel ? 2.0f : 1.2f));
                for (int j = 0; j < n; ++j)
                {
                    const bool hit = patternHit (j, n, v.beats, v.rotate);
                    const auto p = spiralPt ((float) j / (float) juce::jmax (1, n - 1));
                    float d = hit ? 3.0f + 3.0f * (float) j / n : 1.2f;
                    if (j == v.pos) d += 3.0f * v.flash;
                    g.setColour (j == v.pos ? th.text.withAlpha (a) : (hit ? v.col.withAlpha (a) : th.dim.withAlpha (a * 0.45f)));
                    g.fillEllipse (juce::Rectangle<float> (d * 2, d * 2).withCentre (p));
                }
                break;
            }
            case 7: // Dichte – annular segments
            {
                const float r1 = r - band * 0.45f, r2 = r + band * 0.45f;
                const float gap = juce::jmin (0.04f, 0.25f * juce::MathConstants<float>::twoPi / n);
                for (int j = 0; j < n; ++j)
                {
                    const bool hit = patternHit (j, n, v.beats, v.rotate);
                    const float a0 = juce::MathConstants<float>::twoPi * (float) j / n + gap * 0.5f;
                    const float a1 = juce::MathConstants<float>::twoPi * (float) (j + 1) / n - gap * 0.5f;
                    juce::Path seg;
                    seg.addPieSegment (juce::Rectangle<float> (r2 * 2, r2 * 2).withCentre (c), a0, a1, r1 / r2);
                    float alpha = hit ? 0.7f : 0.08f;
                    if (j == v.pos) alpha = hit ? 0.75f + 0.25f * v.flash : 0.35f;
                    g.setColour ((j == v.pos ? v.col.brighter (0.4f) : v.col).withAlpha (a * alpha));
                    g.fillPath (seg);
                }
                break;
            }
            case 8: // Blüte – petals for every hit
            {
                const float len = R0 + (R - R0) * (0.35f + 0.65f * ((float) (t + 1) / kTracks));
                for (int j = 0; j < n; ++j)
                {
                    if (! patternHit (j, n, v.beats, v.rotate)) continue;
                    const float an = stepAngle (j, n);
                    const bool cur = j == v.pos;
                    const float l = len * (cur ? 1.0f + 0.08f * v.flash : 1.0f);
                    const float wdt = juce::jmin (0.45f, 2.2f / (float) juce::jmax (2, v.beats));
                    juce::Path petal;
                    const auto tip = polar (c, l, an);
                    petal.startNewSubPath (polar (c, R0 * 0.5f, an));
                    petal.quadraticTo (polar (c, l * 0.62f, an - wdt), tip);
                    petal.quadraticTo (polar (c, l * 0.62f, an + wdt), polar (c, R0 * 0.5f, an));
                    petal.closeSubPath();
                    g.setColour (v.col.withAlpha (a * (cur ? 0.25f + 0.5f * v.flash : 0.13f)));
                    g.fillPath (petal);
                    g.setColour (v.col.withAlpha (a * (t == sel ? 0.9f : 0.5f)));
                    g.strokePath (petal, juce::PathStrokeType (t == sel ? 1.6f : 1.0f));
                }
                break;
            }
            default: break;
        }
    }
    centreText();
}

// ============================================================================================ LaneEditor
void LaneEditor::paint (juce::Graphics& g)
{
    const auto& th = ed.theme();
    auto b = getLocalBounds().toFloat();
    g.setColour (th.panel);
    g.fillRoundedRectangle (b, 8.0f);
    b = b.reduced (8.0f);
    const int t = ed.selectedTrack(), tg = ed.selectedTarget();
    const auto lane = proc.getLane (t, tg);
    const auto& vals = dragging ? work : lane.v;
    const auto col = th.track[(size_t) t];
    const float cy = b.getCentreY();
    const float bw = b.getWidth() / kMaxSteps;

    g.setColour (th.panel2);
    g.drawHorizontalLine ((int) cy, b.getX(), b.getRight());
    int idx = -1;
    if (proc.transportRunning && lane.on)
        idx = laneIndex (lane, proc.vis[(size_t) t].k.load(), t, tg);

    for (int i = 0; i < kMaxSteps; ++i)
    {
        const bool inLane = i < lane.length;
        auto col_ = juce::Rectangle<float> (b.getX() + bw * i, b.getY(), bw, b.getHeight()).reduced (juce::jmin (1.5f, bw * 0.12f), 0);
        if (i % 4 == 0)
        {
            g.setColour (th.panel2.withAlpha (inLane ? 0.5f : 0.2f));
            g.fillRect (col_.withWidth (1.0f).translated (-1.0f, 0));
        }
        const float v = vals[(size_t) i];
        const float h = v * b.getHeight() * 0.5f;
        auto bar = v >= 0 ? juce::Rectangle<float> (col_.getX(), cy - h, col_.getWidth(), h)
                          : juce::Rectangle<float> (col_.getX(), cy, col_.getWidth(), -h);
        float alpha = inLane ? (lane.on ? 0.85f : 0.4f) : 0.12f;
        if (i == idx) alpha = 1.0f;
        g.setColour ((i == idx ? col.brighter (0.5f) : col).withAlpha (alpha));
        g.fillRect (bar.getHeight() < 1.5f ? bar.withHeight (1.5f).withY (cy - 0.75f) : bar);
        if (i == idx)
        {
            g.setColour (th.text.withAlpha (0.25f));
            g.fillRect (col_.withHeight (3.0f).withY (b.getBottom() - 3.0f));
        }
    }
    // lane end marker
    g.setColour (th.text.withAlpha (0.6f));
    const float xe = b.getX() + bw * lane.length;
    g.drawVerticalLine ((int) xe, b.getY(), b.getBottom());
    g.setFont (font (11.0f));
    g.setColour (th.dim);
    g.drawText ("+100 %", b.withHeight (14), juce::Justification::topRight);
    g.drawText ("-100 %", b.withTrimmedTop (b.getHeight() - 14), juce::Justification::bottomRight);
}

void LaneEditor::drawAt (juce::Point<float> p, bool reset)
{
    auto b = getLocalBounds().toFloat().reduced (8.0f);
    const float bw = b.getWidth() / kMaxSteps;
    auto setOne = [&] (juce::Point<float> q) {
        const int i = (int) std::floor ((q.x - b.getX()) / bw);
        if (! juce::isPositiveAndBelow (i, kMaxSteps)) return;
        float v = (b.getCentreY() - q.y) / (b.getHeight() * 0.5f);
        v = juce::jlimit (-1.0f, 1.0f, v);
        if (std::abs (v) < 0.04f || reset) v = 0.0f;
        work[(size_t) i] = v;
    };
    // interpolate between drag points so fast strokes leave no gaps
    const int stepsN = juce::jmax (1, (int) (std::abs (p.x - lastPt.x) / juce::jmax (1.0f, bw * 0.5f)));
    for (int s = 1; s <= stepsN; ++s)
        setOne (lastPt + (p - lastPt) * ((float) s / stepsN));
    lastPt = p;
}

void LaneEditor::mouseDown (const juce::MouseEvent& e)
{
    proc.undo.beginNewTransaction ("Lane zeichnen");
    work = proc.getLane (ed.selectedTrack(), ed.selectedTarget()).v;
    dragging = true;
    lastPt = e.position;
    drawAt (e.position, e.mods.isRightButtonDown() || e.mods.isAltDown());
    repaint();
}

void LaneEditor::mouseDrag (const juce::MouseEvent& e)
{
    drawAt (e.position, e.mods.isRightButtonDown() || e.mods.isAltDown());
    repaint();
}

void LaneEditor::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
    const int t = ed.selectedTrack(), tg = ed.selectedTarget();
    proc.setLaneValues (t, tg, work);
    if (! proc.getLane (t, tg).on)
        proc.laneTree (t, tg).setProperty (idOn, true, &proc.undo);
    repaint();
}

// ============================================================================================ MidiDragArea
void MidiDragArea::paint (juce::Graphics& g)
{
    const auto& th = ed.theme();
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (isMouseOver() ? th.accent.withAlpha (0.25f) : th.panel2);
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (th.accent);
    const float dashes[] = { 4.0f, 3.0f };
    juce::Path p;
    p.addRoundedRectangle (b.reduced (1.0f), 6.0f);
    juce::Path dashed;
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, p, dashes, 2);
    g.fillPath (dashed);
    g.setColour (th.text);
    g.setFont (font (13.0f, true));
    g.drawText ("MIDI in die DAW ziehen", b.removeFromTop (b.getHeight() * 0.6f), juce::Justification::centredBottom);
    g.setFont (font (11.0f));
    g.setColour (th.dim);
    g.drawText (juce::String (ed.exportBars()) + " Takte, 960 PPQ", b, juce::Justification::centredTop);
}

void MidiDragArea::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true;
    const auto file = proc.exportMidiToTemp (ed.exportBars());
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                               [safe = juce::Component::SafePointer<MidiDragArea> (this)] {
                                                                   if (safe) safe->dragging = false;
                                                               });
}

// ============================================================================================ MatrixView
namespace
{
constexpr int kNameW = 96;
const char* const matrixCols[] = { "An", "M", "S", "Steps", "Beats", "Rotate", "Chance", "Vel", "Swing", "Pitch", "Chord", "Clock", "Gate", "Kanal" };
const char* const matrixIds[] = { "active", "mute", "solo", "steps", "beats", "rotate", "chance", "vel", "swing", "pitch", "chord", "clock", "gate", "chan" };
} // namespace

MatrixView::MatrixView (KreiselProcessor& p, KreiselEditor& e) : proc (p), ed (e)
{
    for (int t = 0; t < kTracks; ++t)
        for (int c = 0; c < kCols; ++c)
        {
            const auto id = pid (t, matrixIds[c]);
            if (c < 3)
            {
                auto* b = new juce::TextButton (c == 0 ? "" : juce::String (matrixCols[c]));
                b->setClickingTogglesState (true);
                b->setColour (juce::TextButton::buttonOnColourId, c == 0 ? ed.theme().track[(size_t) t] : (c == 1 ? juce::Colour (0xffe05555) : juce::Colour (0xffe0c040)));
                cells.add (b);
                addAndMakeVisible (b);
                bAtt.add (new APVTS::ButtonAttachment (proc.apvts, id, *b));
            }
            else
            {
                auto* s = new juce::Slider (juce::Slider::LinearBar, juce::Slider::TextBoxLeft);
                s->setColour (juce::Slider::trackColourId, ed.theme().track[(size_t) t]);
                s->setColour (juce::Slider::textBoxTextColourId, ed.theme().text);
                s->setTextBoxIsEditable (false);
                cells.add (s);
                addAndMakeVisible (s);
                sAtt.add (new APVTS::SliderAttachment (proc.apvts, id, *s));
            }
        }
}

void MatrixView::paint (juce::Graphics& g)
{
    const auto& th = ed.theme();
    g.setColour (th.panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
    auto b = getLocalBounds().reduced (10);
    const float cw = (b.getWidth() - (float) kNameW) / kCols;
    g.setFont (font (11.5f, true));
    g.setColour (th.dim);
    for (int c = 0; c < kCols; ++c)
        g.drawText (matrixCols[c], juce::Rectangle<float> (b.getX() + kNameW + cw * c, (float) b.getY(), cw, 20), juce::Justification::centred);
    const float rh = (b.getHeight() - 26.0f) / kTracks;
    for (int t = 0; t < kTracks; ++t)
    {
        auto row = juce::Rectangle<float> ((float) b.getX(), b.getY() + 26 + rh * t, (float) b.getWidth(), rh);
        if (t == ed.selectedTrack()) { g.setColour (th.panel2.withAlpha (0.5f)); g.fillRoundedRectangle (row, 5); }
        g.setColour (th.track[(size_t) t]);
        g.setFont (font (14.0f, true));
        g.drawText (juce::String (t + 1), row.withWidth (22), juce::Justification::centred);
        g.setFont (font (12.0f, true));
        g.drawFittedText (proc.trackName (t), row.withTrimmedLeft (24).withWidth (kNameW - 26).toNearestInt(), juce::Justification::centredLeft, 1);
    }
}

void MatrixView::resized()
{
    auto b = getLocalBounds().reduced (10);
    const float cw = (b.getWidth() - (float) kNameW) / kCols;
    const float rh = (b.getHeight() - 26.0f) / kTracks;
    for (int t = 0; t < kTracks; ++t)
        for (int c = 0; c < kCols; ++c)
            cells[t * kCols + c]->setBounds (juce::Rectangle<float> (b.getX() + kNameW + cw * c, b.getY() + 26 + rh * t, cw, rh)
                                                 .reduced (2.0f, rh * 0.18f).toNearestInt());
}

// ============================================================================================ RandomPanel
namespace
{
struct RandRange { double lo, hi, step; };
const RandRange randRanges[kRandTargets] = { { 1, 64, 1 }, { 0, 64, 1 }, { 0, 63, 1 }, { 0, 100, 1 }, { 1, 127, 1 },
                                             { 0, 75, 1 }, { 0, 127, 1 }, { 0, kNumChords - 1, 1 }, { 0, kNumDivisions - 1, 1 }, { 5, 200, 1 } };
juce::String randValueText (int i, double v)
{
    const int iv = juce::roundToInt (v);
    if (i == tPitch) return midiNoteName (iv);
    if (i == tChord) return juce::String::fromUTF8 (chords[juce::jlimit (0, kNumChords - 1, iv)].name);
    if (i == tClock) return divisions[juce::jlimit (0, kNumDivisions - 1, iv)].name;
    if (i == tChance || i == tSwing || i == kTargets) return juce::String (iv) + " %";
    return juce::String (iv);
}
} // namespace

RandomPanel::RandomPanel (KreiselProcessor& p, KreiselEditor& e) : proc (p), ed (e)
{
    for (int i = 0; i < kRandTargets; ++i)
    {
        auto* tb = enables.add (new juce::ToggleButton (randNames[i]));
        addAndMakeVisible (tb);
        tb->onClick = [this, i, tb] {
            proc.randTree().setProperty (juce::String (randNames[i]) + "_on", tb->getToggleState(), nullptr);
        };
        auto* s = ranges.add (new juce::Slider (juce::Slider::TwoValueHorizontal, juce::Slider::NoTextBox));
        s->setRange (randRanges[i].lo, randRanges[i].hi, randRanges[i].step);
        s->setColour (juce::Slider::trackColourId, ed.theme().accent);
        addAndMakeVisible (s);
        s->onValueChange = [this, i, s] {
            auto rt = proc.randTree();
            rt.setProperty (juce::String (randNames[i]) + "_min", s->getMinValue(), nullptr);
            rt.setProperty (juce::String (randNames[i]) + "_max", s->getMaxValue(), nullptr);
            repaint();
        };
    }
    for (auto* b : { &rollTrack, &rollAll, &close }) addAndMakeVisible (b);
    rollTrack.onClick = [this] { proc.randomise (ed.selectedTrack()); };
    rollAll.onClick = [this] { proc.randomise (-1); };
    close.onClick = [this] { ed.showRandomPanel (false); };
    refresh();
}

void RandomPanel::refresh()
{
    auto rt = proc.randTree();
    for (int i = 0; i < kRandTargets; ++i)
    {
        const juce::String n (randNames[i]);
        enables[i]->setToggleState ((bool) rt[n + "_on"], juce::dontSendNotification);
        ranges[i]->setMinAndMaxValues ((double) rt[n + "_min"], (double) rt[n + "_max"], juce::dontSendNotification);
    }
}

void RandomPanel::paint (juce::Graphics& g)
{
    const auto& th = ed.theme();
    g.fillAll (th.bg.withAlpha (0.7f));
    auto box = getLocalBounds().withSizeKeepingCentre (620, 520).toFloat();
    g.setColour (th.panel);
    g.fillRoundedRectangle (box, 12.0f);
    g.setColour (th.accent);
    g.drawRoundedRectangle (box, 12.0f, 1.5f);
    g.setColour (th.text);
    g.setFont (font (18.0f, true));
    g.drawText ("Zufallsgenerator", box.reduced (20, 14).withHeight (24), juce::Justification::centredLeft);
    g.setFont (font (12.0f));
    g.setColour (th.dim);
    g.drawText (juce::String::fromUTF8 ("Aktivierte Parameter werden innerhalb von Min/Max gewürfelt (rückgängig machbar). Spur ") + juce::String (ed.selectedTrack() + 1) + juce::String::fromUTF8 (" ist gewählt."),
                box.reduced (20, 14).withTrimmedTop (26).withHeight (18), juce::Justification::centredLeft);
    for (int i = 0; i < kRandTargets; ++i)
    {
        auto r = ranges[i]->getBounds();
        g.setColour (th.text);
        g.setFont (font (12.5f));
        g.drawText (randValueText (i, ranges[i]->getMinValue()) + juce::String::fromUTF8 ("  –  ") + randValueText (i, ranges[i]->getMaxValue()),
                    juce::Rectangle<int> (r.getRight() + 8, r.getY(), 150, r.getHeight()), juce::Justification::centredLeft);
    }
}

void RandomPanel::resized()
{
    auto box = getLocalBounds().withSizeKeepingCentre (620, 520).reduced (20, 14);
    box.removeFromTop (54);
    for (int i = 0; i < kRandTargets; ++i)
    {
        auto row = box.removeFromTop (38);
        enables[i]->setBounds (row.removeFromLeft (120).reduced (0, 6));
        ranges[i]->setBounds (row.removeFromLeft (280).reduced (6, 6));
    }
    auto bottom = box.removeFromBottom (34);
    close.setBounds (bottom.removeFromRight (110));
    bottom.removeFromRight (10);
    rollAll.setBounds (bottom.removeFromRight (170));
    bottom.removeFromRight (10);
    rollTrack.setBounds (bottom.removeFromRight (130));
}

// ============================================================================================ HelpPanel
HelpPanel::HelpPanel (KreiselEditor& e) : ed (e)
{
    text.setMultiLine (true);
    text.setReadOnly (true);
    text.setScrollbarsShown (true);
    text.setFont (font (14.0f));
    guide = (juce::String::fromUTF8 (
        "KREISEL – euklidischer MIDI-Sequencer\n\n"
        "Kreisel hat eingebaute Klänge (synthetisches Drum-Kit nach GM-Belegung, Bass, Lead, Pluck, Fläche, Keys, Glocke, Orgel) "
        "und sendet gleichzeitig MIDI. „Klang“ wählt den Klang pro Spur – „Automatisch“ nimmt ihn aus dem MIDI-Kanal "
        "(10 = Drums nach Note, 1 = Akkorde/Fläche, 2 = Bass, 3 = Lead). „Interne Klänge“ schaltet alle eingebauten Klänge ab, "
        "wenn nur externe Instrumente spielen sollen.\n\n"
        "Externe Instrumente: Instrument auf eine eigene Spur legen und als MIDI-Eingang die Kreisel-Spur wählen "
        "(Studio One: Instrumentenspur-Eingang = Kreisel, Kanal = MIDI Output N, Mithören an; Ableton: MIDI From = Kreisel). "
        "Ohne DAW-Wiedergabe startet und stoppt „▶ Start“ oben links Kreisels eigenen Lauf (Tempo unter „Global“); läuft die DAW, steuert sie den Transport.\n\n"
        "PLUGINS\n"
        "Über „Plugins“ VST3-Ordner eintragen und scannen. Rechtsklick auf einen Spur-Tab lädt pro Spur ein Instrument "
        "(ersetzt den eingebauten Klang) und einen Effekt – im Plugin wie in der Standalone-Fassung. Die Plugin-Fenster öffnen "
        "sich über dasselbe Menü; Plugins und ihre Einstellungen werden mit dem Projekt gespeichert.\n\n"
        "LUPE\n"
        "Rechtsklick irgendwo (außer auf Spur-Tabs und Lanes) schaltet eine Lupe ein, die der Maus folgt; Vergrößerung und Größe im selben Menü.\n\n"
        "SPUREN\n"
        "Acht Spuren mit eigenem Takt (Clock) und eigener Länge – unterschiedliche Längen driften gegeneinander und treffen sich wieder. "
        "Die Spurnamen ergeben sich aus Klang, Drum-Note oder Kanal; Doppelklick auf einen Spur-Tab benennt ihn um.\n"
        "• Steps: Länge des Musters (1–64)\n"
        "• Beats: Anzahl der gleichmäßig verteilten Schläge (euklidischer Algorithmus)\n"
        "• Rotate: verschiebt das Muster\n"
        "• Chance: Wahrscheinlichkeit, dass ein Schlag erklingt\n"
        "• Velocity, Swing (verzögert jeden zweiten Step), Gate (Notenlänge in % eines Steps)\n"
        "• Pitch: Grundnote; mit „Skala quantisieren“ auf die gewählte Skala gezogen (Kanal 10 = Drums bleibt unverändert)\n"
        "• Chord: Akkord pro Schlag – „Skala …“-Akkorde werden aus der Skala geschichtet\n"
        "• Clock: Notenwert eines Steps (auch punktiert und triolisch)\n"
        "• Kanal: MIDI-Kanal 1–16 (Drums klassisch Kanal 10)\n"
        "• Pegel und Klang: Lautstärke und Klang der eingebauten Klangerzeugung\n\n"
        "MODULATION\n"
        "Jede Spur hat 9 Modulations-Lanes (Steps, Beats, Rotate, Chance, Velocity, Swing, Pitch, Chord, Clock) – insgesamt 72. "
        "Eine Lane hat 2–64 Schritte, läuft vorwärts, rückwärts, Ping-Pong oder zufällig und rückt mit jedem Step der Spur weiter. "
        "Zeichnen mit der Maus, Rechtsklick/Alt setzt auf 0. Tiefe skaliert die Lane; die Pitch-Lane bewegt sich bei aktiver Quantisierung in Skalenstufen.\n\n"
        "ZUFALL\n"
        "Für jeden Parameter einen Bereich (Min/Max) festlegen und die gewählte oder alle aktiven Spuren würfeln.\n\n"
        "MIDI-EXPORT\n"
        "1, 2, 4, 8 oder 16 Takte mit 960 PPQ – das Feld direkt auf eine DAW-Spur ziehen oder als .mid speichern. "
        "Jede aktive Spur wird eine eigene MIDI-Spur.\n\n"
        "ANSICHTEN & THEMEN\n"
        "Neun Darstellungen (Band, Ringe, Polygon, Speichen, Raster, Puls, Spirale, Dichte, Blüte), Klick auf Ring/Zeile wählt die Spur. "
        "Matrix zeigt alle Spuren als Tabelle. Sechs Farbthemen, jeweils dunkel oder hell.\n\n"
        "RÜCKGÄNGIG\n"
        "Die letzten 40 Änderungen lassen sich rückgängig machen – auch Preset-Wechsel und Würfeln. Alle Spurparameter sind in der DAW automatisierbar.\n\n"
        "PRESETS\n"
        "Über 200 Werks-Presets in 26 Genres (Untermenüs), dazu „Demos“ ganz unten: längere, sich entwickelnde Stücke mit allen acht Spuren; eigene Presets liegen in Dokumente\\Kreisel\\Presets (*.kreisel)."));
    info = juce::String::fromUTF8 (
        "KREISEL " JucePlugin_VersionString " – euklidischer Sequencer mit eingebauten Klängen und VST3-Slots\n"
        "von Un(d)AbtanzBAR\n\n"
        "LINKS\n"
        "Website: www.undabtanzbar.de\n"
        "GitHub: github.com/Yakobo1970/Kreisel\n\n"
        "ENTWICKLUNG\n"
        "Idee, Konzept und Regie: Kay 'Yakobo' Fiedler.\n"
        "Umgesetzt mit Claude (Anthropic) als KI-Programmierassistent für C++/JUCE-Code, Klangerzeugung, Preset-Bibliothek "
        "und Tests.\n\n"
        "TECHNIK\n"
        "JUCE (VST3 und Standalone), eigener Sequencer-Kern mit euklidischer Verteilung, 72 Modulations-Lanes, "
        "synthetisches Drum-Kit nach GM-Belegung und einfache Synth-Stimmen. VST3-Instrumente und -Effekte werden "
        "pro Spur geladen; die Plugin-Liste liegt in %APPDATA%\\Kreisel.\n\n"
        "UNTERSTÜTZEN\n"
        "Kreisel ist kostenlos. Wer die Entwicklung unterstützen möchte, kann über den Knopf unten eine kleine Spende "
        "per PayPal schicken – vielen Dank!");
    for (auto* b : { &tabGuide, &tabInfo })
    {
        b->setRadioGroupId (303);
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonOnColourId, ed.theme().accent);
        addAndMakeVisible (b);
    }
    tabGuide.onClick = [this] { showPage (0); };
    tabInfo.onClick = [this] { showPage (1); };
    donate.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffd6336c));
    donate.setTooltip (kDonateUrl);
    website.setTooltip (kWebsiteUrl);
    github.setTooltip (kGithubUrl);
    donate.onClick = [] { juce::URL (kDonateUrl).launchInDefaultBrowser(); };
    website.onClick = [] { juce::URL (kWebsiteUrl).launchInDefaultBrowser(); };
    github.onClick = [] { juce::URL (kGithubUrl).launchInDefaultBrowser(); };
    for (auto* b : { &donate, &website, &github }) addAndMakeVisible (b);
    addAndMakeVisible (text);
    addAndMakeVisible (close);
    close.onClick = [this] { ed.showHelp (false); };
    showPage (0);
}

void HelpPanel::showPage (int page)
{
    (page == 0 ? tabGuide : tabInfo).setToggleState (true, juce::dontSendNotification);
    text.setText (page == 0 ? guide : info, false);
    text.moveCaretToTop (false);
}

void HelpPanel::paint (juce::Graphics& g)
{
    const auto& th = ed.theme();
    g.fillAll (th.bg.withAlpha (0.7f));
    auto box = getLocalBounds().withSizeKeepingCentre (760, 640).toFloat();
    g.setColour (th.panel);
    g.fillRoundedRectangle (box, 12.0f);
    g.setColour (th.accent);
    g.drawRoundedRectangle (box, 12.0f, 1.5f);
}

void HelpPanel::resized()
{
    auto box = getLocalBounds().withSizeKeepingCentre (760, 640).reduced (18);
    auto tabs = box.removeFromTop (30);
    tabGuide.setBounds (tabs.removeFromLeft (120));
    tabs.removeFromLeft (6);
    tabInfo.setBounds (tabs.removeFromLeft (120));
    box.removeFromTop (10);
    auto bottom = box.removeFromBottom (32);
    close.setBounds (bottom.removeFromRight (110));
    donate.setBounds (bottom.removeFromLeft (190));
    bottom.removeFromLeft (8);
    website.setBounds (bottom.removeFromLeft (170));
    bottom.removeFromLeft (8);
    github.setBounds (bottom.removeFromLeft (90));
    box.removeFromBottom (10);
    text.setBounds (box);
}

// ============================================================================================ Editor
KreiselEditor::KreiselEditor (KreiselProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    auto ui = proc.uiTree();
    look.setTheme (ui.getProperty ("light", false) ? lightVariant (themes()[(size_t) juce::jlimit (0, 5, (int) ui.getProperty ("theme", 0))])
                                                   : themes()[(size_t) juce::jlimit (0, 5, (int) ui.getProperty ("theme", 0))]);
    setLookAndFeel (&look);
    selTrack = juce::jlimit (0, kTracks - 1, (int) ui.getProperty ("track", 0));
    selTarget = juce::jlimit (0, kTargets - 1, (int) ui.getProperty ("target", 0));

    // ---- header
    for (auto* b : { &prevPreset, &nextPreset, &savePreset, &loadPreset, &undoBtn, &redoBtn, &randomBtn, &helpBtn, &matrixBtn })
        addAndMakeVisible (b);
    addAndMakeVisible (presetBox);
    presetBox.setTextWhenNothingSelected ("Init");
    refreshPresetList();
    presetBox.onChange = [this] {
        const int id = presetBox.getSelectedId();
        if (id <= 0) return;
        if (id < 1000) proc.loadFactoryPreset (id - 1);
        else
        {
            juce::Array<juce::File> files;
            KreiselProcessor::userPresetFolder().findChildFiles (files, juce::File::findFiles, false, "*.kreisel");
            files.sort();
            if (juce::isPositiveAndBelow (id - 1000, files.size())) proc.loadUserPreset (files[id - 1000]);
        }
    };
    prevPreset.onClick = [this] { stepPreset (-1); };
    nextPreset.onClick = [this] { stepPreset (1); };
    savePreset.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Preset speichern",
            KreiselProcessor::userPresetFolder().getChildFile (proc.currentPresetName() + ".kreisel"), "*.kreisel");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc) {
                                  auto f = fc.getResult();
                                  if (f == juce::File()) return;
                                  proc.saveUserPreset (f.withFileExtension ("kreisel"));
                                  refreshPresetList();
                              });
    };
    loadPreset.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("Preset laden", KreiselProcessor::userPresetFolder(), "*.kreisel");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) {
                                  auto f = fc.getResult();
                                  if (f.existsAsFile()) proc.loadUserPreset (f);
                              });
    };
    undoBtn.onClick = [this] { proc.undo.undo(); };
    redoBtn.onClick = [this] { proc.undo.redo(); };
    randomBtn.onClick = [this] { showRandomPanel (randomPanel == nullptr); };
    helpBtn.onClick = [this] { showHelp (helpPanel == nullptr); };
    helpBtn.setTooltip (juce::String::fromUTF8 ("Anleitung und Info (Links, Spende)"));
    matrixBtn.setClickingTogglesState (true);
    matrixBtn.setToggleState ((bool) ui.getProperty ("matrix", false), juce::dontSendNotification);
    matrixBtn.onClick = [this] {
        proc.uiTree().setProperty ("matrix", matrixBtn.getToggleState(), nullptr);
        if (matrixBtn.getToggleState()) { matrix = std::make_unique<MatrixView> (proc, *this); addAndMakeVisible (*matrix); }
        else matrix.reset();
        visual.setVisible (matrix == nullptr);
        resized();
    };

    for (int i = 0; i < kNumVisuals; ++i) visualBox.addItem (juce::String::fromUTF8 (visualNames[i]), i + 1);
    visual.mode = juce::jlimit (0, kNumVisuals - 1, (int) ui.getProperty ("visual", 1));
    visualBox.setSelectedId (visual.mode + 1, juce::dontSendNotification);
    visualBox.onChange = [this] { visual.mode = visualBox.getSelectedId() - 1; proc.uiTree().setProperty ("visual", visual.mode, nullptr); };
    addAndMakeVisible (visualBox);
    for (int i = 0; i < (int) themes().size(); ++i) themeBox.addItem (themes()[(size_t) i].name, i + 1);
    themeBox.setSelectedId ((int) ui.getProperty ("theme", 0) + 1, juce::dontSendNotification);
    themeBox.onChange = [this] { proc.uiTree().setProperty ("theme", themeBox.getSelectedId() - 1, nullptr); applyTheme(); };
    addAndMakeVisible (themeBox);
    lightToggle.setToggleState ((bool) ui.getProperty ("light", false), juce::dontSendNotification);
    lightToggle.onClick = [this] { proc.uiTree().setProperty ("light", lightToggle.getToggleState(), nullptr); applyTheme(); };
    addAndMakeVisible (lightToggle);

    addAndMakeVisible (visual);
    if (matrixBtn.getToggleState()) { matrix = std::make_unique<MatrixView> (proc, *this); addAndMakeVisible (*matrix); visual.setVisible (false); }

    // ---- track tabs
    for (int t = 0; t < kTracks; ++t)
    {
        auto* tab = trackTabs.add (new TrackTab());
        tab->setButtonText ("Spur " + juce::String (t + 1));
        tab->setRadioGroupId (101);
        tab->setClickingTogglesState (true);
        tab->onClick = [this, t] { selectTrack (t); };
        tab->onRename = [this, t] { renameTrack (t); };
        tab->onMenu = [this, t] { showTrackMenu (t); };
        shownNames.add ({});
        addAndMakeVisible (tab);
        auto mk = [&] (juce::OwnedArray<juce::TextButton>& arr, const juce::String& txt, const char* id) {
            auto* b = arr.add (new juce::TextButton (txt));
            b->setClickingTogglesState (true);
            addAndMakeVisible (b);
            tabAtt.add (new APVTS::ButtonAttachment (proc.apvts, pid (t, id), *b));
        };
        mk (activeBtns, "An", "active");
        mk (muteBtns, "M", "mute");
        mk (soloBtns, "S", "solo");
    }

    // ---- knobs
    const char* knobIds[] = { "steps", "beats", "rotate", "chance", "vel", "swing", "pitch", "gate", "chan", "lvl" };
    const char* knobNames[] = { "Steps", "Beats", "Rotate", "Chance", "Velocity", "Swing", "Pitch", "Gate", "Kanal", "Pegel" };
    for (int i = 0; i < 10; ++i)
    {
        auto* k = knobs.add (new Knob());
        k->id = knobIds[i];
        k->s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        k->l.setText (knobNames[i], juce::dontSendNotification);
        k->l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (k->s);
        addAndMakeVisible (k->l);
    }
    for (int i = 0; i < kNumChords; ++i) chordBox.addItem (juce::String::fromUTF8 (chords[i].name), i + 1);
    for (int i = 0; i < kNumDivisions; ++i) clockBox.addItem (divisions[i].name, i + 1);
    for (int i = 0; i < kNumSounds; ++i)
    {
        if (i == sKit || i == sAcid) soundBox.addSeparator();
        soundBox.addItem (juce::String::fromUTF8 (soundNames[i]), i + 1);
    }
    chordLbl.setText ("Chord", juce::dontSendNotification);
    clockLbl.setText ("Clock", juce::dontSendNotification);
    soundLbl.setText ("Klang", juce::dontSendNotification);
    for (auto* c : { &chordBox, &clockBox, &soundBox }) addAndMakeVisible (c);
    for (auto* l : { &chordLbl, &clockLbl, &soundLbl }) { l->setJustificationType (juce::Justification::centredLeft); addAndMakeVisible (l); }

    // ---- global
    for (int i = 0; i < 12; ++i) rootBox.addItem (noteNames[i], i + 1);
    for (int i = 0; i < kNumScales; ++i) scaleBox.addItem (juce::String::fromUTF8 (scales[i].name), i + 1);
    rootAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, "root", rootBox);
    scaleAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, "scale", scaleBox);
    quantAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, "quant", quantToggle);
    runAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, "run", runToggle);
    bpmSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    bpmAtt = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "bpm", bpmSlider);
    internAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, "intern", internToggle);
    volSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    volAtt = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "vol", volSlider);
    volSlider.setTextValueSuffix (" dB");
    globalLbl.setText ("Tonart", juce::dontSendNotification);
    bpmLbl.setText ("Tempo (intern)", juce::dontSendNotification);
    for (auto* c : std::initializer_list<juce::Component*> { &rootBox, &scaleBox, &quantToggle, &runToggle, &bpmSlider, &globalLbl, &bpmLbl,
                                                              &internToggle, &volSlider })
        addAndMakeVisible (c);

    // ---- export
    for (int b : { 1, 2, 4, 8, 16 }) barsBox.addItem (juce::String (b) + (b == 1 ? " Takt" : " Takte"), b);
    barsBox.setSelectedId ((int) ui.getProperty ("bars", 4), juce::dontSendNotification);
    if (barsBox.getSelectedId() == 0) barsBox.setSelectedId (4, juce::dontSendNotification);
    barsBox.onChange = [this] { proc.uiTree().setProperty ("bars", barsBox.getSelectedId(), nullptr); dragArea.repaint(); };
    exportLbl.setText ("MIDI-Export", juce::dontSendNotification);
    saveMidiBtn.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("MIDI speichern",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Kreisel " + proc.currentPresetName() + ".mid"), "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc) {
                                  auto f = fc.getResult();
                                  if (f != juce::File()) writeMidiFile (proc.renderMidi (exportBars()), f.withFileExtension ("mid"));
                              });
    };
    for (auto* c : std::initializer_list<juce::Component*> { &barsBox, &dragArea, &saveMidiBtn, &exportLbl }) addAndMakeVisible (c);

    // ---- lanes
    for (int g = 0; g < kTargets; ++g)
    {
        auto* b = targetTabs.add (new juce::TextButton (targetNames[g]));
        b->setRadioGroupId (202);
        b->setClickingTogglesState (true);
        b->onClick = [this, g] { selectTarget (g); };
        addAndMakeVisible (b);
    }
    laneLen.setSliderStyle (juce::Slider::LinearHorizontal);
    laneLen.setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 20);
    laneLen.setRange (2, kMaxSteps, 1);
    laneDepth.setSliderStyle (juce::Slider::LinearHorizontal);
    laneDepth.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 20);
    laneDepth.setRange (0, 100, 1);
    laneDepth.setTextValueSuffix (" %");
    laneLenLbl.setText (juce::String::fromUTF8 ("Länge"), juce::dontSendNotification);
    laneDepthLbl.setText ("Tiefe", juce::dontSendNotification);
    for (int i = 0; i < kNumLaneDirs; ++i) laneDir.addItem (juce::String::fromUTF8 (laneDirNames[i]), i + 1);
    laneOn.onClick = [this] { setLaneProp (idOn, laneOn.getToggleState()); };
    laneLen.onValueChange = [this] { setLaneProp (idLen, (int) laneLen.getValue()); };
    laneDepth.onValueChange = [this] { setLaneProp (idDepth, laneDepth.getValue() / 100.0); };
    laneDir.onChange = [this] { setLaneProp (idDir, laneDir.getSelectedId() - 1); };
    laneRandom.onClick = [this] { proc.randomiseLane (selTrack, selTarget); };
    laneClear.onClick = [this] {
        proc.undo.beginNewTransaction ("Lane leeren");
        proc.setLaneValues (selTrack, selTarget, {});
    };
    laneCopyAll.onClick = [this] {
        proc.undo.beginNewTransaction ("Lane kopieren");
        auto src = proc.laneTree (selTrack, selTarget);
        for (int t = 0; t < kTracks; ++t)
            if (t != selTrack)
            {
                auto dst = proc.laneTree (t, selTarget);
                for (auto id : { idOn, idLen, idDir, idDepth, idVals })
                    dst.setProperty (id, src[id], &proc.undo);
            }
    };
    laneTitle.setFont (font (14.0f, true));
    for (auto* c : std::initializer_list<juce::Component*> { &laneOn, &laneLen, &laneDepth, &laneLenLbl, &laneDepthLbl, &laneDir,
                                                              &laneRandom, &laneClear, &laneCopyAll, &laneEditor, &laneTitle })
        addAndMakeVisible (c);

    proc.apvts.state.addListener (this);
    addMouseListener (this, true);

    applyTheme();
    selectTrack (selTrack);
    selectTarget (selTarget);
    refreshPresetLabel();
    refreshTrackNames();

    playBtn.onClick = [this] {
        if (proc.hostTransport) return; // the DAW drives the transport
        proc.undo.beginNewTransaction();
        proc.setParam ("run", proc.getParam ("run") > 0.5f ? 0.0f : 1.0f);
    };
    playBtn.setButtonText (juce::String::fromUTF8 (proc.getParam ("run") > 0.5f ? "■ Stop" : "▶ Start"));
    addAndMakeVisible (playBtn);
    pluginsBtn.onClick = [this] { showSettings (settingsPanel == nullptr); };
    pluginsBtn.setTooltip (juce::String::fromUTF8 ("VST3-Ordner, Scan und Plugin-Liste"));
    addAndMakeVisible (pluginsBtn);

    // right mouse button anywhere (except track tabs and lanes): loupe menu
    loupe = std::make_unique<kr::ui::Loupe>();
    addChildComponent (*loupe);
    catcher = std::make_unique<kr::ui::RightClickCatcher>();
    catcher->onRightClick = [this] { showLoupeMenu(); };
    catcher->passThrough = [this] (juce::Point<int> p) {
        if (settingsPanel != nullptr || helpPanel != nullptr || randomPanel != nullptr) return true;
        for (auto* t : trackTabs) if (t->getBounds().contains (p)) return true;
        return laneEditor.getBounds().contains (p);
    };
    addAndMakeVisible (*catcher);
    raiseOverlays();
    setSize (kW, kH);
    startTimerHz (40);
}

KreiselEditor::~KreiselEditor()
{
    proc.apvts.state.removeListener (this);
    removeMouseListener (this);
    stopTimer();
    setLookAndFeel (nullptr);
}

void KreiselEditor::mouseDown (const juce::MouseEvent&)
{
    // every click starts a new undo step (parameter changes are flushed into the state tree asynchronously)
    proc.undo.beginNewTransaction();
}

int KreiselEditor::exportBars() const { return juce::jmax (1, barsBox.getSelectedId()); }

void KreiselEditor::applyTheme()
{
    auto ui = proc.uiTree();
    const int ti = juce::jlimit (0, (int) themes().size() - 1, (int) ui.getProperty ("theme", 0));
    look.setTheme (ui.getProperty ("light", false) ? lightVariant (themes()[(size_t) ti]) : themes()[(size_t) ti]);
    const auto& th = look.theme();
    for (int t = 0; t < kTracks; ++t)
    {
        trackTabs[t]->setColour (juce::TextButton::buttonOnColourId, th.track[(size_t) t]);
        activeBtns[t]->setColour (juce::TextButton::buttonOnColourId, th.track[(size_t) t].withAlpha (0.8f));
        muteBtns[t]->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe05555));
        soloBtns[t]->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe0c040));
    }
    for (auto* k : knobs)
        k->s.setColour (juce::Slider::rotarySliderFillColourId, th.track[(size_t) selTrack]);
    for (auto* s : { &bpmSlider, &volSlider, &laneLen, &laneDepth })
        s->setColour (juce::Slider::trackColourId, th.accent);
    laneLen.setColour (juce::Slider::trackColourId, th.track[(size_t) selTrack]);
    laneDepth.setColour (juce::Slider::trackColourId, th.track[(size_t) selTrack]);
    for (auto* b : targetTabs) b->setColour (juce::TextButton::buttonOnColourId, th.track[(size_t) selTrack]);
    laneTitle.setColour (juce::Label::textColourId, th.text);
    if (matrix != nullptr)
    {
        matrix = std::make_unique<MatrixView> (proc, *this);
        addAndMakeVisible (*matrix);
        resized();
    }
    if (randomPanel) { randomPanel = std::make_unique<RandomPanel> (proc, *this); addAndMakeVisible (*randomPanel); randomPanel->setBounds (getLocalBounds()); }
    sendLookAndFeelChange();
    repaint();
}

void KreiselEditor::selectTrack (int t)
{
    selTrack = juce::jlimit (0, kTracks - 1, t);
    proc.uiTree().setProperty ("track", selTrack, nullptr);
    trackTabs[selTrack]->setToggleState (true, juce::dontSendNotification);
    attachTrackControls();
    applyTheme();
    refreshLaneControls();
}

void KreiselEditor::selectTarget (int g)
{
    selTarget = juce::jlimit (0, kTargets - 1, g);
    proc.uiTree().setProperty ("target", selTarget, nullptr);
    targetTabs[selTarget]->setToggleState (true, juce::dontSendNotification);
    refreshLaneControls();
}

void KreiselEditor::attachTrackControls()
{
    for (auto* k : knobs)
    {
        k->a.reset();
        k->a = std::make_unique<APVTS::SliderAttachment> (proc.apvts, pid (selTrack, k->id.toRawUTF8()), k->s);
    }
    chordAtt.reset();
    clockAtt.reset();
    soundAtt.reset();
    chordAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, pid (selTrack, "chord"), chordBox);
    clockAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, pid (selTrack, "clock"), clockBox);
    soundAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, pid (selTrack, "snd"), soundBox);
}

void KreiselEditor::setLaneProp (const juce::Identifier& id, const juce::var& v)
{
    if (refreshing) return;
    proc.laneTree (selTrack, selTarget).setProperty (id, v, &proc.undo);
}

void KreiselEditor::refreshLaneControls()
{
    const juce::ScopedValueSetter<bool> svs (refreshing, true);
    const auto l = proc.getLane (selTrack, selTarget);
    // read straight from the tree (atomics may lag a listener call behind)
    auto lt = proc.laneTree (selTrack, selTarget);
    laneOn.setToggleState ((bool) lt[idOn], juce::dontSendNotification);
    laneLen.setValue ((int) lt.getProperty (idLen, 16), juce::dontSendNotification);
    laneDepth.setValue (juce::roundToInt ((double) lt.getProperty (idDepth, 1.0) * 100.0), juce::dontSendNotification);
    laneDir.setSelectedId ((int) lt[idDir] + 1, juce::dontSendNotification);
    juce::ignoreUnused (l);
    juce::String unit;
    switch (selTarget)
    {
        case tPitch: unit = proc.getParam ("quant") > 0.5f ? juce::String::fromUTF8 ("±24 Skalenstufen") : juce::String::fromUTF8 ("±24 Halbtöne"); break;
        case tChance: unit = juce::String::fromUTF8 ("±100 %"); break;
        case tSwing: unit = juce::String::fromUTF8 ("±75 %"); break;
        case tVelocity: unit = juce::String::fromUTF8 ("±127"); break;
        case tChord: unit = juce::String::fromUTF8 ("±18 Akkordtypen"); break;
        case tClock: unit = juce::String::fromUTF8 ("±17 Notenwerte"); break;
        default: unit = juce::String::fromUTF8 ("±") + juce::String (targetRange[selTarget]) + " Steps"; break;
    }
    laneTitle.setText (proc.trackName (selTrack) + juce::String::fromUTF8 ("  ·  ") + targetNames[selTarget] + "-Lane  (" + unit + " bei 100 %)",
                       juce::dontSendNotification);
    for (int g = 0; g < kTargets; ++g)
        targetTabs[g]->setButtonText ((proc.getLane (selTrack, g).on ? juce::String::fromUTF8 ("● ") : juce::String()) + targetNames[g]);
    laneEditor.repaint();
}

void KreiselEditor::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& id)
{
    if (tree.hasType ("LANE"))
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<KreiselEditor> (this)] { if (safe) safe->refreshLaneControls(); });
    }
    else if (tree.hasType ("UI") && id.toString().startsWith ("name"))
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<KreiselEditor> (this)] { if (safe) safe->refreshTrackNames(); });
    }
    else if (tree.hasType ("UI") && id.toString() == "preset")
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<KreiselEditor> (this)] { if (safe) safe->refreshPresetLabel(); });
    }
}

void KreiselEditor::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);
    auto* root = presetBox.getRootMenu();
    for (auto& cat : factoryCategories())
    {
        juce::PopupMenu sub;
        int count = 0;
        for (int i = 0; i < numFactoryPresets(); ++i)
            if (factoryCategory (i) == cat) { sub.addItem (i + 1, factoryName (i)); ++count; }
        root->addSubMenu (cat + "  (" + juce::String (count) + ")", sub);
    }
    juce::Array<juce::File> files;
    KreiselProcessor::userPresetFolder().findChildFiles (files, juce::File::findFiles, false, "*.kreisel");
    files.sort();
    if (! files.isEmpty())
    {
        presetBox.addSectionHeading ("Eigene Presets");
        for (int i = 0; i < files.size(); ++i)
            presetBox.addItem (files[i].getFileNameWithoutExtension(), 1000 + i);
    }
    refreshPresetLabel();
}

void KreiselEditor::refreshPresetLabel()
{
    const auto name = proc.currentPresetName();
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemText (i) == name)
        {
            presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
            return;
        }
    presetBox.setSelectedId (0, juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected (name);
}

void KreiselEditor::stepPreset (int delta)
{
    const int n = presetBox.getNumItems();
    if (n == 0) return;
    int idx = presetBox.getSelectedItemIndex();
    idx = idx < 0 ? (delta > 0 ? 0 : n - 1) : (idx + delta + n) % n;
    presetBox.setSelectedItemIndex (idx, juce::sendNotificationSync);
}

void KreiselEditor::showRandomPanel (bool show)
{
    if (show)
    {
        randomPanel = std::make_unique<RandomPanel> (proc, *this);
        addAndMakeVisible (*randomPanel);
        randomPanel->setBounds (getLocalBounds());
    }
    else randomPanel.reset();
    raiseOverlays();
}

void KreiselEditor::showHelp (bool show)
{
    if (show)
    {
        helpPanel = std::make_unique<HelpPanel> (*this);
        addAndMakeVisible (*helpPanel);
        helpPanel->setBounds (getLocalBounds());
    }
    else helpPanel.reset();
    raiseOverlays();
}

void KreiselEditor::timerCallback()
{
    if (visual.isVisible()) visual.tick();
    if (proc.transportRunning) laneEditor.repaint();
    undoBtn.setEnabled (proc.undo.canUndo());
    redoBtn.setEnabled (proc.undo.canRedo());
    static juce::uint32 lastTabsPaint = 0;
    if (++lastTabsPaint % 8 == 0) repaint (rTracks.expanded (4));
    if (lastTabsPaint % 10 == 0) refreshTrackNames();
    if (lastTabsPaint % 2 == 0) updateLoupe();
    {
        const bool host = proc.hostTransport, running = proc.transportRunning || proc.getParam ("run") > 0.5f;
        const auto txt = host ? juce::String::fromUTF8 ("▶ DAW") : (running ? juce::String::fromUTF8 ("■ Stop") : juce::String::fromUTF8 ("▶ Start"));
        if (playBtn.getButtonText() != txt)
        {
            playBtn.setButtonText (txt);
            playBtn.setTooltip (host ? juce::String::fromUTF8 ("Die DAW steuert gerade Start und Stopp.")
                                     : juce::String::fromUTF8 ("Startet oder stoppt Kreisels eigenen Lauf (Tempo im Bereich „Global“)."));
            playBtn.setColour (juce::TextButton::buttonColourId, running && ! host ? theme().accent : theme().panel2);
        }
    }
}

void KreiselEditor::refreshTrackNames()
{
    bool changed = false;
    for (int t = 0; t < kTracks; ++t)
    {
        const bool active = proc.getParam (pid (t, "active")) > 0.5f;
        const auto n = (active || proc.customTrackName (t).isNotEmpty()) ? proc.trackName (t) : "Spur " + juce::String (t + 1);
        if (n != shownNames[t])
        {
            shownNames.set (t, n);
            trackTabs[t]->setButtonText (n);
            trackTabs[t]->setTooltip ("Spur " + juce::String (t + 1) + juce::String::fromUTF8 (" · Doppelklick: umbenennen"));
            changed = true;
        }
    }
    if (changed)
    {
        refreshLaneControls();
        repaint (rKnobs);
        if (matrix) matrix->repaint();
    }
}

void KreiselEditor::showTrackMenu (int t)
{
    proc.hostSettings->reload();
    const auto instruments = proc.hostSettings->plugins (true);
    const auto effects = proc.hostSettings->plugins (false);
    auto pluginMenu = [] (const juce::Array<juce::PluginDescription>& list, int base, const juce::String& currentId) {
        juce::PopupMenu menu;
        juce::String maker;
        juce::PopupMenu sub;
        auto flush = [&] { if (sub.getNumItems() > 0) menu.addSubMenu (maker.isEmpty() ? "Unbekannt" : maker, sub); sub = {}; };
        for (int i = 0; i < list.size(); ++i)
        {
            const auto& d = list.getReference (i);
            if (d.manufacturerName != maker) { flush(); maker = d.manufacturerName; }
            sub.addItem (base + i, d.name, true, d.createIdentifierString() == currentId);
        }
        flush();
        return menu;
    };

    juce::PopupMenu m;
    m.addSectionHeader ("Spur " + juce::String (t + 1) + juce::String::fromUTF8 (" · ") + proc.trackName (t));
    m.addItem (1, juce::String::fromUTF8 ("Umbenennen …"));
    m.addItem (2, "Automatischer Name", proc.customTrackName (t).isNotEmpty());
    m.addSeparator();
    const bool none = instruments.isEmpty() && effects.isEmpty();
    m.addSectionHeader ("Instrument" + (proc.hasHosted (t, 0) ? juce::String::fromUTF8 (": ") + proc.hostedName (t, 0) : juce::String::fromUTF8 (" (eingebauter Klang)")));
    if (! instruments.isEmpty()) m.addSubMenu ("Instrument laden", pluginMenu (instruments, 1000, proc.hostedId (t, 0)));
    if (proc.hasHosted (t, 0))
    {
        m.addItem (3, juce::String::fromUTF8 ("Instrument-Fenster öffnen"));
        m.addItem (4, juce::String::fromUTF8 ("Instrument entfernen (zurück zum eingebauten Klang)"));
    }
    m.addSeparator();
    m.addSectionHeader ("Effekt" + (proc.hasHosted (t, 1) ? juce::String::fromUTF8 (": ") + proc.hostedName (t, 1) : juce::String()));
    if (! effects.isEmpty()) m.addSubMenu ("Effekt laden", pluginMenu (effects, 20000, proc.hostedId (t, 1)));
    if (proc.hasHosted (t, 1))
    {
        m.addItem (5, juce::String::fromUTF8 ("Effekt-Fenster öffnen"));
        m.addItem (6, "Effekt entfernen");
    }
    m.addSeparator();
    if (none) m.addItem (99, juce::String::fromUTF8 ("Noch keine Plugins gescannt"), false);
    m.addItem (7, juce::String::fromUTF8 ("Plugin-Einstellungen …"));

    juce::Component::SafePointer<KreiselEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (trackTabs[t]), [safe, t, instruments, effects] (int r) {
        if (safe == nullptr || r == 0) return;
        auto& p = safe->proc;
        juce::String err;
        if (r == 1) safe->renameTrack (t);
        else if (r == 2) { p.setTrackName (t, {}); safe->refreshTrackNames(); }
        else if (r == 3) p.showHostedEditor (t, 0);
        else if (r == 4) p.unloadHosted (t, 0);
        else if (r == 5) p.showHostedEditor (t, 1);
        else if (r == 6) p.unloadHosted (t, 1);
        else if (r == 7) safe->showSettings (true);
        else if (r >= 1000 && r < 1000 + instruments.size())
        {
            err = p.loadHosted (t, 0, instruments.getReference (r - 1000));
            if (err.isEmpty()) p.showHostedEditor (t, 0);
        }
        else if (r >= 20000 && r < 20000 + effects.size())
        {
            err = p.loadHosted (t, 1, effects.getReference (r - 20000));
            if (err.isEmpty()) p.showHostedEditor (t, 1);
        }
        if (err.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Plugin", err, "OK", safe.getComponent());
        safe->repaint();
    });
}

void KreiselEditor::showSettings (bool show)
{
    if (show)
    {
        settingsPanel = std::make_unique<SettingsPanel> (proc, *this);
        addAndMakeVisible (*settingsPanel);
        settingsPanel->setBounds (getLocalBounds());
    }
    else settingsPanel.reset();
    raiseOverlays();
}

void KreiselEditor::raiseOverlays()
{
    if (catcher) catcher->toFront (false);
    if (loupe) loupe->toFront (false);
}

void KreiselEditor::showLoupeMenu()
{
    auto ui = proc.uiTree();
    const bool on = ui.getProperty ("loupeOn", false);
    const int zoom = ui.getProperty ("loupeZoom", 250), size = ui.getProperty ("loupeSize", 1);
    juce::PopupMenu m;
    m.addSectionHeader ("Lupe");
    m.addItem (1, on ? "Lupe ausschalten" : "Lupe einschalten");
    m.addSeparator();
    m.addSectionHeader (juce::String::fromUTF8 ("Vergrößerung"));
    for (int i = 0; i < (int) std::size (kr::ui::LoupeSettings::kZooms); ++i)
    {
        const int z = kr::ui::LoupeSettings::kZooms[i];
        m.addItem (10 + i, juce::String (z / 100.0, z % 100 == 0 ? 0 : 1).replaceCharacter ('.', ',') + juce::String::fromUTF8 ("×"), true, zoom == z);
    }
    m.addSeparator();
    m.addSectionHeader (juce::String::fromUTF8 ("Lupengröße"));
    const char* sizes[] = { "klein", "mittel", "groß" };
    for (int i = 0; i < 3; ++i) m.addItem (20 + i, juce::String::fromUTF8 (sizes[i]), true, size == i);
    juce::Component::SafePointer<KreiselEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withMousePosition(), [safe] (int r) {
        if (safe == nullptr || r == 0) return;
        auto ui2 = safe->proc.uiTree();
        if (r == 1) ui2.setProperty ("loupeOn", ! (bool) ui2.getProperty ("loupeOn", false), nullptr);
        else if (r >= 10 && r < 20) { ui2.setProperty ("loupeZoom", kr::ui::LoupeSettings::kZooms[r - 10], nullptr); ui2.setProperty ("loupeOn", true, nullptr); }
        else if (r >= 20) { ui2.setProperty ("loupeSize", r - 20, nullptr); ui2.setProperty ("loupeOn", true, nullptr); }
        safe->updateLoupe();
    });
}

void KreiselEditor::updateLoupe()
{
    if (loupe == nullptr) return;
    auto ui = proc.uiTree();
    const auto mouse = getMouseXYRelative();
    if (! (bool) ui.getProperty ("loupeOn", false) || ! getLocalBounds().contains (mouse) || ! isShowing())
    {
        loupe->setVisible (false);
        return;
    }
    const float zoom = (float) (int) ui.getProperty ("loupeZoom", 250) / 100.0f;
    const auto size = kr::ui::LoupeSettings::windowSize ((int) ui.getProperty ("loupeSize", 1));
    const int gap = 48;
    const juce::Point<int> candidates[] = { { mouse.x + gap, mouse.y + gap }, { mouse.x - gap - size.x, mouse.y + gap },
                                            { mouse.x + gap, mouse.y - gap - size.y }, { mouse.x - gap - size.x, mouse.y - gap - size.y } };
    auto pos = candidates[0];
    for (auto c : candidates)
        if (getLocalBounds().contains (juce::Rectangle<int> (c.x, c.y, size.x, size.y))) { pos = c; break; }
    loupe->setBounds (juce::jlimit (0, juce::jmax (0, getWidth() - size.x), pos.x),
                      juce::jlimit (0, juce::jmax (0, getHeight() - size.y), pos.y), size.x, size.y);
    const auto area = juce::Rectangle<float> ((float) size.x / zoom, (float) size.y / zoom).withCentre (mouse.toFloat()).getSmallestIntegerContainer();
    const float display = (float) juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds())->scale;
    loupe->setVisible (false); // keep the loupe itself out of the snapshot
    auto img = createComponentSnapshot (area, true, zoom * display);
    loupe->frame = theme().accent;
    loupe->setImage (img);
    loupe->setVisible (true);
}

void KreiselEditor::renameTrack (int t)
{
    auto* aw = new juce::AlertWindow ("Spur " + juce::String (t + 1) + " umbenennen",
                                      juce::String::fromUTF8 ("Leer lassen für den automatischen Namen (aus Klang, Drum-Note oder MIDI-Kanal)."),
                                      juce::MessageBoxIconType::NoIcon, this);
    aw->addTextEditor ("name", proc.customTrackName (t).isNotEmpty() ? proc.customTrackName (t) : proc.trackName (t), "Name");
    aw->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Abbrechen", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, t, aw] (int r) {
        if (r == 1)
        {
            auto n = aw->getTextEditorContents ("name").trim();
            if (n == proc.trackName (t) && proc.customTrackName (t).isEmpty()) return;
            proc.setTrackName (t, n);
            refreshTrackNames();
        }
    }), true);
}

void KreiselEditor::paint (juce::Graphics& g)
{
    const auto& th = theme();
    g.fillAll (th.bg);
    // header
    g.setColour (th.panel);
    g.fillRect (0, 0, getWidth(), 52);
    g.setColour (th.accent);
    {
        const juce::Point<float> c (30, 26);
        for (int i = 0; i < 3; ++i)
            g.drawEllipse (juce::Rectangle<float> (12.0f + i * 10, 12.0f + i * 10).withCentre (c), 1.6f);
        g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre (polar (c, 16, -0.6f)));
        g.fillEllipse (juce::Rectangle<float> (5, 5).withCentre (polar (c, 11, 2.2f)));
    }
    g.setColour (th.text);
    g.setFont (font (22.0f, true));
    g.drawText ("KREISEL", 54, 0, 120, 52, juce::Justification::centredLeft);

    auto panel = [&] (juce::Rectangle<int> r, const juce::String& title) {
        g.setColour (th.panel);
        g.fillRoundedRectangle (r.toFloat(), 10.0f);
        if (title.isNotEmpty())
        {
            g.setColour (th.dim);
            g.setFont (font (11.5f, true));
            g.drawText (title.toUpperCase(), r.reduced (12, 6).withHeight (16), juce::Justification::centredLeft);
        }
    };
    panel (rTracks, {});
    {
        auto title = "Spur " + juce::String (selTrack + 1) + juce::String::fromUTF8 ("  ·  ") + proc.trackName (selTrack);
        if (proc.hasHosted (selTrack, 0)) title << juce::String::fromUTF8 ("  ·  Instrument: ") << proc.hostedName (selTrack, 0);
        if (proc.hasHosted (selTrack, 1)) title << juce::String::fromUTF8 ("  ·  Effekt: ") << proc.hostedName (selTrack, 1);
        panel (rKnobs, title);
    }
    panel (rGlobal, "Global");
    panel (rExport, {});
    panel (rLane, {});

    // track activity LEDs on tabs
    for (int t = 0; t < kTracks; ++t)
    {
        auto tb = trackTabs[t]->getBounds().toFloat();
        const float f = visual.flash[(size_t) t];
        g.setColour (th.track[(size_t) t].withAlpha (0.25f + 0.75f * f));
        g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre ({ tb.getRight() - 8, tb.getY() - 5 }));
    }
    g.setColour (th.track[(size_t) selTrack]);
    g.fillRoundedRectangle (rKnobs.toFloat().withWidth (4).reduced (0, 10), 2);
}

void KreiselEditor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (52).reduced (8, 10);
    header.removeFromLeft (146);
    playBtn.setBounds (header.removeFromLeft (70));
    header.removeFromLeft (8);
    prevPreset.setBounds (header.removeFromLeft (26));
    header.removeFromLeft (3);
    presetBox.setBounds (header.removeFromLeft (146));
    header.removeFromLeft (3);
    nextPreset.setBounds (header.removeFromLeft (26));
    header.removeFromLeft (6);
    savePreset.setBounds (header.removeFromLeft (72));
    header.removeFromLeft (3);
    loadPreset.setBounds (header.removeFromLeft (58));
    header.removeFromLeft (8);
    undoBtn.setBounds (header.removeFromLeft (80));
    header.removeFromLeft (3);
    redoBtn.setBounds (header.removeFromLeft (80));
    helpBtn.setBounds (header.removeFromRight (30));
    header.removeFromRight (6);
    lightToggle.setBounds (header.removeFromRight (56));
    themeBox.setBounds (header.removeFromRight (80));
    header.removeFromRight (4);
    visualBox.setBounds (header.removeFromRight (80));
    header.removeFromRight (4);
    pluginsBtn.setBounds (header.removeFromRight (62));
    header.removeFromRight (4);
    matrixBtn.setBounds (header.removeFromRight (58));
    header.removeFromRight (4);
    randomBtn.setBounds (header.removeFromRight (58));

    r.reduce (10, 8);
    auto top = r.removeFromTop (500);
    rView = top.removeFromLeft (560);
    visual.setBounds (rView);
    if (matrix) matrix->setBounds (rView);
    top.removeFromLeft (10);

    // right column
    rTracks = top.removeFromTop (78);
    {
        auto tr = rTracks.reduced (8, 10);
        const int w = tr.getWidth() / kTracks;
        for (int t = 0; t < kTracks; ++t)
        {
            auto cell = juce::Rectangle<int> (tr.getX() + w * t, tr.getY(), w, tr.getHeight()).reduced (3, 0);
            trackTabs[t]->setBounds (cell.removeFromTop (30));
            cell.removeFromTop (4);
            const int bw = cell.getWidth() / 3;
            activeBtns[t]->setBounds (cell.removeFromLeft (bw).reduced (1, 0));
            muteBtns[t]->setBounds (cell.removeFromLeft (bw).reduced (1, 0));
            soloBtns[t]->setBounds (cell.reduced (1, 0));
        }
    }
    top.removeFromTop (8);
    rKnobs = top.removeFromTop (250);
    {
        auto kr_ = rKnobs.reduced (12, 8);
        kr_.removeFromTop (18);
        auto row1 = kr_.removeFromTop (110);
        auto row2 = kr_;
        const int kw = row1.getWidth() / 6;
        for (int i = 0; i < 6; ++i)
        {
            auto cell = row1.removeFromLeft (kw);
            knobs[i]->l.setBounds (cell.removeFromTop (16));
            knobs[i]->s.setBounds (cell);
        }
        for (int i = 6; i < 10; ++i)
        {
            auto cell = row2.removeFromLeft (kw);
            knobs[i]->l.setBounds (cell.removeFromTop (16));
            knobs[i]->s.setBounds (cell.withTrimmedBottom (4));
        }
        row2.removeFromLeft (8);
        auto combos = row2.reduced (0, 6);
        const int rowH = juce::jmin (30, combos.getHeight() / 3);
        auto comboRow = [&] (juce::Label& l, juce::ComboBox& c) {
            auto rr = combos.removeFromTop (rowH);
            l.setBounds (rr.removeFromLeft (44));
            c.setBounds (rr.reduced (0, 2));
        };
        comboRow (soundLbl, soundBox);
        comboRow (chordLbl, chordBox);
        comboRow (clockLbl, clockBox);
    }
    top.removeFromTop (8);
    auto bottomRight = top;
    rGlobal = bottomRight.removeFromLeft (bottomRight.getWidth() / 2 - 4);
    bottomRight.removeFromLeft (8);
    rExport = bottomRight;
    {
        auto gr = rGlobal.reduced (12, 8);
        gr.removeFromTop (18);
        auto row = gr.removeFromTop (28);
        rootBox.setBounds (row.removeFromLeft (64));
        row.removeFromLeft (6);
        scaleBox.setBounds (row);
        gr.removeFromTop (2);
        quantToggle.setBounds (gr.removeFromTop (24));
        gr.removeFromTop (4);
        auto row3 = gr.removeFromTop (26);
        runToggle.setBounds (row3.removeFromLeft (120));
        bpmSlider.setBounds (row3);
        gr.removeFromTop (6);
        auto row4 = gr.removeFromTop (26);
        internToggle.setBounds (row4.removeFromLeft (120));
        volSlider.setBounds (row4);
        globalLbl.setBounds ({});
        bpmLbl.setBounds ({});
    }
    {
        auto er = rExport.reduced (10, 10);
        auto row = er.removeFromTop (26);
        exportLbl.setBounds (row.removeFromLeft (90));
        barsBox.setBounds (row);
        er.removeFromTop (6);
        saveMidiBtn.setBounds (er.removeFromBottom (26));
        er.removeFromBottom (6);
        dragArea.setBounds (er);
    }

    // lanes
    r.removeFromTop (10);
    rLane = r;
    {
        auto lr = rLane.reduced (10, 8);
        auto tabs = lr.removeFromLeft (240);
        const int tw = tabs.getWidth() / 3, th_ = juce::jmin (40, tabs.getHeight() / 3);
        for (int g = 0; g < kTargets; ++g)
            targetTabs[g]->setBounds (juce::Rectangle<int> (tabs.getX() + tw * (g % 3), tabs.getY() + th_ * (g / 3), tw, th_).reduced (2));
        auto under = tabs.withTrimmedTop (th_ * 3 + 6);
        laneRandom.setBounds (under.removeFromTop (28).removeFromLeft (tw * 3 / 2 - 2));
        laneClear.setBounds (laneRandom.getBounds().translated (tw * 3 / 2 + 2, 0));
        under.removeFromTop (4);
        laneCopyAll.setBounds (under.removeFromTop (28).withWidth (tw * 3 - 2).translated (2, 0));
        lr.removeFromLeft (12);
        auto ctl = lr.removeFromTop (28);
        laneTitle.setBounds (ctl.removeFromLeft (330));
        laneOn.setBounds (ctl.removeFromLeft (100));
        laneLenLbl.setBounds (ctl.removeFromLeft (44));
        laneLen.setBounds (ctl.removeFromLeft (150));
        ctl.removeFromLeft (8);
        laneDepthLbl.setBounds (ctl.removeFromLeft (40));
        laneDepth.setBounds (ctl.removeFromLeft (150));
        ctl.removeFromLeft (8);
        laneDir.setBounds (ctl);
        lr.removeFromTop (6);
        laneEditor.setBounds (lr);
    }
    if (randomPanel) randomPanel->setBounds (getLocalBounds());
    if (helpPanel) helpPanel->setBounds (getLocalBounds());
    if (settingsPanel) settingsPanel->setBounds (getLocalBounds());
    if (catcher) catcher->setBounds (getLocalBounds());
}

// ============================================================================================ SettingsPanel
int SettingsPanel::FolderModel::getNumRows() { return owner.proc.hostSettings->folders.size(); }

void SettingsPanel::FolderModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    const auto& th = owner.ed.theme();
    if (selected) g.fillAll (th.accent.withAlpha (0.3f));
    g.setColour (th.text);
    g.setFont (font (13.0f));
    g.drawText (owner.proc.hostSettings->folders[row], 8, 0, w - 16, h, juce::Justification::centredLeft);
}

int SettingsPanel::PluginModel::getNumRows() { return owner.shown.size(); }

void SettingsPanel::PluginModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, owner.shown.size())) return;
    const auto& th = owner.ed.theme();
    const auto& d = owner.shown.getReference (row);
    if (selected) g.fillAll (th.accent.withAlpha (0.3f));
    g.setFont (font (12.5f, true));
    g.setColour (d.isInstrument ? th.track[3] : th.track[5]);
    g.drawText (d.isInstrument ? "Instrument" : "Effekt", 8, 0, 80, h, juce::Justification::centredLeft);
    g.setColour (th.dim);
    g.setFont (font (12.5f));
    g.drawText (d.manufacturerName, 92, 0, 180, h, juce::Justification::centredLeft);
    g.setColour (th.text);
    g.drawText (d.name, 276, 0, w - 284, h, juce::Justification::centredLeft);
}

SettingsPanel::SettingsPanel (KreiselProcessor& p, KreiselEditor& e) : proc (p), ed (e)
{
    folderList.setModel (&folderModel);
    pluginList.setModel (&pluginModel);
    folderList.setRowHeight (24);
    pluginList.setRowHeight (22);
    for (auto* l : { &folderList, &pluginList })
    {
        l->setColour (juce::ListBox::backgroundColourId, ed.theme().panel2);
        addAndMakeVisible (l);
    }
    status.setJustificationType (juce::Justification::centredLeft);
    status.setColour (juce::Label::textColourId, ed.theme().text);
    for (auto* c : std::initializer_list<juce::Component*> { &addFolder, &removeFolder, &defaults, &scanNew, &scanAll, &clearList, &close, &status, &progressBar })
        addAndMakeVisible (c);
    progressBar.setPercentageDisplay (true);

    addFolder.onClick = [this] {
        chooser = std::make_unique<juce::FileChooser> ("VST3-Ordner wählen", juce::File ("C:\\Program Files\\Common Files\\VST3"));
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [this] (const juce::FileChooser& fc) {
            auto f = fc.getResult();
            if (f.isDirectory())
            {
                proc.hostSettings->folders.addIfNotAlreadyThere (f.getFullPathName());
                proc.hostSettings->save();
                refresh();
            }
        });
    };
    removeFolder.onClick = [this] {
        const int r = folderList.getSelectedRow();
        if (r < 0) return;
        proc.hostSettings->folders.remove (r);
        proc.hostSettings->save();
        refresh();
    };
    defaults.onClick = [this] {
        for (auto& f : HostSettings::defaultFolders()) proc.hostSettings->folders.addIfNotAlreadyThere (f);
        proc.hostSettings->save();
        refresh();
    };
    scanNew.onClick = [this] { startScan (false); };
    scanAll.onClick = [this] { startScan (true); };
    clearList.onClick = [this] {
        proc.hostSettings->list.clear();
        proc.hostSettings->list.clearBlacklistedFiles();
        proc.hostSettings->save();
        refresh();
    };
    close.onClick = [this] { ed.showSettings (false); };
    proc.hostSettings->reload();
    refresh();
}

SettingsPanel::~SettingsPanel()
{
    stopTimer();
    if (child != nullptr) child->kill();
    if (scanning) proc.hostSettings->save();
    childOut.deleteFile();
}

void SettingsPanel::refreshLists()
{
    shown = proc.hostSettings->plugins (true);
    shown.addArray (proc.hostSettings->plugins (false));
    pluginList.updateContent();
    pluginList.repaint();
}

void SettingsPanel::refresh()
{
    shown = proc.hostSettings->plugins (true);
    shown.addArray (proc.hostSettings->plugins (false));
    folderList.updateContent();
    pluginList.updateContent();
    folderList.repaint();
    pluginList.repaint();
    if (! scanning)
    {
        const int blocked = proc.hostSettings->list.getBlacklistedFiles().size();
        status.setText (juce::String (proc.hostSettings->plugins (true).size()) + " Instrumente, "
                            + juce::String (proc.hostSettings->plugins (false).size()) + " Effekte"
                            + (blocked > 0 ? juce::String::fromUTF8 (" · gesperrt: ") + juce::String (blocked) : juce::String()),
                        juce::dontSendNotification);
    }
}

void SettingsPanel::setButtonsEnabled (bool on)
{
    for (auto* b : { &addFolder, &removeFolder, &defaults, &scanNew, &scanAll, &clearList })
        b->setEnabled (on);
}

void SettingsPanel::startScan (bool full)
{
    if (scanning) return;
    auto& hs = *proc.hostSettings;
    if (! HostSettings::scannerExe().existsAsFile())
    {
        status.setText (juce::String::fromUTF8 ("KreiselScanner.exe fehlt – bitte Kreisel neu installieren."), juce::dontSendNotification);
        return;
    }
    if (full) { hs.list.clear(); hs.list.clearBlacklistedFiles(); }
    juce::FileSearchPath path;
    for (auto& f : hs.folders) path.add (juce::File (f));
    auto& format = *hs.formats.getFormat (0);
    const auto files = format.searchPathsForPlugins (path, true, false);
    const auto blocked = hs.list.getBlacklistedFiles();
    pending.clear();
    failed.clear();
    for (auto& f : files)
        if (! blocked.contains (f) && (full || ! hs.list.isListingUpToDate (f, format)))
            pending.add (f);
    if (pending.isEmpty())
    {
        refresh();
        status.setText (status.getText() + juce::String::fromUTF8 (" · nichts Neues gefunden"), juce::dontSendNotification);
        return;
    }
    scanning = true;
    scanIndex = 0;
    foundNow = 0;
    progress = 0.0;
    setButtonsEnabled (false);
    childOut = hs.settingsFolder().getChildFile ("scan-result.xml");
    launchNext();
    startTimer (100);
}

void SettingsPanel::launchNext()
{
    if (scanIndex >= pending.size()) { endScan(); return; }
    const auto file = pending[scanIndex];
    status.setText (juce::String::fromUTF8 ("Prüfe ") + juce::String (scanIndex + 1) + "/" + juce::String (pending.size())
                        + ": " + juce::File (file).getFileNameWithoutExtension(), juce::dontSendNotification);
    progress = (double) scanIndex / (double) pending.size();
    childOut.deleteFile();
    child = std::make_unique<juce::ChildProcess>();
    childStart = juce::Time::getMillisecondCounterHiRes();
    if (! child->start (juce::StringArray { HostSettings::scannerExe().getFullPathName(), file, childOut.getFullPathName() }, 0))
        finishChild (false);
}

void SettingsPanel::finishChild (bool timedOut)
{
    auto& hs = *proc.hostSettings;
    const auto file = pending[scanIndex];
    int added = 0;
    if (! timedOut)
        if (auto xml = juce::XmlDocument::parse (childOut))
            for (auto* e : xml->getChildIterator())
            {
                juce::PluginDescription d;
                if (d.loadFromXml (*e)) { hs.list.addType (d); ++added; }
            }
    if (added == 0)
    {
        hs.list.addToBlacklist (file); // crashed, hung or not a plug-in: skipped from now on
        failed.add (juce::File (file).getFileNameWithoutExtension() + (timedOut ? " (Zeitlimit)" : ""));
    }
    foundNow += added;
    hs.save(); // keep progress even if something goes wrong later
    child.reset();
    ++scanIndex;
    refreshLists();
    launchNext();
}

void SettingsPanel::endScan()
{
    stopTimer();
    scanning = false;
    childOut.deleteFile();
    setButtonsEnabled (true);
    progress = 1.0;
    refresh();
    auto msg = status.getText() + juce::String::fromUTF8 (" · neu geprüft: ") + juce::String (pending.size());
    if (! failed.isEmpty())
        msg << juce::String::fromUTF8 (" · übersprungen: ") << failed.joinIntoString (", ");
    status.setText (msg, juce::dontSendNotification);
    status.setTooltip (failed.joinIntoString ("\n"));
}

void SettingsPanel::timerCallback()
{
    if (! scanning || child == nullptr) return;
    const double elapsed = (juce::Time::getMillisecondCounterHiRes() - childStart) / 1000.0;
    if (child->isRunning())
    {
        if (elapsed > 90.0) { child->kill(); finishChild (true); }
        return;
    }
    finishChild (false);
}

void SettingsPanel::paint (juce::Graphics& g)
{
    const auto& th = ed.theme();
    g.fillAll (th.bg.withAlpha (0.94f));
    auto card = getLocalBounds().reduced (60, 40).toFloat();
    g.setColour (th.panel);
    g.fillRoundedRectangle (card, 12.0f);
    auto r = getLocalBounds().reduced (84, 56);
    g.setColour (th.text);
    g.setFont (font (20.0f, true));
    g.drawText (juce::String::fromUTF8 ("Einstellungen  ·  VST3-Plugins"), r.removeFromTop (30), juce::Justification::centredLeft);
    g.setColour (th.dim);
    g.setFont (font (13.0f));
    g.drawFittedText (juce::String::fromUTF8 ("Ordner eintragen und scannen. Danach lässt sich pro Spur über den Rechtsklick auf den Spur-Tab ein Instrument "
                                              "(ersetzt den eingebauten Klang) und ein Effekt laden – im Plugin wie in der Standalone-Fassung. "
                                              "Die Liste gilt für alle Kreisel-Instanzen. Jedes Plugin wird in einem eigenen Prüfprogramm geladen: "
                                              "stürzt es ab oder hängt es länger als 90 Sekunden, kommt es auf die Sperrliste – Kreisel selbst läuft weiter."),
                      r.removeFromTop (44), juce::Justification::topLeft, 3);
    g.setColour (th.dim);
    g.setFont (font (11.5f, true));
    g.drawText ("ORDNER", folderList.getBounds().translated (0, -18).withHeight (16), juce::Justification::centredLeft);
    g.drawText ("GEFUNDENE PLUGINS", pluginList.getBounds().translated (0, -18).withHeight (16), juce::Justification::centredLeft);
}

void SettingsPanel::resized()
{
    auto r = getLocalBounds().reduced (84, 56);
    r.removeFromTop (30 + 44 + 24);
    auto folders = r.removeFromTop (130);
    auto fb = folders.removeFromRight (180);
    folderList.setBounds (folders.withTrimmedRight (10));
    addFolder.setBounds (fb.removeFromTop (30));
    fb.removeFromTop (6);
    removeFolder.setBounds (fb.removeFromTop (30));
    fb.removeFromTop (6);
    defaults.setBounds (fb.removeFromTop (30));
    r.removeFromTop (14);
    auto bottom = r.removeFromBottom (34);
    close.setBounds (bottom.removeFromRight (120));
    bottom.removeFromRight (10);
    clearList.setBounds (bottom.removeFromRight (120));
    r.removeFromBottom (10);
    auto scanRow = r.removeFromBottom (30);
    scanNew.setBounds (scanRow.removeFromLeft (180));
    scanRow.removeFromLeft (8);
    scanAll.setBounds (scanRow.removeFromLeft (160));
    scanRow.removeFromLeft (12);
    progressBar.setBounds (scanRow.removeFromRight (220).reduced (0, 3));
    scanRow.removeFromRight (10);
    status.setBounds (scanRow);
    r.removeFromBottom (10);
    r.removeFromTop (20);
    pluginList.setBounds (r);
}
