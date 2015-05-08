/*
 ==============================================================================
 
 HIDDelegate.cpp
 Created: 4 Aug 2014 1:23:01pm
 Author:  antoine l
 
 ==============================================================================
 */

#include <iostream>

#include "FieldComponent.h"

class OctogrisAudioProcessorEditor;

#if WIN32

 Component * CreateHIDComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
 {
 // not implemented yet on windows
 return NULL;
 }

#else
#include "HIDDelegate.h"


//==============================================================================
static const float kSourceRadius = 10;
static const float kSourceDiameter = kSourceRadius * 2;
static const float kSpeakerRadius = 10;
static const float kSpeakerDiameter = kSpeakerRadius * 2;




HIDDelegate::HIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
{
    printf("HIDDelegate created");
    mFilter = filter;
    mEditor = editor;
    float vx, vy ;
}

void HIDDelegate::Handle_DeviceMatchingCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
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

void HIDDelegate::Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
#pragma unused (  inContext, inResult, inSender )
    
    printf("(context: %p, result: 0x%08X, sender: %p, device: %p).\n",
           inContext, inResult, inSender, (void *) inIOHIDDeviceRef);
    
    //what to do when a joystick get unplugged
} // Handle_DeviceRemovalCallback

void HIDDelegate::Handle_IOHIDDeviceInputValueCallback(
                                                       void *          inContext,      // context from IOHIDDeviceRegisterInputValueCallback
                                                       IOReturn        inResult,       // completion result for the input value operation
                                                       void *          inSender,       // IOHIDDeviceRef of the device this element is from
                                                       IOHIDValueRef   inIOHIDValueRef // the new element value
                    //static function called when the joystick is used
) {
    
    IOHIDDeviceRef tIOHIDDeviceRef = (IOHIDDeviceRef) inSender;
    
    do {
        
        
        // is this value's element valid?
        IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(inIOHIDValueRef);           //We get the informations we need from inIOHIDValueRef
        CFStringRef nameKey = (CFStringRef) kIOHIDElementNameKey;
        uint32_t usagePage = IOHIDElementGetUsagePage(tIOHIDElementRef);
        uint32_t usage = IOHIDElementGetUsage(tIOHIDElementRef);
        double min = IOHIDElementGetPhysicalMin(tIOHIDElementRef);
        double max = IOHIDElementGetPhysicalMax(tIOHIDElementRef);
        
        
        double value = IOHIDValueGetScaledValue(inIOHIDValueRef, kIOHIDValueScaleTypePhysical);
        OctogrisAudioProcessorEditor* tempEditor = (OctogrisAudioProcessorEditor*) inContext;  //we get the editor from the context
        if(tempEditor->getHIDDel()!=NULL)
        {
      
            tempEditor->getHIDDel()->JoystickUsed(usage, value,min,max);  //calling Joystick used the function that will modify the source position
        
        if(usagePage==9)   //buttons
        {
            double state = IOHIDValueGetScaledValue(inIOHIDValueRef, kIOHIDValueScaleTypePhysical);
            
            
            if(state==1)  //being pressed
            {
                tempEditor->getHIDDel()->setButtonPressedTab(usage,1);
                tempEditor->getMover()->begin(usage-1, kOsc);
            }
            if(state==0)  //released
            {
                tempEditor->getHIDDel()->setButtonPressedTab(usage,0);
                tempEditor->getMover()->end(kOsc);
            }
            
            
        }
        
        if (!tIOHIDElementRef) {
            printf("tIOHIDElementRef == NULL\n");
            break;                                                              // (no)
        }
        
        // length ok?
        CFIndex length = IOHIDValueGetLength(inIOHIDValueRef);
        if (length > sizeof(double_t)) {
            break;                                                              // (no)
        }
        
        }
        
    }
    while (false);
    
}   // Handle_IOHIDDeviceInputValueCallback

