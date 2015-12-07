/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 FieldComponent.h
 Created: 15 Jan 2014 10:59:44am
 
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
    
    void clearTrajectoryPath();
    void updatePositionTrace(float p_fX, float p_fY);
private:
	OctogrisAudioProcessor *mFilter;
	SourceMover *mMover;
	
	SelectionType mSelectionType;
	int mSelectedItem;
	
	ModifierKeys mLastKeys;
	float mSavedValue;

	FPoint convertSourceRT(float r, float t);
    Path m_oTrajectoryPath;
    float m_fStartPathX;
    float m_fStartPathY;
    float m_fEndPathX;
    float m_fEndPathY;
    bool m_bPathJustStarted;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FieldComponent)
};


#endif  // FIELDCOMPONENT_H_INCLUDED
