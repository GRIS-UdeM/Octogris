/*
  ==============================================================================

    OctoLeap.h
    Created: 4 Aug 2014 1:23:01pm
    Author:  makira

  ==============================================================================
*/

#ifndef OCTOLEAP_H_INCLUDED
#define OCTOLEAP_H_INCLUDED


#include "PluginEditor.h"
class OctoLeap : public ReferenceCountedObject , public Leap::Listener
{
public:
    typedef ReferenceCountedObjectPtr<OctoLeap> Ptr;
    
    OctoLeap(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
    virtual void onConnect(const Leap::Controller& controller);
    void onDisconnect(const Leap::Controller& controller);
    void onFrame(const Leap::Controller& controller);
    
    void onServiceDisconnect(const Leap::Controller& controller);

    
private:
    OctogrisAudioProcessor *mFilter;
    OctogrisAudioProcessorEditor *mEditor;
        
    ScopedPointer<Leap::Controller> mController;
    
    int32_t mPointableId;
    bool mLastPositionValid;
    Leap::Vector mLastPosition;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoLeap)
    
};

OctoLeap * CreateLeapComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
void updateLeapComponent(Component * leapComponent);

#endif  // OCTOLEAP_H_INCLUDED
