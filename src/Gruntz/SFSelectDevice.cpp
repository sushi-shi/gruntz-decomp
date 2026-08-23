#include <rva.h>

#include <Gruntz/SFSelectDevice.h>

#include <Dsndmgr/SfManager.h>
#include <Gruntz/SoundFont.h>
#include <Gruntz/SoundFontPath.h>
#include <ProcAddr.h>

#include <stdio.h>
#include <string.h>

// The Creative SoundFont router-select MIDI SysEx template: F0, manufacturer
// 00 20 21 (Creative), 5F, four 7-bit payload bytes, F7. Retail keeps it
// initialized in .data at 0x213df8; SFManager_SelectBestDevice pokes
// SF_GetRouterID's value into the payload [7..10] as 7-bit chunks. Modeling
// the four payload bytes as separate globals is impossible: cl 4-aligns
// separate chars, while retail's are 1-aligned inside this nonzero block.
DATA(0x00213df8)
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

DATA(0x0024da80)
u16 g_sfDeviceIndex = 0;
DATA(0x0024da84)
DWORD g_sfCandidateSampleBytes = 0;
DATA(0x0024da88)
i32 g_sfManagerResult = 0;
DATA(0x0024da90)
char g_sfTraceBuffer[0x3c];
DATA(0x0024dacc)
CSFMIDILocation g_sfMidiLocation;
DATA(0x0024dad0)
CSFBufferObject g_sfBufferObject;
DATA(0x0024dae0)
char g_sfMusic4[0x100];
DATA(0x0024dbe0)
DWORD g_staticSampleBytes = 0;
DATA(0x0024dc28)
char g_sfLocal4[0x100];
DATA(0x0024dd28)
u16 g_sfDeviceId = 0;
DATA(0x0024dd30)
char g_sfMusic[0x100];
DATA(0x0024de30)
char g_sfLocal[0x100];
DATA(0x0024df30)
CSFCapsObject g_sfCaps;
DATA(0x0024df98)
u16 g_sfOpenAttemptsRemaining = 0;
DATA(0x0024df9c)
DWORD g_sfRouterId = 0;
DATA(0x0024dfa0)
char g_sfDir[0x100];
DATA(0x0024e0a0)
DWORD g_sfVer = 0;
DATA(0x0024e0a4)
u16 g_sfDeviceCount = 0;
DATA(0x0024e0a8)
HMODULE g_sfDll = NULL;
DATA(0x0024e0ac)
SfManagerFactory* g_sfManagerFactory = NULL;
DATA(0x0024e0b0)
SFMANL101API* g_sfDevice = NULL;
DATA(0x0024e0b8)
i32 g_sfReady = 0;
DATA(0x0024e0c0)
u8 g_sfDeviceRatings[344] = {0};

// @early-stop
RVA(0x000f8970, 0x3b4)
i32 SFManager_SelectBestDevice() {
    g_sfDll = LoadLibraryA("SFMAN32.DLL");
    if (g_sfDll == NULL) {
        return 0;
    }

    ProcAddr<SfManagerFactory*> mgrProc;
    mgrProc.m_raw = GetProcAddress(g_sfDll, "SFManager");
    SfManagerFactory* fn = mgrProc.m_fn;
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
    g_routerSysEx[10] = static_cast<unsigned char>(((v >> 0x18) & 0x7f));
    g_sfReady = 1;
    g_routerSysEx[8] = static_cast<unsigned char>(((v >> 8) & 0x7f));
    g_routerSysEx[9] = static_cast<unsigned char>(((v >> 0x10) & 0x7f));
    return 1;
}
