#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr double minBpm = 40.0;

    inline long long floorDiv (long long a, long long b)
    {
        auto q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0)))
            --q;
        return q;
    }

    // Curva de crossfade "raised cosine": 0 -> 1 sin escalones
    inline float fadeCurve (float t) { return 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * t); }

    juce::String percentText (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }
}

const juce::StringArray NewProjectAudioProcessor::rateNames {
    "1/1", "1/2", "1/2D", "1/4", "1/8", "1/16", "1/32", "1/64",
    "1/4D", "1/8D", "1/4T", "1/8T"
};

//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "Parameters", createParameterLayout())
{
    bypassParam  = apvts.getRawParameterValue ("bypass");
    outTrimParam = apvts.getRawParameterValue ("outTrim");
    rateParam        = apvts.getRawParameterValue ("glitchRate");
    repeatsParam     = apvts.getRawParameterValue ("repeats");
    reverseProbParam = apvts.getRawParameterValue ("reverseProb");
    flangerParam     = apvts.getRawParameterValue ("flangerDepth");
    reverbMixParam   = apvts.getRawParameterValue ("reverbMix");
    reverbProbParam  = apvts.getRawParameterValue ("reverbProb");
    panFxParam       = apvts.getRawParameterValue ("panFx");
    mixParam         = apvts.getRawParameterValue ("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout NewProjectAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using PID = juce::ParameterID;
    auto pct = juce::AudioParameterFloatAttributes().withStringFromValueFunction (percentText);
    const juce::NormalisableRange<float> unit (0.0f, 1.0f, 0.01f);

    layout.add (std::make_unique<juce::AudioParameterChoice> (PID { "glitchRate", 1 }, "Rate", rateNames, 3));
    layout.add (std::make_unique<juce::AudioParameterInt>    (PID { "repeats", 1 }, "Repeats", 1, 16, 4));
    layout.add (std::make_unique<juce::AudioParameterFloat>  (PID { "reverseProb", 1 }, "Reverse", unit, 0.0f, pct));
    layout.add (std::make_unique<juce::AudioParameterFloat>  (PID { "flangerDepth", 1 }, "Flanger",
                                                              juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f, 0.6f), 0.0f, pct));
    layout.add (std::make_unique<juce::AudioParameterFloat>  (PID { "reverbMix", 1 }, "Reverb", unit, 0.0f, pct));
    layout.add (std::make_unique<juce::AudioParameterFloat>  (PID { "reverbProb", 1 }, "Reverb Prob", unit, 1.0f, pct));
    // Pan FX: cuanto más alto, más lejos salta cada repetición a la izquierda o a la derecha (al azar)
    layout.add (std::make_unique<juce::AudioParameterFloat>  (PID { "panFx", 1 }, "Pan FX", unit, 0.0f, pct));
    layout.add (std::make_unique<juce::AudioParameterFloat>  (PID { "mix", 1 }, "Mix", unit, 1.0f, pct));
    moai::addOutputParameters (layout);
    return layout;
}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    outputStage.prepare (sampleRate, juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels()), juce::jmax (samplesPerBlock, 512));
    setLatencySamples (0);   // El efecto es 100% en tiempo real: no agrega latencia

    const int numCh = juce::jlimit (1, maxChannels, getTotalNumOutputChannels());
    const int block = juce::jmax (samplesPerBlock, 512);

    fadeLen = juce::jmax (16, (int) std::round (sampleRate * 0.003));   // 3 ms de crossfade

    // Slice más largo posible: 1/1 a 40 BPM, + espacio para la cola del crossfade
    const int maxSlice = (int) std::ceil (4.0 * 60.0 / minBpm * sampleRate);
    sliceBuffer.setSize (numCh, maxSlice + fadeLen + 1);
    sliceBuffer.clear();

    curPeriod = recPeriod = lastPos = noPeriod;
    curRep = -1;
    curLen = recLen = prevLen = 0;
    curLive = prevLive = true;
    curReversed = prevReversed = false;
    crossfading = false;
    internalSamplePos = 0;

    dryBuffer.setSize (numCh, block);
    wetBuffer.setSize (numCh, block);
    fxBuffer .setSize (numCh, block);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) block, (juce::uint32) numCh };

    flangerDelay.prepare (spec);
    flangerDelay.setMaximumDelayInSamples ((int) std::ceil (sampleRate * 0.012));
    flangerDelay.reset();
    smoothedFlanger.reset (sampleRate, 0.05);
    smoothedFlanger.setCurrentAndTargetValue (flangerParam->load());

    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.7f;
    rp.damping  = 0.45f;
    rp.wetLevel = 0.45f;   // el reverb de JUCE es muy fuerte a 1.0
    rp.dryLevel = 0.0f;
    rp.width    = 1.0f;
    reverb.setParameters (rp);
    reverb.prepare (spec);   // antes faltaba
    reverb.reset();
    reverbActive = false;

    smoothedReverbSend.reset (sampleRate, 0.01);
    smoothedReverbSend.setCurrentAndTargetValue (0.0f);
    smoothedReverbMix.reset (sampleRate, 0.03);
    smoothedReverbMix.setCurrentAndTargetValue (reverbMixParam->load());
    smoothedMix.reset (sampleRate, 0.02);
    smoothedMix.setCurrentAndTargetValue (mixParam->load());
    panBuffer.setSize (1, block);
    curPanPos = prevPanPos = 0.0f;
}

