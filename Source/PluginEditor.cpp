#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace Palette
{
    const juce::Colour background { 0xff050608 };
    const juce::Colour labelText  { 0xffaeb6c8 };
    const juce::Colour dimText    { 0xff6f7a92 };
    const juce::Colour faintText  { 0xff56607a };
    const juce::Colour track      { 0xff14171f };
    const juce::Colour tick       { 0xff3a4152 };
    const juce::Colour bezelTop   { 0xff5a6478 };
    const juce::Colour bezelMid   { 0xff1a1e28 };
    const juce::Colour bezelBot   { 0xff3a4254 };

    // Colores fijos de las perillas: no cambian con el valor
    const juce::Colour green  { 57, 255, 136 };    // Repeats, Reverse
    const juce::Colour blue   { 46, 123, 255 };    // Flanger, Reverb
    const juce::Colour purple { 204, 51, 255 };    // Verb Prob
    const juce::Colour white  { 236, 242, 255 };   // Pan, Dry/Wet

    const juce::Colour circuit  { 0xff2e7bff };
    const juce::Colour circuit2 { 0xff3fa9ff };

    const juce::Colour lcdText  { 0xff8fd9ae };
    const juce::Colour lcdGlow  { 0xff4fbf7f };
    const juce::Colour lcdArrow { 0xff5fb884 };
}

juce::Font MoaiUI::font (float size, bool bold, float letterSpacing)
{
    auto opts = juce::FontOptions().withPointHeight (size).withStyle (bold ? "Bold" : "Regular");
    return juce::Font (opts).withExtraKerningFactor (letterSpacing / size);
}

// Reducción de imagen en pasos de a la mitad: evita el aliasing de un solo reescalado grande
static juce::Image smoothRescale (juce::Image src, int w, int h)
{
    w = juce::jmax (1, w);
    h = juce::jmax (1, h);
    while (src.getWidth() >= w * 2 && src.getHeight() >= h * 2)
        src = src.rescaled (src.getWidth() / 2, src.getHeight() / 2, juce::Graphics::highResamplingQuality);
    return src.rescaled (w, h, juce::Graphics::highResamplingQuality);
}

// Ángulo JUCE (0 = arriba, sentido horario) -> punto
static juce::Point<float> polar (juce::Point<float> c, float r, float a)
{
    return { c.x + r * std::sin (a), c.y - r * std::cos (a) };
}

//==============================================================================
MoaiLookAndFeel::MoaiLookAndFeel()
{
    knobImage = juce::ImageCache::getFromMemory (BinaryData::knob_body_png, BinaryData::knob_body_pngSize);

    setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff0d0f15));
    setColour (juce::PopupMenu::textColourId,                  Palette::labelText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Palette::green.withAlpha (0.18f));
    setColour (juce::PopupMenu::highlightedTextColourId,       Palette::lcdText);
}

juce::Font MoaiLookAndFeel::getPopupMenuFont()
{
    return MoaiUI::font (15.0f, true, 1.0f);
}

const juce::Image& MoaiLookAndFeel::knobAtPixelSize (int px)
{
    px = juce::jlimit (16, knobImage.getWidth(), px);
    auto it = knobCache.find (px);
    if (it == knobCache.end())
        it = knobCache.emplace (px, smoothRescale (knobImage, px, px)).first;
    return it->second;
}

void MoaiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int /*height*/,
                                        float pos, float startAngle, float endAngle, juce::Slider& slider)
{
    const float R    = (float) slider.getProperties().getWithDefault ("R", 40.0);
    const auto  col  = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const float half = R + 30.0f;
    const float s    = (float) width / (2.0f * half);

    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::scale (s).translated ((float) x, (float) y));
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

    const juce::Point<float> c (half, half);
    const float angle = startAngle + pos * (endAngle - startAngle);

    // El brillo solo aparece cerca del máximo (suave, no "luz de discoteca")
    const float tt    = juce::jlimit (0.0f, 1.0f, (pos - 0.72f) / 0.28f);
    const float inten = 0.06f + 0.94f * tt * tt * (3.0f - 2.0f * tt);

    // Marcas de escala
    g.setColour (Palette::tick);
    for (int k = 0; k <= 10; ++k)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) k / 10.0f;
        g.drawLine ({ polar (c, R + 22.0f, a), polar (c, R + 26.0f, a) }, 1.5f);
    }

    // Pista
    const float arcR = R + 14.0f;
    const juce::PathStrokeType roundStroke (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    juce::Path trackPath;
    trackPath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Palette::track);
    g.strokePath (trackPath, roundStroke);

    // Arco de valor + halo
    if (pos > 0.001f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);

        juce::Path halo;
        juce::PathStrokeType (8.0f + 5.0f * inten, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)
            .createStrokedPath (halo, arc);
        juce::DropShadow (col.withAlpha (0.08f + 0.42f * inten), 10, {}).drawForPath (g, halo);

        g.setColour (col);
        g.strokePath (arc, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    if (pos > 0.97f)
    {
        juce::Path aura;
        aura.addEllipse (c.x - (R + 10.0f), c.y - (R + 10.0f), 2.0f * (R + 10.0f), 2.0f * (R + 10.0f));
        juce::DropShadow (col.withAlpha (0.06f), 10, {}).drawForPath (g, aura);
    }

    // Sombra bajo la perilla
    {
        const juce::Point<float> sc (c.x + 2.0f, c.y + 5.0f);
        const float sr = R * 1.1f + 4.0f;
        juce::ColourGradient grad (juce::Colours::black.withAlpha (0.55f), sc,
                                   juce::Colours::transparentBlack, { sc.x + sr, sc.y }, true);
        grad.addColour (0.82, juce::Colours::black.withAlpha (0.5f));
        g.setGradientFill (grad);
        g.fillEllipse (sc.x - sr, sc.y - sr, sr * 2.0f, sr * 2.0f);
    }

    // Cuerpo moleteado (render de Blender)
    {
        const float side = 2.26f * R;
        const int px = juce::roundToInt (side * g.getInternalContext().getPhysicalPixelScaleFactor());
        g.setOpacity (1.0f);
        g.drawImage (knobAtPixelSize (px), { c.x - side * 0.5f, c.y - side * 0.5f, side, side });
    }

    // Indicador
    const auto p1 = polar (c, R * 0.18f, angle);
    const auto p2 = polar (c, R * 0.66f, angle);
    juce::Path pointer;
    pointer.startNewSubPath (p1);
    pointer.lineTo (p2);

    juce::Path pointerGlow;
    juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (pointerGlow, pointer);
    juce::DropShadow (col.withAlpha (0.15f + 0.35f * inten), 4, {}).drawForPath (g, pointerGlow);

    g.setColour (slider.isEnabled() ? col : col.withAlpha (0.4f));
    g.strokePath (pointer, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
RateDisplay::RateDisplay (juce::RangedAudioParameter& p)
    : param (p),
      attachment (p, [this] (float v) { index = juce::roundToInt (v); repaint(); }, nullptr)
{
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (&p))
        names = choice->choices;

    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle ("Rate");
    attachment.sendInitialUpdate();
}

void RateDisplay::setIndex (int newIndex)
{
    newIndex = juce::jlimit (0, juce::jmax (0, names.size() - 1), newIndex);
    if (newIndex != index)
        attachment.setValueAsCompleteGesture ((float) newIndex);
}

void RateDisplay::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / designW;
    g.addTransform (juce::AffineTransform::scale (s));

    const juce::Rectangle<float> lcd (4.0f, 4.0f, 380.0f, 96.0f);

    // Bisel cromado
    juce::ColourGradient bezel (Palette::bezelTop, 0.0f, 0.0f, Palette::bezelBot, 0.0f, designH, false);
    bezel.addColour (0.5, Palette::bezelMid);
    g.setGradientFill (bezel);
    g.fillRoundedRectangle (lcd.expanded (4.0f), 14.0f);

    g.setColour (juce::Colour (0xff030805));
    g.fillRoundedRectangle (lcd, 11.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (lcd, 11.0f, 1.0f);
    g.setColour (juce::Colour (0xff06120c).withAlpha (0.95f));
    g.fillRoundedRectangle (lcd.reduced (6.0f), 8.0f);

    // Valor
    const auto text = names[index];
    auto f = MoaiUI::font (44.0f, true, 4.0f);
    const float maxW = 240.0f;
    const float w = juce::GlyphArrangement::getStringWidth (f, text);
    if (w > maxW)
        f = MoaiUI::font (44.0f * maxW / w, true, 4.0f * maxW / w);

    juce::GlyphArrangement ga;
    ga.addLineOfText (f, text, 0.0f, 0.0f);
    const auto tb = ga.getBoundingBox (0, -1, true);
    juce::Path glyphs;
    ga.createPath (glyphs);
    // centrado horizontal y vertical dentro del LCD
    const float spacing = f.getExtraKerningFactor() * f.getHeight();
    glyphs.applyTransform (juce::AffineTransform::translation (lcd.getCentreX() - (tb.getCentreX() - spacing * 0.5f),
                                                               lcd.getCentreY() - tb.getCentreY()));

    juce::DropShadow (Palette::lcdGlow.withAlpha (0.35f), 5, {}).drawForPath (g, glyphs);
    g.setColour (Palette::lcdText);
    g.fillPath (glyphs);

    // Flechas
    const float cy = lcd.getCentreY();
    juce::Path up, down;
    up.startNewSubPath (lcd.getX() + 26.0f, cy + 6.0f);
    up.lineTo (lcd.getX() + 36.0f, cy - 6.0f);
    up.lineTo (lcd.getX() + 46.0f, cy + 6.0f);
    down.startNewSubPath (lcd.getRight() - 36.0f, cy - 6.0f);
    down.lineTo (lcd.getRight() - 26.0f, cy + 6.0f);
    down.lineTo (lcd.getRight() - 16.0f, cy - 6.0f);

    const auto arrowStroke = juce::PathStrokeType (2.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt);
    g.setColour (Palette::lcdArrow.withAlpha (index > 0 ? 1.0f : 0.35f));
    g.strokePath (up, arrowStroke);
    g.setColour (Palette::lcdArrow.withAlpha (index < names.size() - 1 ? 1.0f : 0.35f));
    g.strokePath (down, arrowStroke);
}

void RateDisplay::mouseDown (const juce::MouseEvent& e)
{
    const float xd = e.position.x * designW / (float) getWidth();

    if (xd < 80.0f)            { setIndex (index - 1); return; }   // flecha arriba: valor anterior
    if (xd > designW - 80.0f)  { setIndex (index + 1); return; }   // flecha abajo: valor siguiente

    juce::PopupMenu menu;
    for (int i = 0; i < names.size(); ++i)
        menu.addItem (i + 1, names[i], true, i == index);

    juce::Component::SafePointer<RateDisplay> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                        [safe] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->setIndex (result - 1);
                        });
}

