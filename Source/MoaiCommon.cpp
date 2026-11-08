#include "MoaiCommon.h"

namespace moai
{
const char* const pluginVersion = "1.1.0";

juce::Font uiFont (float size, bool bold, float letterSpacing)
{
    auto opts = juce::FontOptions().withPointHeight (size).withStyle (bold ? "Bold" : "Regular");
    return juce::Font (opts).withExtraKerningFactor (letterSpacing / size);
}

static juce::String utf8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

//==============================================================================
// Dibujo
void drawScreen (juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::ColourGradient bezel (Colours::bezelTop, 0, r.getY() - 3, Colours::bezelBot, 0, r.getBottom() + 3, false);
    bezel.addColour (0.5, Colours::bezelMid);
    g.setGradientFill (bezel);
    g.fillRoundedRectangle (r.expanded (3.0f), 9.0f);
    g.setColour (juce::Colour (0xff030805));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, 7.0f, 1.0f);
    g.setColour (juce::Colour (0xff06100b));
    g.fillRoundedRectangle (r.reduced (3.0f), 5.0f);
}

void drawMetalButton (juce::Graphics& g, juce::Rectangle<float> r, bool highlighted, bool down)
{
    const float rad = juce::jmin (5.5f, r.getHeight() * 0.5f);
    juce::ColourGradient bezel (Colours::bezelTop, 0, r.getY() - 2, Colours::bezelBot, 0, r.getBottom() + 2, false);
    bezel.addColour (0.5, Colours::bezelMid);
    g.setGradientFill (bezel);
    g.fillRoundedRectangle (r.expanded (2.0f), rad + 1.5f);

    auto top = juce::Colour (0xff2a303d), bot = juce::Colour (0xff11141b);
    if (highlighted) { top = top.brighter (0.25f); bot = bot.brighter (0.2f); }
    if (down) std::swap (top, bot);
    g.setGradientFill (juce::ColourGradient (top, 0, r.getY(), bot, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, rad, 1.0f);
}

void drawPill (juce::Graphics& g, juce::Rectangle<float> r)
{
    const float rad = r.getHeight() * 0.5f;
    g.setColour (Colours::mintGlow.withAlpha (0.16f));
    g.fillRoundedRectangle (r, rad);
    g.setColour (Colours::mint);
    g.drawRoundedRectangle (r, rad, 1.1f);
}

//==============================================================================
// Settings
Settings::Settings (const juce::String& pluginName) : name (pluginName)
{
    file = std::make_unique<juce::PropertiesFile> (options());
}

juce::PropertiesFile::Options Settings::options() const
{
    juce::PropertiesFile::Options o;
    o.applicationName     = name;
    o.folderName          = "Moaiact";
    o.filenameSuffix      = "settings";
    o.osxLibrarySubFolder = "Application Support";
    o.commonToAllUsers    = false;
    return o;
}

static bool systemIsSpanish()
{
    return juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("es")
        || juce::SystemStats::getDisplayLanguage().startsWithIgnoreCase ("es");
}

int  Settings::uiScalePercent() const      { return juce::jlimit (75, 150, file->getIntValue ("uiScale", 100)); }
bool Settings::tooltipsEnabled() const     { return file->getBoolValue ("tooltips", true); }
bool Settings::spanishHelp() const         { return file->getBoolValue ("spanishHelp", systemIsSpanish()); }

void Settings::setUiScalePercent (int p)   { file->setValue ("uiScale", p);       file->saveIfNeeded(); if (onChange) onChange(); }
void Settings::setTooltipsEnabled (bool b) { file->setValue ("tooltips", b);      file->saveIfNeeded(); if (onChange) onChange(); }
void Settings::setSpanishHelp (bool b)     { file->setValue ("spanishHelp", b);   file->saveIfNeeded(); if (onChange) onChange(); }

//==============================================================================
// Ayuda
void Help::add (std::initializer_list<HelpText> list)
{
    for (auto& h : list)
        texts[h.id] = { utf8 (h.en), utf8 (h.es) };
}

juce::String Help::get (const juce::String& id, bool spanish) const
{
    auto it = texts.find (id);
    if (it == texts.end()) return {};
    return spanish ? it->second.second : it->second.first;
}

void addCommonHelp (Help& h)
{
    h.add ({
        { "preset",   "Presets: click the name to open the list. The arrows load the previous / next preset. A * means the preset was changed.",
                      "Presets: haz clic en el nombre para abrir la lista. Las flechas cargan el preset anterior / siguiente. Un * indica que el preset fue modificado." },
        { "save",     "Save the current sound as a user preset.",
                      "Guarda el sonido actual como preset de usuario." },
        { "undo",     "Undo the last change.",
                      "Deshace el último cambio." },
        { "redo",     "Redo the last undone change.",
                      "Rehace el último cambio deshecho." },
        { "ab",       "A / B: two independent settings to compare. Click A or B to switch.",
                      "A / B: dos ajustes independientes para comparar. Haz clic en A o B para cambiar." },
        { "abcopy",   "Copy the active slot to the other one.",
                      "Copia el ajuste activo en el otro." },
        { "out",      "Output level. Drag up / down to trim the output (-12 to +6 dB). Double-click: 0 dB.",
                      "Nivel de salida. Arrastra arriba / abajo para ajustar la salida (-12 a +6 dB). Doble clic: 0 dB." },
        { "bypass",   "Bypass: hear the original signal. The switch is smooth, without clicks.",
                      "Bypass: escucha la señal original. El cambio es suave, sin clics." },
        { "settings", "Settings: interface size, tooltips, help language and information.",
                      "Ajustes: tamaño de la interfaz, ayudas, idioma de la ayuda e información." },
        { "uiScale",  "Size of the plugin window. You can also drag the lower right corner.",
                      "Tamaño de la ventana del plugin. También puedes arrastrar la esquina inferior derecha." },
        { "tooltips", "Show or hide these help boxes.",
                      "Muestra u oculta estos cuadros de ayuda." },
        { "language", "Language of the help boxes. The interface always stays in English.",
                      "Idioma de los cuadros de ayuda. La interfaz siempre se mantiene en inglés." },
        { "reset",    "Return every control to its default value (Init).",
                      "Vuelve todos los controles a su valor por defecto (Init)." },
        { "about",    "Version and credits.",
                      "Versión y créditos." },
        { "gestures", "Shift + drag: fine adjustment  ·  Double-click: default value  ·  Right-click: type a value",
                      "Shift + arrastrar: ajuste fino  ·  Doble clic: valor por defecto  ·  Clic derecho: escribir un valor" },
    });
}

//==============================================================================
// Parámetros comunes + estado
void addOutputParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using PID = juce::ParameterID;
    layout.add (std::make_unique<juce::AudioParameterBool> (PID { "bypass", 2 }, "Bypass", false));
    auto db = juce::AudioParameterFloatAttributes().withLabel ("dB")
                  .withStringFromValueFunction ([] (float v, int) { return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; });
    layout.add (std::make_unique<juce::AudioParameterFloat> (PID { "outTrim", 2 }, "Output",
                                                             juce::NormalisableRange<float> (-12.0f, 6.0f, 0.1f), 0.0f, db));
}

void writeState (juce::AudioProcessor& proc, juce::AudioProcessorValueTreeState& apvts, PresetManager& pm, juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", stateVersion, nullptr);
    state.setProperty ("pluginVersion", pluginVersion, nullptr);
    pm.writeTo (state);
    if (auto xml = state.createXml())
        proc.copyXmlToBinary (*xml, dest);
}

