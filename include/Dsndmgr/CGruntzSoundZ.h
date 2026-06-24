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

#include <Mfc.h> // real MFC CMapStringToOb / CObject / CString / POSITION

// The inner per-bank sound object (allocated 0x60 bytes, vtable @ 0x5ef700). Only
// the load-bearing virtual slots are declared so the dispatches lower to the exact
// `mov eax,[obj]; call [eax+N]` __thiscall sequences. Declarations only - never
// defined here, so no ??_7 is emitted in this TU; the retail vtable is referenced
// only by address (reloc-masked) when an instance is created.
class CGruntzSoundInnerZ {
public:
    virtual void Slot00();                    // +0x00
    virtual int ScalarDtor(int flag);         // +0x04  scalar deleting dtor
    virtual void Slot08();                    // +0x08
    virtual void Slot0C();                    // +0x0c
    virtual void Slot10();                    // +0x10
    virtual int Init(int a1, int a2, int a3); // +0x14  one-time setup
    virtual void Slot18();                    // +0x18
    virtual void Slot1C();                    // +0x1c
    virtual void Slot20();                    // +0x20
    virtual int Play(int hDriver, int a2);    // +0x24
    virtual void Slot28();                    // +0x28
    virtual void Slot2C();                    // +0x2c
    virtual int Stop();                       // +0x30  stop / status query
};

class CGruntzSoundZ : public CMapStringToOb {
public:
    ~CGruntzSoundZ();           // body at RVA 0x086040
    int Shutdown_1384f0();      // defined in the sibling AIL TU (RVA 0x1384f0)
    void StopAndFlush_138530(); // stop current + destroy every map entry
    CGruntzSoundInnerZ* CreateBank_138670(int a1, int a2, int a3);
    void Insert_138700(CGruntzSoundInnerZ* inner);
    CGruntzSoundInnerZ* Lookup_138730(const char* key);
    int Play_138840(int a1, int a2);
    void StopCurrent_1388a0();
    int IsPlaying_138920();

    CGruntzSoundInnerZ* m_pCurrent; // +0x1c
    int m_digHandle;                // +0x20
    int m_mdiHandle;                // +0x24
    int m_enabled;                  // +0x28
};

#endif // GRUNTZ_DSNDMGR_CGRUNTZSOUNDZ_H
