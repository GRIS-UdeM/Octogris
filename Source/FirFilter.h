/*
  ==============================================================================

    FirFilter.h
    Created: 2 Aug 2014 10:24:24am
    Author:  makira

  ==============================================================================
*/

#ifndef FIRFILTER_H_INCLUDED
#define FIRFILTER_H_INCLUDED

#include <cstring>

#define kBufferSize (256)

class FirFilter
{
public:
    FirFilter():mFir(0), mPos(0) { reset(); setSampleRate(0); }
	
	void reset();
	void setSampleRate(int sr);
	float process(float sample, int distance);
	
private:
	int mFir;
	int mPos;
	float mBuf[kBufferSize];
};



#endif  // FIRFILTER_H_INCLUDED