void readState (juce::AudioProcessor& proc, juce::AudioProcessorValueTreeState& apvts, PresetManager& pm, const void* data, int size)
{
    auto xml = proc.getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    auto tree = juce::ValueTree::fromXml (*xml);
    const auto original = tree.createCopy();

    // Limpia lo que no es un parámetro antes de entregarlo a la APVTS
    for (int i = tree.getNumChildren(); --i >= 0;)
        if (! tree.getChild (i).hasProperty ("id"))
            tree.removeChild (i, nullptr);

    // Proyectos guardados con una versión anterior: los parámetros nuevos parten en su valor por defecto
    for (auto* p : proc.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! tree.getChildWithProperty ("id", rp->getParameterID()).isValid())
            {
                juce::ValueTree child ("PARAM");
                child.setProperty ("id", rp->getParameterID(), nullptr);
                child.setProperty ("value", rp->convertFrom0to1 (rp->getDefaultValue()), nullptr);
                tree.appendChild (child, nullptr);
            }

    apvts.replaceState (tree);
    pm.readFrom (original);   // nombre del preset + A/B (versión 2); después, para que no quede como "modificado"
}

//==============================================================================
// OutputStage
void OutputStage::prepare (double sampleRate, int numChannels, int maxBlock)
{
    input.setSize (juce::jmax (1, numChannels), juce::jmax (1, maxBlock));
    wet.reset (sampleRate, 0.03);
    trim.reset (sampleRate, 0.05);
}

void OutputStage::begin (const juce::AudioBuffer<float>& buffer, int numChannels, int numSamples)
{
    if (input.getNumSamples() < numSamples || input.getNumChannels() < numChannels)
        input.setSize (juce::jmax (numChannels, input.getNumChannels()), numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        input.copyFrom (ch, 0, buffer, ch, 0, numSamples);
}

void OutputStage::end (juce::AudioBuffer<float>& buffer, int numChannels, int numSamples, bool bypassed, float trimDb)
{
    wet.setTargetValue (bypassed ? 0.0f : 1.0f);
    trim.setTargetValue (juce::Decibels::decibelsToGain (trimDb));

    const bool smooth = wet.isSmoothing() || trim.isSmoothing();
    if (! smooth)
    {
        const float w = wet.getCurrentValue(), t = trim.getCurrentValue();
        if (w <= 0.0f)
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.copyFrom (ch, 0, input, ch, 0, numSamples);
        else if (t != 1.0f)
            buffer.applyGain (0, numSamples, t);
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float w = wet.getNextValue(), t = trim.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* d = buffer.getWritePointer (ch);
                d[i] = d[i] * w * t + input.getSample (ch, i) * (1.0f - w);
            }
        }
    }

    auto updatePeak = [] (std::atomic<float>& p, float v)
    {
        float old = p.load();
        while (v > old && ! p.compare_exchange_weak (old, v)) {}
    };
    if (numChannels > 0) updatePeak (peakL, buffer.getMagnitude (0, 0, numSamples));
    updatePeak (peakR, buffer.getMagnitude (numChannels > 1 ? 1 : 0, 0, numSamples));
}

//==============================================================================
// PresetManager
static const juce::String presetExtension (".moaipreset");

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s, juce::UndoManager& u, const juce::String& name,
                              std::vector<FactoryPreset> f, juce::StringArray excludedIds)
    : apvts (s), undo (u), pluginName (name), factory (std::move (f)), excluded (std::move (excludedIds))
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.addParameterListener (rp->getParameterID(), this);
}

PresetManager::~PresetManager()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.removeParameterListener (rp->getParameterID(), this);
}

juce::RangedAudioParameter* PresetManager::param (const juce::String& id) const
{
    return apvts.getParameter (id);
}

void PresetManager::parameterChanged (const juce::String& id, float)
{
    if (loading || excluded.contains (id)) return;
    if (! modified.exchange (true))
        ++changeCount;
}

juce::File PresetManager::getUserFolder() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Moaiact").getChildFile (pluginName).getChildFile ("Presets");
}

juce::StringArray PresetManager::getUserNames() const
{
    juce::StringArray names;
    for (auto& f : getUserFolder().findChildFiles (juce::File::findFiles, false, "*" + presetExtension))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

juce::StringArray PresetManager::orderedList() const
{
    juce::StringArray list { "Init" };
    for (auto& p : factory) list.add (p.name);
    list.addArray (getUserNames());
    return list;
}

void PresetManager::setCurrent (const juce::String& name, bool isUser)
{
    {
        const juce::SpinLock::ScopedLockType l (nameLock);
        currentName = name;
    }
    currentIsUser = isUser;
    modified = false;
    ++changeCount;
}

juce::ValueTree PresetManager::snapshot() const
{
    juce::ValueTree v ("Values");
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! excluded.contains (rp->getParameterID()))
                v.setProperty (rp->getParameterID(), rp->convertFrom0to1 (rp->getValue()), nullptr);
    return v;
}

void PresetManager::applySnapshot (const juce::ValueTree& values)
{
    std::vector<std::pair<juce::String, float>> list;
    for (int i = 0; i < values.getNumProperties(); ++i)
    {
        auto id = values.getPropertyName (i).toString();
        list.push_back ({ id, (float) values.getProperty (id) });
    }
    applyValues (list);
}

// Todos los parámetros (salvo los excluidos) vuelven a su valor por defecto y luego se aplican los del preset.
// Se hace parámetro por parámetro para que el DAW y el Undo lo registren.
void PresetManager::applyValues (const std::vector<std::pair<juce::String, float>>& values)
{
    const juce::ScopedValueSetter<bool> svs (loading, true);
    undo.beginNewTransaction();

    for (auto* p : apvts.processor.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p);
        if (rp == nullptr || excluded.contains (rp->getParameterID())) continue;

        float target = rp->getDefaultValue();
        for (auto& v : values)
            if (v.first == rp->getParameterID())
                target = rp->convertTo0to1 (v.second);

        if (std::abs (rp->getValue() - target) > 1.0e-6f)
        {
            rp->beginChangeGesture();
            rp->setValueNotifyingHost (target);
            rp->endChangeGesture();
        }
    }
}

void PresetManager::loadInit()
{
    applyValues ({});
    setCurrent ("Init", false);
}

void PresetManager::loadFactory (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) factory.size())) return;
    applyValues (factory[(size_t) index].values);
    setCurrent (factory[(size_t) index].name, false);
}

bool PresetManager::loadUser (const juce::String& name)
{
    auto file = getUserFolder().getChildFile (name + presetExtension);
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr) return false;
    auto tree = juce::ValueTree::fromXml (*xml);
    auto values = tree.getChildWithName ("Values");
    if (! values.isValid()) return false;
    applySnapshot (values);
    setCurrent (name, true);
    return true;
}

bool PresetManager::saveUser (const juce::String& rawName)
{
    auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return false;
    auto folder = getUserFolder();
    if (! folder.createDirectory()) return false;

    juce::ValueTree tree ("MoaiPreset");
    tree.setProperty ("plugin", pluginName, nullptr);
    tree.setProperty ("version", pluginVersion, nullptr);
    tree.setProperty ("name", name, nullptr);
    tree.appendChild (snapshot(), nullptr);

    if (auto xml = tree.createXml())
        if (xml->writeTo (folder.getChildFile (name + presetExtension)))
        {
            setCurrent (name, true);
            return true;
        }
    return false;
}

bool PresetManager::deleteUser (const juce::String& name)
{
    auto file = getUserFolder().getChildFile (name + presetExtension);
    if (! file.existsAsFile()) return false;
    const bool ok = file.moveToTrash();   // a la papelera: se puede recuperar
    if (ok && currentIsUser && getCurrentName() == name)
        setCurrent (name, false), modified = true, ++changeCount;
    return ok;
}

