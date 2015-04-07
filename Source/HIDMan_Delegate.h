//
//  HIDMan_Delegate.h
//  Octogris2
//
//  Created by GRIS on 2015-03-05.
//
//

#ifndef __Octogris2__HIDMan_Delegate__
#define __Octogris2__HIDMan_Delegate__

#include <stdio.h>
#include "HID_Utilities_External.h"
#include "PluginEditor.h"
#include "Leap.h"

class HIDMan_Delegate
{
public:
    HIDMan_Delegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
    void buttonClicker (Button *button);
    void Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
    void Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
    CFMutableDictionaryRef hu_CreateMatchingDictionary(uint32_t inUsagePage, uint32_t inUsage);
    void onFrame(const Leap::Controller& controller);
    void update();
    OSStatus Terminate_HID(void *inContext);
    OSStatus Initialize_HID(void *inContext);
};


Component * CreateHIDDelegateComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
void updateJoystickComponent(Component * joystickComponent);


/*static OSStatus Initialize_HID(void *inContext);
static OSStatus Terminate_HID(void *inContext);

static void Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
static void Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
static CFMutableDictionaryRef hu_CreateMatchingDictionary(uint32_t inUsagePage, uint32_t inUsage);*/

#endif /* defined(__Octogris2__HIDMan_Delegate__) */
