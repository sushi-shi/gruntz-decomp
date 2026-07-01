// KeyRecv_f8ec0.h - the SFMAN32.DLL soundfont device interface (the object at
// *0x64e0b0). One shape shared by SFManager::SelectBestDevice (the device
// picker, which drives the +0x00..+0x1c __cdecl query/select slots the factory
// fills) and InitKeys_f8ec0 (which dispatches the +0x34 key-scan callback slot).
// Both are views of the same receiver; all slots are __cdecl function pointers.
#ifndef GRUNTZ_GRUNTZ_KEYRECV_F8EC0_H
#define GRUNTZ_GRUNTZ_KEYRECV_F8EC0_H

#include <Ints.h>
#include <Win32.h> // WORD

// The +0x34 key-scan callback: (key code, scratch word) -> void.
typedef void(__cdecl* Slot34Fn_f8ec0)(WORD code, WORD* scratch);

struct KeyRecv_f8ec0 {
    void(__cdecl* GetCount)(u16* out);             // +0x00
    void(__cdecl* QueryCaps)(u16 idx, void* caps); // +0x04
    void(__cdecl* GetId)(u16 idx, i32* out);       // +0x08
    char m_pad0c[0x10 - 0xc];
    i32(__cdecl* Select)(u16 idx);    // +0x10
    void(__cdecl* Deselect)(u16 idx); // +0x14
    char m_pad18[0x1c - 0x18];
    void(__cdecl* GetRating)(u16 idx, void* buf, i32* out); // +0x1c
    char m_pad20[0x34 - 0x20];
    Slot34Fn_f8ec0 m_34; // +0x34  key-scan callback slot
};

#endif // GRUNTZ_GRUNTZ_KEYRECV_F8EC0_H