static void loadByName (PresetManager& pm, const juce::String& name, int factoryCount)
{
    if (name == "Init") { pm.loadInit(); return; }
    for (int i = 0; i < factoryCount; ++i)
        if (pm.getFactoryName (i) == name) { pm.loadFactory (i); return; }
    pm.loadUser (name);
}

void PresetManager::next()
{
    auto list = orderedList();
    int i = list.indexOf (getCurrentName());
    loadByName (*this, list[(i + 1) % list.size()], getNumFactory());
}

void PresetManager::previous()
{
    auto list = orderedList();
    int i = list.indexOf (getCurrentName());
    if (i < 0) i = 0;
    loadByName (*this, list[(i - 1 + list.size()) % list.size()], getNumFactory());
}

void PresetManager::selectSlot (int slot)
{
    if (slot == activeSlot || ! juce::isPositiveAndBelow (slot, 2)) return;
    slots[activeSlot] = snapshot();
    slotNames[activeSlot] = getCurrentName() + (isModified() ? "*" : "");
    const bool wasUser = currentIsUser;
    activeSlot = slot;

    if (slots[slot].isValid())
    {
        applySnapshot (slots[slot]);
        auto n = slotNames[slot];
        const bool mod = n.endsWith ("*");
        setCurrent (mod ? n.dropLastCharacters (1) : n, wasUser);
        modified = mod;
    }
    ++changeCount;
}

void PresetManager::copyActiveToOther()
{
    const int other = 1 - activeSlot;
    slots[other] = snapshot();
    slotNames[other] = getCurrentName() + (isModified() ? "*" : "");
    ++changeCount;
}

void PresetManager::writeTo (juce::ValueTree& state) const
{
    state.setProperty ("presetName", getCurrentName(), nullptr);
    state.setProperty ("presetUser", currentIsUser, nullptr);
    state.setProperty ("presetModified", isModified(), nullptr);
    state.setProperty ("abSlot", activeSlot, nullptr);
    const int other = 1 - activeSlot;
    if (slots[other].isValid())
    {
        juce::ValueTree s ("OtherSlot");
        s.setProperty ("name", slotNames[other], nullptr);
        s.appendChild (slots[other].createCopy(), nullptr);
        state.appendChild (s, nullptr);
    }
}

void PresetManager::readFrom (const juce::ValueTree& state)
{
    {
        const juce::SpinLock::ScopedLockType l (nameLock);
        currentName = state.getProperty ("presetName", "Init").toString();
    }
    currentIsUser = (bool) state.getProperty ("presetUser", false);
    modified = (bool) state.getProperty ("presetModified", false);

    // Proyecto de la v1.0 (sin nombre de preset): si algún control no está en su valor por defecto, se marca "Init *"
    if (! state.hasProperty ("presetName"))
        for (auto* p : apvts.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                if (! excluded.contains (rp->getParameterID()) && std::abs (rp->getValue() - rp->getDefaultValue()) > 1.0e-4f)
                    modified = true;
    activeSlot = juce::jlimit (0, 1, (int) state.getProperty ("abSlot", 0));
    slots[0] = slots[1] = {};
    auto s = state.getChildWithName ("OtherSlot");
    if (s.isValid())
    {
        slots[1 - activeSlot] = s.getChildWithName ("Values").createCopy();
        slotNames[1 - activeSlot] = s.getProperty ("name").toString();
    }
    ++changeCount;
}

//==============================================================================
// MoaiSlider
MoaiSlider::MoaiSlider() {}

void MoaiSlider::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showValueEditor();
        return;
    }
    setMouseDragSensitivity (e.mods.isShiftDown() ? 1500 : 250);
    juce::Slider::mouseDown (e);
}

void MoaiSlider::mouseUp (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    juce::Slider::mouseUp (e);
    setMouseDragSensitivity (250);
}

namespace
{
    // Cuadro para escribir el valor de una perilla (se borra solo al terminar)
    class ValueEntry : public juce::TextEditor
    {
    public:
        ValueEntry (MoaiSlider& s, float scale) : slider (&s)
        {
            setJustification (juce::Justification::centred);
            setFont (uiFont (13.0f * scale, true, 1.0f));
            setColour (backgroundColourId, juce::Colour (0xff06100b));
            setColour (textColourId, Colours::mint);
            setColour (outlineColourId, Colours::screenText);
            setColour (focusedOutlineColourId, Colours::mint);
            setColour (highlightColourId, Colours::mintGlow.withAlpha (0.3f));
            setColour (juce::CaretComponent::caretColourId, Colours::mint);
            setText (s.getTextFromValue (s.getValue()), false);
            selectAll();
            onReturnKey = [this] { commit(); };
            onEscapeKey = [this] { finish(); };
            onFocusLost = [this] { finish(); };
        }

    private:
        void commit()
        {
            if (slider != nullptr)
            {
                auto text = getText().trim().toLowerCase();
                double v = text.retainCharacters ("0123456789.,-").replaceCharacter (',', '.').getDoubleValue();
                if (text.containsChar ('k')) v *= 1000.0;
                auto* p = slider->parameter;
                const auto shown = slider->getTextFromValue (slider->getValue());
                if (p != nullptr && shown.containsChar ('%') && slider->getMaximum() <= 1.0)
                    v /= 100.0;
                if (text.retainCharacters ("0123456789").isNotEmpty())
                    slider->setValue (juce::jlimit (slider->getMinimum(), slider->getMaximum(), v), juce::sendNotificationSync);
            }
            finish();
        }

        void finish()
        {
            if (done) return;
            done = true;
            juce::Component::SafePointer<juce::Component> self (this);
            juce::MessageManager::callAsync ([self] { delete self.getComponent(); });
        }

        juce::Component::SafePointer<MoaiSlider> slider;
        bool done = false;
    };
}

void MoaiSlider::showValueEditor()
{
    auto* top = getTopLevelComponent();
    if (top == nullptr) return;
    const float scale = (float) getWidth() / 140.0f;   // perillas de ~140 px de diseño
    auto* ed = new ValueEntry (*this, juce::jlimit (0.7f, 1.6f, scale));
    auto centre = top->getLocalPoint (this, getLocalBounds().getCentre());
    const int w = juce::jmax (70, (int) (getWidth() * 0.7f)), h = juce::jmax (22, (int) (getHeight() * 0.18f));
    ed->setBounds (juce::Rectangle<int> (w, h).withCentre (centre));
    top->addAndMakeVisible (ed);
    ed->grabKeyboardFocus();
}

//==============================================================================
// Segmented
Segmented::Segmented (juce::StringArray o) : options (std::move (o)) {}

void Segmented::setSelected (int i) { selected = i; repaint(); }

void Segmented::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float s = r.getHeight() / 30.0f;
    drawScreen (g, r.reduced (3.0f * s));
    const float w = (r.getWidth() - 12.0f * s) / (float) options.size();
    for (int i = 0; i < options.size(); ++i)
    {
        auto cell = juce::Rectangle<float> (r.getX() + 6.0f * s + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f * s, 7.0f * s);
        if (i == selected) drawPill (g, cell);
        g.setColour (i == selected ? Colours::mint : Colours::screenOff);
        g.setFont (uiFont (10.5f * s, true, 1.2f * s));
        g.drawText (options[i], cell, juce::Justification::centred);
    }
}

void Segmented::mouseDown (const juce::MouseEvent& e)
{
    const float w = (float) getWidth() / (float) options.size();
    int i = juce::jlimit (0, options.size() - 1, (int) ((float) e.x / w));
    if (i != selected)
    {
        setSelected (i);
        if (onChange) onChange (i);
    }
}

//==============================================================================
// BarButton
BarButton::BarButton (const juce::String& text, Icon i) : juce::Button (text), icon (i) {}

void BarButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat();
    const float s = r.getHeight() / 28.0f;
    auto inner = r.reduced (2.0f * s);
    const auto textCol = isEnabled() ? (highlighted ? Colours::labelText : Colours::dimText) : Colours::faint.withAlpha (0.6f);

    if (round)
    {
        juce::ColourGradient bezel (Colours::bezelTop, 0, r.getY(), Colours::bezelBot, 0, r.getBottom(), false);
        bezel.addColour (0.5, Colours::bezelMid);
        g.setGradientFill (bezel);
        g.fillEllipse (r);
        auto top = juce::Colour (0xff2a303d), bot = juce::Colour (0xff11141b);
        if (highlighted) { top = top.brighter (0.25f); bot = bot.brighter (0.2f); }
        if (down) std::swap (top, bot);
        g.setGradientFill (juce::ColourGradient (top, 0, inner.getY(), bot, 0, inner.getBottom(), false));
        auto in2 = r.reduced (2.5f * s);
        g.fillEllipse (in2);
        g.setColour (juce::Colours::black);
        g.drawEllipse (in2, 1.0f);
    }
    else
        drawMetalButton (g, inner, highlighted, down);

    const auto c = inner.getCentre();
    if (icon == Icon::undo || icon == Icon::redo)
    {
        const float d = icon == Icon::undo ? 1.0f : -1.0f;
        juce::Path p;
        const float rr = 5.5f * s;
        p.addCentredArc (c.x, c.y + 1.5f * s, rr, rr, 0.0f, d * -2.1f, d * 1.1f, true);
        g.setColour (textCol);
        g.strokePath (p, juce::PathStrokeType (1.6f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const auto tip = p.getPointAlongPath (0.0f);
        juce::Path head;
        head.addTriangle (tip.x - 3.2f * s, tip.y - 0.5f * s, tip.x + 3.2f * s, tip.y - 0.5f * s, tip.x, tip.y + 3.6f * s);
        head.applyTransform (juce::AffineTransform::rotation (d * 0.5f, tip.x, tip.y));
        g.fillPath (head);
    }
    else if (icon == Icon::gear)
    {
        juce::Path gear;
        const float ro = 7.4f * s, ri = 5.6f * s;
        for (int i = 0; i < 16; ++i)
        {
            const float a0 = juce::MathConstants<float>::twoPi * (float) i / 16.0f;
            const float a1 = a0 + juce::MathConstants<float>::twoPi / 16.0f;
            const float rad = (i % 2 == 0) ? ro : ri;
            auto pt = [&] (float a) { return juce::Point<float> (c.x + rad * std::sin (a), c.y - rad * std::cos (a)); };
            if (i == 0) gear.startNewSubPath (pt (a0)); else gear.lineTo (pt (a0));
            gear.lineTo (pt (a1));
        }
        gear.closeSubPath();
        gear.addEllipse (c.x - 2.6f * s, c.y - 2.6f * s, 5.2f * s, 5.2f * s);
        gear.setUsingNonZeroWinding (false);
        g.setColour (highlighted ? Colours::labelText.brighter (0.2f) : Colours::labelText);
        g.fillPath (gear);
    }
    else if (icon == Icon::close)
    {
        g.setColour (textCol);
        const float k = 4.5f * s;
        g.drawLine (c.x - k, c.y - k, c.x + k, c.y + k, 1.6f * s);
        g.drawLine (c.x - k, c.y + k, c.x + k, c.y - k, 1.6f * s);
    }
    else
    {
        auto textArea = inner;
        const bool on = hasLed && (ledOn || getToggleState());   // BYPASS activo
        const juce::Colour fluor { 0xffffee00 };                // amarillo flúor
        if (on)
        {
            // Borde y fondo con brillo amarillo
            juce::Path frame;
            frame.addRoundedRectangle (inner, juce::jmin (5.5f * s, inner.getHeight() * 0.5f));
            juce::DropShadow (fluor.withAlpha (0.55f), (int) (9 * s), {}).drawForPath (g, frame);
            g.setColour (fluor.withAlpha (0.12f));
            g.fillPath (frame);
            g.setColour (fluor.withAlpha (0.9f));
            g.strokePath (frame, juce::PathStrokeType (1.2f * s));
        }
        if (hasLed)
        {
            const juce::Point<float> led (inner.getX() + 14.0f * s, c.y);
            if (on)
            {
                juce::Path halo;
                halo.addEllipse (led.x - 6 * s, led.y - 6 * s, 12 * s, 12 * s);
                juce::DropShadow (fluor, (int) (8 * s), {}).drawForPath (g, halo);
            }
            g.setColour (on ? fluor : juce::Colour (0xff1b2a22));
            g.fillEllipse (led.x - 3.5f * s, led.y - 3.5f * s, 7.0f * s, 7.0f * s);
            textArea = textArea.withTrimmedLeft (20.0f * s);
        }
        g.setColour (on ? fluor : textCol);
        g.setFont (uiFont (10.0f * s, true, 1.3f * s));
        g.drawText (getButtonText(), textArea, juce::Justification::centred);
    }
}

//==============================================================================
// TopBar
TopBar::TopBar (PresetManager& pm, juce::UndoManager& um, juce::AudioProcessorValueTreeState& s,
                OutputStage& out, float dw)
    : presets (pm), undo (um), apvts (s), meter (out), designW (dw)
{
    for (auto* b : std::initializer_list<juce::Component*> { &saveButton, &undoButton, &redoButton, &bypassButton, &settingsButton })
        addAndMakeVisible (b);

    bypassButton.hasLed = true;
    bypassButton.setClickingTogglesState (true);
    settingsButton.round = true;

    if (auto* p = apvts.getParameter ("bypass"))
        bypassAttachment = std::make_unique<juce::ButtonParameterAttachment> (*p, bypassButton, &undo);

    saveButton.onClick     = [this] { if (onSave) onSave(); };
    undoButton.onClick     = [this] { undo.undo(); };
    redoButton.onClick     = [this] { undo.redo(); };
    settingsButton.onClick = [this] { if (onSettings) onSettings(); };

    setRepaintsOnMouseActivity (false);
    startTimerHz (30);
}

juce::Rectangle<float> TopBar::presetScreen() const { return { 28.0f, 13.0f, juce::jmin (410.0f, designW * 0.335f), 26.0f }; }
juce::Rectangle<float> TopBar::prevArea() const     { auto r = presetScreen(); return { r.getX() + 64.0f, r.getY(), 22.0f, r.getHeight() }; }
juce::Rectangle<float> TopBar::nextArea() const     { auto r = presetScreen(); return { r.getX() + 86.0f, r.getY(), 22.0f, r.getHeight() }; }
juce::Rectangle<float> TopBar::nameArea() const     { auto r = presetScreen(); return r.withTrimmedLeft (110.0f); }
juce::Rectangle<float> TopBar::abScreen() const     { auto r = presetScreen(); return { r.getRight() + 216.0f, 13.0f, 92.0f, 26.0f }; }
juce::Rectangle<float> TopBar::slotArea (int i) const { auto r = abScreen(); return { r.getX() + 4.0f + 34.0f * (float) i, r.getY(), 28.0f, r.getHeight() }; }
juce::Rectangle<float> TopBar::copyArea() const     { auto r = abScreen(); return { r.getX() + 68.0f, r.getY(), 24.0f, r.getHeight() }; }
juce::Rectangle<float> TopBar::outScreen() const
{
    const float left = abScreen().getRight() + 24.0f;
    const float right = designW - 28.0f - 112.0f - 44.0f - 22.0f;
    return { left, 13.0f, juce::jmin (220.0f, right - left), 26.0f };
}

void TopBar::resized()
{
    const float s = scale();
    auto sc = [s] (float x, float y, float w, float h)
    {
        return juce::Rectangle<float> (x * s, y * s, w * s, h * s).toNearestInt();
    };
    const float px = presetScreen().getRight();
    saveButton.setBounds (sc (px + 40.0f - 31.0f, 12.0f, 62.0f, 28.0f));
    undoButton.setBounds (sc (px + 96.0f + 22.0f - 22.0f, 12.0f, 44.0f, 28.0f));
    redoButton.setBounds (sc (px + 96.0f + 68.0f - 22.0f, 12.0f, 44.0f, 28.0f));
    const float rx = designW - 28.0f;
    bypassButton.setBounds (sc (rx - 112.0f - 46.0f, 10.0f, 92.0f, 32.0f));
    settingsButton.setBounds (sc (rx - 16.0f - 15.0f, 11.0f, 30.0f, 30.0f));
}

void TopBar::paint (juce::Graphics& g)
{
    const float s = scale();
    g.addTransform (juce::AffineTransform::scale (s));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b0d12), 0, 0, juce::Colour (0xff06070a), 0, barHeight, false));
    g.fillRect (0.0f, 0.0f, designW, barHeight);
    g.setColour (juce::Colour (0xff232937));
    g.fillRect (0.0f, barHeight - 1.0f, designW, 1.0f);

    // ---- Presets ----
    auto ps = presetScreen();
    drawScreen (g, ps);
    const float cy = ps.getCentreY();
    g.setColour (Colours::screenText);
    g.setFont (uiFont (8.5f, true, 2.0f));
    g.drawText ("PRESET", juce::Rectangle<float> (ps.getX() + 12.0f, ps.getY(), 60.0f, ps.getHeight()), juce::Justification::centredLeft);

    auto triangle = [&g] (juce::Rectangle<float> r, bool right, juce::Colour c)
    {
        juce::Path t;
        const auto m = r.getCentre();
        if (right) t.addTriangle (m.x - 3.5f, m.y - 4.5f, m.x - 3.5f, m.y + 4.5f, m.x + 4.0f, m.y);
        else       t.addTriangle (m.x + 3.5f, m.y - 4.5f, m.x + 3.5f, m.y + 4.5f, m.x - 4.0f, m.y);
        g.setColour (c);
        g.fillPath (t);
    };
    triangle (prevArea(), false, Colours::mint);
    triangle (nextArea(), true, Colours::mint);

    auto na = nameArea();
    juce::String name = presets.getCurrentName().toUpperCase();
    if (presets.isModified()) name << " *";
    g.setColour (Colours::mint);
    g.setFont (uiFont (11.0f, true, 1.0f));
    g.drawFittedText (name, na.withTrimmedRight (34.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    {
        juce::Path down;
        const float x = na.getRight() - 18.0f;
        down.addTriangle (x - 4.0f, cy - 2.5f, x + 4.0f, cy - 2.5f, x, cy + 3.0f);
        g.setColour (Colours::screenText);
        g.fillPath (down);
    }

    // ---- A / B ----
    auto ab = abScreen();
    drawScreen (g, ab);
    for (int i = 0; i < 2; ++i)
    {
        const bool on = presets.getActiveSlot() == i;
        auto pill = juce::Rectangle<float> (24.0f, 16.0f).withCentre ({ ab.getX() + 18.0f + 34.0f * (float) i, cy });
        if (on) drawPill (g, pill);
        g.setColour (on ? Colours::mint : Colours::screenOff);
        g.setFont (uiFont (10.5f, true));
        g.drawText (i == 0 ? "A" : "B", pill, juce::Justification::centred);
    }
    {
        // ⇄ dibujado (dos flechas)
        const auto c = copyArea().getCentre().translated (-2.0f, 0.0f);
        g.setColour (Colours::screenText);
        g.drawLine (c.x - 6.0f, c.y - 3.0f, c.x + 5.0f, c.y - 3.0f, 1.3f);
        g.drawLine (c.x - 5.0f, c.y + 3.0f, c.x + 6.0f, c.y + 3.0f, 1.3f);
        juce::Path h;
        h.addTriangle (c.x + 7.0f, c.y - 3.0f, c.x + 3.5f, c.y - 5.6f, c.x + 3.5f, c.y - 0.4f);
        h.addTriangle (c.x - 7.0f, c.y + 3.0f, c.x - 3.5f, c.y + 0.4f, c.x - 3.5f, c.y + 5.6f);
        g.fillPath (h);
    }

    // ---- OUT: medidor + trim ----
    auto os = outScreen();
    if (os.getWidth() > 90.0f)
    {
        drawScreen (g, os);
        g.setColour (Colours::screenText);
        g.setFont (uiFont (8.5f, true, 2.0f));
        g.drawText ("OUT", juce::Rectangle<float> (os.getX() + 10.0f, os.getY(), 30.0f, os.getHeight()), juce::Justification::centredLeft);

        const float trimDb = apvts.getParameter ("outTrim") != nullptr
                               ? apvts.getParameter ("outTrim")->convertFrom0to1 (apvts.getParameter ("outTrim")->getValue()) : 0.0f;
        juce::String t = (trimDb > 0.05f ? "+" : "") + juce::String (trimDb, 1) + " dB";
        if (std::abs (trimDb) < 0.05f) t = "0.0 dB";
        g.setColour (draggingTrim ? Colours::mint.brighter (0.3f) : Colours::mint);
        g.setFont (uiFont (10.0f, true, 0.5f));
        auto valueArea = juce::Rectangle<float> (os.getRight() - 56.0f, os.getY(), 48.0f, os.getHeight());
        g.drawText (t, valueArea, juce::Justification::centredRight);

        // Barras L / R (escala -48 .. +6 dBFS)
        auto bars = juce::Rectangle<float> (os.getX() + 40.0f, cy - 6.0f, valueArea.getX() - os.getX() - 48.0f, 12.0f);
        auto toX = [&] (float lin)
        {
            const float db = juce::Decibels::gainToDecibels (lin, -60.0f);
            return bars.getX() + bars.getWidth() * juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 54.0f);
        };
        const float zeroX = toX (1.0f);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto lane = juce::Rectangle<float> (bars.getX(), bars.getY() + (float) ch * 7.0f, bars.getWidth(), 5.0f);
            g.setColour (juce::Colour (0xff0b1a12));
            g.fillRoundedRectangle (lane, 1.5f);
            const float lvl = ch == 0 ? levelL : levelR;
            const float x = toX (lvl);
            if (x > lane.getX() + 0.5f)
            {
                juce::ColourGradient grad (Colours::screenText, lane.getX(), 0, Colours::mint, zeroX, 0, false);
                g.setGradientFill (grad);
                g.fillRoundedRectangle (lane.withRight (juce::jmin (x, zeroX)), 1.5f);
                if (x > zeroX)
                {
                    g.setColour (Colours::red);
                    g.fillRect (lane.withLeft (zeroX).withRight (x));
                }
            }
            const float hold = ch == 0 ? holdL : holdR;
            const float hx = toX (hold);
            if (hold > 0.0001f)
            {
                g.setColour (hold >= 1.0f ? Colours::red : Colours::mint);
                g.fillRect (hx - 1.0f, lane.getY(), 1.5f, lane.getHeight());
            }
        }
        g.setColour (Colours::screenOff.withAlpha (0.8f));
        g.fillRect (zeroX, bars.getY() - 2.0f, 1.0f, bars.getHeight() + 2.0f);
    }
}

