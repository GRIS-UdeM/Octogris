/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 FirFilter.h
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
