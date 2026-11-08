#pragma once

#include <JuceHeader.h>

//==============================================================================
// MOAI COMMON - funciones compartidas por Moai Glitch y Moai Glitch Multirate
//
//   - Presets: de fábrica + de usuario (Documentos\Moaiact\<plugin>\Presets)
//   - A / B, Undo / Redo, Bypass sin clics, Output trim + medidor
//   - Settings globales (tamaño de la interfaz, tooltips, idioma de la ayuda)
//   - Ayuda contextual en inglés / español (la interfaz siempre en inglés)
//   - Barra superior, panel de Settings, panel About y ventana para guardar
//   - Perilla con Shift = ajuste fino y clic derecho = escribir el valor
//==============================================================================
namespace moai
{
    constexpr float barHeight = 52.0f;   // alto de diseño de la barra superior
    extern const char* const pluginVersion;

    juce::Font uiFont (float size, bool bold, float letterSpacing = 0.0f);

    namespace Colours
    {
        const juce::Colour background { 0xff050608 };
        const juce::Colour labelText  { 0xffaeb6c8 };
        const juce::Colour dimText    { 0xff8c97ae };
        const juce::Colour faint      { 0xff56607a };
        const juce::Colour bezelTop   { 0xff5a6478 };
        const juce::Colour bezelMid   { 0xff1a1e28 };
        const juce::Colour bezelBot   { 0xff3a4254 };
        const juce::Colour screenText { 0xff5f9c79 };
        const juce::Colour screenOff  { 0xff3f7a58 };
        const juce::Colour mint       { 0xff8fd9ae };
        const juce::Colour mintGlow   { 0xff57d99a };
        const juce::Colour green      { 57, 255, 136 };
        const juce::Colour blue       { 46, 123, 255 };
        const juce::Colour amber      { 0xffffb84d };
        const juce::Colour red        { 0xffff5a5a };
    }

    // Piezas de dibujo comunes (coordenadas de diseño)
    void drawScreen (juce::Graphics&, juce::Rectangle<float> r);                       // pantallita menta con bisel
    void drawMetalButton (juce::Graphics&, juce::Rectangle<float> r, bool highlighted, bool down);
    void drawPill (juce::Graphics&, juce::Rectangle<float> r);                         // opción activa (menta)

    //==========================================================================
    // Ajustes globales del plugin (no dependen del proyecto del DAW)
    class Settings
    {
    public:
        explicit Settings (const juce::String& pluginName);

        int  uiScalePercent() const;           // 75 / 100 / 125 / 150
        void setUiScalePercent (int);
        bool tooltipsEnabled() const;
        void setTooltipsEnabled (bool);
        bool spanishHelp() const;              // idioma de la ayuda
        void setSpanishHelp (bool);

        std::function<void()> onChange;

    private:
        juce::PropertiesFile::Options options() const;
        std::unique_ptr<juce::PropertiesFile> file;
        juce::String name;
    };

    //==========================================================================
    // Textos de ayuda (tooltips) en inglés y español
    struct HelpText { const char* id; const char* en; const char* es; };

    class Help
    {
    public:
        void add (std::initializer_list<HelpText>);
        juce::String get (const juce::String& id, bool spanish) const;
    private:
        std::map<juce::String, std::pair<juce::String, juce::String>> texts;
    };

    void addCommonHelp (Help&);   // barra superior, settings, gestos

