/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 LevelComponent.h
 Created: 23 Jan 2014 8:09:25am
 
 Developers: Antoine Missout, Vincent Berthiaume
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
