//
//  HIDDelegate.h
//  Octogris2
//
//  Created by GRIS on 2015-03-10.
//
//

#ifndef __Octogris2__HIDDelegate__
#define __Octogris2__HIDDelegate__

#if JUCE_WINDOWS

#else

#include <stdio.h>
#include "HID_Utilities_External.h"
#include "PluginEditor.h"

class HIDDelegate : public ReferenceCountedObject
{
public:
    typedef ReferenceCountedObjectPtr<HIDDelegate> Ptr;
    
    virtual ~HIDDelegate() {};
    HIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
    OSStatus Initialize_HID(void *inContext);
    static HIDDelegate * CreateHIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor);
    
    static void Handle_IOHIDDeviceInputValueCallback(void * inContext,IOReturn inResult,void * inSender, IOHIDValueRef   inIOHIDValueRef);
    static void Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
    static void Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef);
    void JoystickUsed(uint32_t usage, float scaledValue, double min, double max);
    CFMutableDictionaryRef hu_CreateMatchingDictionary(uint32_t inUsagePage, uint32_t inUsage);
    FPoint getSourcePoint(int i);
    void setButtonPressedTab(u_int32_t index, bool state);
    bool getButtonPressedTab(u_int32_t index);
    int getNbButton(){return nbButton;};
    IOHIDDeviceRef getDeviceRef(){return gDeviceRef;}
    CFSetRef getDeviceSetRef(){return gDeviceSetRef;}

private:
    OctogrisAudioProcessor *mFilter;
    OctogrisAudioProcessorEditor *mEditor;
    int nbButton;
    bool* buttonPressedTab;
    float vx, vy, equRes  ;
    
    CFSetRef gDeviceSetRef;
    IOHIDDeviceRef gDeviceRef;
   
    
    
    
    
    //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HIDDelegate)
};

#endif

#endif  // OCTOLEAP_H_INCLUDED
