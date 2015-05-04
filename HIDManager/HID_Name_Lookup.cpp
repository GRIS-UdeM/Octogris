//     File: HID_Name_Lookup.c
// Abstract: HID Name Lookup Utilities.
//  Version: 2.0
//
// Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
// Inc. ("Apple") in consideration of your agreement to the following
// terms, and your use, installation, modification or redistribution of
// this Apple software constitutes acceptance of these terms.  If you do
// not agree with these terms, please do not use, install, modify or
// redistribute this Apple software.
//
// In consideration of your agreement to abide by the following terms, and
// subject to these terms, Apple grants you a personal, non-exclusive
// license, under Apple's copyrights in this original Apple software (the
// "Apple Software"), to use, reproduce, modify and redistribute the Apple
// Software, with or without modifications, in source and/or binary forms;
// provided that if you redistribute the Apple Software in its entirety and
// without modifications, you must retain this notice and the following
// text and disclaimers in all such redistributions of the Apple Software.
// Neither the name, trademarks, service marks or logos of Apple Inc. may
// be used to endorse or promote products derived from the Apple Software
// without specific prior written permission from Apple.  Except as
// expressly stated in this notice, no other rights or licenses, express or
// implied, are granted by Apple herein, including but not limited to any
// patent rights that may be infringed by your derivative works or by other
// works in which the Apple Software may be incorporated.
//
// The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
// MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
// THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
// OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
// MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
// AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
// STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2014 Apple Inc. All Rights Reserved.
//
// *****************************************************

#if JUCE_MAC

#pragma mark - includes & imports
// *****************************************************
#include "HID_Utilities_External.h"
// *****************************************************
#pragma mark - typedefs, enums, defines, etc.
// *****************************************************
#define FAKE_MISSING_NAMES       0  // for debugging; returns the vendor, product & cookie (or usage info) as numbers.
#define VERBOSE_ELEMENT_NAMES    0  // set true to include vender & product names in element names (useful for debugging)

#define kNameKeyCFStringRef      CFSTR("Name")
// *****************************************************
#pragma mark - local (static) function prototypes
// *****************************************************

#if false   // currently unused
static SInt32 hu_SaveToXMLFile(CFPropertyListRef inCFPRef, CFURLRef inCFURLRef);
static SInt32 hu_XMLSave(CFPropertyListRef inCFPropertyListRef, CFStringRef inResourceName, CFStringRef inResourceExtension);
#endif // if 0
static CFPropertyListRef hu_CreateFromXMLURL(CFURLRef inCFURLRef);
static CFPropertyListRef hu_CreateFromXMLResource(CFStringRef inResourceName, CFStringRef inResourceExtension);

#if false   // currently unused
static Boolean hu_AddVendorProductToCFDict(CFMutableDictionaryRef	inCFMutableDictionaryRef,
                                           uint32_t					inVendorID,
                                           CFStringRef				inVendorCFStringRef,
                                           uint32_t					inProductID,
                                           CFStringRef				inProductCFStringRef);
static Boolean hu_AddDeviceElementToUsageXML(IOHIDDeviceRef inIOHIDDeviceRef, IOHIDElementRef inIOHIDElementRef);
#endif // if 0
// *****************************************************
#pragma mark - exported globals
// *****************************************************
#pragma mark - local (static) globals
// *****************************************************
static CFPropertyListRef gCookieCFPropertyListRef = NULL;
static CFPropertyListRef gUsageCFPropertyListRef = NULL;

// *****************************************************
#pragma mark - exported function implementations
// *****************************************************

/*************************************************************************
 *
 * HIDGetVendorNameFromVendorID(inVendorID, inProductID, inCookie, outCStrName)
 *
 * Purpose: Uses an devices vendor ID to generate a name for it.
 *
 * Notes:	Uses HID_device_usage_strings.plist (XML) file to store dictionary of names
 *
 * Inputs:  inVendorID  - the elements vendor ID
 * Returns: CFStringRef	- the vendor name
 */
CFStringRef HIDCopyVendorNameFromVendorID(uint32_t inVendorID) {
	CFStringRef result = NULL;
	if (!gUsageCFPropertyListRef) {
		gUsageCFPropertyListRef =
        hu_CreateFromXMLResource(CFSTR("HID_device_usage_strings"), CFSTR("plist"));
	}
	if (gUsageCFPropertyListRef) {
		if (CFDictionaryGetTypeID() == CFGetTypeID(gUsageCFPropertyListRef)) {
			CFDictionaryRef vendorCFDictionaryRef;
			CFStringRef vendorKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), inVendorID);
			if (vendorKeyCFStringRef) {
                
				if (CFDictionaryGetValueIfPresent((CFDictionaryRef)gUsageCFPropertyListRef, vendorKeyCFStringRef,
				                                  (const void **) &vendorCFDictionaryRef))
				{
					CFStringRef vendorCFStringRef = NULL;
					if (CFDictionaryGetValueIfPresent(vendorCFDictionaryRef, kNameKeyCFStringRef,
					                                  (const void **) &vendorCFStringRef) && vendorCFStringRef)
					{
						result = CFStringCreateCopy(kCFAllocatorDefault, vendorCFStringRef);
					}
				}
				CFRelease(vendorKeyCFStringRef);
			}
		}
		// ++ CFRelease(gUsageCFPropertyListRef);	// Leak this !
	}
    
#if FAKE_MISSING_NAMES
	if (!result) {
		result = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, @"#{ V: %ld}#", inVendorID);
	}
    
#endif // FAKE_MISSING_NAMES
	return (result);
}   // HIDGetVendorNameFromVendorID