void RateDisplay::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY > 0.0f)       setIndex (index - 1);
    else if (wheel.deltaY < 0.0f)  setIndex (index + 1);
}

//==============================================================================
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lnf);

    titleImage = juce::ImageCache::getFromMemory (BinaryData::title_chrome_png, BinaryData::title_chrome_pngSize);
    buildBackground();

    // ---- Rate ----
    auto* rateParam = audioProcessor.apvts.getParameter ("glitchRate");
    jassert (rateParam != nullptr);
    rateDisplay = std::make_unique<RateDisplay> (*rateParam);
    addAndMakeVisible (*rateDisplay);

    // ---- Perillas: colores fijos ----
    addKnob ("repeats",      "REPEATS",   Palette::green,  650.0f, 300.0f, 44.0f);
    addKnob ("reverseProb",  "REVERSE",   Palette::green,  900.0f, 300.0f, 44.0f);
    addKnob ("flangerDepth", "FLANGER",   Palette::blue,   150.0f, 640.0f, 40.0f);
    addKnob ("panFx",        "PAN FX",    Palette::blue,   345.0f, 640.0f, 40.0f);
    addKnob ("reverbMix",    "REVERB",    Palette::blue,   540.0f, 640.0f, 40.0f);
    addKnob ("reverbProb",   "VERB PROB", Palette::purple, 735.0f, 640.0f, 40.0f);
    addKnob ("mix",          "DRY/WET",   Palette::white,  930.0f, 640.0f, 40.0f);

    // ---- v1.1: barra de presets, ajustes y ayuda (el diseño queda igual, 52 px más abajo) ----
    help.add ({
        { "glitchRate",   "Rhythmic division of the glitch, synced to the DAW tempo. Use the arrows, the mouse wheel or click the name to open the list.",
                          "División rítmica del glitch, sincronizada al tempo del DAW. Usa las flechas, la rueda del mouse o haz clic en el nombre para abrir la lista." },
        { "repeats",      "How many times each slice is repeated before a new one is recorded.",
                          "Cuántas veces se repite cada trozo antes de grabar uno nuevo." },
        { "reverseProb",  "Chance that each repeat plays backwards.",
                          "Probabilidad de que cada repetición suene al revés." },
        { "flangerDepth", "Flanger on the glitch, synced to the tempo.",
                          "Flanger sobre el glitch, sincronizado al tempo." },
        { "panFx",        "Pan FX: each repeat jumps at random to the left or to the right. Higher = wider jumps.",
                          "Pan FX: cada repetición salta al azar a la izquierda o a la derecha. Más alto = saltos más abiertos." },
        { "reverbMix",    "Amount of reverb on the glitch.",
                          "Cantidad de reverb sobre el glitch." },
        { "reverbProb",   "Chance that each repeat is sent to the reverb.",
                          "Probabilidad de que cada repetición vaya al reverb." },
        { "mix",          "Balance between the original signal (dry) and the glitch (wet).",
                          "Balance entre la señal original (dry) y el glitch (wet)." },
    });

    moai::AboutInfo about { "Moai Glitch", {}, titleImage,
                            { "Made with JUCE  -  juce.com",
                              "VST is a registered trademark of Steinberg Media Technologies GmbH",
                              juce::String (juce::CharPointer_UTF8 ("\xc2\xa9 2026 Moaiact. All rights reserved.")) } };
    chrome = std::make_unique<moai::EditorChrome> (*this, audioProcessor.presets, audioProcessor.undoManager, audioProcessor.apvts,
                                                   audioProcessor.outputStage, audioProcessor.settings, help,
                                                   MoaiUI::designW, MoaiUI::designH, 840, about);
    chrome->onHelpChanged = [this] { applyTooltips(); };
    applyTooltips();
    resized();
}

