// SFSelectDevice.cpp - SFManager::SelectBestDevice (RVA 0xf8970), the music
// (SFMAN32.DLL soundfont) device picker. Sibling of BuildSoundFontPath /
// SfDeviceInitKeys on the same *0x64e0b0 receiver. It dynamically loads SFMAN32.DLL,
// resolves the exported "SFManager" factory (a pointer-to-function-pointer data
// export), instantiates the device interface, queries every device's caps +
// rating, traces each via sprintf, selects the highest-rated installable device
// (retrying with that device disabled if Select fails), reads back its caps/id,
// unpacks the 4 low-7-bit id bytes, and flags init done. All state lives in module
// statics; frameless (FPO). Only offsets / code bytes are load-bearing; the DLL
// imports come via <Win32.h>, sprintf via <stdio.h>, the interface slots are
// reloc-masked __cdecl callees.

#include <rva.h>

#include <Win32.h>  // LoadLibraryA / GetProcAddress / FreeLibrary
#include <stdio.h>  // sprintf (0x11f890)
#include <string.h> // memset (rep stos intrinsic)

#include <Gruntz/SfManagerDevice.h> // the SFMAN32 device interface (*0x64e0b0)
#include <Globals.h>

typedef i32(__cdecl* SFFactory)(i32 flags, SfManagerDevice** out);

extern WORD g_sfDeviceId; // 0x64dd28 (best device index)

DATA(0x0024e0ac)
extern SFFactory* g_factory_64e0ac; // the "SFManager" data export (ptr-to-fnptr)

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
    SFFactory* fn = (SFFactory*)GetProcAddress(g_sfDll, "SFManager");
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

    g_sfDevice->GetCount(&g_sfDeviceCount);
    if (g_sfDeviceCount == 0) {
        return 0;
    }

    for (g_idx_64da80 = 0; g_idx_64da80 < g_sfDeviceCount; g_idx_64da80++) {
        memset(&g_caps_64df30, 0, 0x66);
        g_caps_64df30 = 0x66;
        g_sfDevice->QueryCaps(g_idx_64da80, &g_caps_64df30);
        sprintf(g_traceBuf_64da90, "Querying %s", &g_capsName_64df46);
        if (g_capsFlags_64df36 & 0x40000000) {
            g_ratings_64e0c0[g_idx_64da80] = 0x20;
        } else if (g_capsFlags_64df36 & 0x80000000) {
            g_ratings_64e0c0[g_idx_64da80] = 0x80;
        } else {
            g_sfDevice->Select(g_idx_64da80);
            g_sfDevice->GetRating(g_idx_64da80, &g_ratingBuf_64dbe0, (i32*)&g_ratingRaw_64da84);
            u8 r = (u8)((g_ratingRaw_64da84 >> 0x13) + 0x40);
            g_ratings_64e0c0[g_idx_64da80] = r;
            if (r == 0x40) {
                g_ratings_64e0c0[g_idx_64da80] = 0;
            }
            g_sfDevice->Deselect(g_idx_64da80);
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
            if (g_sfDevice->Select(g_sfDeviceId) != 0) {
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
    g_sfDevice->QueryCaps(g_sfDeviceId, &g_caps_64df30);
    if (g_capsFlags_64df36 & 0x80000000) {
        g_sfVer = (u32)-1;
    } else {
        g_sfDevice->GetRating(g_sfDeviceId, &g_ratingBuf_64dbe0, (i32*)&g_sfVer);
    }
    g_sfDevice->GetId(g_sfDeviceId, (i32*)&g_id_64df9c);
    u32 v = g_id_64df9c;
    g_id0_613dff = (char)(v & 0x7f);
    g_id3_613e02 = (char)((v >> 0x18) & 0x7f);
    g_sfReady = (void*)1;
    g_id1_613e00 = (char)((v >> 8) & 0x7f);
    g_id2_613e01 = (char)((v >> 0x10) & 0x7f);
    return 1;
}