/*************************************************************************
 *
 * HIDGetProductNameFromVendorProductID(inVendorID, inProductID, outCStrName)
 *
 * Purpose: Uses an elements vendor, product & usage info to generate a name for it.
 *
 * Notes:	Uses HID_device_usage_strings.plist (XML) file to store dictionary of names
 *
 * Inputs: inVendorID - the elements vendor ID
 *			inProductID - the elements product ID
 * Returns: CFStringRef	- the vendor name
 */
CFStringRef HIDCopyProductNameFromVendorProductID(uint32_t inVendorID, uint32_t inProductID) {
	CFStringRef result = NULL;
	if (!gUsageCFPropertyListRef) {
		gUsageCFPropertyListRef =
        hu_CreateFromXMLResource(CFSTR("HID_device_usage_strings"), CFSTR("plist"));
	}
	if (gUsageCFPropertyListRef) {
		if (CFDictionaryGetTypeID() == CFGetTypeID(gUsageCFPropertyListRef)) {
			// first we make our vendor ID key
			CFStringRef vendorKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), inVendorID);
			if (vendorKeyCFStringRef) {
				// and use it to look up our vendor dictionary
				CFDictionaryRef vendorCFDictionaryRef;
				if (CFDictionaryGetValueIfPresent((CFDictionaryRef)gUsageCFPropertyListRef,
				                                  vendorKeyCFStringRef,
				                                  (const void **) &vendorCFDictionaryRef))
				{
					// pull our vendor name our of that dictionary
					CFStringRef vendorCFStringRef = NULL;
					if (CFDictionaryGetValueIfPresent(vendorCFDictionaryRef,
					                                  kNameKeyCFStringRef,
					                                  (const void **) &vendorCFStringRef))
					{
#if FAKE_MISSING_NAMES
						CFRetain(vendorCFStringRef);    // so we can CFRelease it later
					} else {
						vendorCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault,
						                                             NULL,
                                                                     CFSTR("V: %@"),
						                                             vendorKeyCFStringRef);
#endif                  // if FAKE_MISSING_NAMES
					}
                    
					// now we make our product ID key
					CFStringRef productKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault,
					                                                             NULL,
                                                                                 CFSTR("%d"),
					                                                             inProductID);
					if (productKeyCFStringRef) {
						// and use that key to look up our product dictionary in the vendor dictionary
						CFDictionaryRef productCFDictionaryRef;
						if (CFDictionaryGetValueIfPresent(vendorCFDictionaryRef,
						                                  productKeyCFStringRef,
						                                  (const void **) &productCFDictionaryRef))
						{
							// pull our product name our of the product dictionary
							CFStringRef productCFStringRef = NULL;
							if (CFDictionaryGetValueIfPresent(productCFDictionaryRef,
							                                  kNameKeyCFStringRef,
							                                  (const void **) &productCFStringRef))
							{
#if FAKE_MISSING_NAMES
								CFRetain(productCFStringRef);   // so we can CFRelease it later
							} else {
								productCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault,
								                                              NULL,
                                                                              CFSTR("P: %@"),
								                                              kNameKeyCFStringRef);
#endif                          // if FAKE_MISSING_NAMES
							}
                            
							CFStringRef fullCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault,
							                                                       NULL,
                                                                                   CFSTR("%@ %@"),
							                                                       vendorCFStringRef,
							                                                       productCFStringRef);
							if (fullCFStringRef) {
								// CFShow(fullCFStringRef);
								result = CFStringCreateCopy(kCFAllocatorDefault, fullCFStringRef);
								CFRelease(fullCFStringRef);
							}
                            
#if FAKE_MISSING_NAMES
							if (productCFStringRef) {
								CFRelease(productCFStringRef);
							}
                            
#endif                      // if FAKE_MISSING_NAMES
						}
                        
                        CFRelease(productKeyCFStringRef);
					}
                    
#if FAKE_MISSING_NAMES
					if (vendorCFStringRef) {
                        CFRelease(vendorCFStringRef);
					}
                    
#endif              // if FAKE_MISSING_NAMES
				}
                
                CFRelease(vendorKeyCFStringRef);
			}
		}
        
		// ++ CFRelease(gUsageCFPropertyListRef);	// Leak this !
	}
    
#if FAKE_MISSING_NAMES
	sprintf(outCStrName, "#{ V: %ld, P: %ld, U: %ld: %ld}#", inVendorID, inProductID, inUsagePage, inUsage);
	result = true;
#endif // FAKE_MISSING_NAMES
	return (result);
}   // HIDGetProductNameFromVendorProductID

/*************************************************************************
 *
 * HIDGetElementNameFromVendorProductCookie(inVendorID, inProductID, inCookie)
 *
 * Purpose: Uses an elements vendor, product & cookie to generate a name for it.
 *
 * Notes:	Uses HID_cookie_strings.plist (XML) file to store dictionary of names
 *
 * Inputs:  inVendorID  - the elements vendor ID
 *			inProductID - the elements product ID
 *			inCookie	- the elements cookie
 * Returns: Boolean		- if successful
 */
