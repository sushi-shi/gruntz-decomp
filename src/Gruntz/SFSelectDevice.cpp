#include <rva.h>
#include <Gruntz/SoundFont.h> // shared decls (CGruntzMgr::Run boot audio path)

#include <Win32.h>  // LoadLibraryA / GetProcAddress / FreeLibrary
#include <stdio.h>  // sprintf (0x11f890)
#include <string.h> // memset (rep stos intrinsic)

#include <Dsndmgr/SfManager.h> // real SFMANL101API device + the SFManager factory
#include <Globals.h>

extern WORD g_sfDeviceId; // 0x64dd28 (best device index; shared with soundfontpath)

extern "C" {
    DATA(0x00213dff)
    char g_id0_613dff = 0;
    DATA(0x00213e00)
    char g_id1_613e00 = 0;
    DATA(0x00213e01)
    char g_id2_613e01 = 0;
    DATA(0x00213e02)
    char g_id3_613e02 = 0;
}

DATA(0x0024da80)
u16 g_idx_64da80 = 0; // current device index
DATA(0x0024da84)
u32 g_ratingRaw_64da84 = 0;
DATA(0x0024da88)
i32 g_factoryRc_64da88 = 0;
DATA(0x0024df30)
u16 g_caps_64df30 = 0; // caps query buffer base / size field
DATA(0x0024df36)
u32 g_capsFlags_64df36 = 0; // caps flags (caps + 6)
DATA(0x0024df98)
u16 g_remaining_64df98 = 0;
DATA(0x0024df9c)
u32 g_id_64df9c = 0; // packed device id
DATA(0x0024e0a0)
u32 g_sfVer = 0; // build/version selector
DATA(0x0024e0a4)
u16 g_sfDeviceCount = 0; // SFMAN32 device count
DATA(0x0024e0a8)
void* g_sfDll = 0; // SFMAN32.DLL handle
DATA(0x0024e0b0)
SFMANL101API* g_sfDevice = 0; // SFMAN32 device interface
DATA(0x0024e0b8)
void* g_sfReady = 0; // device-selected flag (1 = ready)
DATA(0x0024e0ac)
SfManagerFactory* g_factory_64e0ac = 0; // the "SFManager" data export (ptr-to-fnptr)
DATA(0x0024e0c0)
u8 g_ratings_64e0c0[344] = {0}; // per-device rating bytes

