#include "PluginProcessor.h"
#include "PluginEditor.h"

SamplerAudioProcessor::SamplerAudioProcessor() :
    AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    formatManager.registerBasicFormats();

    for (int i = 0; i < numVoices; ++i)

    {

        midiPlaybackEngine.addVoice(new juce::SamplerVoice());

    }

    midiPlaybackEngine.addSound(loadSound("C5", 72, { 24, 25, 36, 37, 48, 49, 60, 61, 72, 73 }, BinaryData::c5_wav, BinaryData::c5_wavSize));
    midiPlaybackEngine.addSound(loadSound("D5", 74, { 26, 27, 38, 39, 50, 51, 62, 63, 74, 75, 86, 87, 98, 99 }, BinaryData::d5_wav, BinaryData::d5_wavSize));
    midiPlaybackEngine.addSound(loadSound("E5", 76, { 28, 40, 52, 64, 76, 88, 100 }, BinaryData::e5_wav, BinaryData::e5_wavSize));
    midiPlaybackEngine.addSound(loadSound("F5", 77, { 29, 30, 41, 42, 53, 54, 65, 66, 77, 78, 89, 90, 101, 102 }, BinaryData::f5_wav, BinaryData::f5_wavSize));
    midiPlaybackEngine.addSound(loadSound("G5", 79, { 31, 32, 43, 44, 55, 56, 67, 68, 79, 80, 91, 92, 103, 104}, BinaryData::g5_wav, BinaryData::g5_wavSize));
    midiPlaybackEngine.addSound(loadSound("A5", 81, { 21, 22, 33, 34, 45, 46, 57, 58, 69, 70, 81, 82, 93, 94, 105, 106 }, BinaryData::a5_wav, BinaryData::a5_wavSize));
    midiPlaybackEngine.addSound(loadSound("B5", 83, { 23, 35, 47, 59, 71, 83, 95, 107 }, BinaryData::b5_wav, BinaryData::b5_wavSize));
    midiPlaybackEngine.addSound(loadSound("C6", 84, { 84, 85, 96, 97, 108 }, BinaryData::c6_wav, BinaryData::c6_wavSize));
}

bool SamplerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SamplerAudioProcessor::prepareToPlay(double newSampleRate, int maximumBlockSize)
{
    juce::ignoreUnused(newSampleRate, maximumBlockSize);

    midiPlaybackEngine.setCurrentPlaybackSampleRate(newSampleRate);

    reverb.setSampleRate(newSampleRate);
    reverb.reset();

    oldDecay = -1.0f;
    oldReverbAmount = -1.0f;
}

void SamplerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    clearUnusedOutputChannels(buffer);

    updateDecay();

    updateReverb();

    midiPlaybackEngine.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    reverb
        .processStereo(buffer.getWritePointer(0),
                       buffer.getWritePointer(1),
                       buffer.getNumSamples());
}

void SamplerAudioProcessor::clearUnusedOutputChannels(juce::AudioBuffer<float>& buffer) const
{
    for ( auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }
}

void SamplerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void SamplerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorEditor* SamplerAudioProcessor::createEditor()
{
    return new SamplerAudioProcessorEditor(*this);
//    return new juce::GenericAudioProcessorEditor(*this);
}

juce::AudioProcessorValueTreeState::ParameterLayout SamplerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto valueToPercentageAsString = [](float value, int) { return juce::String(int(value)) + "%"; };

    // Decay
    layout.add(std::make_unique<juce::AudioParameterFloat>
    (
        juce::ParameterID{"decay", 1},
         "Decay",
         juce::NormalisableRange<float>(0.0f, 100.0f),
         0.0f,
         juce::AudioParameterFloatAttributes()
         .withStringFromValueFunction(valueToPercentageAsString))
    );

    // Reverb
    layout.add(std::make_unique<juce::AudioParameterFloat>
    (
        juce::ParameterID{"reverb", 1},
         "Reverb",
         juce::NormalisableRange<float>(0.0f, 100.0f),
         0.0f,
         juce::AudioParameterFloatAttributes()
         .withStringFromValueFunction(valueToPercentageAsString))
    );

    return layout;
}

juce::SamplerSound* SamplerAudioProcessor::loadSound(const juce::String name,
                                                     int originalMidiNote,
                                                     const std::vector<int>& midiNoteSet,
                                                     const void* data,
                                                     size_t sizeInBytes)
{
    auto inputStream = std::make_unique<juce::MemoryInputStream>(data,
                                                                 sizeInBytes, false);

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(std::move(inputStream)));

    if (reader != nullptr)
    {
        juce::BigInteger midiNotes;

        for (auto note : midiNoteSet)
        {
            midiNotes.setBit(note);
        }

        const double attack = 0.0;
        const double release = 0.1;
        const double sampleLength = 10.0;

        return new juce::SamplerSound(name,
                                      *reader,
                                      midiNotes,
                                      originalMidiNote,
                                      attack,
                                      release,
                                      sampleLength);
    }
    return nullptr;
}

void SamplerAudioProcessor::updateDecay()
{
    auto decay = apvts.getRawParameterValue("decay")->load();

    if ( !juce::approximatelyEqual(oldDecay, decay))
    {
        auto normalizedDecay = decay * 0.01f;

        // Quadratic function
        float skewValue = normalizedDecay * normalizedDecay;

        // Make the real minimum a value of 0.05
        float decayTime = 0.95f * skewValue + 0.05f;

        for( int i = 0; i < midiPlaybackEngine.getNumSounds(); ++i )
        {
            if( auto* sound = dynamic_cast<juce::SamplerSound*>(midiPlaybackEngine.getSound(i).get()))
            {
                sound->setEnvelopeParameters({0.0f, decayTime, 0.1f, 0.05f});
            }
        }

        oldDecay = decay;
    }
}

void SamplerAudioProcessor::updateReverb()
{
    auto reverbAmount = apvts.getRawParameterValue("reverb")->load();

    if (! juce::approximatelyEqual(oldReverbAmount, reverbAmount))
    {
        auto normalizedReverbAmount = juce::jlimit(0.0f, 1.0f, reverbAmount * 0.01f);

        juce::Reverb::Parameters params;
        params.roomSize = 0.2f + (0.8f * normalizedReverbAmount);
        params.damping = 0.5f;
        params.wetLevel = 0.33f * normalizedReverbAmount;
        params.dryLevel = 0.5f - (0.1f * normalizedReverbAmount);
        params.width = 1.0f;
        params.freezeMode = 0.0f;

        reverb.setParameters(params);

        oldReverbAmount = reverbAmount;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SamplerAudioProcessor();
}