CFStringRef HIDCopyElementNameFromVendorProductCookie(uint32_t inVendorID, uint32_t inProductID, IOHIDElementCookie inCookie) {
	CFStringRef result = NULL;
	// Look in the XML file first
	if (!gCookieCFPropertyListRef) {
		gCookieCFPropertyListRef =
        hu_CreateFromXMLResource(CFSTR("HID_cookie_strings"), CFSTR("plist"));
	}
	if (gCookieCFPropertyListRef) {
		if (CFDictionaryGetTypeID() == CFGetTypeID(gCookieCFPropertyListRef)) {
			CFStringRef vendorKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), inVendorID);
			if (vendorKeyCFStringRef) {
				CFDictionaryRef vendorCFDictionaryRef;
				if (CFDictionaryGetValueIfPresent((CFDictionaryRef)gCookieCFPropertyListRef,
				                                  vendorKeyCFStringRef,
				                                  (const void **) &vendorCFDictionaryRef))
				{
					CFDictionaryRef productCFDictionaryRef;
					CFStringRef productKeyCFStringRef;
					CFStringRef vendorCFStringRef;
					if (CFDictionaryGetValueIfPresent(vendorCFDictionaryRef,
					                                  kNameKeyCFStringRef,
					                                  (const void **) &vendorCFStringRef))
					{
						// CFShow(vendorCFStringRef);
					}
                    
					productKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), inProductID);
					if (CFDictionaryGetValueIfPresent(vendorCFDictionaryRef,
					                                  productKeyCFStringRef,
					                                  (const void **) &productCFDictionaryRef))
					{
						CFStringRef fullCFStringRef = NULL;
						CFStringRef productCFStringRef;
						if (CFDictionaryGetValueIfPresent(productCFDictionaryRef,
						                                  kNameKeyCFStringRef,
						                                  (const void **) &productCFStringRef))
						{
							// CFShow(productCFStringRef);
						}
                        
						CFStringRef cookieKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault,
						                                                            NULL,
						                                                            CFSTR("%d"),
						                                                            inCookie);
						CFStringRef cookieCFStringRef;
						if (CFDictionaryGetValueIfPresent(productCFDictionaryRef,
						                                  cookieKeyCFStringRef,
						                                  (const void **) &cookieCFStringRef))
						{
#if VERBOSE_ELEMENT_NAMES
							fullCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault,
							                                           NULL,
                                                                       CFSTR("%@ %@ %@"),
							                                           vendorCFStringRef,
							                                           productCFStringRef,
							                                           cookieCFStringRef);
#else                       // if VERBOSE_ELEMENT_NAMES
							fullCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault,
							                                           NULL,
                                                                       CFSTR("%@"),
							                                           cookieCFStringRef);
#endif                      // VERBOSE_ELEMENT_NAMES
							// CFShow(cookieCFStringRef);
						}
                        
#if FAKE_MISSING_NAMES
						else {
							fullCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault,
							                                           NULL,
                                                                       CFSTR("%@ %@ # %@"),
							                                           vendorCFStringRef,
							                                           productCFStringRef,
							                                           cookieKeyCFStringRef);
						}
#endif                  // FAKE_MISSING_NAMES
						if (fullCFStringRef) {
							// CFShow(fullCFStringRef);
							result = CFStringCreateCopy(kCFAllocatorDefault, fullCFStringRef);
							CFRelease(	fullCFStringRef);
						}
                        
                        CFRelease(	cookieKeyCFStringRef);
					}
                    
                    CFRelease(	productKeyCFStringRef);
				}
                
                CFRelease(	vendorKeyCFStringRef);
			}
		}
        
		// ++ CFRelease(gCookieCFPropertyListRef);	// Leak this !
	}
    
#if FAKE_MISSING_NAMES
	sprintf(outCStrName, "#{ V: %ld, P: %ld, C: %ld}#", inVendorID, inProductID, inCookie);
#endif // FAKE_MISSING_NAMES
	return (result);
}   // HIDGetElementNameFromVendorProductCookie

/*************************************************************************
 *
 * HIDGetElementNameFromVendorProductUsage(inVendorID, inProductID, inUsagePage, inUsage)
 *
 * Purpose: Uses an elements vendor, product & usage info to generate a name for it.
 *
 * Notes:	Uses HID_device_usage_strings.plist (XML) file to store dictionary of names
 *
 * Inputs: inVendorID - the elements vendor ID
 *			inProductID - the elements product ID
 *			inUsagePage	- the elements usage page
 *			inUsage		- the elements usage
 * Returns: Boolean		- if successful
 */
CFStringRef HIDCopyElementNameFromVendorProductUsage(uint32_t	inVendorID,
                                                     uint32_t	inProductID,
                                                     uint32_t	inUsagePage,
                                                     uint32_t	inUsage) {
	CFStringRef result = NULL;
	if (!gUsageCFPropertyListRef) {
		gUsageCFPropertyListRef =
        hu_CreateFromXMLResource(CFSTR("HID_device_usage_strings"), CFSTR("plist"));
	}
	if (gUsageCFPropertyListRef) {
		if (
		    CFDictionaryGetTypeID() == CFGetTypeID(gUsageCFPropertyListRef))
		{
			CFStringRef vendorKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), inVendorID);
			if (vendorKeyCFStringRef) {
				CFDictionaryRef vendorCFDictionaryRef;
				if (CFDictionaryGetValueIfPresent((CFDictionaryRef)gUsageCFPropertyListRef, vendorKeyCFStringRef,
				                                  (const void **) &vendorCFDictionaryRef))
				{
					CFStringRef vendorCFStringRef = NULL;
					if (CFDictionaryGetValueIfPresent(vendorCFDictionaryRef, kNameKeyCFStringRef,
					                                  (const void **) &vendorCFStringRef))
					{
						vendorCFStringRef = CFStringCreateCopy(kCFAllocatorDefault, vendorCFStringRef);
					} else {
						vendorCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("v: %d"), inVendorID);
						// CFShow(vendorCFStringRef);
					}
                    
					CFStringRef productKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), inProductID);
                    
					CFDictionaryRef productCFDictionaryRef;
					if (CFDictionaryGetValueIfPresent(vendorCFDictionaryRef, productKeyCFStringRef,
					                                  (const void **) &productCFDictionaryRef))
					{
						CFStringRef fullCFStringRef = NULL;
                        
						CFStringRef productCFStringRef;
						if (CFDictionaryGetValueIfPresent(productCFDictionaryRef, kNameKeyCFStringRef,
						                                  (const void **) &productCFStringRef))
						{
							// CFShow(productCFStringRef);
						}
                        
						CFStringRef usageKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d:%d"), inUsagePage, inUsage);
						CFStringRef usageCFStringRef;
						if (CFDictionaryGetValueIfPresent(productCFDictionaryRef, usageKeyCFStringRef,
						                                  (const void **) &usageCFStringRef))
						{
#if VERBOSE_ELEMENT_NAMES
							fullCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ %@ %@"), vendorCFStringRef, productCFStringRef,
							                                           usageCFStringRef);
#else                       // if VERBOSE_ELEMENT_NAMES
							fullCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@"), usageCFStringRef);
#endif                      // VERBOSE_ELEMENT_NAMES
							// CFShow(usageCFStringRef);
						}
                        
