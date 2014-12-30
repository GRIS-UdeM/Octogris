/*
  ==============================================================================

    LevelComponent.cpp
    Created: 23 Jan 2014 8:09:25am
    Author:  makira

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "LevelComponent.h"

static const float kMinLevel = -60.f;
static const float kMaxLevel = 1.f;

//==============================================================================
LevelComponent::LevelComponent(OctogrisAudioProcessor* filter, int index)
:
	mFilter(filter),
	mIndex(index),
	mLevelAdjustment(1),
	mShowLevel(0),
	mLastProcessCounter(0)
{
	
}

LevelComponent::~LevelComponent()
{
	
}

void LevelComponent::paint (Graphics& g)
{
	float level = linearToDb(mShowLevel);
	if (level < kMinLevel) level = kMinLevel;
	else if (level > kMaxLevel) level = kMaxLevel;

	const float yellowStart = -6;
	float hue;
	if (level > 0) hue = 0;
	else if (level < yellowStart) hue = 1 / 3.f;
	else
	{
		float p = (level - yellowStart) / (-yellowStart); // 0 .. 1
		hue = (1 - p) / 3.f;
	}
	
	//fprintf(stderr, "speaker %d linear: %.3f dB: %.1f hue: %.3f\n", mIndex, mLastLevel, level, hue);
	
	g.fillAll (Colours::black);
	g.setColour (Colour::fromHSV(hue, 1, 1, 1));
	Rectangle<int> r(0, 0, getWidth() * (level - kMinLevel) / (kMaxLevel - kMinLevel), getHeight());
	g.fillRect(r);
}
