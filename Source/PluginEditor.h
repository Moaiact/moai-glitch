#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Interfaz "chrome" de Moai Glitch.
// Todo se dibuja en coordenadas de diseño de 1120 x 816 y se escala al tamaño
// real de la ventana. Las imágenes renderizadas en Blender (título cromado y
// cuerpo de la perilla) vienen en BinaryData; arcos, brillo, indicador, LCD y
// líneas de circuito se dibujan en código para que un solo render sirva para
// todos los colores.
namespace MoaiUI
{
    constexpr float designW = 1120.0f;
    constexpr float designH = 816.0f;

    juce::Font font (float size, bool bold, float letterSpacing = 0.0f);
}

//==============================================================================
class MoaiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MoaiLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;

    juce::Font getPopupMenuFont() override;

    // Tooltips con el estilo Moai
    juce::Rectangle<int> getTooltipBounds (const juce::String& t, juce::Point<int> p, juce::Rectangle<int> a) override { return moai::tooltipBounds (t, p, a); }
    void drawTooltip (juce::Graphics& g, const juce::String& t, int w, int h) override { moai::drawTooltip (g, t, w, h); }

private:
    juce::Image knobImage;
    std::map<int, juce::Image> knobCache;   // cuerpo de perilla ya reducido a cada tamaño en píxeles

    const juce::Image& knobAtPixelSize (int px);
};

//==============================================================================
// Pantalla LCD del Rate: flechas para subir/bajar, clic al centro abre la lista,
// rueda del mouse también cambia el valor.
class RateDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit RateDisplay (juce::RangedAudioParameter&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static constexpr float designW = 388.0f;   // LCD + bisel
    static constexpr float designH = 104.0f;

private:
    void setIndex (int newIndex);

    juce::RangedAudioParameter& param;
    juce::StringArray names;
    int index = 0;
    juce::ParameterAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RateDisplay)
};

//==============================================================================
class NewProjectAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        moai::MoaiSlider slider;     // Shift = ajuste fino, clic derecho = escribir el valor
        juce::String paramID;
        juce::String label;
        juce::Colour colour;
        float cx = 0, cy = 0, R = 40;   // centro y radio en coordenadas de diseño
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct CircuitTrace
    {
        float startX, endX, y, width;
        juce::Colour colour;
        float fade;
        bool roundPad;
        float viaX;   // < 0 = sin vía intermedia
    };

    void addKnob (const juce::String& paramID, const juce::String& text, juce::Colour colour,
                  float cx, float cy, float R);
    void buildBackground();
    void drawTitleCircuit (juce::Graphics&) const;
    static void drawPanel (juce::Graphics&, juce::Rectangle<float>, const juce::String& title);

    void applyTooltips();

    NewProjectAudioProcessor& audioProcessor;
    MoaiLookAndFeel lnf;   // Debe declararse antes que los componentes
    moai::Help help;
    std::unique_ptr<moai::EditorChrome> chrome;   // barra superior, paneles y tooltips (v1.1)

    std::vector<std::unique_ptr<Knob>> knobs;
    std::unique_ptr<RateDisplay> rateDisplay;

    juce::Image titleImage, titleScaled, noiseImage;
    std::vector<CircuitTrace> traces;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor)
};
