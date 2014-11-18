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

Component * CreateLeapComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
void updateLeapComponent(Component * leapComponent);

#endif  // OCTOLEAP_H_INCLUDED
