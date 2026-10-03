#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/SFSelectDevice.h>

#include <Dsndmgr/SfManager.h>
#include <Gruntz/PathBuffer.h>
#include <Gruntz/SoundFont.h>
#include <Gruntz/SoundFontPath.h>

#include <stdio.h>
#include <string.h>

unsigned char g_routerSysEx[12] = {
    0xf0,
    0x00,
    0x20,
    0x21,
    0x5f,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xf7,
};

u16 g_sfDeviceIndex = 0;

DWORD g_sfCandidateSampleBytes = 0;

i32 g_sfManagerResult = 0;

char g_sfTraceBuffer[0x3c];

CSFMIDILocation g_sfMidiLocation;

CSFBufferObject g_sfBufferObject;

char g_sfMusic4[GRUNTZ_PATH_BUFFER_SIZE];

DWORD g_staticSampleBytes = 0;

char g_sfLocal4[GRUNTZ_PATH_BUFFER_SIZE];

u16 g_sfDeviceId = 0;

char g_sfMusic[GRUNTZ_PATH_BUFFER_SIZE];

char g_sfLocal[GRUNTZ_PATH_BUFFER_SIZE];

CSFCapsObject g_sfCaps;

u16 g_sfOpenAttemptsRemaining = 0;

DWORD g_sfRouterId = 0;

char g_sfDir[GRUNTZ_PATH_BUFFER_SIZE];

DWORD g_sfVer = 0;

u16 g_sfDeviceCount = 0;

HMODULE g_sfDll = NULL;

SfManagerFactory* g_sfManagerFactory = NULL;

SFMANL101API* g_sfDevice = NULL;

b32 g_sfReady = false;

u8 g_sfDeviceRatings[344] = {0};

i32 SFManager_SelectBestDevice() {
    g_sfDll = LoadLibraryA("SFMAN32.DLL");
    if (g_sfDll == NULL) {
        return 0;
    }

    SfManagerFactory* fn =
        reinterpret_cast<SfManagerFactory*>(GetProcAddress(g_sfDll, "SFManager"));
    g_sfManagerFactory = fn;
    if (fn == NULL) {
        FreeLibrary(g_sfDll);
        return 0;
    }
    g_sfManagerResult = (*fn)(0x10000, &g_sfDevice);
    if (g_sfManagerResult != 0) {
        FreeLibrary(g_sfDll);
        return 0;
    }

    g_sfDevice->SF_GetNumDevs(&g_sfDeviceCount);
    if (g_sfDeviceCount == 0) {
        return 0;
    }

    for (g_sfDeviceIndex = 0; g_sfDeviceIndex < g_sfDeviceCount; g_sfDeviceIndex++) {
        memset(&g_sfCaps, 0, sizeof(g_sfCaps));
        g_sfCaps.m_SizeOf = sizeof(g_sfCaps);
        g_sfDevice->SF_GetDevCaps(g_sfDeviceIndex, &g_sfCaps);
        sprintf(g_sfTraceBuffer, "Querying %s ", g_sfCaps.m_DevName);
        if (!(g_sfCaps.m_DevCaps & 0x40000000)) {
            if (!(g_sfCaps.m_DevCaps & 0x80000000)) {
                g_sfDevice->SF_Open(g_sfDeviceIndex);
                g_sfDevice->SF_QueryStaticSampleMemorySize(
                    g_sfDeviceIndex,
                    &g_staticSampleBytes,
                    &g_sfCandidateSampleBytes
                );
                u8 r = static_cast<u8>(((g_sfCandidateSampleBytes >> 0x13) + 0x40));
                g_sfDeviceRatings[g_sfDeviceIndex] = r;
                if (r == SF_DEVICE_RATING_UNUSABLE) {
                    g_sfDeviceRatings[g_sfDeviceIndex] = 0;
                }
                g_sfDevice->SF_Close(g_sfDeviceIndex);
            } else {
                g_sfDeviceRatings[g_sfDeviceIndex] = 0x80;
            }
        } else {
            g_sfDeviceRatings[g_sfDeviceIndex] = 0x20;
        }
    }

    g_sfOpenAttemptsRemaining = g_sfDeviceCount;
    if (g_sfDeviceCount > 0) {
        do {
            g_sfDeviceId = 0;
            sprintf(g_sfTraceBuffer, "Device 0's rating is %d", g_sfDeviceRatings[0] & 0xff);
            g_sfOpenAttemptsRemaining--;
            for (g_sfDeviceIndex = 1; g_sfDeviceIndex < g_sfDeviceCount; g_sfDeviceIndex++) {
                if (g_sfDeviceRatings[g_sfDeviceIndex] > g_sfDeviceRatings[g_sfDeviceId]) {
                    g_sfDeviceId = g_sfDeviceIndex;
                    sprintf(
                        g_sfTraceBuffer,
                        "Device %d's rating is %d",
                        g_sfDeviceIndex,
                        g_sfDeviceRatings[g_sfDeviceIndex] & 0xff
                    );
                }
            }
            sprintf(g_sfTraceBuffer, "Best Device number is %d", g_sfDeviceId);
            if (g_sfDevice->SF_Open(g_sfDeviceId) != 0) {
                g_sfDeviceRatings[g_sfDeviceId] = 0;
            } else {
                g_sfOpenAttemptsRemaining = 0;
            }
        } while (g_sfOpenAttemptsRemaining > 0);
    }

    if (g_sfDeviceRatings[g_sfDeviceId] == 0) {
        FreeLibrary(g_sfDll);
        return 0;
    }

    memset(&g_sfCaps, 0, sizeof(g_sfCaps));
    g_sfCaps.m_SizeOf = sizeof(g_sfCaps);
    g_sfDevice->SF_GetDevCaps(g_sfDeviceId, &g_sfCaps);
    if (!(g_sfCaps.m_DevCaps & 0x80000000)) {
        g_sfDevice->SF_QueryStaticSampleMemorySize(g_sfDeviceId, &g_staticSampleBytes, &g_sfVer);
    } else {
        g_sfVer = static_cast<DWORD>(-1);
    }
    g_sfDevice->SF_GetRouterID(g_sfDeviceId, &g_sfRouterId);
    DWORD v = g_sfRouterId;
    g_routerSysEx[7] = static_cast<unsigned char>((v & 0x7f));
    g_routerSysEx[8] = static_cast<unsigned char>(((v >> 8) & 0x7f));
    g_routerSysEx[9] = static_cast<unsigned char>(((v >> 0x10) & 0x7f));
    g_routerSysEx[10] = static_cast<unsigned char>(((v >> 0x18) & 0x7f));
    g_sfReady = true;
    return 1;
}
