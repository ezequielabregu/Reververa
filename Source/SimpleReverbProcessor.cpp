/*
  ==============================================================================

   Copyright 2021, 2022 Ezequiel Abregu

   Simple Reverb is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
 
   Simple Reverb is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with Simple Reverb. If not, see <http://www.gnu.org/licenses/>.

  ==============================================================================
*/

#include "SimpleReverbProcessor.h"
#include "SimpleReverbEditor.h"
#include "ParamNames.h"

//==============================================================================
SimpleReverbAudioProcessor::SimpleReverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    updateReverbSettings();
}

SimpleReverbAudioProcessor::~SimpleReverbAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleReverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleReverbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleReverbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleReverbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleReverbAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleReverbAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleReverbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleReverbAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String SimpleReverbAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void SimpleReverbAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

//==============================================================================
void SimpleReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;

    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    reverb.prepare (spec);
}

void SimpleReverbAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SimpleReverbAudioProcessor::updateReverbSettings()
{
    params.roomSize   = apvts.getParameter (ParamNames::size)->getValue();
    params.damping    = apvts.getParameter (ParamNames::damp)->getValue();
    params.width      = apvts.getParameter (ParamNames::width)->getValue();
    params.wetLevel   = apvts.getParameter (ParamNames::dryWet)->getValue();
    params.dryLevel   = 1.0f - apvts.getParameter (ParamNames::dryWet)->getValue();
    params.freezeMode = apvts.getParameter (ParamNames::freeze)->getValue();

    reverb.setParameters (params);
}

void SimpleReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    updateReverbSettings();

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    reverb.process (ctx);
}

//==============================================================================
bool SimpleReverbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleReverbAudioProcessor::createEditor()
{
    return new SimpleReverbAudioProcessorEditor (*this, undoManager);
}

//==============================================================================
void SimpleReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos (destData, true);
    apvts.state.writeToStream (mos);
}

void SimpleReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data,
                                               static_cast<size_t> (sizeInBytes));

    if (tree.isValid())
        apvts.replaceState (tree);
}

juce::AudioProcessorValueTreeState& SimpleReverbAudioProcessor::getPluginState() { return apvts; }

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleReverbAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleReverbAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;

    const auto range = juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f, 1.0f);
    const auto defaultValue = 50.0f;

    auto stringFromValue = [](float value, int)
    {
        if (value < 10.0f)
            return juce::String (value, 2) + " %";
        else if (value < 100.0f)
            return juce::String (value, 1) + " %";
        else
            return juce::String (value, 0) + " %";
    };

    layout.add (std::make_unique<juce::AudioParameterFloat> (ParamNames::size,
                                                             ParamNames::size,
                                                             range,
                                                             defaultValue,
                                                             juce::String(),
                                                             juce::AudioProcessorParameter::genericParameter,
                                                             stringFromValue,
                                                             nullptr));

    layout.add (std::make_unique<juce::AudioParameterFloat> (ParamNames::damp,
                                                             ParamNames::damp,
                                                             range,
                                                             defaultValue,
                                                             juce::String(),
                                                             juce::AudioProcessorParameter::genericParameter,
                                                             stringFromValue,
                                                             nullptr));


    layout.add (std::make_unique<juce::AudioParameterFloat> (ParamNames::width,
                                                             ParamNames::width,
                                                             range,
                                                             defaultValue,
                                                             juce::String(),
                                                             juce::AudioProcessorParameter::genericParameter,
                                                             stringFromValue,
                                                             nullptr));

    layout.add (std::make_unique<juce::AudioParameterFloat> (ParamNames::dryWet,
                                                             ParamNames::dryWet,
                                                             range,
                                                             defaultValue,
                                                             juce::String(),
                                                             juce::AudioProcessorParameter::genericParameter,
                                                             stringFromValue,
                                                             nullptr));

    layout.add (std::make_unique<juce::AudioParameterBool> (ParamNames::freeze, ParamNames::freeze, false));

    return layout;
}