void NewProjectAudioProcessor::ensureBlockCapacity (int numSamples)
{
    // Solo ocurre si el host manda un bloque más grande que el anunciado
    if (dryBuffer.getNumSamples() < numSamples)
    {
        dryBuffer.setSize (dryBuffer.getNumChannels(), numSamples, false, false, true);
        wetBuffer.setSize (wetBuffer.getNumChannels(), numSamples, false, false, true);
        fxBuffer .setSize (fxBuffer .getNumChannels(), numSamples, false, false, true);
        panBuffer.setSize (1, numSamples, false, false, true);
    }
}

bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

//==============================================================================
void NewProjectAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();
    outputStage.begin (buffer, numCh, n);
    processCore (buffer);
    outputStage.end (buffer, numCh, n, bypassParam->load() > 0.5f, outTrimParam->load());
}

void NewProjectAudioProcessor::processCore (juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int numCh = juce::jmin (numIn, buffer.getNumChannels(), sliceBuffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (numCh <= 0 || n <= 0)
        return;

    ensureBlockCapacity (n);

    // ---- 1. Tempo y posición, sincronizados al host cuando está en Play ----
    const double sr = getSampleRate();
    double bpm = 120.0;
    long long blockStart = internalSamplePos;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = juce::jlimit (minBpm, 400.0, *b);
            if (pos->getIsPlaying())
                if (auto ppq = pos->getPpqPosition())
                    blockStart = (long long) std::llround (*ppq * (60.0 / bpm) * sr);
        }
    internalSamplePos += n;

    const double samplesPerBeat = 60.0 / bpm * sr;
    const int rateIndex = juce::jlimit (0, (int) std::size (rateBeats) - 1, (int) rateParam->load());
    const int maxSlice  = sliceBuffer.getNumSamples() - fadeLen - 1;
    const int L         = juce::jlimit (32, maxSlice, (int) std::lround (samplesPerBeat * rateBeats[rateIndex]));
    const int repeats   = juce::jlimit (1, 16, (int) repeatsParam->load());
    const long long P   = (long long) L * repeats;
    const int F         = juce::jmin (fadeLen, L / 4);
    const float reverseProb = reverseProbParam->load();
    const float reverbProb  = reverbProbParam->load();
    const float panAmount   = panFxParam->load();
    float* panPos = panBuffer.getWritePointer (0);

    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, n);

    // ---- 2. Stutter (muestra a muestra) ----
    for (int i = 0; i < n; ++i)
    {
        const long long pos = blockStart + i;
        const long long p   = floorDiv (pos, P);
        const long long k   = pos - p * P;
        const long long rep = k / L;
        const int inSlice   = (int) (k % L);
        const bool contiguous = (pos == lastPos + 1);
        lastPos = pos;

        // ¿Empieza una nueva repetición (o el host saltó de posición)?
        if (p != curPeriod || rep != curRep || L != curLen || ! contiguous)
        {
            const bool boundary = contiguous && inSlice == 0;

            prevLive     = curLive;
            prevReversed = curReversed;
            prevPanPos   = curPanPos;
            prevLen      = curLen;
            crossfading  = boundary && curPeriod != noPeriod && F > 0;

            curPeriod = p;
            curRep    = rep;
            curLen    = L;

            if (rep == 0)
            {
                // 1ª pasada: suena en vivo y se graba (solo si la tomamos desde su inicio)
                curLive = true;
                curReversed = false;
                if (inSlice == 0) { recPeriod = p; recLen = L; }
                else              { recPeriod = noPeriod; }
            }
            else
            {
                const bool valid = (recPeriod == p && recLen == L);
                curLive = ! valid;
                curReversed = valid && reverseProb > 0.0f && random.nextFloat() < reverseProb;
            }

            // Reverb estocástico: se decide en cada repetición
            reverbActive = reverbProb > 0.0f && random.nextFloat() < reverbProb;

            // Pan FX: cada repetición cae a un lado al azar; la perilla define qué tan lejos
            if (panAmount > 0.0f)
            {
                const float side = random.nextBool() ? 1.0f : -1.0f;
                curPanPos = panAmount * side * (0.35f + 0.65f * random.nextFloat());
            }
            else
            {
                curPanPos = 0.0f;
            }
            if (! crossfading)
                prevPanPos = curPanPos;   // salto del host: sin transición
        }

        // Posición de paneo de esta muestra (con el mismo crossfade que el audio)
        panPos[i] = curPanPos;
        if (crossfading && inSlice < F)
        {
            const float g = fadeCurve ((float) (inSlice + 1) / (float) (F + 1));
            panPos[i] = curPanPos * g + prevPanPos * (1.0f - g);
        }

        const bool recording = (rep == 0 && recPeriod == p);
        const bool tailRec   = (rep == 1 && recPeriod == p && inSlice < F);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float x = dryBuffer.getSample (ch, i);
            const float* slice = sliceBuffer.getReadPointer (ch);

            float out = curLive ? x : slice[curReversed ? (L - 1 - inSlice) : inSlice];

            // Crossfade al inicio de cada repetición: se mezcla con la "continuación"
            // natural de lo que sonaba antes -> sin clics ni huecos de volumen
            if (crossfading && inSlice < F)
            {
                float prev;
                if (prevLive)          prev = x;                                        // seguía el audio en vivo
                else if (prevReversed) prev = slice[juce::jmin (inSlice, prevLen - 1)]; // rebote desde el inicio
                else                   prev = slice[prevLen + inSlice];                 // cola grabada tras el slice

                const float g = fadeCurve ((float) (inSlice + 1) / (float) (F + 1));
                out = out * g + prev * (1.0f - g);
            }

            wetBuffer.setSample (ch, i, out);

            // Grabar DESPUÉS de leer (la lectura del crossfade usa los datos anteriores)
            auto* sliceW = sliceBuffer.getWritePointer (ch);
            if (recording) sliceW[inSlice] = x;
            if (tailRec)   sliceW[L + inSlice] = x;
        }

        if (crossfading && inSlice >= F)
            crossfading = false;
    }

    // ---- 2b. Pan FX sobre la señal glitcheada (potencia constante) ----
    if (numCh == 2)
    {
        for (int i = 0; i < n; ++i)
        {
            if (panPos[i] == 0.0f)
                continue;
            const float angle = (panPos[i] + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            wetBuffer.setSample (0, i, wetBuffer.getSample (0, i) * std::cos (angle) * juce::MathConstants<float>::sqrt2);
            wetBuffer.setSample (1, i, wetBuffer.getSample (1, i) * std::sin (angle) * juce::MathConstants<float>::sqrt2);
        }
    }

    // ---- 3. Flanger: retardo modulado 0.3–5 ms con interpolación cúbica ----
    //      LFO sincronizado al tempo (1 ciclo cada 2 compases), canales a 90° (estéreo)
    smoothedFlanger.setTargetValue (flangerParam->load());
    if (smoothedFlanger.isSmoothing() || smoothedFlanger.getTargetValue() > 0.0001f)
    {
        const double lfoPeriod = samplesPerBeat * 8.0;
        for (int i = 0; i < n; ++i)
        {
            const float depth = smoothedFlanger.getNextValue();
            const double phase = std::fmod ((double) (blockStart + i) / lfoPeriod, 1.0);
            const float fb  = 0.8f * depth;
            const float wetAmt = 0.5f * depth;
            const float sweepMs = 0.3f + 4.7f * depth;

            for (int ch = 0; ch < numCh; ++ch)
            {
                const double ph = (phase + 0.25 * ch) * juce::MathConstants<double>::twoPi;
                const float lfo = 0.5f - 0.5f * (float) std::cos (ph);
                const float delaySamples = (0.3f + lfo * sweepMs) * 0.001f * (float) sr;

                const float in = wetBuffer.getSample (ch, i);
                const float delayed = flangerDelay.popSample (ch, delaySamples);
                flangerDelay.pushSample (ch, in + std::tanh (delayed * fb));   // feedback saturado = estable
                wetBuffer.setSample (ch, i, in * (1.0f - wetAmt) + delayed * wetAmt);
            }
        }
    }
    else
    {
        // Mantener la línea de retardo alimentada para que al subir la perilla no haya saltos
        for (int i = 0; i < n; ++i)
            for (int ch = 0; ch < numCh; ++ch)
            {
                flangerDelay.popSample (ch, 1.0f);
                flangerDelay.pushSample (ch, wetBuffer.getSample (ch, i));
            }
    }

    // ---- 4. Reverb estocástico (send) ----
    smoothedReverbMix.setTargetValue (reverbMixParam->load());
    smoothedReverbSend.setTargetValue (reverbActive ? 1.0f : 0.0f);
    for (int i = 0; i < n; ++i)
    {
        const float send = smoothedReverbSend.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
            fxBuffer.setSample (ch, i, wetBuffer.getSample (ch, i) * send);
    }
    juce::dsp::AudioBlock<float> fxBlock (fxBuffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
    reverb.process (juce::dsp::ProcessContextReplacing<float> (fxBlock));

    // ---- 5. Dry/Wet ----
    smoothedMix.setTargetValue (mixParam->load());

    for (int i = 0; i < n; ++i)
    {
        const float verb = smoothedReverbMix.getNextValue();
        const float m    = smoothedMix.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            // Al subir el reverb se baja un poco la señal directa para no saturar
            float wet = wetBuffer.getSample (ch, i) * (1.0f - 0.35f * verb) + fxBuffer.getSample (ch, i) * verb;
            buffer.setSample (ch, i, dryBuffer.getSample (ch, i) * (1.0f - m) + wet * m);
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor (*this);
}

void NewProjectAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    moai::writeState (*this, apvts, presets, destData);
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    moai::readState (*this, apvts, presets, data, sizeInBytes);
}

void NewProjectAudioProcessor::setCurrentProgram (int index)
{
    if (index == currentProgram || ! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;
    currentProgram = index;
    if (index == 0) presets.loadInit();
    else            presets.loadFactory (index - 1);
}

const juce::String NewProjectAudioProcessor::getProgramName (int index)
{
    if (index == 0) return "Init";
    return juce::isPositiveAndBelow (index - 1, presets.getNumFactory()) ? presets.getFactoryName (index - 1) : juce::String();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}

//==============================================================================
// Presets de fábrica (valores reales; lo que no aparece queda en su valor por defecto)
// Rate: 0 1/1, 1 1/2, 2 1/2D, 3 1/4, 4 1/8, 5 1/16, 6 1/32, 7 1/64, 8 1/4D, 9 1/8D, 10 1/4T, 11 1/8T
std::vector<moai::FactoryPreset> NewProjectAudioProcessor::factoryPresets()
{
    return {
        { "Classic Vocal Chop", { { "glitchRate", 4 },  { "repeats", 4 },  { "reverseProb", 0.15f }, { "reverbMix", 0.20f }, { "reverbProb", 0.50f }, { "panFx", 0.20f } } },
        { "Drum Fill",          { { "glitchRate", 5 },  { "repeats", 8 },  { "flangerDepth", 0.10f }, { "panFx", 0.15f } } },
        { "Half-Time Stutter",  { { "glitchRate", 1 },  { "repeats", 2 },  { "reverseProb", 0.10f }, { "reverbMix", 0.15f }, { "reverbProb", 0.40f } } },
        { "Triplet Bounce",     { { "glitchRate", 11 }, { "repeats", 6 },  { "reverseProb", 0.20f }, { "panFx", 0.50f } } },
        { "Dotted Echo",        { { "glitchRate", 9 },  { "repeats", 5 },  { "reverbMix", 0.30f }, { "reverbProb", 0.60f }, { "panFx", 0.35f }, { "mix", 0.60f } } },
        { "Spatial Tape Stop",  { { "glitchRate", 3 },  { "repeats", 3 },  { "reverseProb", 0.50f }, { "flangerDepth", 0.35f }, { "reverbMix", 0.35f }, { "reverbProb", 0.70f }, { "panFx", 0.70f } } },
        { "Full Reverse",       { { "glitchRate", 3 },  { "repeats", 4 },  { "reverseProb", 1.00f }, { "reverbMix", 0.25f }, { "reverbProb", 0.50f } } },
        { "Broken Atmosphere",  { { "glitchRate", 2 },  { "repeats", 6 },  { "reverseProb", 0.60f }, { "flangerDepth", 0.50f }, { "reverbMix", 0.70f }, { "reverbProb", 1.00f }, { "panFx", 0.60f }, { "mix", 0.80f } } },
        { "Parallel Glitch",    { { "glitchRate", 5 },  { "repeats", 6 },  { "reverseProb", 0.30f }, { "flangerDepth", 0.20f }, { "reverbMix", 0.20f }, { "reverbProb", 0.50f }, { "panFx", 0.40f }, { "mix", 0.45f } } },
        { "Glitch Machine",     { { "glitchRate", 6 },  { "repeats", 12 }, { "reverseProb", 0.40f }, { "flangerDepth", 0.25f }, { "panFx", 0.50f } } },
        { "Wide Shimmer",       { { "glitchRate", 4 },  { "repeats", 4 },  { "flangerDepth", 0.60f }, { "reverbMix", 0.55f }, { "reverbProb", 0.80f }, { "panFx", 1.00f }, { "mix", 0.70f } } },
        { "Subtle Movement",    { { "glitchRate", 3 },  { "repeats", 2 },  { "reverseProb", 0.10f }, { "flangerDepth", 0.15f }, { "reverbMix", 0.15f }, { "reverbProb", 0.30f }, { "panFx", 0.30f }, { "mix", 0.30f } } },
    };
}