#if FAKE_MISSING_NAMES
						else {
							fullCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ %@ # %@"), vendorCFStringRef, productCFStringRef,
							                                           usageKeyCFStringRef);
						}
#endif                  // FAKE_MISSING_NAMES
						if (fullCFStringRef) {
							// CFShow(fullCFStringRef);
							result = CFStringCreateCopy(kCFAllocatorDefault, fullCFStringRef);
							CFRelease(	fullCFStringRef);
						}
                        
                        CFRelease(	usageKeyCFStringRef);
					}
					if (vendorCFStringRef) {
                        CFRelease(	vendorCFStringRef);
					}
                    
                    CFRelease(	productKeyCFStringRef);
				}
                
                CFRelease(	vendorKeyCFStringRef);
			}
		}
        
		// ++ CFRelease(gUsageCFPropertyListRef);	// Leak this !
	}
    
#if FAKE_MISSING_NAMES
	sprintf(outCStrName, "#{ V: %ld, P: %ld, U: %ld: %ld}#", inVendorID, inProductID, inUsagePage, inUsage);
	result = true;
#endif // FAKE_MISSING_NAMES
	return (result);
}   // HIDGetElementNameFromVendorProductUsage

#if false	// currently unused
/*************************************************************************
 *
 * HIDAddDeviceToXML(inDevice)
 *
 * Purpose: Adds a devices info to the HID_device_usage_strings.plist(XML) file
 *
 * Inputs: inDevice		- the device
 * Returns: Boolean		- if successful
 */
static Boolean HIDAddDeviceToXML(IOHIDDeviceRef inIOHIDDeviceRef) {
	Boolean result = false;
	if (HIDIsValidDevice(inIOHIDDeviceRef)) {
		CFStringRef vendorCFStringRef = IOHIDDevice_GetManufacturer(inIOHIDDeviceRef);
		CFStringRef productCFStringRef = IOHIDDevice_GetProduct(inIOHIDDeviceRef);
		if (vendorCFStringRef && productCFStringRef) {
#if false       // don't update the cookie xml file
			gCookieCFPropertyListRef =
            hu_CreateFromXMLResource(CFSTR("HID_cookie_strings"), CFSTR("plist"));
			if (gCookieCFPropertyListRef) {
				CFMutableDictionaryRef tCFMutableDictionaryRef =
                CFDictionaryCreateMutableCopy(
                                              kCFAllocatorDefault,
                                              0,
                                              gCookieCFPropertyListRef);
				if (tCFMutableDictionaryRef) {
					if (hu_AddVendorProductToCFDict(tCFMutableDictionaryRef, vendorID, vendorCFStringRef, productID,
					                                productCFStringRef))
					{
						hu_XMLSave(tCFMutableDictionaryRef, CFSTR("HID_cookie_strings"), CFSTR("plist"));
						result = true;
					}
                    
					CFRelease(tCFMutableDictionaryRef);
				}
			}
            
#endif      // if 0
			if (gUsageCFPropertyListRef) {
                CFRelease(gUsageCFPropertyListRef);
			}
            
			gUsageCFPropertyListRef =
            hu_CreateFromXMLResource(CFSTR("HID_device_usage_strings"), CFSTR("plist"));
			if (gUsageCFPropertyListRef) {
				CFMutableDictionaryRef tCFMutableDictionaryRef =
                CFDictionaryCreateMutableCopy(
                                              kCFAllocatorDefault,
                                              0,
                                              gUsageCFPropertyListRef);
				if (tCFMutableDictionaryRef) {
					uint32_t vendorID = IOHIDDevice_GetVendorID(inIOHIDDeviceRef);
					uint32_t productID = IOHIDDevice_GetProductID(inIOHIDDeviceRef);
					if (hu_AddVendorProductToCFDict(tCFMutableDictionaryRef, vendorID, vendorCFStringRef, productID,
					                                productCFStringRef))
					{
						hu_XMLSave(tCFMutableDictionaryRef, CFSTR("HID_device_usage_strings"), CFSTR("plist"));
						result = true;
					}
                    
					CFRelease(tCFMutableDictionaryRef);
				}
			}
		}
	}
    
	return (result);
}   // HIDAddDeviceToXML

/*************************************************************************
 *
 * HIDAddDeviceElementToXML(inDevice, inElement)
 *
 * Purpose: Adds a devices info to the HID_device_usage_strings.plist(XML) file
 *
 * Inputs: inDevice		- the device
 *			inElement	- the element
 *
 * Returns: Boolean		- if successful
 */
Boolean HIDAddDeviceElementToXML(IOHIDDeviceRef inIOHIDDeviceRef, IOHIDElementRef inIOHIDElementRef) {
	Boolean result = false;
	if (HIDIsValidElement(inIOHIDElementRef)) {
		if (HIDAddDeviceToXML(inIOHIDDeviceRef)) {
			result = true;
		}
		if (hu_AddDeviceElementToUsageXML(inIOHIDDeviceRef, inIOHIDElementRef)) {
			result = true;
		}
	}
    
	return (result);
}   // HIDAddDeviceElementToXML
#endif // if 0
/*************************************************************************
 *
 * HIDGetTypeName(inIOHIDElementType, outCStrName)
 *
 * Purpose: return a C string for a given element type(see IOHIDKeys.h)
 * Notes:	returns "Unknown Type" for invalid types
 *
 * Inputs: inIOHIDElementType	- type element type
 *			outCStrName			- address where to store element type string
 *
 * Returns: outCStrName			- the element type string
 */

