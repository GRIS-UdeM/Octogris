/*
 ==============================================================================
 
 HIDDelegate.cpp
 Created: 4 Aug 2014 1:23:01pm
 Author:  antoine l
 
 ==============================================================================
 */

#include <iostream>
#include "../JuceLibraryCode/JuceHeader.h"
#include "HIDDelegate.h"
#include "FieldComponent.h"

/*#if JUCE_WINDOWS
 Component * CreateHIDComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
 {
 // not implemented yet on windows
 return NULL;
 }
 #else*/

//==============================================================================
static const float kSourceRadius = 10;
static const float kSourceDiameter = kSourceRadius * 2;


/** HIDDelegate constructor taking two arguments and initializaing its others components by default */

HIDDelegate::HIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor):
mFilter (filter),
mEditor (editor),
nbButton(0),
buttonPressedTab(NULL),
vx(0),
vy(0),
deviceSetRef(NULL),
deviceRef(NULL)
{
    
}

/** Handle_DeviceMatchingCallback is called whenever a HID Device matching the Matching Dictionnary is connected. In our case not much is done.  */
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
}
/** Handle_DeviceRemovalCallback is called whenever a HID Device matching the Matching Dictionnary is physically disconnected. In our case we call the uncheckJoystickButton method which will uncheck the joystick checkbox and reinitialize the IOHIDManager as NULL  */

void HIDDelegate::Handle_DeviceRemovalCallback(void *inContext, IOReturn inResult, void *inSender, IOHIDDeviceRef inIOHIDDeviceRef) {
#pragma unused (  inContext, inResult, inSender )
    
    printf("(context: %p, result: 0x%08X, sender: %p, device: %p).\n",
           inContext, inResult, inSender, (void *) inIOHIDDeviceRef);
    OctogrisAudioProcessorEditor * tempEditor = (OctogrisAudioProcessorEditor*) inContext;
    tempEditor->uncheckJoystickButton();
    
    //what to do when a joystick get unplugged
}

/** Handle_IOHIDDeviceInputValueCallback is called evereytime the connected joystick is used, the type of use and value of use are recovered from IOHIDValueRef sent by the event. First the method convert the IOHIDValueRef to a IOHIDElementRef which allow us to get the usagePage (type of control), the usage (the id of the control), the PhysicalMin and PhysicalMax which are 0 and 1 for common buttons or the max can vary from 256 to 1024 in our experience for the axis from one joystick to an other. We use the physical maximum to get a normalized value otherwise a less precise joystick would not permit mouvement accross the whole circle.
 Exemple for usagePage, usage and value, if I press the button 5 of my joystick usagePage will be 9(Id of the button type and usage will be 5 (number of the button)and value will be 1 (1 if pressed and 0 if not) */
