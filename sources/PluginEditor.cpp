#include "PluginEditor.h"

SamplerAudioProcessorEditor::SamplerAudioProcessorEditor(SamplerAudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p)
{
    setOpaque(true);
    setSize(windowWidth, windowHeight);
}

SamplerAudioProcessorEditor::~SamplerAudioProcessorEditor()
{
}

void SamplerAudioProcessorEditor::paint(juce::Graphics& g)
{
//    g.fillAll(juce::Colours::darkgrey);
    g.drawImage(Images::getBackground(), { 0, 0, windowWidth, windowHeight });
}

void SamplerAudioProcessorEditor::resized()
{
}