void HIDGetTypeName(IOHIDElementType inIOHIDElementType, char *outCStrName) {
	switch (inIOHIDElementType) {
		case kIOHIDElementTypeInput_Misc:
		{
			sprintf(outCStrName, "Miscellaneous Input");
			break;
		}
            
		case kIOHIDElementTypeInput_Button:
		{
			sprintf(outCStrName, "Button Input");
			break;
		}
            
		case kIOHIDElementTypeInput_Axis:
		{
			sprintf(outCStrName, "Axis Input");
			break;
		}
            
		case kIOHIDElementTypeInput_ScanCodes:
		{
			sprintf(outCStrName, "Scan Code Input");
			break;
		}
            
		case kIOHIDElementTypeOutput:
		{
			sprintf(outCStrName, "Output");
			break;
		}
            
		case kIOHIDElementTypeFeature:
		{
			sprintf(outCStrName, "Feature");
			break;
		}
            
		case kIOHIDElementTypeCollection:
		{
			sprintf(outCStrName, "Collection");
			break;
		}
            
		default:
		{
			sprintf(outCStrName, "Unknown Type");
			break;
		}
	} // switch
    
}   // HIDGetTypeName

// *************************************************************************
//
// HIDCopyUsagePageName(inUsagePage)
//
// Purpose:	return a CFStringRef string for a given usage page (see IOUSBHIDParser.h)
//
// Notes:	returns usage page in CFString form for unknown values
//
// Inputs:	inUsagePage	- the usage page
//
// Returns:	CFStringRef	- the resultant string
//

CFStringRef HIDCopyUsagePageName(uint32_t inUsagePage) {
	static CFPropertyListRef tCFPropertyListRef = NULL;
	CFStringRef result = NULL;
	if (!tCFPropertyListRef) {
		tCFPropertyListRef =
        hu_CreateFromXMLResource(CFSTR("HID_usage_strings"), CFSTR("plist"));
	}
	if (tCFPropertyListRef) {
		if (
		    CFDictionaryGetTypeID() == CFGetTypeID(tCFPropertyListRef))
		{
			CFStringRef pageKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("0x%4.4X"), inUsagePage);
			if (pageKeyCFStringRef) {
				CFDictionaryRef pageCFDictionaryRef;
				if (CFDictionaryGetValueIfPresent((CFDictionaryRef)tCFPropertyListRef, pageKeyCFStringRef,
				                                  (const void **) &pageCFDictionaryRef))
				{
					CFStringRef pageCFStringRef;
					if (CFDictionaryGetValueIfPresent(pageCFDictionaryRef,
					                                  kNameKeyCFStringRef,
					                                  (const void **) &pageCFStringRef))
					{
						result = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@"), pageCFStringRef);
					}
                    
#if FAKE_MISSING_NAMES
					else {
						// no name data for this page key (so we use the key)
						result = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("#%@"), pageKeyCFStringRef);
					}
				} else {
					// no name data for this page key (so we use the key)
					result = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("#%@"), pageKeyCFStringRef);
#endif              // if FAKE_MISSING_NAMES
				}
                
				CFRelease(pageKeyCFStringRef);
			}
		}
        
		// CFRelease(tCFPropertyListRef);	// Leak this!
		// tCFPropertyListRef = NULL;
	}
    
	return (result);
}   // HIDCopyUsagePageName

// *************************************************************************
//
// HIDCopyUsageName(inUsagePage, inUsage)
//
// Purpose:	return a CFStringRef string for a given usage page & usage(see IOUSBHIDParser.h)
//
// Notes:	returns usage page and usage values in CFString form for unknown values
//
// Inputs:	inUsagePage	- the usage page
// inUsage		- the usage
//
// Returns:	CFStringRef	- the resultant string
//

CFStringRef HIDCopyUsageName(uint32_t inUsagePage, uint32_t inUsage) {
	static CFPropertyListRef tCFPropertyListRef = NULL;
	CFStringRef result = NULL;
	if (!tCFPropertyListRef) {
		tCFPropertyListRef =
        hu_CreateFromXMLResource(CFSTR("HID_usage_strings"), CFSTR("plist"));
	}
	if (tCFPropertyListRef) {
		if (CFDictionaryGetTypeID() == CFGetTypeID(tCFPropertyListRef)) {
			CFStringRef pageKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("0x%4.4X"), inUsagePage);
			if (pageKeyCFStringRef) {
				CFDictionaryRef pageCFDictionaryRef;
				if (CFDictionaryGetValueIfPresent((CFDictionaryRef)tCFPropertyListRef, pageKeyCFStringRef,
				                                  (const void **) &pageCFDictionaryRef))
				{
					CFStringRef pageCFStringRef;
					if (CFDictionaryGetValueIfPresent(pageCFDictionaryRef, kNameKeyCFStringRef,
					                                  (const void **) &pageCFStringRef))
					{
						CFStringRef usageKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("0x%4.4X"), inUsage);
						if (usageKeyCFStringRef) {
							CFStringRef usageCFStringRef;
							if (CFDictionaryGetValueIfPresent(pageCFDictionaryRef, usageKeyCFStringRef,
							                                  (const void **) &usageCFStringRef))
							{
								result = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ %@"), pageCFStringRef, usageCFStringRef);
							}
                            
#if FAKE_MISSING_NAMES
							else {
								result = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ #%@"), pageCFStringRef, usageKeyCFStringRef);
							}
#endif                      // if FAKE_MISSING_NAMES
							CFRelease(usageKeyCFStringRef);
						}
					} else {
						// no name data for this page
					}
				} else {
					// no data for this page
				}
                CFRelease(pageKeyCFStringRef);
			}
		}
		// CFRelease(tCFPropertyListRef);	// Leak this!
		// tCFPropertyListRef = NULL;
	}
    
	return (result);
}   // HIDCopyUsageName

