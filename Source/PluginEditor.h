/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class FirstWebViewTestAudioProcessorEditor
: public juce::AudioProcessorEditor
, public juce::Button::Listener
{
public:
    FirstWebViewTestAudioProcessorEditor (FirstWebViewTestAudioProcessor&);
    ~FirstWebViewTestAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void buttonClicked(juce::Button*) override;

private:
    juce::WebBrowserComponent::Options createWebOptions();
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
    std::vector<std::byte> streamToBytes(juce::InputStream& stream);
    juce::WebBrowserComponent::Resource ResourceFromFile(const juce::File& file);

    FirstWebViewTestAudioProcessor& audioProcessor;
    
    std::unique_ptr<juce::WebBrowserComponent> webComp;
    juce::TextButton emitButton;
    juce::TextButton funcButton;
    juce::Label listenerLabel;
    juce::Label nativeFuncLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FirstWebViewTestAudioProcessorEditor)
};