void TopBar::timerCallback()
{
    auto decay = [] (float& level, float& hold, int& count, float peak)
    {
        level = juce::jmax (peak, level * 0.82f);
        if (peak >= hold) { hold = peak; count = 45; }
        else if (--count <= 0) hold *= 0.9f;
    };
    decay (levelL, holdL, holdCountL, meter.peakL.exchange (0.0f));
    decay (levelR, holdR, holdCountR, meter.peakR.exchange (0.0f));

    undoButton.setEnabled (undo.canUndo());
    redoButton.setEnabled (undo.canRedo());

    const float s = scale();
    repaint (outScreen().expanded (4.0f).transformedBy (juce::AffineTransform::scale (s)).toNearestInt());

    if (presets.getChangeCount() != lastChange)
    {
        lastChange = presets.getChangeCount();
        repaint();
    }
}

void TopBar::showPresetMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Init", true, presets.getCurrentName() == "Init" && ! presets.isCurrentUser());
    menu.addSectionHeader ("FACTORY");
    for (int i = 0; i < presets.getNumFactory(); ++i)
        menu.addItem (100 + i, presets.getFactoryName (i), true,
                      ! presets.isCurrentUser() && presets.getCurrentName() == presets.getFactoryName (i));

    auto users = presets.getUserNames();
    menu.addSectionHeader ("USER");
    if (users.isEmpty())
        menu.addItem (2, "(no user presets yet)", false);
    for (int i = 0; i < users.size(); ++i)
        menu.addItem (1000 + i, users[i], true, presets.isCurrentUser() && presets.getCurrentName() == users[i]);

    menu.addSeparator();
    menu.addItem (3, "Save As...");
    const bool canDelete = presets.isCurrentUser() && users.contains (presets.getCurrentName());
    menu.addItem (4, "Delete \"" + presets.getCurrentName() + "\"", canDelete);
    menu.addItem (5, "Open Presets Folder");

    const float s = scale();
    auto area = localAreaToGlobal (presetScreen().transformedBy (juce::AffineTransform::scale (s)).toNearestInt());
    juce::Component::SafePointer<TopBar> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (area).withMinimumWidth (area.getWidth()),
        [safe, users] (int r)
        {
            if (safe == nullptr || r == 0) return;
            auto& pm = safe->presets;
            if (r == 1) pm.loadInit();
            else if (r == 3) { if (safe->onSave) safe->onSave(); }
            else if (r == 4) pm.deleteUser (pm.getCurrentName());
            else if (r == 5) { auto f = pm.getUserFolder(); f.createDirectory(); f.startAsProcess(); }
            else if (r >= 1000) pm.loadUser (users[r - 1000]);
            else if (r >= 100) pm.loadFactory (r - 100);
            safe->repaint();
        });
}

