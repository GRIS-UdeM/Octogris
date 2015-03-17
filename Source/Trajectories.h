/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 Trajectories.h
 Created: 3 Aug 2014 11:42:38am
 
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

#ifndef TRAJECTORIES_H_INCLUDED
#define TRAJECTORIES_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class OctogrisAudioProcessor;

class Trajectory : public ReferenceCountedObject
{
public:
	typedef ReferenceCountedObjectPtr<Trajectory> Ptr;

	static int NumberOfTrajectories();
	static String GetTrajectoryName(int i);
	static Trajectory::Ptr CreateTrajectory(int i, OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source);
	
public:
	virtual ~Trajectory() {}
	
	bool process(float seconds, float beats);
	float progress();
	void stop();
	
protected:
	virtual void spInit() {}
	virtual void spProcess(float duration, float seconds) = 0;
	
private:
	void start();
	
protected:
	Trajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source);
	
	OctogrisAudioProcessor *mFilter;
	bool mStarted, mStopped;
	float mDone;
	float mDuration;
	float mTotalDuration;
	bool mBeats;
	int mSource;
};

#endif  // TRAJECTORIES_H_INCLUDED
