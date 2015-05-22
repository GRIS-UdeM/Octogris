/*
  ==============================================================================

    Routing.cpp
    Created: 31 Mar 2015 6:06:27am
    Author:  makira

  ==============================================================================
*/

#include "Routing.h"

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

void Router::accumulate(int channels, int frames, const AudioSampleBuffer &buffer)
{
	SpinLock::ScopedLockType guard(mLock);
	for (int c = 0; c < channels; c++)
		mOutputs.addFrom(c, 0, buffer, c, 0, frames);
}

float ** Router::outputBuffers(int frames)
{
	if (frames > kMaxSize)
	{
		printf("unexpected frames size: %d\n", frames);
		jassertfalse;
		return NULL;
	}
	
	return mOutputs.getArrayOfWritePointers();
}

