/*
  ==============================================================================

    Trajectories.h
    Created: 3 Aug 2014 11:42:38am
    Author:  makira

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