CFMutableDictionaryRef HIDDelegate::hu_CreateMatchingDictionary(uint32_t inUsagePage, uint32_t inUsage) {
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


OSStatus HIDDelegate::Initialize_HID(void *inContext) {
    printf("(context: %p)", inContext);
    
    OSStatus result = -1;
    do {    // TRY / THROW block
        // create the manager
        IOOptionBits ioOptionBits = kIOHIDManagerOptionNone;
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
                
                
                
                gDeviceSetRef = IOHIDManagerCopyDevices(gIOHIDManagerRef);
                if(gDeviceSetRef!=0x0)
                {
                    CFIndex ind = CFSetGetCount(gDeviceSetRef);
                    int nbJoysticks = (int)ind;
                    std::string nbJoyStr = std::to_string(nbJoysticks);
                    CFTypeRef array[nbJoysticks];
                    CFSetGetValues(gDeviceSetRef, array);
                    
                    for(int i=0; i< nbJoysticks;i++)
                    {
                        if(CFGetTypeID(array[i])== IOHIDDeviceGetTypeID())
                        {
                            gDeviceRef = (IOHIDDeviceRef)array[i];
                            
                        }
                        IOHIDDeviceRegisterInputValueCallback(gDeviceRef, Handle_IOHIDDeviceInputValueCallback, inContext);
                        uint32_t usagePage = kHIDPage_GenericDesktop;
                        uint32_t usage = kHIDUsage_GD_Joystick;
                        if (IOHIDDeviceConformsTo(gDeviceRef, usagePage, usage)) {
                            IOReturn resultFromDevOp = IOHIDDeviceOpen(gDeviceRef, kIOHIDOptionsTypeNone);
                            //std::cout << "Joystick number 1 " +  nbJoyStr + " joysticks connected \n ";
                            
                            CFArrayRef elementRefTab = IOHIDDeviceCopyMatchingElements(gDeviceRef, NULL, kIOHIDOptionsTypeNone);
                            IOHIDDeviceScheduleWithRunLoop(gDeviceRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
                            CFIndex nbElement = CFArrayGetCount(elementRefTab);
                            nbButton = nbElement-13;
                            buttonPressedTab = new bool[nbButton]{false};
                            gElementCFArrayRef =  IOHIDDeviceCopyMatchingElements(gDeviceRef,
                                                                                  NULL,
                                                                                  kIOHIDOptionsTypeNone);
                            
                            if (gElementCFArrayRef) {
                                
                                
                                for (CFIndex i = 0; i<nbElement; i++) {
                                    IOHIDElementRef tIOHIDElementRef  = (IOHIDElementRef) CFArrayGetValueAtIndex(gElementCFArrayRef,i);
                                    
                                    IOHIDElementType tIOHIDElementType = IOHIDElementGetType(tIOHIDElementRef);
                                    if (tIOHIDElementType > kIOHIDElementTypeInput_ScanCodes) {
                                        continue;
                                    }
                                    
                                    uint32_t reportSize = IOHIDElementGetReportSize(tIOHIDElementRef);
                                    uint32_t reportCount = IOHIDElementGetReportCount(tIOHIDElementRef);
                                    if ((reportSize * reportCount) > 64) {
                                        continue;
                                    }
                                    
                                    uint32_t usagePage = IOHIDElementGetUsagePage(tIOHIDElementRef);
                                    uint32_t usage = IOHIDElementGetUsage(tIOHIDElementRef);
                                    if (!usagePage || !usage) {
                                        continue;
                                    }
                                    if (-1 == usage) {
                                        continue;
                                    }
                                    
                                }
                            }
                        }
                    }
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


void HIDDelegate::JoystickUsed(uint32_t usage, float scaledValue, double minValue, double maxValue)
{

    for(int i =0; i< getNbButton(); i++)    //Sweep accross all the joystick button to check which is being pressed
    {
        if(this->getButtonPressedTab(i))
        {
            
            FPoint newPoint;
            //Switch to detect what part of the device is being used
            switch (usage) {
                case 48:
                    
                    //printf("Axe X !!!! \n");
                    vx = (scaledValue  / maxValue);
                    //Converting the scaled value of the X axis send by te joystick to the type of coordinates used by setSourceXY
                    newPoint = mFilter->getSourceXY01(i);
                    //vy = newPoint.getY();
                    if(((vx-0.5)*(vx-0.5))+((vy-0.5)*(vy-0.5))<=1.26)
                    {
                        
                        newPoint.setX(vx);   //modifying the old point into the new one
                        newPoint.setY(vy);
                        mEditor->getMover()->move(newPoint, kOsc);
                        equRes = ((vx-0.5)*(vx-0.5))+((vy-0.5)*(vy-0.5));
                        printf("I : %f \n",equRes);
                    }
                    else
                    {
                    }
                    
                    
                    
                     //mFilter->setSourceXY(i, newPoint);  //Setting the new point as the new coordinates to the controled source.
                    //printf("Scaled value : %f \n", scaledValue);
                    break;
                case 49:
                    //printf("Axe Y !!!! \n");
                    vy = (1 - (scaledValue  / (maxValue)));
                    //Converting the scaled value of the Y axis send by te joystick to the type of coordinates used by setSourceXY
                    newPoint = mFilter->getSourceXY01(i);
                    //vx = newPoint.getX();
                    if(((vx-0.5)*(vx-0.5))+((vy-0.5)*(vy-0.5))<=1.26)
                    {
                        newPoint.setX(vx);
                        newPoint.setY(vy);
                        
                        mEditor->getMover()->move(newPoint, kOsc);
                        equRes = ((vx-0.5)*(vx-0.5))+((vy-0.5)*(vy-0.5));
                        printf("I : %f \n",equRes);
                    }
                    else
                    {
                    }
                    
                   
                    
                    //newPoint = *new FPoint(test.getX(),vy*2);  //creating a new point with the old X value and the new Y value
                    //mFilter->setSourceXY(i, newPoint);   //Setting the new point as the new coordinates to the controled source.
                    
                    //printf("Scaled value : %f \n", scaledValue);
                    break;
                case 53:
                    //printf("Axe RZ !!!! \n");
                    break;
                case 54:
                    printf("Slider !!!! \n");
                    break;
                case 57:
                    printf("Hat Switch !!!! \n");
                    break;
                default:
                    break;
            }
            mEditor->repaint();
        }
        
    }
    
    
    
}
FPoint HIDDelegate::getSourcePoint(int i)  //function to get the source coordinate with it's number (Copy/Paste from FieldComponent)
{
    if(i!=-1)
    {
        const int fieldWidth = mEditor->getWidth();
        FPoint p = mFilter->getSourceXY01(i);
        float x = p.x * (fieldWidth - kSourceDiameter) + kSourceRadius;
        float y = p.y * (fieldWidth - kSourceDiameter) + kSourceRadius;
        return FPoint(x, fieldWidth - y);
    }
}

void HIDDelegate::setButtonPressedTab(u_int32_t usage, bool state)  //Get and Set to use the button pressed array
{
    buttonPressedTab[usage-1]=state;
}
bool HIDDelegate::getButtonPressedTab(u_int32_t index)
{
    return buttonPressedTab[index];
}

HIDDelegate * HIDDelegate::CreateHIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
{
    return new HIDDelegate(filter, editor);
}

#endif