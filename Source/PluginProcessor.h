#pragma once

#include <JuceHeader.h>
#include "MoaiCommon.h"

//==============================================================================
// MOAI GLITCH (versión pequeña)
// Un solo stutter sincronizado al tempo:
//   - La 1ª pasada de cada ciclo suena en vivo y se graba (cero latencia).
//   - Las siguientes "Repeats - 1" pasadas repiten ese trozo.
//   - Cada repetición puede ir al revés según la perilla Reverse (probabilidad).
//==============================================================================
class NewProjectAudioProcessor : public juce::AudioProcessor
{
public:
    static const juce::StringArray rateNames;
    static constexpr double rateBeats[] = { 4.0, 2.0, 3.0, 1.0, 0.5, 0.25, 0.125, 0.0625,
                                            1.5, 0.75, 2.0 / 3.0, 1.0 / 3.0 };

    NewProjectAudioProcessor();
    ~NewProjectAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Moai Glitch"; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    // Programas del DAW = Init + presets de fábrica
    int getNumPrograms() override                               { return 1 + presets.getNumFactory(); }
    int getCurrentProgram() override                            { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override  {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::UndoManager undoManager;   // debe crearse antes que la APVTS

    // TODOS los parámetros viven aquí; la interfaz se conecta solo a esto.
    juce::AudioProcessorValueTreeState apvts;

    // ---- Funciones comunes Moai (presets, A/B, bypass, salida, ajustes) ----
    static std::vector<moai::FactoryPreset> factoryPresets();
    moai::OutputStage outputStage;
    moai::PresetManager presets { apvts, undoManager, getName(), factoryPresets(), { "bypass", "outTrim" } };
    moai::Settings settings { getName() };

private:
    void processCore (juce::AudioBuffer<float>&);
    int currentProgram = 0;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* outTrimParam = nullptr;
    void ensureBlockCapacity (int numSamples);

    // ---- Parámetros (lectura segura desde el hilo de audio) ----
    std::atomic<float>* rateParam        = nullptr;
    std::atomic<float>* repeatsParam     = nullptr;
    std::atomic<float>* reverseProbParam = nullptr;
    std::atomic<float>* flangerParam     = nullptr;
    std::atomic<float>* reverbMixParam   = nullptr;
    std::atomic<float>* reverbProbParam  = nullptr;
    std::atomic<float>* panFxParam       = nullptr;
    std::atomic<float>* mixParam         = nullptr;

    // ---- Motor de stutter ----
    static constexpr int maxChannels = 2;
    juce::AudioBuffer<float> sliceBuffer;   // trozo grabado + cola corta para crossfades
    int  fadeLen = 128;

    static constexpr long long noPeriod = std::numeric_limits<long long>::min();
    long long curPeriod = noPeriod, recPeriod = noPeriod, lastPos = noPeriod;
    long long curRep = -1;
    int  curLen = 0, recLen = 0, prevLen = 0;
    bool curLive = true, curReversed = false;
    bool prevLive = true, prevReversed = false;
    bool crossfading = false;
    float curPanPos = 0.0f, prevPanPos = 0.0f;   // Pan FX: posición de la repetición actual / anterior
    long long internalSamplePos = 0;

    juce::Random random;

    // ---- Flanger: línea de retardo con interpolación cúbica + LFO sincronizado ----
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> flangerDelay;
    juce::SmoothedValue<float> smoothedFlanger;

    // ---- Reverb estocástico ----
    juce::dsp::Reverb reverb;
    bool reverbActive = false;
    juce::SmoothedValue<float> smoothedReverbSend, smoothedReverbMix;

    juce::SmoothedValue<float> smoothedMix;

    // Búferes de trabajo (reservados en prepareToPlay, nunca en processBlock)
    juce::AudioBuffer<float> dryBuffer, wetBuffer, fxBuffer;
    juce::AudioBuffer<float> panBuffer;   // Pan FX: posición L/R por muestra (1 canal)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessor)
};
