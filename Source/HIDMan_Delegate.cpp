//
//  HIDMan_Delegate.cpp
//  Octogris2
//
//  Created by GRIS on 2015-03-05.
//
//
#include <iostream>
#include "HIDMan_Delegate.h"

#if JUCE_WINDOWS
Component * CreateHIDDelegateComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
{
    // not implemented yet on windows
    return NULL;
}
#else

#include "Leap.h"

class HID_Delegate : public Component, public Button::Listener, public Leap::Listener
{
public:
    HIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor):
    mFilter(filter),
    mEditor(editor),
    mPointableId(-1),
    mLastPositionValid(0)
    {
        // mx is the x coord at which we start to put the components my is the y coord a which we start to put the components
        const int m = 10, dh = 18, cw = 200;
        int x = m, y = m + dh;
        mEnable = new ToggleButton();
        mEnable->setButtonText("Enable Joystick");
        mEnable->setSize(cw, dh);
        mEnable->setTopLeftPosition(x, y);
        mEnable->addListener(this);
        mEnable->setToggleState(mFilter->getIsJoystickEnabled(), dontSendNotification);
        addAndMakeVisible(mEnable);
        
        x += dh + m;
        
        mState = new Label();
        mState->setText("", dontSendNotification);
        mState->setSize(cw , dh);
        mState->setJustificationType(Justification::left);
        mState->setMinimumHorizontalScale(1);
        mState->setTopLeftPosition(x, y);
        addAndMakeVisible(mState);
    }
    
    //
    // this is called once for each connected device
    //
    static void Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
#pragma unused (  inContext, inSender )
        
        printf("(context: %p, result: 0x%08X, sender: %p, device: %p)",
               inContext, inResult, inSender, (void *) inIOHIDDeviceRef);
#ifdef DEBUG
        HIDDumpDeviceInfo(inIOHIDDeviceRef);
#endif // def DEBUG
        uint32_t vendorID = IOHIDDevice_GetVendorID(inIOHIDDeviceRef);
        uint32_t productID = IOHIDDevice_GetProductID(inIOHIDDeviceRef);
        if ((vendorID != 0x12BA) || (productID != 0x0030)) {
            //what to do when a joystick get plugged
        }
    } // Handle_DeviceMatchingCallback
    
    //
    // this is called once for each disconnected device
    //
    static void Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
