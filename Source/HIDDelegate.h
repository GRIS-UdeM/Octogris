/*!
 * ==============================================================================
 *
 *  HIDDelegate.h
 *  Created: 12 March 2015 1:23:01pm
 *  Author:  Antoine L.
 *  Description :
 *  HIDDelegate allows you to create a object handling the HIDManager through the help of Reference
 *  Counted Object which deletes it in a safe and proper way.
 *  First create the IOHIDManager then the HIDDelegate that you initialize with the Initialize_HID method.
 *  HIDDelegate constructor needs two arguments which are the addresses of
 *  the main components of the plugin the Audio Processor and the Audio Processor
 *  Editor.
 * ==============================================================================
 */

#ifndef __Octogris2__HIDDelegate__
#define __Octogris2__HIDDelegate__

#include <stdio.h>
#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "HID_Utilities_External.h"

class HIDDelegate : public ReferenceCountedObject
{
public:
    typedef ReferenceCountedObjectPtr<HIDDelegate> Ptr;
    static HIDDelegate::Ptr CreateHIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
    
    OSStatus Initialize_HID(void *inContext);
    
    
    static void Handle_IOHIDDeviceInputValueCallback(void * inContext,IOReturn inResult,void * inSender, IOHIDValueRef   inIOHIDValueRef);
    static void Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
    static void Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
    void JoystickUsed(uint32_t usage, float scaledValue, double min, double max);
    CFDictionaryRef hu_CreateMatchingDictionary(uint32_t inUsagePage, uint32_t inUsage);
    FPoint getSourcePoint(int i);
    void setButtonPressedTab(u_int32_t index, bool state);
    bool getButtonPressedTab(u_int32_t index);
    int getNbButton(){return nbButton;};
    IOHIDDeviceRef getDeviceRef(){return deviceRef;}
    CFSetRef getDeviceSetRef(){return deviceSetRef;}
    virtual ~HIDDelegate() {};
    
    protected :
    HIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
    
private:
    OctogrisAudioProcessor *mFilter;
    OctogrisAudioProcessorEditor *mEditor;
    int nbButton;
    bool* buttonPressedTab;
    float vx, vy;
    
    CFSetRef deviceSetRef;
    IOHIDDeviceRef deviceRef;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HIDDelegate)
};



#endif  // OCTOLEAP_H_INCLUDED