// *****************************************************
#pragma mark - local (static) function implementations
// *****************************************************
#if false	// currently unused
/*************************************************************************
 *
 * hu_SaveToXMLFile(inCFPRef, inCFURLRef)
 *
 * Purpose: save a property list into an XML file
 *
 * Inputs: inCFPRef		- the data
 *			inCFURLRef	- URL for the file
 *
 * Returns: SInt32		- error code (if any)
 */
static SInt32 hu_SaveToXMLFile(CFPropertyListRef inCFPRef, CFURLRef inCFURLRef) {
	CFDataRef xmlCFDataRef;
	SInt32 error = coreFoundationUnknownErr;
    
	// Convert the property list into XML data.
	xmlCFDataRef = CFPropertyListCreateXMLData(kCFAllocatorDefault, inCFPRef);
	if (xmlCFDataRef) {
		// Write the XML data to the file.
		(void) CFURLWriteDataAndPropertiesToResource(inCFURLRef, xmlCFDataRef, NULL, &error);
        
		// Release the XML data
        CFRelease(xmlCFDataRef);
	}
    
	return (error);
}   // hu_SaveToXMLFile
#endif // if 0
/*************************************************************************
 *
 * hu_CreateFromXMLURL(inCFURLRef)
 *
 * Purpose: load a property list from an XML file
 *
 * Inputs: inCFURLRef			- URL for the file
 *
 * Returns: CFPropertyListRef - the data
 */
static CFPropertyListRef hu_CreateFromXMLURL(CFURLRef inCFURLRef) {
	CFPropertyListRef myCFPropertyListRef = NULL;
    
    CFReadStreamRef readStreamRef = CFReadStreamCreateWithFile(kCFAllocatorDefault, inCFURLRef);
    if (readStreamRef) {
        if (CFReadStreamOpen(readStreamRef)) {
            CFErrorRef cfError;
            myCFPropertyListRef = CFPropertyListCreateWithStream(kCFAllocatorDefault,
                                                                 readStreamRef,
                                                                 0,
                                                                 kCFPropertyListImmutable,
                                                                 NULL,
                                                                 &cfError);
            CFReadStreamClose(readStreamRef);
        }
        CFRelease(readStreamRef);
    }
	return (myCFPropertyListRef);
}       // hu_CreateFromXMLURL

#if false	// currently unused
/*************************************************************************
 *
 * hu_XMLSave(inCFPropertyListRef, inResourceName, inResourceExtension)
 *
 * Purpose: save a CFPropertyListRef into a resource(XML) file
 *
 * Inputs: inCFPropertyListRef - the data
 *			inResourceName		- name of the resource file
 *			inResourceExtension - extension of the resource file
 *
 * Returns: SInt32				- error code (if any)
 */
static SInt32 hu_XMLSave(CFPropertyListRef inCFPropertyListRef, CFStringRef inResourceName, CFStringRef inResourceExtension) {
	CFURLRef resFileCFURLRef;
	SInt32 error = -1;
    
	// check the main (application) bundle
	resFileCFURLRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), inResourceName, inResourceExtension, NULL);
	if (!resFileCFURLRef) {
		// check this specific (HID_Utilities framework) bundle
		CFBundleRef tCFBundleRef = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.HID_Utilities"));
		if (tCFBundleRef) {
			resFileCFURLRef = CFBundleCopyResourceURL(tCFBundleRef, inResourceName, inResourceExtension, NULL);
		}
	}
	if (!resFileCFURLRef) {
		// check bundles already loaded or otherwise known to the current process
		CFArrayRef tCFArrayRef = CFBundleGetAllBundles();
		CFIndex idx, cnt = CFArrayGetCount(tCFArrayRef);
		for (idx = 0; idx < cnt; idx++) {
			CFBundleRef tCFBundleRef = (CFBundleRef) CFArrayGetValueAtIndex(tCFArrayRef, idx);
			if (tCFBundleRef) {
				resFileCFURLRef = CFBundleCopyResourceURL(tCFBundleRef, inResourceName, inResourceExtension, NULL);
				if (resFileCFURLRef) {
					break;
				}
			}
		}
	}
	if (resFileCFURLRef) {
		error = hu_SaveToXMLFile(inCFPropertyListRef, resFileCFURLRef);
        CFRelease(resFileCFURLRef);
	}
    
	return (error);
}   // hu_XMLSave
#endif // if 0
/*************************************************************************
 *
 * hu_CreateFromXMLResource(inResourceName, inResourceExtension)
 *
 * Purpose: Load a resource(XML) file into a CFPropertyListRef
 *
 * Inputs: inResourceName		- name of the resource file
 *			inResourceExtension - extension of the resource file
 *
 * Returns: CFPropertyListRef - the data
 */
