/*
  ==============================================================================

    LevelComponent.h
    Created: 23 Jan 2014 8:09:25am
    Author:  makira

  ==============================================================================
*/

#ifndef LEVELCOMPONENT_H_INCLUDED
#define LEVELCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
/*
*/
class LevelComponent : public Component
{
public:
    LevelComponent(OctogrisAudioProcessor* filter, int index);
    ~LevelComponent();

    void paint (Graphics&);
	void refreshIfNeeded()
	{
		float level;
	
		uint64_t processCounter = mFilter->getProcessCounter();
		if (mLastProcessCounter != processCounter)
		{
			mLastProcessCounter = processCounter;
			mLevelAdjustment = 1;
			level = mFilter->getLevel(mIndex);
		}
		else
		{
			mLevelAdjustment *= 0.8;
			level = mLevelAdjustment * mFilter->getLevel(mIndex);
		}
		
		if (mShowLevel != level)
		{
			mShowLevel = level;
			repaint();
		}
	}
	
private:
	OctogrisAudioProcessor *mFilter;
	int mIndex;
	float mLevelAdjustment;
	float mShowLevel;
	uint64_t mLastProcessCounter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelComponent)
};


#endif  // LEVELCOMPONENT_H_INCLUDED
