/*
  ==============================================================================

    Routing.cpp
    Created: 31 Mar 2015 6:06:27am
    Author:  makira

  ==============================================================================
*/

#include "Routing.h"

#define kChannels 16
#define kMaxSize (1024)

Router & Router::instance()
{
	static Router r;
	return r;
}

Router::Router()
:
	mOutputs(kChannels, kMaxSize)
{

}

Router::~Router()
{

}

void Router::accumulate(int channels, const AudioSampleBuffer &buffer)
{
	int frames = buffer.getNumSamples();
	SpinLock::ScopedLockType guard(mLock);
	for (int c = 0; c < channels; c++)
		mOutputs.addFrom(c, 0, buffer, c, 0, frames);
}

float ** Router::outputBuffers(int frames)
{
	jassert(frames < kMaxSize);
	return mOutputs.getArrayOfWritePointers();
}