static CFPropertyListRef hu_CreateFromXMLResource(CFStringRef inResourceName, CFStringRef inResourceExtension) {
	CFURLRef resFileCFURLRef;
	CFPropertyListRef tCFPropertyListRef = NULL;
    
	// check the main (application) bundle
	resFileCFURLRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), inResourceName, inResourceExtension, NULL);
	if (!resFileCFURLRef) {
		// check this specific (HID_Utilities framework) bundle
		CFBundleRef tCFBundleRef = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.HID_Utilities"));
		if (tCFBundleRef) {
			resFileCFURLRef = CFBundleCopyResourceURL(tCFBundleRef, inResourceName, inResourceExtension, NULL);
		}
	}
	if (!resFileCFURLRef) {
		// check bundles already loaded or otherwise known to the current process
		CFArrayRef tCFArrayRef = CFBundleGetAllBundles();
		CFIndex idx, cnt = CFArrayGetCount(tCFArrayRef);
		for (idx = 0; idx < cnt; idx++) {
			CFBundleRef tCFBundleRef = (CFBundleRef) CFArrayGetValueAtIndex(tCFArrayRef, idx);
			if (tCFBundleRef) {
				resFileCFURLRef = CFBundleCopyResourceURL(tCFBundleRef, inResourceName, inResourceExtension, NULL);
				if (resFileCFURLRef) {
					break;
				}
			}
		}
	}
	if (resFileCFURLRef) {
		tCFPropertyListRef = hu_CreateFromXMLURL(resFileCFURLRef);
        CFRelease(resFileCFURLRef);
	}
    
	return (tCFPropertyListRef);
}   // hu_CreateFromXMLResource

#if false	// currently unused
/*************************************************************************
 *
 * hu_AddVendorProductToCFDict(inCFMutableDictionaryRef, inVendorID, inVendorCFStringRef, inProductID, inProductCFStringRef)
 *
 * Purpose: add a vendor & product to a dictionary
 *
 * Inputs: inCFMutableDictionaryRef - the dictionary
 *			inVendorID				- the elements vendor ID
 *			inProductID				- the elements product ID
 *			inProductCFStringRef	- the string to be added
 *
 * Returns: Boolean		- if successful
 */
static Boolean hu_AddVendorProductToCFDict(CFMutableDictionaryRef	inCFMutableDictionaryRef,
                                           uint32_t					inVendorID,
                                           CFStringRef				inVendorCFStringRef,
                                           uint32_t					inProductID,
                                           CFStringRef				inProductCFStringRef) {
	Boolean result = false;
	if (inCFMutableDictionaryRef && (CFDictionaryGetTypeID() == CFGetTypeID(inCFMutableDictionaryRef))) {
		CFMutableDictionaryRef vendorCFMutableDictionaryRef;
		CFStringRef vendorKeyCFStringRef;
        
		CFMutableDictionaryRef productCFMutableDictionaryRef;
		CFStringRef productKeyCFStringRef;
        
		// if the vendor dictionary doesn't exist
		vendorKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld"), inVendorID);
		if (CFDictionaryGetValueIfPresent(inCFMutableDictionaryRef, vendorKeyCFStringRef,
		                                  (const void **) &vendorCFMutableDictionaryRef))
		{
			// copy it.
			vendorCFMutableDictionaryRef = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, vendorCFMutableDictionaryRef);
		} else {    // ...otherwise...
            // create it.
			vendorCFMutableDictionaryRef = CFDictionaryCreateMutable(kCFAllocatorDefault,
			                                                         0,
			                                                         &kCFTypeDictionaryKeyCallBacks,
			                                                         &kCFTypeDictionaryValueCallBacks);
			result = true;
		}
		// if the vendor name key doesn't exist
		if (!CFDictionaryContainsKey(vendorCFMutableDictionaryRef, kNameKeyCFStringRef)) {
			// create it.
			CFDictionaryAddValue(vendorCFMutableDictionaryRef, kNameKeyCFStringRef, inVendorCFStringRef);
			result = true;
		}
        
		// if the product key exists in the vendor dictionary
		productKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld"), inProductID);
		if (CFDictionaryGetValueIfPresent(vendorCFMutableDictionaryRef, productKeyCFStringRef,
		                                  (const void **) &productCFMutableDictionaryRef))
		{
			// copy it.
			productCFMutableDictionaryRef = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, productCFMutableDictionaryRef);
		} else {    // ...otherwise...
            // create it.
			productCFMutableDictionaryRef = CFDictionaryCreateMutable(kCFAllocatorDefault,
			                                                          0,
			                                                          &kCFTypeDictionaryKeyCallBacks,
			                                                          &kCFTypeDictionaryValueCallBacks);
			result = true;
		}
		// if the product name key doesn't exist
		if (!CFDictionaryContainsKey(productCFMutableDictionaryRef, kNameKeyCFStringRef)) {
			// create it.
			CFDictionaryAddValue(productCFMutableDictionaryRef, kNameKeyCFStringRef, inProductCFStringRef);
			result = true;
		}
		if (vendorCFMutableDictionaryRef) {
			if (productCFMutableDictionaryRef) {
				if (result) {
					CFDictionarySetValue(vendorCFMutableDictionaryRef, productKeyCFStringRef, productCFMutableDictionaryRef);
				}
                
				CFRelease(productCFMutableDictionaryRef);
			}
			if (result) {
				CFDictionarySetValue(inCFMutableDictionaryRef, vendorKeyCFStringRef, vendorCFMutableDictionaryRef);
			}
            
			CFRelease(	vendorCFMutableDictionaryRef);
		}
		if (productKeyCFStringRef) {
			CFRelease(	productKeyCFStringRef);
		}
		if (vendorKeyCFStringRef) {
			CFRelease(	vendorKeyCFStringRef);
		}
	}
    
	return (result);
}   // hu_AddVendorProductToCFDict

/*************************************************************************
 *
 * hu_AddDeviceElementToUsageXML(inDevice, inElement)
 *
 * Purpose: add a device and it's elements to our usage(XML) file
 *
 * Inputs: inDevice		- the device
 *			inElement	- the element
 *
 * Returns: Boolean		- if successful
 */