// @early-stop
// MSVC5 u16-global codegen wall (~74%): the full control flow - the SFMAN32.DLL
// load/factory/instantiate chain, the per-device caps+rating query loop, the three
// sprintf traces, the highest-rating selection with Select-fail retry, and the
// final caps/id readback + 4x &0x7f id-byte unpack - is reconstructed and matches
// retail's logic/calls/strings (frame is frameless/FPO, no stack slot, like
// retail). Residual is the pervasive 16-bit codegen shape: retail reads the WORD
// globals used as array indices via dword-load + `and 0xffff` while this /O2
// recompile uses a word-load + `xor;mov cx,dx` zero-extend; retail hoists the
// 0x20 rating constant into bl and places the two flag-handler stores out-of-line
// (`test;jne`) while ours inlines them. Not source-steerable (the word-global
// load width + small-block placement are the allocator's). Logic complete.
RVA(0x000f8970, 0x3b4)
i32 SFManager_SelectBestDevice() {
    g_sfDll = LoadLibraryA("SFMAN32.DLL");
    if (g_sfDll == 0) {
        return 0;
    }
    SfManagerFactory* fn = reinterpret_cast<SfManagerFactory*>(GetProcAddress(g_sfDll, "SFManager"));
    g_factory_64e0ac = fn;
    if (fn == 0) {
        FreeLibrary(g_sfDll);
        return 0;
    }
    g_factoryRc_64da88 = (*fn)(0x10000, &g_sfDevice);
    if (g_factoryRc_64da88 != 0) {
        FreeLibrary(g_sfDll);
        return 0;
    }

    g_sfDevice->SF_GetNumDevs(&g_sfDeviceCount);
    if (g_sfDeviceCount == 0) {
        return 0;
    }

    for (g_idx_64da80 = 0; g_idx_64da80 < g_sfDeviceCount; g_idx_64da80++) {
        memset(&g_caps_64df30, 0, 0x66);
        g_caps_64df30 = 0x66;
        g_sfDevice->SF_GetDevCaps(g_idx_64da80, reinterpret_cast<PSFCAPSOBJECT>(&g_caps_64df30));
        sprintf(g_traceBuf_64da90, "Querying %s", &g_capsName_64df46);
        if (g_capsFlags_64df36 & 0x40000000) {
            g_ratings_64e0c0[g_idx_64da80] = 0x20;
        } else if (g_capsFlags_64df36 & 0x80000000) {
            g_ratings_64e0c0[g_idx_64da80] = 0x80;
        } else {
            g_sfDevice->SF_Open(g_idx_64da80);
            g_sfDevice->SF_QueryStaticSampleMemorySize(
                g_idx_64da80,
                reinterpret_cast<PDWORD>(&g_ratingBuf_64dbe0),
                reinterpret_cast<PDWORD>(&g_ratingRaw_64da84)
            );
            u8 r = static_cast<u8>(((g_ratingRaw_64da84 >> 0x13) + 0x40));
            g_ratings_64e0c0[g_idx_64da80] = r;
            if (r == 0x40) {
                g_ratings_64e0c0[g_idx_64da80] = 0;
            }
            g_sfDevice->SF_Close(g_idx_64da80);
        }
    }

    g_remaining_64df98 = g_sfDeviceCount;
    if (g_sfDeviceCount != 0) {
        do {
            g_sfDeviceId = 0;
            sprintf(g_traceBuf_64da90, "Device 0's rating is %d", g_ratings_64e0c0[0] & 0xff);
            g_remaining_64df98--;
            for (g_idx_64da80 = 1; g_idx_64da80 < g_sfDeviceCount; g_idx_64da80++) {
                if (g_ratings_64e0c0[g_idx_64da80] > g_ratings_64e0c0[g_sfDeviceId]) {
                    g_sfDeviceId = g_idx_64da80;
                    sprintf(
                        g_traceBuf_64da90,
                        "Device %d's rating is %d",
                        g_idx_64da80,
                        g_ratings_64e0c0[g_idx_64da80] & 0xff
                    );
                }
            }
            sprintf(g_traceBuf_64da90, "Best Device number is %d", g_sfDeviceId);
            if (g_sfDevice->SF_Open(g_sfDeviceId) != 0) {
                g_ratings_64e0c0[g_sfDeviceId] = 0;
            } else {
                g_remaining_64df98 = 0;
            }
        } while (g_remaining_64df98 != 0);
    }

    if (g_ratings_64e0c0[g_sfDeviceId] == 0) {
        FreeLibrary(g_sfDll);
        return 0;
    }

    memset(&g_caps_64df30, 0, 0x66);
    g_caps_64df30 = 0x66;
    g_sfDevice->SF_GetDevCaps(g_sfDeviceId, reinterpret_cast<PSFCAPSOBJECT>(&g_caps_64df30));
    if (g_capsFlags_64df36 & 0x80000000) {
        g_sfVer = static_cast<u32>(-1);
    } else {
        g_sfDevice->SF_QueryStaticSampleMemorySize(
            g_sfDeviceId,
            reinterpret_cast<PDWORD>(&g_ratingBuf_64dbe0),
            reinterpret_cast<PDWORD>(&g_sfVer)
        );
    }
    g_sfDevice->SF_GetRouterID(g_sfDeviceId, reinterpret_cast<PDWORD>(&g_id_64df9c));
    u32 v = g_id_64df9c;
    g_id0_613dff = static_cast<char>((v & 0x7f));
    g_id3_613e02 = static_cast<char>(((v >> 0x18) & 0x7f));
    g_sfReady = reinterpret_cast<void*>(1);
    g_id1_613e00 = static_cast<char>(((v >> 8) & 0x7f));
    g_id2_613e01 = static_cast<char>(((v >> 0x10) & 0x7f));
    return 1;
}