void NewProjectAudioProcessorEditor::applyTooltips()
{
    for (auto& k : knobs)
        k->slider.setTooltip (chrome->tip (k->paramID, true));
    rateDisplay->setTooltip (chrome->tip ("glitchRate"));
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor()
{
    chrome.reset();
    setLookAndFeel (nullptr);
}

void NewProjectAudioProcessorEditor::addKnob (const juce::String& paramID, const juce::String& text, juce::Colour colour,
                                              float cx, float cy, float R)
{
    auto k = std::make_unique<Knob>();
    k->paramID = paramID;
    k->slider.parameter = audioProcessor.apvts.getParameter (paramID);
    k->label = text;
    k->colour = colour;
    k->cx = cx;
    k->cy = cy;
    k->R = R;

    auto& s = k->slider;
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setColour (juce::Slider::rotarySliderFillColourId, colour);
    s.getProperties().set ("R", R);
    s.setTitle (text);
    s.onValueChange = [this] { repaint(); };   // el valor se escribe debajo de la perilla
    addAndMakeVisible (s);

    jassert (audioProcessor.apvts.getParameter (paramID) != nullptr);
    k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, paramID, s);

    if (auto* param = audioProcessor.apvts.getParameter (paramID))
        s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    knobs.push_back (std::move (k));
}