void HIDDelegate::Handle_IOHIDDeviceInputValueCallback(
                                                       void *          inContext,      // context from IOHIDDeviceRegisterInputValueCallback
                                                       IOReturn        inResult,       // completion result for the input value operation
                                                       void *          inSender,       // IOHIDDeviceRef of the device this element is from
                                                       IOHIDValueRef   inIOHIDValueRef // the new element value
//static function called when the joystick is used
) {
    
    
    do {
        
        
        IOHIDElementRef tIOHIDElementRef = IOHIDValueGetElement(inIOHIDValueRef);
        //We get the informations we need from inIOHIDValueRef
        uint32_t usagePage = IOHIDElementGetUsagePage(tIOHIDElementRef);
        uint32_t usage = IOHIDElementGetUsage(tIOHIDElementRef);
        double min = IOHIDElementGetPhysicalMin(tIOHIDElementRef);
        double max = IOHIDElementGetPhysicalMax(tIOHIDElementRef);
        
        double value = IOHIDValueGetScaledValue(inIOHIDValueRef, kIOHIDValueScaleTypePhysical);
        OctogrisAudioProcessorEditor* tempEditor = (OctogrisAudioProcessorEditor*) inContext;  //we get the editor from the context
        if(tempEditor->getHIDDel()!=NULL)
        {
            if(usagePage==1)   //axis
            {
                tempEditor->getHIDDel()->JoystickUsed(usage, value,min,max);  //calling Joystick used the function that will modify the source position
            }
            if(usagePage==9)   //buttons
            {
                double state = IOHIDValueGetScaledValue(inIOHIDValueRef, kIOHIDValueScaleTypePhysical);
                
                
                if(state==1)  //being pressed
                {
                    if(usage<= tempEditor->getNbSources() )
                    {
                        tempEditor->getHIDDel()->setButtonPressedTab(usage,1);
                        
                        tempEditor->getMover()->begin(usage-1, kHID);
                    }
                }
                if(state==0)  //released
                {
                    if(usage<= tempEditor ->getNbSources() )
                    {
                        tempEditor->getHIDDel()->setButtonPressedTab(usage,0);
                        tempEditor->getMover()->end(kHID);
                    }
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
    
}

/**hu_CreateMatchingDictionary method creates a matching dictionnary that allow you to only look for one type of HID devices, in our case the joysticks. As this method is also used as a static method in the HID_Utilities there probably is a better way to use it but this is the way it was explained to me on the website I used to create my hid communications. */
JUCE_COMPILER_WARNING("hu_CreateMatchingDictionary method creates a matching dictionnary that allow you to only look for one type of HID devices, in our case the joysticks. As this method is also used as a static method in the HID_Utilities there probably is a better way to use it but this is the way it was explained to me on the website I used to create my hid communications")
CFDictionaryRef HIDDelegate::hu_CreateMatchingDictionary(uint32_t inUsagePage, uint32_t inUsage) {
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

/** Initialize_HID has to be used once you have created your HIDDelegate in order for the programme to know all there is to know about your joystick (address, number of buttons, setting the callback etc) */
OSStatus HIDDelegate::Initialize_HID(void *inContext) {
    printf("(context: %p)", inContext);
    
    OSStatus result = -1;
    do {    // TRY / THROW block
        // create the manager
        IOOptionBits ioOptionBits = kIOHIDManagerOptionNone;
        if (!gIOHIDManagerRef) {
            printf("%s: Could not create IOHIDManager.\n", __PRETTY_FUNCTION__);
            break;  // THROW
        }
        
        // register our matching & removal callbacks
        IOHIDManagerRegisterDeviceMatchingCallback(gIOHIDManagerRef, Handle_DeviceMatchingCallback, inContext);
        IOHIDManagerRegisterDeviceRemovalCallback(gIOHIDManagerRef, Handle_DeviceRemovalCallback, inContext);
        
        
        if(CFGetTypeID(gIOHIDManagerRef)==IOHIDManagerGetTypeID())
        {
            CFMutableArrayRef matchingCFArrayRef = CFArrayCreateMutable(kCFAllocatorDefault,0, &kCFTypeArrayCallBacks);
            if(matchingCFArrayRef)
            {
                CFDictionaryRef matchingCFDictJoysticktRef = hu_CreateMatchingDictionary(kHIDPage_GenericDesktop,kHIDUsage_GD_Joystick);
                CFDictionaryRef matchingCFDictGamePadtRef = hu_CreateMatchingDictionary(kHIDPage_GenericDesktop,kHIDUsage_GD_GamePad);
                CFArrayAppendValue( matchingCFArrayRef, matchingCFDictJoysticktRef );
                CFArrayAppendValue( matchingCFArrayRef, matchingCFDictGamePadtRef );
                if(matchingCFArrayRef)
                {
                    // setup matching dictionary
                    IOHIDManagerSetDeviceMatchingMultiple(gIOHIDManagerRef, matchingCFArrayRef);
                } else {
                    fprintf(stderr, "%s: hu_CreateDeviceMatchingDictionary failed.", __PRETTY_FUNCTION__);
                    break;
                }
                
                
                
                deviceSetRef = IOHIDManagerCopyDevices(gIOHIDManagerRef);
                if(deviceSetRef!=0x0)
                {
                    CFIndex ind = CFSetGetCount(deviceSetRef);
                    int nbJoysticks = (int)ind;
                    std::string nbJoyStr = std::to_string(nbJoysticks);
                    CFTypeRef array[nbJoysticks];
                    CFSetGetValues(deviceSetRef, array);
                    
                    for(int i=0; i< nbJoysticks;i++)
                    {
                        if(CFGetTypeID(array[i])== IOHIDDeviceGetTypeID())
                        {
                            deviceRef = (IOHIDDeviceRef)array[i];
                            
                        }
                        IOHIDDeviceRegisterInputValueCallback(deviceRef, Handle_IOHIDDeviceInputValueCallback, inContext);
                        uint32_t usagePage = kHIDPage_GenericDesktop;
                        uint32_t usage = kHIDUsage_GD_Joystick;
                        if (IOHIDDeviceConformsTo(deviceRef, usagePage, usage)) {
                            std::cout << "Joystick number 1 " +  nbJoyStr + " joysticks connected \n ";
                            
                            CFArrayRef elementRefTab = IOHIDDeviceCopyMatchingElements(deviceRef, NULL, kIOHIDOptionsTypeNone);
                            //Scheduling each detected device with the running loop to get the Handle_IOHIDDeviceInputValueCallback to be called
                            //IOHIDDeviceScheduleWithRunLoop(deviceRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
                            CFIndex nbElement = CFArrayGetCount(elementRefTab);
                            //There was usually 13 elements which weren't buttons on the devices I tried
                            nbButton = nbElement-13;
                            buttonPressedTab = new bool[nbButton]{false};
                            gElementCFArrayRef =  IOHIDDeviceCopyMatchingElements(deviceRef,
                                                                                  NULL,
                                                                                  kIOHIDOptionsTypeNone);
                            
                            if (gElementCFArrayRef) {
                                
                                //Verifying each elements of the devices to detect any problems.
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
}

void HIDDelegate::readJoystickValuesAndUsingThem()
{
    CFIndex ind = CFSetGetCount(deviceSetRef);
    int nbJoysticks = (int)ind;
    
    
    std::string nbJoyStr = std::to_string(nbJoysticks);
    CFTypeRef array[nbJoysticks];
    CFSetGetValues(deviceSetRef, array);
    for(int i=0; i< nbJoysticks;i++)
    {
        if(CFGetTypeID(array[i])== IOHIDDeviceGetTypeID())
        {
            CFIndex nbElement = CFArrayGetCount(gElementCFArrayRef);
            if (gElementCFArrayRef) {
                
                
                for (CFIndex i = 0; i<nbElement; i++) {
                    IOHIDElementRef tIOHIDElementRef  = (IOHIDElementRef) CFArrayGetValueAtIndex(gElementCFArrayRef,i);
                    uint32_t usagePage = IOHIDElementGetUsagePage(tIOHIDElementRef);
                    uint32_t usage = IOHIDElementGetUsage(tIOHIDElementRef);
                    double min = IOHIDElementGetPhysicalMin(tIOHIDElementRef);
                    double max = IOHIDElementGetPhysicalMax(tIOHIDElementRef);
                    double value =IOHIDElement_GetValue(tIOHIDElementRef,  kIOHIDValueScaleTypePhysical);
                    if(mEditor->getHIDDel()!=NULL)
                    {
                        if(usagePage==1)   //axis
                        {
                            if(!std::isnan(value))
                            {
                                mEditor->getHIDDel()->JoystickUsed(usage, value,min,max);  //calling Joystick used the function that will modify the source position
                            }
                        }
                        if(usagePage==9)   //buttons
                        {
                            if(value==1)  //being pressed
                            {
                                if(usage<= mEditor->getNbSources() )
                                {
                                    mEditor->getHIDDel()->setButtonPressedTab(usage,1);
                                    
                                    mEditor->getMover()->begin(usage-1, kHID);
                                }
                            }
                            if(value==0)  //released
                            {
                                if(usage<= mEditor ->getNbSources() )
                                {
                                    mEditor->getHIDDel()->setButtonPressedTab(usage,0);
                                    
                                }
                            }
                            
                            
                        }
                    }

                }
            }

            
        }
    }
    mEditor->getMover()->end(kHID);
    
}

/** JoystickUsed is called, to handle the effect of the use of the axis while pressing a button on the joystick, by Handle_IOHIDDeviceInputValueCallback because as a static method it is quite limited.
 We give JoystickUsed the usage to know which axis is being used, the scaledValue to know how much the joystick is bent. MaxValue is used to know the resolution of the axis. */
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
                    //Normalizing the coordinate from every joystick as float between 0 and 1, multiplied by the size of the panel to have the new x coordinate of the new point.
                    vx = (scaledValue  / maxValue);
                    newPoint = mFilter->getSourceXY01(i);
                    //vy = newPoint.getY();
                    if(((vx-0.5)*(vx-0.5))+((vy-0.5)*(vy-0.5))<=1.26)
                    {
                        newPoint.setX(vx);   //modifying the old point into the new one
                        newPoint.setY(vy);
                        mEditor->getMover()->move(newPoint, kHID);  //Using de move method from the source mover so the other sources f
                    }
                    
                    break;
                case 49:
                    //Normalizing the coordonate from every joystick as float between -1 and 1, multiplied by the size of the panel to have the new x coordinate of the new point.
                    vy = (1 - (scaledValue  / (maxValue)));
                    //Converting the scaled value of the Y axis send by te joystick to the type of coordinates used by setSourceXY
                    newPoint = mFilter->getSourceXY01(i);
                    //vx = newPoint.getX();
                    if(((vx-0.5)*(vx-0.5))+((vy-0.5)*(vy-0.5))<=1.26)
                    {
                        newPoint.setX(vx);
                        newPoint.setY(vy);
                        
                        mEditor->getMover()->move(newPoint, kHID);
                    }
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

void HIDDelegate::setButtonPressedTab(u_int32_t usage, bool state)  //Get and Set to use the button pressed array
{
    buttonPressedTab[usage-1]=state;
}
bool HIDDelegate::getButtonPressedTab(u_int32_t index)
{
    return buttonPressedTab[index];
}
HIDDelegate::Ptr HIDDelegate::CreateHIDDelegate(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
{
    return new HIDDelegate(filter, editor);
}

