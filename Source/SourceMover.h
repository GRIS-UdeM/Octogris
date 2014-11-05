/*
  ==============================================================================

    SourceMover.h
    Created: 8 Aug 2014 1:04:53pm
    Author:  makira

  ==============================================================================
*/

#ifndef SOURCEMOVER_H_INCLUDED
#define SOURCEMOVER_H_INCLUDED

#include "PluginProcessor.h"

typedef enum
{
	kVacant,
	kField,
	kOsc,
	kLeap
} MoverType;

class SourceMover
{
public:
	SourceMover(OctogrisAudioProcessor *filter);
    void updateNumberOfSources();
	
	void begin(int s, MoverType mt);
	void move(FPoint p, MoverType mt);
	void end(MoverType mt);
	
private:
	OctogrisAudioProcessor *mFilter;
	MoverType mMover;
	int mSelectedItem;
	
	Array<FPoint> mSourcesDownXY;
	Array<FPoint> mSourcesDownRT;
	Array<float> mSourcesAngularOrder;
};




#endif  // SOURCEMOVER_H_INCLUDED