//==============================================================================
void NewProjectAudioProcessorEditor::buildBackground()
{
    // Textura de metal cepillado muy tenue (vetas verticales)
    juce::Random rng (4);
    const int nw = 560, nh = 408;
    noiseImage = juce::Image (juce::Image::ARGB, nw, nh, true);
    {
        juce::Image::BitmapData px (noiseImage, juce::Image::BitmapData::writeOnly);
        for (int x = 0; x < nw; ++x)
        {
            const float a0 = rng.nextFloat(), a1 = rng.nextFloat(), a2 = rng.nextFloat();
            for (int y = 0; y < nh; ++y)
            {
                const float t = (float) y / (float) (nh - 1);
                const float v = t < 0.5f ? juce::jmap (t * 2.0f, a0, a1) : juce::jmap ((t - 0.5f) * 2.0f, a1, a2);
                px.setPixelColour (x, y, juce::Colour (115, 128, 158).withAlpha (0.03f * v * v));
            }
        }
    }

    // Líneas de circuito que salen horizontalmente de los costados del título
    // (misma secuencia pseudoaleatoria que el diseño aprobado)
    juce::int64 seed = 19;
    auto rnd = [&seed]
    {
        seed = (seed * 16807) % 2147483647;
        return (double) seed / 2147483647.0;
    };

    const float TL = 304.0f, TR = 812.0f;
    for (double y = 47.0; y <= 97.0; y += 6.2)
    {
        for (int side : { -1, 1 })
        {
            if (rnd() < 0.18)
                continue;

            CircuitTrace t;
            t.startX = side < 0 ? TL : TR;
            const float L = 70.0f + (float) std::pow (rnd(), 0.8) * (side < 0 ? 250.0f : 260.0f);
            t.endX = t.startX + (float) side * L;
            t.y = (float) y;
            t.colour = rnd() < 0.75 ? Palette::circuit : Palette::circuit2;
            t.width = rnd() < 0.3 ? 1.8f : 1.1f;
            t.fade = juce::jmax (0.25f, 1.0f - L / 330.0f);
            t.roundPad = rnd() < 0.7;
            t.viaX = -1.0f;
            if (rnd() < 0.3)
                t.viaX = t.startX + (float) side * L * (float) (0.4 + rnd() * 0.3);
            traces.push_back (t);
        }
    }
}

void NewProjectAudioProcessorEditor::drawTitleCircuit (juce::Graphics& g) const
{
    for (const auto& t : traces)
    {
        const bool left = t.endX < t.startX;
        const float fadeTo = left ? 30.0f : 1090.0f;
        g.setGradientFill (juce::ColourGradient (Palette::circuit, t.startX, 0.0f,
                                                 Palette::circuit.withAlpha (0.0f), fadeTo, 0.0f, false));
        g.fillRect (juce::Rectangle<float> (juce::jmin (t.startX, t.endX), t.y - t.width * 0.5f,
                                            std::abs (t.endX - t.startX), t.width));

        if (t.roundPad)
        {
            g.setColour (Palette::background);
            g.fillEllipse (t.endX - 2.8f, t.y - 2.8f, 5.6f, 5.6f);
            g.setColour (t.colour.withAlpha (t.fade));
            g.drawEllipse (t.endX - 2.8f, t.y - 2.8f, 5.6f, 5.6f, 1.1f);
        }
        else
        {
            g.setColour (t.colour.withAlpha (t.fade));
            g.fillRoundedRectangle (t.endX - (left ? 6.0f : 0.0f), t.y - 1.8f, 6.0f, 3.6f, 1.0f);
        }

        if (t.viaX >= 0.0f)
        {
            g.setColour (t.colour.withAlpha (juce::jmin (1.0f, t.fade + 0.2f)));
            g.fillEllipse (t.viaX - 1.6f, t.y - 1.6f, 3.2f, 3.2f);
        }
    }
}

void NewProjectAudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0d0f15).withAlpha (0.8f), 0.0f, r.getY(),
                                             juce::Colour (0xff08090d).withAlpha (0.8f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 14.0f);

    juce::ColourGradient bezel (Palette::bezelTop, 0.0f, r.getY(), Palette::bezelBot, 0.0f, r.getBottom(), false);
    bezel.addColour (0.5, Palette::bezelMid);
    g.setGradientFill (bezel);
    g.drawRoundedRectangle (r, 14.0f, 1.5f);

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (1.5f), 13.0f, 1.0f);

    g.setColour (Palette::dimText);
    g.setFont (MoaiUI::font (12.0f, true, 3.0f));
    g.drawSingleLineText (title, juce::roundToInt (r.getX() + 22.0f), juce::roundToInt (r.getY() + 28.0f));
}

