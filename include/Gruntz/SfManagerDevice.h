// SfManagerDevice.h - the SFMAN32.DLL soundfont device interface (the object at
// *0x64e0b0), a flat table of __cdecl function pointers the "SFManager" factory
// fills. The one canonical shape used by SFManager::SelectBestDevice (the device
// picker: the query/select/rating slots), BuildSoundFontPath (the +0x2c soundfont
// loader) and SfDeviceInitKeys (the +0x34 key-scan callback). All fields named; the
// intervening m_XX slots are interface entries Gruntz does not call.
#ifndef GRUNTZ_GRUNTZ_SFMANAGERDEVICE_H
#define GRUNTZ_GRUNTZ_SFMANAGERDEVICE_H

#include <rva.h>   // SIZE_UNKNOWN
#include <Win32.h> // WORD

// The +0x34 key-scan callback: (key code, scratch word) -> void.
typedef void(__cdecl* SfKeyScanFn)(WORD code, WORD* scratch);
// An interface slot Gruntz does not invoke (unknown __cdecl signature).
typedef void(__cdecl* SfSlotFn)();

struct SfManagerDevice {
    void(__cdecl* GetCount)(u16* out);                      // +0x00
    void(__cdecl* QueryCaps)(u16 idx, void* caps);          // +0x04
    void(__cdecl* GetId)(u16 idx, i32* out);                // +0x08
    SfSlotFn m_0c;                                          // +0x0c
    i32(__cdecl* Select)(u16 idx);                          // +0x10
    void(__cdecl* Deselect)(u16 idx);                       // +0x14
    SfSlotFn m_18;                                          // +0x18
    void(__cdecl* GetRating)(u16 idx, void* buf, i32* out); // +0x1c
    SfSlotFn m_20;                                          // +0x20
    SfSlotFn m_24;                                          // +0x24
    SfSlotFn m_28;                                          // +0x28
    // +0x2c soundfont loader: (device id token, config block A, config block B).
    // BuildSoundFontPath drives this to load the first existing Gruntz SF2.
    i32(__cdecl* Load)(u16 token, void* cfgA, void* cfgB); // +0x2c
    SfSlotFn m_30;                                         // +0x30
    SfKeyScanFn m_34;                                      // +0x34  key-scan callback slot
};
SIZE_UNKNOWN(SfManagerDevice); // SFMAN32.DLL-owned; only its low slots are modeled

#endif // GRUNTZ_GRUNTZ_SFMANAGERDEVICE_H