void TopBar::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scale();
    if (prevArea().contains (p))       { presets.previous(); repaint(); }
    else if (nextArea().contains (p))  { presets.next(); repaint(); }
    else if (presetScreen().contains (p)) showPresetMenu();
    else if (slotArea (0).contains (p)) { presets.selectSlot (0); repaint(); }
    else if (slotArea (1).contains (p)) { presets.selectSlot (1); repaint(); }
    else if (copyArea().contains (p))   { presets.copyActiveToOther(); repaint(); }
    else if (outScreen().contains (p))
    {
        if (auto* t = apvts.getParameter ("outTrim"))
        {
            undo.beginNewTransaction();
            draggingTrim = true;
            trimDragStart = t->convertFrom0to1 (t->getValue());
            t->beginChangeGesture();
        }
    }
}

void TopBar::mouseDrag (const juce::MouseEvent& e)
{
    if (! draggingTrim) return;
    if (auto* t = apvts.getParameter ("outTrim"))
    {
        const float perPixel = e.mods.isShiftDown() ? 0.02f : 0.1f;
        const float db = juce::jlimit (-12.0f, 6.0f, trimDragStart - (float) e.getDistanceFromDragStartY() * perPixel / scale()
                                                     + (float) e.getDistanceFromDragStartX() * perPixel * 0.5f / scale());
        t->setValueNotifyingHost (t->convertTo0to1 (std::round (db * 10.0f) / 10.0f));
        repaint();
    }
}

void TopBar::mouseUp (const juce::MouseEvent&)
{
    if (! draggingTrim) return;
    draggingTrim = false;
    if (auto* t = apvts.getParameter ("outTrim"))
        t->endChangeGesture();
    repaint();
}

void TopBar::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (outScreen().contains (e.position / scale()))
        if (auto* t = apvts.getParameter ("outTrim"))
        {
            undo.beginNewTransaction();
            t->beginChangeGesture();
            t->setValueNotifyingHost (t->getDefaultValue());
            t->endChangeGesture();
            repaint();
        }
}

void TopBar::mouseMove (const juce::MouseEvent& e)
{
    const auto p = e.position / scale();
    const bool clickable = presetScreen().contains (p) || slotArea (0).contains (p) || slotArea (1).contains (p)
                        || copyArea().contains (p);
    setMouseCursor (outScreen().contains (p) ? juce::MouseCursor::UpDownResizeCursor
                    : clickable ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void TopBar::applyHelp (const Help* h, bool es)
{
    help = h;
    spanish = es;
    auto tip = [&] (const char* id) { return h != nullptr ? h->get (id, es) : juce::String(); };
    saveButton.setTooltip (tip ("save"));
    undoButton.setTooltip (tip ("undo"));
    redoButton.setTooltip (tip ("redo"));
    bypassButton.setTooltip (tip ("bypass"));
    settingsButton.setTooltip (tip ("settings"));
}

juce::String TopBar::getTooltip()
{
    if (help == nullptr) return {};
    const auto p = getMouseXYRelative().toFloat() / scale();
    if (presetScreen().contains (p)) return help->get ("preset", spanish);
    if (copyArea().contains (p))     return help->get ("abcopy", spanish);
    if (abScreen().contains (p))     return help->get ("ab", spanish);
    if (outScreen().contains (p))    return help->get ("out", spanish);
    return {};
}

//==============================================================================
// Overlays
Overlay::Overlay (float w, float h) : designW (w), designH (h)
{
    addAndMakeVisible (closeButton);
    closeButton.onClick = [this] { close(); };
}

juce::Rectangle<int> Overlay::scaled (juce::Rectangle<float> r) const
{
    return r.transformedBy (juce::AffineTransform::scale (scale())).toNearestInt();
}

void Overlay::close()
{
    setVisible (false);
    if (onClose) onClose();
}

void Overlay::mouseDown (const juce::MouseEvent& e)
{
    if (! panelArea().contains (e.position / scale()))
        close();
}

void Overlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.62f));
    const float s = scale();
    g.addTransform (juce::AffineTransform::scale (s));
    auto r = panelArea();

    juce::Path shape;
    shape.addRoundedRectangle (r, 14.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.8f), 30, { 0, 8 }).drawForPath (g, shape);

    juce::ColourGradient bezel (Colours::bezelTop, 0, r.getY(), Colours::bezelBot, 0, r.getBottom(), false);
    bezel.addColour (0.5, Colours::bezelMid);
    g.setGradientFill (bezel);
    g.fillRoundedRectangle (r, 14.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff10131a), 0, r.getY(), juce::Colour (0xff07080c), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r.reduced (3.0f), 12.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r.reduced (3.0f), 12.0f, 1.0f);

    paintPanel (g);
}

//---------------------------------------------------------------- Settings
SettingsOverlay::SettingsOverlay (Settings& st, float w, float h) : Overlay (w, h), settings (st)
{
    for (auto* c : std::initializer_list<juce::Component*> { &scaleSel, &tipsSel, &langSel, &resetButton, &aboutButton })
        addAndMakeVisible (c);

    scaleSel.onChange = [this] (int i) { settings.setUiScalePercent (75 + 25 * i); };
    tipsSel.onChange  = [this] (int i) { settings.setTooltipsEnabled (i == 0); };
    langSel.onChange  = [this] (int i) { settings.setSpanishHelp (i == 1); };
    resetButton.onClick = [this] { if (onReset) onReset(); };
    aboutButton.onClick = [this] { if (onAbout) onAbout(); };
    sync();
}