void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / MoaiUI::designW;
    g.addTransform (juce::AffineTransform::scale (s));
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

    // Fondo
    g.fillAll (Palette::background);
    g.addTransform (juce::AffineTransform::translation (0.0f, moai::barHeight));   // v1.1: barra superior arriba
    g.drawImage (noiseImage, { 0.0f, 0.0f, MoaiUI::designW, MoaiUI::designH });

    // Circuito del título + emblema cromado
    drawTitleCircuit (g);
    {
        // El PNG viene recortado con márgenes fijos alrededor de las letras:
        // 3 % a cada lado, 10 % arriba y 30 % abajo (sombra).
        const float solidW = 520.0f;
        const float solidH = solidW * ((float) titleImage.getHeight() / 1.40f) / ((float) titleImage.getWidth() / 1.06f);
        const juce::Rectangle<float> dst (300.0f - solidW * 0.03f, 28.0f - solidH * 0.10f, solidW * 1.06f, solidH * 1.40f);
        const float phys = g.getInternalContext().getPhysicalPixelScaleFactor();
        const int pw = juce::roundToInt (dst.getWidth() * phys);
        if (titleScaled.getWidth() != pw)
            titleScaled = smoothRescale (titleImage, pw, juce::roundToInt (dst.getHeight() * phys));
        g.setOpacity (1.0f);   // drawImage usa la opacidad del último color
        g.drawImage (titleScaled, dst);
    }

    g.setColour (Palette::dimText);
    g.setFont (MoaiUI::font (12.0f, false, 6.0f));
    g.drawSingleLineText ("TEMPO-SYNCED STUTTER", 563, 128, juce::Justification::horizontallyCentred);

    // Paneles
    drawPanel (g, { 32.0f, 160.0f, 1056.0f, 290.0f }, "GLITCH");
    drawPanel (g, { 32.0f, 470.0f, 1056.0f, 316.0f }, "FX  /  OUTPUT");

    // Textos del Rate
    g.setColour (Palette::labelText);
    g.setFont (MoaiUI::font (13.0f, true, 2.0f));
    g.drawSingleLineText ("RATE", 277, 210, juce::Justification::horizontallyCentred);
    g.setColour (Palette::faintText);
    g.setFont (MoaiUI::font (11.0f, false, 3.0f));
    g.drawSingleLineText ("RHYTHMIC DIVISION", 278, 356,
                          juce::Justification::horizontallyCentred);

    // Nombre y valor de cada perilla
    for (const auto& k : knobs)
    {
        const int cx = juce::roundToInt (k->cx);
        g.setColour (Palette::labelText);
        g.setFont (MoaiUI::font (13.0f, true, 2.0f));
        g.drawSingleLineText (k->label, cx + 1, juce::roundToInt (k->cy - k->R - 28.0f), juce::Justification::horizontallyCentred);

        g.setColour (k->colour);
        g.setFont (MoaiUI::font (13.0f, true));
        g.drawSingleLineText (k->slider.getTextFromValue (k->slider.getValue()), cx,
                              juce::roundToInt (k->cy + k->R + 40.0f), juce::Justification::horizontallyCentred);
    }
}

void NewProjectAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / MoaiUI::designW;
    auto scaled = [s] (float x, float y, float w, float h)
    {
        return juce::Rectangle<float> (x * s, (y + moai::barHeight) * s, w * s, h * s).toNearestInt();
    };

    if (chrome != nullptr)
        chrome->layout();

    rateDisplay->setBounds (scaled (82.0f, 226.0f, RateDisplay::designW, RateDisplay::designH));

    for (auto& k : knobs)
    {
        const float half = k->R + 30.0f;
        k->slider.setBounds (scaled (k->cx - half, k->cy - half, half * 2.0f, half * 2.0f));
    }
}