static Boolean hu_AddDeviceElementToUsageXML(IOHIDDeviceRef inIOHIDDeviceRef, IOHIDElementRef inIOHIDElementRef) {
	Boolean result = false;
	if (gUsageCFPropertyListRef) {
        CFRelease(gUsageCFPropertyListRef);
	}
    
	gUsageCFPropertyListRef =
    hu_CreateFromXMLResource(CFSTR("HID_device_usage_strings"), CFSTR("plist"));
	if (gUsageCFPropertyListRef) {
		CFMutableDictionaryRef tCFMutableDictionaryRef =
        CFDictionaryCreateMutableCopy(
                                      kCFAllocatorDefault,
                                      0,
                                      gUsageCFPropertyListRef);
		if (tCFMutableDictionaryRef) {
			CFMutableDictionaryRef vendorCFMutableDictionaryRef;
            
			CFMutableDictionaryRef productCFMutableDictionaryRef;
			CFStringRef productKeyCFStringRef;
            
			CFStringRef usageKeyCFStringRef;
            
			// if the vendor dictionary exists...
			uint32_t vendorID = IOHIDDevice_GetVendorID(inIOHIDDeviceRef);
			CFStringRef vendorKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld"), vendorID);
			if (vendorKeyCFStringRef) {
				if (CFDictionaryGetValueIfPresent(tCFMutableDictionaryRef, vendorKeyCFStringRef,
				                                  (const void **) &vendorCFMutableDictionaryRef))
				{
					// ...copy it...
					vendorCFMutableDictionaryRef = CFDictionaryCreateMutableCopy(kCFAllocatorDefault,
					                                                             0,
					                                                             vendorCFMutableDictionaryRef);
				} else {        // ...otherwise...
                    // ...create it.
					vendorCFMutableDictionaryRef = CFDictionaryCreateMutable(kCFAllocatorDefault,
					                                                         0,
					                                                         &kCFTypeDictionaryKeyCallBacks,
					                                                         &kCFTypeDictionaryValueCallBacks);
					result = true;
				}
				// if the vendor name key doesn't exist...
				if (!CFDictionaryContainsKey(vendorCFMutableDictionaryRef, kNameKeyCFStringRef)) {
					CFStringRef manCFStringRef = IOHIDDevice_GetManufacturer(inIOHIDDeviceRef);
					// ...create it.
                    CFDictionaryAddValue(vendorCFMutableDictionaryRef, kNameKeyCFStringRef, manCFStringRef);
					result = true;
				}
                
				// if the product key exists in the vendor dictionary...
				uint32_t productID = IOHIDDevice_GetProductID(inIOHIDDeviceRef);
				productKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld"), productID);
				if (CFDictionaryGetValueIfPresent(vendorCFMutableDictionaryRef, productKeyCFStringRef,
				                                  (const void **) &productCFMutableDictionaryRef))
				{
					// ...copy it...
					productCFMutableDictionaryRef = CFDictionaryCreateMutableCopy(kCFAllocatorDefault,
					                                                              0,
					                                                              productCFMutableDictionaryRef);
				} else {        // ...otherwise...
                    // ...create it.
					productCFMutableDictionaryRef = CFDictionaryCreateMutable(kCFAllocatorDefault,
					                                                          0,
					                                                          &kCFTypeDictionaryKeyCallBacks,
					                                                          &kCFTypeDictionaryValueCallBacks);
					result = true;
				}
				// if the product name key doesn't exist...
				if (!CFDictionaryContainsKey(productCFMutableDictionaryRef, kNameKeyCFStringRef)) {
					CFStringRef productCFStringRef = IOHIDDevice_GetProduct(inIOHIDDeviceRef);
					// ...create it.
                    CFDictionaryAddValue(productCFMutableDictionaryRef, kNameKeyCFStringRef, productCFStringRef);
					result = true;
				}
                
				// if the usage key doesn't exist in the product dictionary...
				uint32_t usagePage = IOHIDElementGetUsagePage(inIOHIDElementRef);
				uint32_t usage = IOHIDElementGetUsagePage(inIOHIDElementRef);
				usageKeyCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld:%ld"), usagePage, usage);
				if (usageKeyCFStringRef) {
					if (!CFDictionaryContainsKey(productCFMutableDictionaryRef, usageKeyCFStringRef)) {
						// find it's generic name
						CFStringRef usageCFStringRef = HIDCopyUsageName(usagePage, usage);
						if (usageCFStringRef) {
							// and add that.
							CFDictionaryAddValue(productCFMutableDictionaryRef, usageKeyCFStringRef, usageCFStringRef);
							result = true;
							CFRelease(usageCFStringRef);
						}
					}
                    
                    CFRelease(usageKeyCFStringRef);
				}
				if (vendorCFMutableDictionaryRef) {
					if (productCFMutableDictionaryRef) {
						if (result) {
							CFDictionarySetValue(vendorCFMutableDictionaryRef, productKeyCFStringRef, productCFMutableDictionaryRef);
						}
                        
						CFRelease(productCFMutableDictionaryRef);
					}
					if (result) {
						CFDictionarySetValue(tCFMutableDictionaryRef, vendorKeyCFStringRef, vendorCFMutableDictionaryRef);
					}
                    
					CFRelease(	vendorCFMutableDictionaryRef);
				}
                
                CFRelease(	vendorKeyCFStringRef);
			}
			if (productKeyCFStringRef) {
                CFRelease(	productKeyCFStringRef);
			}
			if (result) {
                hu_XMLSave(tCFMutableDictionaryRef, CFSTR("HID_device_usage_strings"), CFSTR("plist"));
			}
            
			CFRelease(tCFMutableDictionaryRef);
		}
	}
    
	return (result);
}   // hu_AddDeviceElementToUsageXML

#endif // if 0

#endif