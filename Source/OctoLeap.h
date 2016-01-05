/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 OctoLeap.h
 Created: 4 Aug 2014 1:23:01pm
 
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

#ifndef OCTOLEAP_H_INCLUDED
#define OCTOLEAP_H_INCLUDED

extern int gIsLeapConnected;

//#if WIN32
//
//#else

#include "PluginEditor.h"
class OctoLeap : public ReferenceCountedObject , public Leap::Listener
{
public:
    typedef ReferenceCountedObjectPtr<OctoLeap> Ptr;
    //! Interface to call the builder using the ReferenceCountedObject benefits
    static OctoLeap::Ptr CreateLeapComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
    //!Builder
    OctoLeap(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
    //! Called when a Leap Motion is connected to the computer
    virtual void onConnect(const Leap::Controller& controller);
    //! Called when a Leap Motion is disconnected to the computer
    void onDisconnect(const Leap::Controller& controller);
    //!Called each time a frame is captured by the Leap Motion
    void onFrame(const Leap::Controller& controller);
    //! Called when a Leap Motion service is unreachable (Crashed)
    void onServiceDisconnect(const Leap::Controller& controller);
    //! Destroyer
    virtual ~OctoLeap(){
    }
    
private:
    OctogrisAudioProcessor *mFilter;
    OctogrisAudioProcessorEditor *mEditor;
        
    ScopedPointer<Leap::Controller> mController;
    
    int32_t mPointableId;
    //* Boolean telling if the last frame had a valid position
    bool mLastPositionValid;
    //* Vector giving the last frame's position
    Leap::Vector mLastPosition;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoLeap)
    
};

void updateLeapComponent(Component * leapComponent);

//#endif

#endif  // OCTOLEAP_H_INCLUDED
