/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class DelayismAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DelayismAudioProcessor();
    ~DelayismAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS::ParameterLayout createParams();
    APVTS params;

private:
    
    juce::AudioBuffer<float> delayBuffer;
    int writePosition { 0 }; // tracks position on the delay buffer where data from main buffer will be copied
    
    void fillBuffer(int channel, juce::AudioBuffer<float>& buffer);
    void readFromBuffer(int channel, juce::AudioBuffer<float>& delayBuffer, juce::AudioBuffer<float>& buffer);
    void updateBufferPositions(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer);
    
//    juce::SmoothValue<float, ValueSmoothingTypes:Linear> gain; // also works
    juce::LinearSmoothedValue<float> gain;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayismAudioProcessor)
};
