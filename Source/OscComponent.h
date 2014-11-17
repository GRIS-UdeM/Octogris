/*
  ==============================================================================

    OscComponent.h
    Created: 8 Aug 2014 9:27:08am
    Author:  makira

  ==============================================================================
*/

#ifndef OSCCOMPONENT_H_INCLUDED
#define OSCCOMPONENT_H_INCLUDED

#include "PluginEditor.h"

class OscComponent;

HeartbeatComponent * CreateOscComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
void updateOscComponent(HeartbeatComponent* oscComponent);

#endif  // OSCCOMPONENT_H_INCLUDED