void SettingsOverlay::setHelp (const juce::String& scaleTip, const juce::String& tipsTip, const juce::String& langTip,
                               const juce::String& resetTip, const juce::String& aboutTip)
{
    scaleSel.setTooltip (scaleTip);
    tipsSel.setTooltip (tipsTip);
    langSel.setTooltip (langTip);
    resetButton.setTooltip (resetTip);
    aboutButton.setTooltip (aboutTip);
}

void SettingsOverlay::sync()
{
    scaleSel.setSelected ((settings.uiScalePercent() - 75) / 25);
    tipsSel.setSelected (settings.tooltipsEnabled() ? 0 : 1);
    langSel.setSelected (settings.spanishHelp() ? 1 : 0);
}

juce::Rectangle<float> SettingsOverlay::panelArea() const
{
    return juce::Rectangle<float> (560.0f, 360.0f).withCentre ({ designW * 0.5f, designH * 0.5f });
}

void SettingsOverlay::resized()
{
    auto r = panelArea();
    const float x = r.getX(), y = r.getY();
    closeButton.setBounds (scaled ({ r.getRight() - 46.0f, y + 16.0f, 30.0f, 28.0f }));
    scaleSel.setBounds (scaled ({ x + 230.0f, y + 78.0f, 300.0f, 34.0f }));
    tipsSel.setBounds  (scaled ({ x + 230.0f, y + 130.0f, 160.0f, 34.0f }));
    langSel.setBounds  (scaled ({ x + 230.0f, y + 182.0f, 230.0f, 34.0f }));
    resetButton.setBounds (scaled ({ x + 36.0f, y + 256.0f, 200.0f, 32.0f }));
    aboutButton.setBounds (scaled ({ x + 252.0f, y + 256.0f, 110.0f, 32.0f }));
}

void SettingsOverlay::paintPanel (juce::Graphics& g)
{
    auto r = panelArea();
    g.setColour (Colours::labelText);
    g.setFont (uiFont (14.0f, true, 4.0f));
    g.drawText ("SETTINGS", juce::Rectangle<float> (r.getX() + 36.0f, r.getY() + 18.0f, 300.0f, 26.0f), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff232937));
    g.fillRect (r.getX() + 36.0f, r.getY() + 56.0f, r.getWidth() - 72.0f, 1.0f);

    g.setFont (uiFont (10.5f, true, 2.0f));
    g.setColour (Colours::dimText);
    const char* labels[] = { "INTERFACE SIZE", "TOOLTIPS", "HELP LANGUAGE" };
    for (int i = 0; i < 3; ++i)
        g.drawText (labels[i], juce::Rectangle<float> (r.getX() + 36.0f, r.getY() + 78.0f + 52.0f * (float) i, 190.0f, 34.0f),
                    juce::Justification::centredLeft);

    g.fillRect (r.getX() + 36.0f, r.getY() + 236.0f, r.getWidth() - 72.0f, 1.0f);
    g.setColour (Colours::faint);
    g.setFont (uiFont (9.5f, false, 1.5f));
    g.drawText (juce::String ("VERSION ") + pluginVersion + "  -  MOAIACT",
                juce::Rectangle<float> (r.getX() + 36.0f, r.getBottom() - 46.0f, r.getWidth() - 72.0f, 24.0f),
                juce::Justification::centredLeft);
}

//---------------------------------------------------------------- About
AboutOverlay::AboutOverlay (const juce::String& n, const juce::String& sub, juce::Image t, juce::StringArray c, float w, float h)
    : Overlay (w, h), name (n), subtitle (sub), title (t), credits (std::move (c)) {}

juce::Rectangle<float> AboutOverlay::panelArea() const
{
    const float h = 250.0f + (subtitle.isNotEmpty() ? 28.0f : 0.0f) + 17.0f * (float) credits.size();
    return juce::Rectangle<float> (600.0f, h).withCentre ({ designW * 0.5f, designH * 0.5f });
}

void AboutOverlay::paintPanel (juce::Graphics& g)
{
    auto r = panelArea();
    closeButton.setBounds (scaled ({ r.getRight() - 46.0f, r.getY() + 16.0f, 30.0f, 28.0f }));

    float y = r.getY() + 40.0f;
    if (title.isValid())
    {
        const float tw = 380.0f, th = tw * (float) title.getHeight() / (float) title.getWidth();
        g.setOpacity (1.0f);
        g.drawImage (title, juce::Rectangle<float> (tw, th).withCentre ({ r.getCentreX(), y + th * 0.5f }),
                     juce::RectanglePlacement::stretchToFit);
        y += th + 4.0f;
    }
    if (subtitle.isNotEmpty())
    {
        g.setColour (juce::Colour (0xffb4d2ff));
        g.setFont (uiFont (13.0f, false, 12.0f));
        g.drawText (subtitle, juce::Rectangle<float> (r.getX(), y, r.getWidth(), 22.0f), juce::Justification::centred);
        y += 28.0f;
    }
    y += 6.0f;
    g.setColour (Colours::mint);
    g.setFont (uiFont (11.5f, true, 2.0f));
    g.drawText (juce::String ("VERSION ") + pluginVersion + "  -  VST3", juce::Rectangle<float> (r.getX(), y, r.getWidth(), 20.0f),
                juce::Justification::centred);
    y += 24.0f;
    g.setColour (Colours::labelText);
    g.setFont (uiFont (11.0f, false, 1.5f));
    g.drawText (utf8 ("Designed by Moaiact  ·  Chile"), juce::Rectangle<float> (r.getX(), y, r.getWidth(), 20.0f), juce::Justification::centred);
    y += 34.0f;

    g.setColour (juce::Colour (0xff232937));
    g.fillRect (r.getX() + 60.0f, y - 10.0f, r.getWidth() - 120.0f, 1.0f);
    g.setColour (Colours::dimText);
    g.setFont (uiFont (9.5f, false, 0.6f));
    for (auto& c : credits)
    {
        g.drawText (c, juce::Rectangle<float> (r.getX() + 30.0f, y, r.getWidth() - 60.0f, 16.0f), juce::Justification::centred);
        y += 17.0f;
    }
}

//---------------------------------------------------------------- Save
SaveOverlay::SaveOverlay (float w, float h) : Overlay (w, h)
{
    addAndMakeVisible (nameEditor);
    addAndMakeVisible (okButton);
    addAndMakeVisible (cancelButton);
    nameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff06100b));
    nameEditor.setColour (juce::TextEditor::textColourId, Colours::mint);
    nameEditor.setColour (juce::TextEditor::outlineColourId, Colours::screenOff);
    nameEditor.setColour (juce::TextEditor::focusedOutlineColourId, Colours::mint);
    nameEditor.setColour (juce::TextEditor::highlightColourId, Colours::mintGlow.withAlpha (0.3f));
    nameEditor.setColour (juce::CaretComponent::caretColourId, Colours::mint);
    nameEditor.setInputRestrictions (40);
    nameEditor.onReturnKey = [this] { commit(); };
    nameEditor.onEscapeKey = [this] { close(); };
    okButton.onClick = [this] { commit(); };
    cancelButton.onClick = [this] { close(); };
}

void SaveOverlay::open (const juce::String& suggested)
{
    nameEditor.setText (suggested, false);
    nameEditor.selectAll();
    setVisible (true);
    toFront (true);
    nameEditor.grabKeyboardFocus();
}

void SaveOverlay::commit()
{
    auto n = nameEditor.getText().trim();
    if (n.isEmpty()) return;
    if (onSave) onSave (n);
    close();
}

juce::Rectangle<float> SaveOverlay::panelArea() const
{
    return juce::Rectangle<float> (480.0f, 210.0f).withCentre ({ designW * 0.5f, designH * 0.5f });
}

