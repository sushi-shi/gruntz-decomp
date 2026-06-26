// CGruntzSoundZ.h - a positional/zoned sound bank manager in the Dsndmgr module
// (the 0x138xxx region, adjacent to the AIL/DirectSound subsystem).
//
// CGruntzSoundZ derives from MFC CMapStringToOb (the map sits at offset 0; its
// base ~CMapStringToOb is called directly from the scalar destructor, and the
// StopAll teardown iterates the map via GetNextAssoc with `this` = the map). It
// adds NO virtuals, so it reuses CMapStringToOb's vftable - no ??_7CGruntzSoundZ
// is emitted. Past the 0x1c-byte base it holds:
//   +0x1c  m_pCurrent  - the currently-playing inner sound (vtable-dispatched)
//   +0x20  m_digHandle - the AIL/digital driver handle (passed to inner->Play)
//   +0x24  m_mdiHandle - the AIL MIDI driver handle (mirrored to a .data global)
//   +0x28  m_enabled   - "sound active" flag (set by Init, gates every op)
//
// Field names are placeholders; offsets + emitted code bytes are load-bearing.
#ifndef GRUNTZ_DSNDMGR_CGRUNTZSOUNDZ_H
#define GRUNTZ_DSNDMGR_CGRUNTZSOUNDZ_H

#include <Ints.h>

#include <Mfc.h> // real MFC CMapStringToOb / CObject / CString / POSITION

// The inner per-bank sound object (allocated 0x60 bytes, vtable @ 0x5ef700). Only
// the load-bearing virtual slots are declared so the dispatches lower to the exact
// `mov eax,[obj]; call [eax+N]` __thiscall sequences. Declarations only - never
// defined here, so no ??_7 is emitted in this TU; the retail vtable is referenced
// only by address (reloc-masked) when an instance is created.
class CGruntzSoundInnerZ {
public:
    virtual void Slot00();                    // +0x00
    virtual i32 ScalarDtor(i32 flag);         // +0x04  scalar deleting dtor
    virtual void Slot08();                    // +0x08
    virtual void Slot0C();                    // +0x0c
    virtual void Slot10();                    // +0x10
    virtual i32 Init(i32 a1, i32 a2, i32 a3); // +0x14  one-time setup (3 args)
    virtual i32 Init2(i32 a1, i32 a2);        // +0x18  alternate setup (2 args)
    virtual void Slot1C();                    // +0x1c
    virtual void Slot20();                    // +0x20
    virtual i32 Play(i32 hDriver, i32 a2);    // +0x24
    virtual void Slot28();                    // +0x28
    virtual void Slot2C();                    // +0x2c
    virtual i32 Stop();                       // +0x30  stop / status query

    char m_pad0[0x48]; // through +0x44..+0x5c seeded by the create helpers
    i32 m_48;          // +0x48  per-bank flag the manager re-sync path reads
};

class CGruntzSoundZ : public CMapStringToOb {
public:
    ~CGruntzSoundZ();           // body at RVA 0x086040
    i32 Shutdown_1384f0();      // defined in the sibling AIL TU (RVA 0x1384f0)
    void StopAndFlush_138530(); // stop current + destroy every map entry
    CGruntzSoundInnerZ* CreateBank2_1385e0(i32 a1, i32 a2);
    CGruntzSoundInnerZ* CreateBank_138670(i32 a1, i32 a2, i32 a3);
    void Insert_138700(CGruntzSoundInnerZ* inner);
    CGruntzSoundInnerZ* Lookup_138730(const char* key);
    i32 PlayCreate2_138780(i32 a1, i32 a2, i32 a3);
    i32 PlayCreate3_1387e0(i32 a1, i32 a2, i32 a3, i32 a4);
    i32 Play_138840(i32 a1, i32 a2);
    void StopCurrent_1388a0();
    i32 Restart_1388c0(i32 a1);
    void StopAll_1388f0();
    i32 IsPlaying_138920();

    CGruntzSoundInnerZ* m_pCurrent; // +0x1c
    i32 m_digHandle;                // +0x20
    i32 m_mdiHandle;                // +0x24
    i32 m_enabled;                  // +0x28
};

#endif // GRUNTZ_DSNDMGR_CGRUNTZSOUNDZ_H
