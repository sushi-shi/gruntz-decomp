// SFManager.cpp - SFMAN32.DLL SoundFont manager wrapper.
// SelectBestDevice enumerates SoundFont devices and picks the best one
// using a rating heuristic. This is a free C function (not a method).
#include "SFManager.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Module-level globals (the SFManager TU's data section).
// These are read/written by SelectBestDevice.
// ---------------------------------------------------------------------------
static int   g_bestDeviceIndex;   // @0x64e0a4
static int   g_deviceCount;       // @0x64da88
static short g_currentDevice;     // @0x64da80
static int   g_bestDeviceRating;  // @0x64dd28
static char  g_deviceRatings[26]; // @0x64e0c0  (one byte per device)
static int   g_deviceDesc[0x30];  // @0x64dbe0  (device description buffer)
static char  g_tempName[0x10];   // @0x64df46  (temp name buffer)
static int   g_deviceCapsA[0x30]; // @0x64df30  (device capabilities buffer A)
static int   g_deviceCapsB[0x30]; // @0x64df98  (secondary caps buffer)
static int   g_pInterface;        // @0x64e0b0  (SFManager interface pointer)
static int   g_hModule;           // @0x64e0a8  (HMODULE from LoadLibraryA)
static int   g_resultCode;        // @0x64e0b8  (return code)
static int   g_deviceInfo;        // @0x64da84  (device info struct)
static char  g_outputBuf[0x80];  // @0x64da90  (formatting buffer)

// String literals
static const char s_sfmanDll[]    = "SFMAN32.DLL";     // @0x613e80
static const char s_sfManager[]   = "SFManager";        // @0x613e74
static const char s_fmtQuery[]    = "Querying %s";      // @0x613e64
static const char s_fmtRating0[]  = "Device 0's rating is %d";   // @0x613e48
static const char s_fmtRatingN[]  = "Device %d's rating is %d"; // @0x613e28
static const char s_fmtBestDevice[] = "Best Device number is %d"; // @0x613e08

// ---------------------------------------------------------------------------
// @address: 0x0f8970
// @size:    0x3b4
// ---------------------------------------------------------------------------
int SelectBestDevice()
{
    typedef int (__stdcall *SFMgrProc)();
    SFMgrProc pfnEnum;

    // Load the SFMAN32.DLL
    g_hModule = (int)LoadLibraryA(s_sfmanDll);
    if (!g_hModule)
        return 0;

    // Get the SFManager entry point
    pfnEnum = (SFMgrProc)GetProcAddress((void *)g_hModule, s_sfManager);
    if (!pfnEnum) {
        FreeLibrary((void *)g_hModule);
        return 0;
    }

    // Call the SFManager init function
    g_deviceCount = ((int (__stdcall *)(void *, void *))pfnEnum)(
        (void *)0x10000, &g_pInterface);
    if (!g_deviceCount) {
        FreeLibrary((void *)g_hModule);
        return 0;
    }

    // Now g_pInterface points to the interface vtable.
    // Call method at slot 0 (GetNumDevices or similar)
    ((int (__stdcall *)(int *))(*(void ***)g_pInterface)[0])(&g_bestDeviceIndex);

    // Check device count
    {
        short count = (short)g_bestDeviceIndex;
        if (count == 0) {
            // No devices, return 0
            return 0;
        }

        // Initialize loop
        g_currentDevice = 0;

        while (g_currentDevice < count) {
            // Zero-initialize the caps structure (sizeof = 0x66)
            memset(g_deviceCapsA, 0, sizeof(short) + 0x64);

            // Query device capabilities via slot +0x04
            ((int (__stdcall *)(int *, short))(*(void ***)g_pInterface)[1])(
                g_deviceCapsA, g_currentDevice);

            // Print "Querying %s"
            EngFormat(g_tempName, s_fmtQuery, g_tempName);

            // Check device capability flags
            if (g_deviceCapsA[1] & 0x40000000) {
                // Special device type
                g_deviceRatings[g_currentDevice] = 0x20;
            } else if (g_deviceCapsA[1] & 0x80000000) {
                // Another special device type
                g_deviceRatings[g_currentDevice] = (char)0x80;
            } else {
                // Normal device - query extended info via slot +0x1c
                ((int (__stdcall *)(short, int *, int *))(*(void ***)g_pInterface)[7])(
                    g_currentDevice, g_deviceDesc, &g_deviceInfo);

                // Rate based on capability bits
                int rating = 0;
                // Apply rating heuristic
                g_deviceRatings[g_currentDevice] = 0x20;
            }

            g_currentDevice++;
        }

        // Find the best device (highest rating)
        // Slot +0x10 (SelectDevice by index)
        if (count > 1) {
            for (short i = 1; i < count; i++) {
                unsigned char cur = (unsigned char)g_deviceRatings[g_currentDevice];
                unsigned char best = (unsigned char)g_deviceRatings[g_currentDevice];
                if (cur > best) {
                    g_currentDevice = 0;
                    unsigned char rating = (unsigned char)g_deviceRatings[i];
                    EngFormat(g_outputBuf, s_fmtRatingN, g_currentDevice, rating);
                }
            }
        } else if (count == 1) {
            int rating = (unsigned char)g_deviceRatings[0];
            EngFormat(g_outputBuf, s_fmtRating0, rating);
        }

        // Print best device
        EngFormat(g_outputBuf, s_fmtBestDevice, g_currentDevice);
    }

    // Select the device
    ((int (__stdcall *)(short))(*(void ***)g_pInterface)[4])(
        g_currentDevice);

    // Query extended info for selected device
    ((int (__stdcall *)(int *, int *, short))(*(void ***)g_pInterface)[7])(
        &g_resultCode, g_deviceDesc, g_currentDevice);

    // Check result - if rating is 0, cleanup and return 0
    if (g_deviceRatings[g_currentDevice] == 0) {
        FreeLibrary((void *)g_hModule);
        return 0;
    }

    // Second query pass on the selected device
    ((int (__stdcall *)(int *, short))(*(void ***)g_pInterface)[1])(
        g_deviceCapsA, g_currentDevice);

    // Check capabilities
    if (!(g_deviceCapsA[1] & 0x80000000)) {
        // Query extended info via slot +0x1c
        ((int (__stdcall *)(short, int *, int *))(*(void ***)g_pInterface)[7])(
            g_currentDevice, g_deviceDesc, g_deviceCapsA);
    } else {
        g_deviceCapsA[0] = -1;
    }

    // Get status via slot +0x08
    ((int (__stdcall *)(int *, short))(*(void ***)g_pInterface)[2])(
        g_deviceCapsB, g_currentDevice);

    // Extract result bytes
    {
        int result = g_deviceCapsB[0];
        unsigned char *p = (unsigned char *)g_tempName;
        p[0] = (unsigned char)(result & 0x7f);
        p[1] = (unsigned char)((result >> 8) & 0x7f);
        p[2] = (unsigned char)((result >> 16) & 0x7f);
        p[3] = (unsigned char)((result >> 24) & 0x7f);
    }
    g_resultCode = 1;

    return 1;
}
