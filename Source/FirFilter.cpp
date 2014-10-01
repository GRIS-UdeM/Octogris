/*
  ==============================================================================

    FirFilter.cpp
    Created: 2 Aug 2014 10:24:24am
    Author:  makira

  ==============================================================================
*/

#include <cstdio>

#include "FirFilter.h"

#include "../JuceLibraryCode/JuceHeader.h"
#if JUCE_MSVC
  #pragma warning (disable: 4305)  // (truncation warning)
#endif

#include "_firs.h"

void FirFilter::reset()
{
	mPos = kFirSize - 1;
	memset(mBuf, 0, sizeof(mBuf));
}

void FirFilter::setSampleRate(int sr)
{
	for (int i = 0; i < sizeof(kSampleRates)/sizeof(kSampleRates[0]); i++)
	{
		if (kSampleRates[i] == sr)
		{
			mFir = i;
			return;
		}
	}
	
	if (sr) fprintf(stderr, "Octogris: unsupported sample rate %d for filtering...\n", sr);
	mFir = -1;
}

float FirFilter::process(float sample, int distance)
{
	if (mPos == kBufferSize)
	{
		memcpy(mBuf, mBuf + kBufferSize - (kFirSize - 1), (kFirSize - 1) * sizeof(float));
		mPos = kFirSize - 1;
	}
	mBuf[mPos++] = sample;
	
	if (distance < 0 || mFir < 0)
		return sample;
		
	distance /= kDistanceMultipler;
		
	if (distance > kMaxOffset)
		distance = kMaxOffset;
	
	// apply fir
	const float *b = mBuf + mPos - kFirSize;
	const float *fir = kFirs[mFir][distance];
	
	float sum = 0;
	for (int i = 0; i < kFirSize; i++)
		sum += fir[i] * *b++;
	return sum;
}