void SaveOverlay::resized()
{
    auto r = panelArea();
    closeButton.setBounds (scaled ({ r.getRight() - 46.0f, r.getY() + 16.0f, 30.0f, 28.0f }));
    nameEditor.setBounds (scaled ({ r.getX() + 36.0f, r.getY() + 76.0f, r.getWidth() - 72.0f, 34.0f }));
    nameEditor.applyFontToAllText (uiFont (14.0f * scale(), true, 1.0f));
    nameEditor.setIndents ((int) (10.0f * scale()), (int) (8.0f * scale()));
    okButton.setBounds (scaled ({ r.getRight() - 36.0f - 100.0f, r.getY() + 140.0f, 100.0f, 32.0f }));
    cancelButton.setBounds (scaled ({ r.getRight() - 36.0f - 216.0f, r.getY() + 140.0f, 104.0f, 32.0f }));
}

void SaveOverlay::paintPanel (juce::Graphics& g)
{
    auto r = panelArea();
    g.setColour (Colours::labelText);
    g.setFont (uiFont (14.0f, true, 4.0f));
    g.drawText ("SAVE PRESET", juce::Rectangle<float> (r.getX() + 36.0f, r.getY() + 18.0f, 300.0f, 26.0f), juce::Justification::centredLeft);
    g.setColour (Colours::faint);
    g.setFont (uiFont (9.5f, false, 1.2f));
    g.drawText ("SAME NAME = REPLACE", juce::Rectangle<float> (r.getX() + 36.0f, r.getY() + 116.0f, 300.0f, 16.0f),
                juce::Justification::centredLeft);
}

//==============================================================================
// EditorChrome
EditorChrome::EditorChrome (juce::AudioProcessorEditor& ed, PresetManager& pm, juce::UndoManager& um,
                            juce::AudioProcessorValueTreeState& apvts, OutputStage& out, Settings& st, Help& h,
                            float dw, float ch, int defW, AboutInfo about)
    : editor (ed), presets (pm), undo (um), settings (st), help (h), designW (dw), contentH (ch), defaultWidth (defW),
      bar (pm, um, apvts, out, dw),
      settingsPanel (st, dw, ch + barHeight),
      aboutPanel (about.name, about.subtitle, about.titleImage, about.credits, dw, ch + barHeight),
      savePanel (dw, ch + barHeight)
{
    addCommonHelp (help);

    editor.addAndMakeVisible (bar);
    for (auto* o : std::initializer_list<Overlay*> { &settingsPanel, &aboutPanel, &savePanel })
    {
        editor.addChildComponent (o);
        o->setAlwaysOnTop (true);
    }

    bar.onSettings = [this] { settingsPanel.sync(); settingsPanel.setVisible (true); settingsPanel.toFront (false); };
    bar.onSave = [this]
    {
        savePanel.open (presets.isCurrentUser() ? presets.getCurrentName() : juce::String());
    };
    savePanel.onSave = [this] (const juce::String& n) { presets.saveUser (n); bar.repaint(); };
    settingsPanel.onAbout = [this] { settingsPanel.setVisible (false); aboutPanel.setVisible (true); aboutPanel.toFront (false); };
    settingsPanel.onReset = [this] { presets.loadInit(); bar.repaint(); };
    aboutPanel.onClose = [this] { settingsPanel.sync(); settingsPanel.setVisible (true); };

    undoListener = std::make_unique<UndoGestureListener> (undo);
    editor.addMouseListener (undoListener.get(), true);

    settings.onChange = [this] { settingsChanged(); };

    // Tamaño: % guardado en Settings, se puede seguir ajustando con la esquina
    editor.setResizable (true, true);
    lastScale = settings.uiScalePercent();
    setContentHeight (contentH);

    settingsChanged();
}

EditorChrome::~EditorChrome()
{
    settings.onChange = nullptr;
    editor.removeMouseListener (undoListener.get());
}

void EditorChrome::setContentHeight (float h)
{
    contentH = h;
    const double aspect = designW / fullHeight();
    const int minW = juce::roundToInt (defaultWidth * 0.6667f), maxW = juce::roundToInt (defaultWidth * 1.5f);
    editor.getConstrainer()->setFixedAspectRatio (aspect);
    editor.setResizeLimits (minW, juce::roundToInt (minW / aspect), maxW, juce::roundToInt (maxW / aspect));
    const int w = editor.getWidth() > 0 ? editor.getWidth() : juce::roundToInt (defaultWidth * lastScale / 100.0f);
    editor.setSize (w, juce::roundToInt (w / aspect));
    for (auto* o : std::initializer_list<Overlay*> { &settingsPanel, &aboutPanel, &savePanel })
        o->setDesignHeight (fullHeight());
}

void EditorChrome::setScale (int percent)
{
    const int w = juce::roundToInt (defaultWidth * percent / 100.0f);
    editor.setSize (w, juce::roundToInt (w * fullHeight() / designW));
}

void EditorChrome::settingsChanged()
{
    if (settings.uiScalePercent() != lastScale)
    {
        lastScale = settings.uiScalePercent();
        setScale (lastScale);
    }

    if (settings.tooltipsEnabled())
    {
        if (tooltipWindow == nullptr)
            tooltipWindow = std::make_unique<juce::TooltipWindow> (&editor, 700);
    }
    else
        tooltipWindow.reset();

    const bool es = settings.spanishHelp();
    bar.applyHelp (settings.tooltipsEnabled() ? &help : nullptr, es);
    auto t = [&] (const char* id) { return settings.tooltipsEnabled() ? help.get (id, es) : juce::String(); };
    settingsPanel.setHelp (t ("uiScale"), t ("tooltips"), t ("language"), t ("reset"), t ("about"));
    if (onHelpChanged) onHelpChanged();
}

juce::String EditorChrome::tip (const juce::String& id, bool withGestures) const
{
    if (! settings.tooltipsEnabled()) return {};
    const bool es = settings.spanishHelp();
    auto text = help.get (id, es);
    if (withGestures && text.isNotEmpty())
        text << "\n" << help.get ("gestures", es);
    return text;
}

void EditorChrome::layout()
{
    const float s = (float) editor.getWidth() / designW;
    bar.setBounds (0, 0, editor.getWidth(), juce::roundToInt (barHeight * s));
    for (auto* o : std::initializer_list<juce::Component*> { &settingsPanel, &aboutPanel, &savePanel })
        o->setBounds (editor.getLocalBounds());
}

//==============================================================================
// Tooltip
static constexpr int tipMaxWidth = 340;

static juce::TextLayout tipLayout (const juce::String& text)
{
    juce::AttributedString s;
    s.setJustification (juce::Justification::topLeft);
    auto lines = juce::StringArray::fromLines (text);
    for (int i = 0; i < lines.size(); ++i)
    {
        const bool extra = i > 0;
        s.append ((i > 0 ? "\n" : "") + lines[i], uiFont (extra ? 11.0f : 12.5f, false),
                  extra ? Colours::dimText : juce::Colour (0xffc9d1e2));
    }
    juce::TextLayout tl;
    tl.createLayoutWithBalancedLineLengths (s, (float) tipMaxWidth);
    return tl;
}

juce::Rectangle<int> tooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    auto tl = tipLayout (text);
    const int w = (int) std::ceil (tl.getWidth()) + 26, h = (int) std::ceil (tl.getHeight()) + 18;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 18,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h)
               .constrainedWithin (parent);
}

void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto r = juce::Rectangle<float> ((float) width, (float) height);
    g.setColour (juce::Colour (0xff0d1118));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (Colours::blue.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    tipLayout (text).draw (g, r.reduced (13.0f, 9.0f));
}
}