#pragma unused (  inContext, inResult, inSender )
        
        printf("(context: %p, result: 0x%08X, sender: %p, device: %p).\n",
               inContext, inResult, inSender, (void *) inIOHIDDeviceRef);
        
        //what to do when a joystick get unplugged
    } // Handle_DeviceRemovalCallback
    CFMutableDictionaryRef hu_CreateMatchingDictionary(uint32_t inUsagePage, uint32_t inUsage) {
        // create a dictionary to add usage page/usages to
        CFMutableDictionaryRef refHIDMatchDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
        if (refHIDMatchDictionary) {
            if (inUsagePage) {
                // Add key for device type to refine the matching dictionary.
                CFNumberRef pageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inUsagePage);
                if (pageCFNumberRef) {
                    CFDictionarySetValue(refHIDMatchDictionary,
                                         CFSTR(kIOHIDPrimaryUsagePageKey), pageCFNumberRef);
                    CFRelease(pageCFNumberRef);
                    // note: the usage is only valid if the usage page is also defined
                    if (inUsage) {
                        CFNumberRef usageCFNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inUsage);
                        if (usageCFNumberRef) {
                            CFDictionarySetValue(refHIDMatchDictionary,
                                                 CFSTR(kIOHIDPrimaryUsageKey), usageCFNumberRef);
                            CFRelease(usageCFNumberRef);
                        } else {
                            fprintf(stderr, "%s: CFNumberCreate(usage) failed.", __PRETTY_FUNCTION__);
                        }
                    }
                } else {
                    fprintf(stderr, "%s: CFNumberCreate(usage page) failed.", __PRETTY_FUNCTION__);
                }
            }
        } else {
            fprintf(stderr, "%s: CFDictionaryCreateMutable failed.", __PRETTY_FUNCTION__);
        }
        
        return (refHIDMatchDictionary);
    }   // hu_CreateMatchingDictionary

    
    OSStatus Initialize_HID(void *inContext) {
        printf("(context: %p)", inContext);
        
        OSStatus result = -1;
        
        do {    // TRY / THROW block
            // create the manager
            IOOptionBits ioOptionBits = kIOHIDManagerOptionNone;
            //IOOptionBits ioOptionBits = kIOHIDManagerOptionUsePersistentProperties;
            //IOOptionBits ioOptionBits = kIOHIDManagerOptionUsePersistentProperties | kIOHIDManagerOptionDoNotLoadProperties;
            gIOHIDManagerRef = IOHIDManagerCreate(kCFAllocatorDefault, ioOptionBits);
            if (!gIOHIDManagerRef) {
                printf("%s: Could not create IOHIDManager.\n", __PRETTY_FUNCTION__);
                break;  // THROW
            }
            
            // register our matching & removal callbacks
            IOHIDManagerRegisterDeviceMatchingCallback(gIOHIDManagerRef, Handle_DeviceMatchingCallback, inContext);
            IOHIDManagerRegisterDeviceRemovalCallback(gIOHIDManagerRef, Handle_DeviceRemovalCallback, inContext);
            
            // schedule us with the run loop
            IOHIDManagerScheduleWithRunLoop(gIOHIDManagerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
            if(CFGetTypeID(gIOHIDManagerRef)==IOHIDManagerGetTypeID())
            {
                CFMutableArrayRef matchingCFArrayRef = CFArrayCreateMutable(kCFAllocatorDefault,0, &kCFTypeArrayCallBacks);
                if(matchingCFArrayRef)
                {
                    CFDictionaryRef matchingCFDictRef = hu_CreateMatchingDictionary(kHIDPage_GenericDesktop,kHIDUsage_GD_Joystick);
                    if(matchingCFDictRef)
                    {
                        // setup matching dictionary
                        IOHIDManagerSetDeviceMatching(gIOHIDManagerRef, matchingCFDictRef);
                    } else {
                        fprintf(stderr, "%s: hu_CreateDeviceMatchingDictionary failed.", __PRETTY_FUNCTION__);
                        break;
                    }
                    
                    
                }
                
            }
            // open it
            IOReturn tIOReturn = IOHIDManagerOpen(gIOHIDManagerRef, kIOHIDOptionsTypeNone);
            if (kIOReturnSuccess != tIOReturn) {
                printf("%s: IOHIDManagerOpen error: 0x%08u (\"%s\" - \"%s\").\n",
                       __PRETTY_FUNCTION__,
                       tIOReturn);
                break;  // THROW
            }
            
            printf("IOHIDManager (%p) creaded and opened!", (void *) gIOHIDManagerRef);
        } while (false);
        
    Oops:;
        return (result);
    }   // Initialize_HID
    
    
    OSStatus Terminate_HID(void *inContext) {
#if false
        IOHIDManagerSaveToPropertyDomain(gIOHIDManagerRef,
                                         kCFPreferencesCurrentApplication,
                                         kCFPreferencesCurrentUser,
                                         kCFPreferencesCurrentHost,
                                         kIOHIDOptionsTypeNone);
        return (IOHIDManagerClose(gIOHIDManagerRef, kIOHIDOptionsTypeNone));
#else
        return (noErr);
#endif
    }

    
    
    
    
private :
    OctogrisAudioProcessor *mFilter;
    OctogrisAudioProcessorEditor *mEditor;
    
    ScopedPointer<ToggleButton> mEnable;
    ScopedPointer<Label> mState;
    
   
    int32_t mPointableId;
    bool mLastPositionValid;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HIDDelegate)
};





#endif

