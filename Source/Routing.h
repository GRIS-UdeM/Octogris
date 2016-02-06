/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginEditor.h
 
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

#ifndef ROUTING_H_INCLUDED
#define ROUTING_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#define kChannels (16)
#define kMaxSize (1024)

class Router
{
private:
	Router();
	~Router();

	AudioSampleBuffer mOutputs;
	SpinLock mLock;

public:
	static Router & instance();
	
	void accumulate(int channels, int frames, const AudioSampleBuffer &buffer);
	void reset() { mOutputs.clear(); }
	void clear(int channel) { mOutputs.clear(channel, 0, mOutputs.getNumSamples()); }
	
	float ** outputBuffers(int frames);
};

#endif  // ROUTING_H_INCLUDED
