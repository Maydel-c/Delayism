/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DelayismAudioProcessor::DelayismAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
params(*this, nullptr, "Parameters", createParams())
#endif
{
}

DelayismAudioProcessor::~DelayismAudioProcessor()
{
}

//==============================================================================
const juce::String DelayismAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelayismAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayismAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayismAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelayismAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelayismAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DelayismAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayismAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelayismAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelayismAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DelayismAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    auto delayBufferSize = sampleRate * 2.0;
    delayBuffer.setSize(getTotalNumOutputChannels(), static_cast<int>(delayBufferSize));
    
    gain.reset(sampleRate, 0.0005f);
}

void DelayismAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayismAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DelayismAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {

        fillBuffer(channel, buffer);
        readFromBuffer(channel, delayBuffer, buffer);
        fillBuffer(channel, buffer);
    }
    
    updateBufferPositions(buffer, delayBuffer);
    
    auto g = params.getRawParameterValue("Gain")->load();
    gain.setTargetValue(g);
    
//    DBG(g);
    DBG(gain.getNextValue()); // display gain value as the smoothing is happening
    
}

void DelayismAudioProcessor::fillBuffer(int channel, juce::AudioBuffer<float>& buffer)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();
    
    auto* channelData = buffer.getWritePointer (channel);
    
    // Check to see if main buffer can copy to delay buffer without needing to wrap
    if (delayBufferSize > bufferSize + writePosition)
    {
        // copy main buffer contents to delay buffer
        delayBuffer.copyFrom(channel, writePosition, channelData, bufferSize);
    }
    else
    {
        // determine how much space is left at the end of delay buffer
        auto numSamplesToEnd = delayBufferSize - writePosition;
        
        // Copy that amount of content from main buffer to delay buffer
        delayBuffer.copyFrom(channel, writePosition, channelData, numSamplesToEnd);
        
        // Calculate how much content is remaining to copy
        auto numSamplesToStart = bufferSize - numSamplesToEnd;
        
        // Copy that much amount at the beginning of delay buffer
        delayBuffer.copyFrom(channel, 0, channelData + numSamplesToEnd, numSamplesToStart);
    }
}

void DelayismAudioProcessor::updateBufferPositions(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();
    
    writePosition += bufferSize; // position jumps by this amount of samples on every iteration
    writePosition %= delayBufferSize; // bound position value to stay inside delayBufferSize
}

void DelayismAudioProcessor::readFromBuffer(int channel, juce::AudioBuffer<float>& delayBuffer, juce::AudioBuffer<float>& buffer)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();
    
    auto readPosition = writePosition - (getSampleRate() * 0.5f);
    auto g = 0.7f;
    
    if (readPosition < 0)
    {
        readPosition += delayBufferSize;
    }
    
    if(readPosition + bufferSize < delayBufferSize)
    {
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), bufferSize, g, g);
    }
    else
    {
        auto numSamplesToEnd = delayBufferSize - readPosition; // samples that can be added to the main buffer before the delay before loops again
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), numSamplesToEnd, g, g);
        
        auto numSamplesToStart = bufferSize - numSamplesToEnd; // remaining samples are picked up from the start of the delay buffer
        buffer.addFromWithRamp(channel, numSamplesToEnd, delayBuffer.getReadPointer(channel, 0), numSamplesToStart, g, g);
    }
}

//==============================================================================
bool DelayismAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelayismAudioProcessor::createEditor()
{
    return new DelayismAudioProcessorEditor (*this);
}

//==============================================================================
void DelayismAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DelayismAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout DelayismAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::AudioParameterFloat>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"Gain", 1}, "Gain", 0.0f, 1.0f, 0.0f));
    
    return { params.begin(), params.end() };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayismAudioProcessor();
}
