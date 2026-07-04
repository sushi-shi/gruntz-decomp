// vendor/sfman-1.01/sfman32.h - the Gruntz-side dispatch view of the SFMAN32.DLL
// SoundFont Master Manager device interface Gruntz reaches at runtime.
//
// The authoritative SDK header is the genuine `SFMAN.H` vendored alongside this file
// (Creative Technology / E-mu Systems, "SoundFont Master Manager", Revision 1.01).
// This companion header carries the Gruntz-side dispatch struct because the retail
// SFMAN32.DLL revision Gruntz linked differs from SFMAN.H's SFMANL101API in one
// detail: SFMAN.H declares SF_GetLoadedBankPathname (slot +0x34) with 3 args, but
// the retail SfDeviceInitKeys (RVA 0xf8ec0) invokes that slot with only 2 __cdecl
// args (`push loc; push idx; call [ecx+0x34]; add esp,8`), so SFMANL101API cannot be
// the byte-exact dispatch type. The slot NAMES + the confirmed slot layout come from
// SFMAN.H (verified via dump_target 0xf8970/0xf8f30/0xf8ec0):
//   +0x00 SF_GetNumDevs   +0x04 SF_GetDevCaps    +0x08 SF_GetRouterID
//   +0x10 SF_Open         +0x14 SF_Close         +0x1c SF_QueryStaticSampleMemorySize
//   +0x2c SF_LoadBank     +0x34 SF_GetLoadedBankPathname (2-arg retail under-call)
//
// SFMAN32.DLL is NOT statically imported - the engine LoadLibraryA("SFMAN32.DLL")s it
// (SFMAN.H's SF_MASTER_MANAGER_FILENAME) and GetProcAddress's the single exported
// "SFManager" DATA symbol (SFMAN.H's SF_FUNCTION_TABLE_NAME): a pointer to the device-
// factory function ptr. Calling that factory fills a device receiver (the global at
// *0x64e0b0), a flat __cdecl function-pointer table dispatched `mov eax,[recv];
// call [eax+slot]`. The 4x&0x7f id-byte unpack in SelectBestDevice is SFMAN.H's
// SFMAN_GET_ROUTING_INDEX macro.
//
// Self-contained: a vendored SDK header pulls no core-tree headers. SFDEVINDEX is a
// 16-bit `unsigned short` (WORD); the LRESULT return + id/rating scalars are 32-bit int.
#ifndef GRUNTZ_VENDOR_SFMAN32_H
#define GRUNTZ_VENDOR_SFMAN32_H

typedef unsigned short SFDEVINDEX; // SFMAN.H device index (WORD)

// An interface slot Gruntz does not invoke (see SFMAN.H's SFMANL101API for its real
// signature; unknown __cdecl arg shape from Gruntz's side).
typedef void(__cdecl* SfSlotFn)();

// The SFMAN32 device interface (the receiver at *0x64e0b0): the flat __cdecl function-
// pointer table the "SFManager" factory fills. Slot names from SFMAN.H's SFMANL101API;
// the intervening m_XX slots are entries Gruntz does not reach.
struct SfManagerDevice {
    void(__cdecl* SF_GetNumDevs)(unsigned short* out);        // +0x00  (PWORD)
    void(__cdecl* SF_GetDevCaps)(SFDEVINDEX idx, void* caps); // +0x04  (PSFCAPSOBJECT)
    void(__cdecl* SF_GetRouterID)(SFDEVINDEX idx, int* out);  // +0x08  (PDWORD)
    SfSlotFn m_0c;                                            // +0x0c  SF_IsDeviceFree
    int(__cdecl* SF_Open)(SFDEVINDEX idx);                    // +0x10
    void(__cdecl* SF_Close)(SFDEVINDEX idx);                  // +0x14
    SfSlotFn m_18;                                            // +0x18  SF_IsMIDIBankUsed
    // +0x1c query static sample-memory size (idx, PDWORD, PDWORD).
    void(__cdecl* SF_QueryStaticSampleMemorySize)(SFDEVINDEX idx, void* a, int* b); // +0x1c
    SfSlotFn m_20; // +0x20  SF_GetAllSynthEmulations
    SfSlotFn m_24; // +0x24  SF_GetSynthEmulation
    SfSlotFn m_28; // +0x28  SF_SelectSynthEmulation
    // +0x2c load a SoundFont bank (idx, PSFMIDILOCATION, PSFBUFFEROBJECT).
    // BuildSoundFontPath drives this to load the first existing Gruntz SF2.
    int(__cdecl* SF_LoadBank)(SFDEVINDEX idx, void* location, void* buffer); // +0x2c
    SfSlotFn m_30; // +0x30  SF_GetLoadedBankDescriptor
    // +0x34 SF_GetLoadedBankPathname; retail invokes it with 2 __cdecl args (the SFMAN.H
    // 1.01 prototype's 3rd PSFBUFFEROBJECT omitted), so it is modeled 2-arg to match the
    // call bytes. SfDeviceInitKeys sweeps this over MIDI bank indices 1..0x7f.
    void(__cdecl* SF_GetLoadedBankPathname)(SFDEVINDEX idx, unsigned short* location); // +0x34
};

// The DLL's single exported entry point: "SFManager" is a DATA export - a pointer to
// this factory function pointer. `(*factory)(flags, &recv)` instantiates the device
// interface into `recv`; returns 0 on success.
typedef int(__cdecl* SfManagerFactory)(int flags, SfManagerDevice** out);

#endif // GRUNTZ_VENDOR_SFMAN32_H
