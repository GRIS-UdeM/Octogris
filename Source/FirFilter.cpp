/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 FirFilter.cpp
 Created: 2 Aug 2014 10:24:24am
 
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
//            std::cout << "mFir: " << mFir << std::endl;
//            std::cout << "i: " << i << std::endl;
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