    //==========================================================================
    // Presets de fábrica: nombre + valores (en unidades reales). Lo no listado = valor por defecto.
    struct FactoryPreset
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> values;
    };

    //==========================================================================
    // Parámetros comunes y estado del proyecto (con versión)
    constexpr int stateVersion = 2;     // 1 = v1.0 (sin presets), 2 = v1.1
    void addOutputParameters (juce::AudioProcessorValueTreeState::ParameterLayout&);   // bypass + outTrim

    class PresetManager;
    void writeState (juce::AudioProcessor&, juce::AudioProcessorValueTreeState&, PresetManager&, juce::MemoryBlock&);
    void readState  (juce::AudioProcessor&, juce::AudioProcessorValueTreeState&, PresetManager&, const void* data, int size);

    //==========================================================================
    // Bypass sin clics + Output trim + medidor de salida.
    // Uso en processBlock:  begin (buffer)  ->  procesamiento normal  ->  end (buffer)
    class OutputStage
    {
    public:
        void prepare (double sampleRate, int numChannels, int maxBlock);
        void begin (const juce::AudioBuffer<float>& buffer, int numChannels, int numSamples);
        void end (juce::AudioBuffer<float>& buffer, int numChannels, int numSamples, bool bypassed, float trimDb);

        std::atomic<float> peakL { 0.0f }, peakR { 0.0f };   // para el medidor (los lee la interfaz)

    private:
        juce::AudioBuffer<float> input;
        juce::SmoothedValue<float> wet, trim;
    };

    //==========================================================================
    // Presets, A / B y estado del "preset modificado"
    class PresetManager : private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        PresetManager (juce::AudioProcessorValueTreeState&, juce::UndoManager&, const juce::String& pluginName,
                       std::vector<FactoryPreset> factory, juce::StringArray excludedIds);
        ~PresetManager() override;

        // ---- Lista: 0 = Init, 1..N fábrica, luego los de usuario ----
        int  getNumFactory() const                { return (int) factory.size(); }
        const juce::String& getFactoryName (int i) const { return factory[(size_t) i].name; }
        juce::StringArray getUserNames() const;
        juce::File getUserFolder() const;

        void loadInit();
        void loadFactory (int index);
        bool loadUser (const juce::String& name);
        bool saveUser (const juce::String& name);       // crea o sobrescribe
        bool deleteUser (const juce::String& name);
        void next();
        void previous();

        juce::String getCurrentName() const       { const juce::SpinLock::ScopedLockType l (nameLock); return currentName; }
        bool isCurrentUser() const                { return currentIsUser; }
        bool isModified() const                   { return modified.load(); }

        // ---- A / B ----
        int  getActiveSlot() const                { return activeSlot; }
        void selectSlot (int slot);                     // guarda el actual y carga el otro
        void copyActiveToOther();

        // ---- Estado en el proyecto del DAW ----
        void writeTo (juce::ValueTree& state) const;   // nombre del preset + A/B
        void readFrom (const juce::ValueTree& state);

        int getChangeCount() const                { return changeCount.load(); }   // la barra lo revisa con su timer

    private:
        void parameterChanged (const juce::String&, float) override;
        juce::ValueTree snapshot() const;
        void applySnapshot (const juce::ValueTree& values);
        void applyValues (const std::vector<std::pair<juce::String, float>>& values);
        void setCurrent (const juce::String& name, bool isUser);
        juce::RangedAudioParameter* param (const juce::String& id) const;
        juce::StringArray orderedList() const;          // Init, fábrica..., usuario...

        juce::AudioProcessorValueTreeState& apvts;
        juce::UndoManager& undo;
        juce::String pluginName;
        std::vector<FactoryPreset> factory;
        juce::StringArray excluded;                     // parámetros que un preset no toca (p. ej. bypass)
        juce::String currentName { "Init" };
        bool currentIsUser = false;
        std::atomic<bool> modified { false };
        bool loading = false;
        int activeSlot = 0;
        juce::ValueTree slots[2];
        juce::String slotNames[2];
        std::atomic<int> changeCount { 0 };
        mutable juce::SpinLock nameLock;
    };

    //==========================================================================
    // Perilla con Shift = ajuste fino y clic derecho = escribir el valor
    class MoaiSlider : public juce::Slider
    {
    public:
        MoaiSlider();
        void mouseDown (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        juce::RangedAudioParameter* parameter = nullptr;   // para convertir el texto escrito
    private:
        void showValueEditor();
    };

    //==========================================================================
    // Selector de opciones con estilo de pantallita (para Settings)
    class Segmented : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit Segmented (juce::StringArray options);
        void setSelected (int);
        int  getSelected() const { return selected; }
        std::function<void (int)> onChange;
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
    private:
        juce::StringArray options;
        int selected = 0;
    };

    //==========================================================================
    // Botón metálico con texto o ícono
    class BarButton : public juce::Button
    {
    public:
        enum class Icon { none, undo, redo, gear, close };
        BarButton (const juce::String& text, Icon icon = Icon::none);
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
        bool ledOn = false;                      // para BYPASS
        bool hasLed = false;
        bool round = false;                      // engranaje
    private:
        Icon icon;
    };

    //==========================================================================
    // Barra superior: PRESET ◀ ▶ nombre ▾ | SAVE | ↶ ↷ | A B ⇄ | OUT | BYPASS | ⚙
    class TopBar : public juce::Component, public juce::TooltipClient, private juce::Timer
    {
    public:
        TopBar (PresetManager&, juce::UndoManager&, juce::AudioProcessorValueTreeState&,
                OutputStage&, float designWidth);

        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;

        void applyHelp (const Help*, bool spanish);    // nullptr = sin tooltips
        void tick() { timerCallback(); }               // (pruebas)
        juce::String getTooltip() override;            // tooltip según la zona bajo el mouse

        std::function<void()> onSave, onSettings;

    private:
        void timerCallback() override;
        void showPresetMenu();
        juce::Rectangle<float> presetScreen() const, prevArea() const, nextArea() const, nameArea() const,
                               abScreen() const, slotArea (int) const, copyArea() const, outScreen() const;
        float scale() const { return (float) getWidth() / designW; }

        PresetManager& presets;
        juce::UndoManager& undo;
        juce::AudioProcessorValueTreeState& apvts;
        OutputStage& meter;
        float designW;

        BarButton saveButton { "SAVE" }, undoButton { "", BarButton::Icon::undo }, redoButton { "", BarButton::Icon::redo },
                  bypassButton { "BYPASS" }, settingsButton { "", BarButton::Icon::gear };
        std::unique_ptr<juce::ButtonParameterAttachment> bypassAttachment;
        const Help* help = nullptr;
        bool spanish = false;
        int lastChange = -1;

        float levelL = 0.0f, levelR = 0.0f, holdL = 0.0f, holdR = 0.0f;
        int holdCountL = 0, holdCountR = 0;
        float trimDragStart = 0.0f;
        bool draggingTrim = false;
    };

    //==========================================================================
    // Paneles que se abren sobre la interfaz (Settings, About, Save)
    class Overlay : public juce::Component
    {
    public:
        Overlay (float designW, float designH);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;   // clic fuera del panel = cerrar
        void close();
        void setDesignHeight (float h) { designH = h; resized(); repaint(); }
        std::function<void()> onClose;
    protected:
        float scale() const { return (float) getWidth() / designW; }
        virtual juce::Rectangle<float> panelArea() const = 0;     // coordenadas de diseño
        virtual void paintPanel (juce::Graphics&) {}              // contenido dibujado (diseño)
        juce::Rectangle<int> scaled (juce::Rectangle<float>) const;
        float designW, designH;
        BarButton closeButton { "", BarButton::Icon::close };
    };

    class SettingsOverlay : public Overlay
    {
    public:
        SettingsOverlay (Settings&, float designW, float designH);
        void resized() override;
        void sync();
        void setHelp (const juce::String& scaleTip, const juce::String& tipsTip, const juce::String& langTip,
                      const juce::String& resetTip, const juce::String& aboutTip);
        std::function<void()> onReset, onAbout;
    protected:
        juce::Rectangle<float> panelArea() const override;
        void paintPanel (juce::Graphics&) override;
    private:
        Settings& settings;
        Segmented scaleSel { { "75%", "100%", "125%", "150%" } },
                  tipsSel  { { "ON", "OFF" } },
                  langSel  { { "ENGLISH", juce::String (juce::CharPointer_UTF8 ("ESPA\xc3\x91OL")) } };
        BarButton resetButton { "RESET TO DEFAULTS" }, aboutButton { "ABOUT" };
    };

    class AboutOverlay : public Overlay
    {
    public:
        AboutOverlay (const juce::String& pluginName, const juce::String& subtitle, juce::Image title,
                      juce::StringArray credits, float designW, float designH);
    protected:
        juce::Rectangle<float> panelArea() const override;
        void paintPanel (juce::Graphics&) override;
    private:
        juce::String name, subtitle;
        juce::Image title;
        juce::StringArray credits;
    };

    class SaveOverlay : public Overlay
    {
    public:
        SaveOverlay (float designW, float designH);
        void open (const juce::String& suggestedName);
        void resized() override;
        std::function<void (const juce::String&)> onSave;
    protected:
        juce::Rectangle<float> panelArea() const override;
        void paintPanel (juce::Graphics&) override;
    private:
        void commit();
        juce::TextEditor nameEditor;
        BarButton okButton { "SAVE" }, cancelButton { "CANCEL" };
    };

    //==========================================================================
    // Tooltip con el estilo Moai (se usa desde el LookAndFeel de cada plugin)
    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height);
    juce::Rectangle<int> tooltipBounds (const juce::String& text, juce::Point<int> screenPos, juce::Rectangle<int> parentArea);

    //==========================================================================
    // Todo lo que se agrega al editor de cada plugin: barra superior, paneles, tooltips, tamaño.
    // El diseño original queda intacto: solo se dibuja 52 px más abajo.
    struct AboutInfo
    {
        juce::String name, subtitle;
        juce::Image titleImage;
        juce::StringArray credits;
    };

    class EditorChrome
    {
    public:
        EditorChrome (juce::AudioProcessorEditor&, PresetManager&, juce::UndoManager&, juce::AudioProcessorValueTreeState&,
                      OutputStage&, Settings&, Help&, float designW, float contentH, int defaultWidth, AboutInfo);
        ~EditorChrome();

        void layout();                                   // llamar desde resized()
        void setContentHeight (float h);                 // Multirate: espectro abierto / plegado
        float fullHeight() const { return contentH + barHeight; }
        juce::String tip (const juce::String& id, bool withGestures = false) const;   // "" si los tooltips están apagados
        std::function<void()> onHelpChanged;             // el editor vuelve a poner sus tooltips

    private:
        void settingsChanged();
        void setScale (int percent);

        juce::AudioProcessorEditor& editor;
        PresetManager& presets;
        juce::UndoManager& undo;
        Settings& settings;
        Help& help;
        float designW, contentH;
        int defaultWidth;
        int lastScale = 0;

        TopBar bar;
        SettingsOverlay settingsPanel;
        AboutOverlay aboutPanel;
        SaveOverlay savePanel;
        std::unique_ptr<juce::TooltipWindow> tooltipWindow;
        std::unique_ptr<juce::MouseListener> undoListener;
    };

    //==========================================================================
    // Inicia una nueva transacción de Undo en cada clic dentro de la interfaz
    class UndoGestureListener : public juce::MouseListener
    {
    public:
        explicit UndoGestureListener (juce::UndoManager& u) : undo (u) {}
        void mouseDown (const juce::MouseEvent&) override { undo.beginNewTransaction(); }
    private:
        juce::UndoManager& undo;
    };
}
