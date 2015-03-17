/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 OscComponent.h
 Created: 8 Aug 2014 9:27:08am
 
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

#ifndef OSCCOMPONENT_H_INCLUDED
#define OSCCOMPONENT_H_INCLUDED

#include "PluginEditor.h"

class OscComponent;

HeartbeatComponent * CreateOscComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
void updateOscComponent(HeartbeatComponent* oscComponent);

#endif  // OSCCOMPONENT_H_INCLUDED
