/*
  ==============================================================================

    FieldComponent.h
    Created: 15 Jan 2014 10:59:44am
    Author:  makira

  ==============================================================================
*/

#ifndef FIELDCOMPONENT_H_INCLUDED
#define FIELDCOMPONENT_H_INCLUDED

#include "PluginProcessor.h"
#include "SourceMover.h"

typedef enum
{
	kNoSelection,
	kSelectedSource,
	kSelectedSpeaker
} SelectionType;

class FieldComponent : public Component
{
public:
    FieldComponent(OctogrisAudioProcessor* filter, SourceMover *mover);
    ~FieldComponent();

    void paint (Graphics&);
	
	void mouseDown (const MouseEvent &event);
 	void mouseDrag (const MouseEvent &event);
 	void mouseUp (const MouseEvent &event);
	
	FPoint getSourcePoint(int i);
	FPoint getSpeakerPoint(int i);
	float getDistance(int source, int speaker);
 
private:
	OctogrisAudioProcessor *mFilter;
	SourceMover *mMover;
	
	SelectionType mSelectionType;
	int mSelectedItem;
	
	ModifierKeys mLastKeys;
	float mSavedValue;

	FPoint convertSourceRT(float r, float t);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FieldComponent)
};


#endif  // FIELDCOMPONENT_H_INCLUDED
