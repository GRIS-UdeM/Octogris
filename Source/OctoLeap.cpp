/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 OctoLeap.cpp
 Created: 4 Aug 2014 1:23:01pm
 
 Developers: Antoine Missout, Vincent Berthiaume, Antoine Landrieu
 
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

#include <iostream>

#if WIN32
class Component;
class OctogrisAudioProcessor;
class OctogrisAudioProcessorEditor;
Component * CreateLeapComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
{
    // not implemented yet on windows
    return NULL;
}

#else


#include "OctoLeap.h"
#include "Leap.h"

int gIsLeapConnected = NULL;

OctoLeap::OctoLeap(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor):
mFilter(filter),
mEditor(editor),
mController(nullptr),
mPointableId(-1),
mLastPositionValid(0)
{ }

void OctoLeap::onConnect(const Leap::Controller& controller) {
    const MessageManagerLock mmLock;
    mEditor->getmStateLeap()->setText("Leap connected", dontSendNotification);
}
void OctoLeap::onServiceDisconnect(const Leap::Controller& controller ) {
    printf("Service Leap Disconnected");
}
//Not dispatched when running in a debugger
void OctoLeap::onDisconnect(const Leap::Controller& controller) {
    const MessageManagerLock mmLock;
    mEditor->getmStateLeap()->setText("Leap disconnected", dontSendNotification);
}
void OctoLeap::onFrame(const Leap::Controller& controller) {
    //std::cout << "New frame available" << std::endl;
    if(controller.hasFocus()) {
        Leap::Frame frame = controller.frame();
        if (mPointableId >= 0) {
            Leap::Pointable p = frame.pointable(mPointableId);
            if (!p.isValid() || !p.isExtended()) {
                mPointableId = -1;
                mLastPositionValid = false;
                //std::cout << "pointable not valid or not extended" << std::endl;
                
                mEditor->getMover()->end(kLeap);
            } else {
                Leap::Vector pos = p.tipPosition();
                //std::cout << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z << std::endl;
                
                const float zPlane1 = 50;	// 5 cm
                const float zPlane2 = 100;	// 10 cm
                
                if (pos.z < zPlane2) {
                    if (mLastPositionValid) {
                        Leap::Vector delta = pos - mLastPosition;
                        //std::cout << "dx: " << delta.x << " dy: " << delta.y << " dz: " << delta.z << std::endl;
                        
                        float scale = 1 / 100.;
                        if (pos.z > zPlane1) {
                            float s = 1 - (pos.z - zPlane1) / (zPlane2 - zPlane1);
                            scale *= s;
                            
                        }
                        //std::cout << "scale: " << scale << std::endl;
                        
                        int src = mEditor->getOscLeapSource();
                        
                        FPoint sp = mFilter->getSourceXY01(src);
                        sp.x += delta.x * scale;
                        sp.y += delta.y * scale;
                        mEditor->getMover()->move(sp, kLeap);
                        
                        mEditor->fieldChanged();
                    } else {
                        //std::cout << "pointable last pos not valid" << std::endl;
                        mEditor->getMover()->begin(mEditor->getOscLeapSource(), kLeap);
                    }
                    
                    mLastPosition = pos;
                    mLastPositionValid = true;
                } else {
                    //std::cout << "pointable not touching plane" << std::endl;
                    mLastPositionValid = false;
                    mEditor->getMover()->end(kLeap);
                }
            }
        }
        if (mPointableId < 0) {
            Leap::PointableList pl = frame.pointables().extended();
            if (pl.count() > 0) {
                mPointableId = pl[0].id();
                //std::cout << "got new pointable: " << mPointableId << std::endl;
            }
        }
    }
}

OctoLeap::Ptr OctoLeap::CreateLeapComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
{
    return new OctoLeap(filter, editor);
}
#endif