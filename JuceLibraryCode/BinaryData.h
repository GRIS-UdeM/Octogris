/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#ifndef BINARYDATA_H_1750437_INCLUDED
#define BINARYDATA_H_1750437_INCLUDED

namespace BinaryData
{
    extern const char*   logoGris_png;
    const int            logoGris_pngSize = 400483;

    extern const char*   libLeap_dylib;
    const int            libLeap_dylibSize = 2280096;

    extern const char*   HID_Config_Utilities_h;
    const int            HID_Config_Utilities_hSize = 338;

    extern const char*   HID_Error_Handler_h;
    const int            HID_Error_Handler_hSize = 326;

    extern const char*   HID_Name_Lookup_h;
    const int            HID_Name_Lookup_hSize = 318;

    extern const char*   HID_Queue_Utilities_h;
    const int            HID_Queue_Utilities_hSize = 334;

    extern const char*   HID_Utilities_h;
    const int            HID_Utilities_hSize = 310;

    extern const char*   HID_Utilities_External_h;
    const int            HID_Utilities_External_hSize = 21349;

    extern const char*   ImmHIDUtilAddOn_h;
    const int            ImmHIDUtilAddOn_hSize = 2911;

    extern const char*   IOHIDDevice__h;
    const int            IOHIDDevice__hSize = 15496;

    extern const char*   IOHIDElement__h;
    const int            IOHIDElement__hSize = 14041;

    extern const char*   IOHIDLib__h;
    const int            IOHIDLib__hSize = 3029;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Number of elements in the namedResourceList array.
    const int namedResourceListSize = 12;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();
}

#endif
