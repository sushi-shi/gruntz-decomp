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

#include <rva.h>

#include <Mfc.h> // real MFC CMapStringToOb / CObject / CString / POSITION

// The inner per-bank sound object (allocated 0x60 bytes, vtable @ 0x5ef700).
// ALL-VTABLES phase: modeled REAL-POLYMORPHIC (16 virtuals in slot order from the
// retail vtable 0x5ef700). cl auto-emits ??_7CGruntzSoundInnerZ@@6B@ and stamps the
// vptr in the (inline) ctor; the create helpers construct it via placement-new so
// the vptr store falls out implicitly (was a manual `*(void**)raw = &innerVtable`).
// Slots whose bodies live in other TUs are declared-only (external slot refs in the
// emitted vtable); slot 0 is kept non-pure (a concrete embeddable class - retail
// slot 0 is sub_1bef01, not __purecall).
class CGruntzSoundInnerZ {
public:
    virtual void Slot00();         // [0]  0x1bef01
    virtual ~CGruntzSoundInnerZ(); // [1]  0x138a30  (scalar-deleting dtor; defined externally)
    virtual void Slot08();         // [2]  0x0028ec
    virtual void Slot0C();         // [3]  0x00106e
    virtual void Slot10();         // [4]  0x004034
    // slot 5 = the in-memory decode (SoundBankLoad's DecodeBuf(buf, len, arg)); the
    // create path also drives it as the 3-arg one-time setup.
    virtual i32 DecodeBuf(i32 a1, i32 a2, i32 a3); // [5]  0x138c20  in-memory decode / setup
    // slot 6 = Load(name, arg): SoundBankLoad opens `name`, slurps it into m_loadBuffer
    // and runs DecodeBuf, or forwards a ".." name to LoadSpecial.
    virtual i32 Load(i32 name, i32 arg);   // [6]  0x138aa0  load a bank by name
    virtual void Slot1C();                 // [7]  0x138dd0
    virtual i32 IsStarted();               // [8]  0x138a10  "is started" gate (IsBusy)
    virtual i32 Play(i32 hDriver, i32 a2); // [9]  0x138e10
    virtual i32 StopAll();                 // [10] 0x138e90  CGruntzSoundZ::StopAll forwards here
    virtual i32 StopBank(i32 a1);          // [11] 0x138ed0  CGruntzSoundZ::StopBank forwards here
    virtual i32 Stop();                    // [12] 0x138e60  stop / status query
    virtual void Slot34();                 // [13] 0x138f20
    virtual void Slot38();                 // [14] 0x138a20
    // slot 15 = the special ".."-name load handler (SoundBankLoad's LoadSpecial).
    virtual i32 LoadSpecial(const char* path, i32 arg); // [15] 0x138d50

    // Inline ctor: cl stamps ??_7 first, then seeds the fields in retail store
    // order (was the create helpers' manual stamp + field seed).
    CGruntzSoundInnerZ() {
        m_name[0] = 0;
        m_44 = 0;
        m_48 = 0;
        m_4c = 0;
        m_tempoPct = 0x64;
        m_volumePct = 0x64;
        m_seqHandle = 0;
        m_loadBuffer = 0;
    }

    i32 IsBusy(); // RVA 0x138f60 - IsStarted() gate + AIL_sequence_status(m_seqHandle)

    char m_name[0x40];  // +0x04  inline map key/name buffer
    i32 m_44;           // +0x44  seeded 0 by the ctor (role unproven)
    i32 m_48;           // +0x48  seeded 0 (role unproven)
    i32 m_4c;           // +0x4c  seeded 0 (role unproven)
    i32 m_volumePct;    // +0x50  seeded 0x64 (AIL volume percent default 100)
    i32 m_tempoPct;     // +0x54  seeded 0x64 (AIL tempo percent default 100)
    i32 m_seqHandle;    // +0x58  AIL sequence handle (queried by IsBusy)
    char* m_loadBuffer; // +0x5c  owned load buffer (operator new; SoundBankLoad::Load)
};
SIZE(CGruntzSoundInnerZ, 0x60);       // allocated 0x60 bytes (inner sound object)
VTBL(CGruntzSoundInnerZ, 0x001ef700); // cl-emitted ??_7CGruntzSoundInnerZ@@6B@ (16-slot)

class CGruntzSoundZ : public CMapStringToOb {
public:
    ~CGruntzSoundZ();    // body at RVA 0x086040
    i32 Shutdown();      // defined in the sibling AIL TU (RVA 0x1384f0)
    void StopAndFlush(); // stop current + destroy every map entry
    CGruntzSoundInnerZ* CreateBank2(i32 a1, i32 a2);
    CGruntzSoundInnerZ* CreateBank(i32 a1, i32 a2, i32 a3);
    void Insert(CGruntzSoundInnerZ* inner);
    CGruntzSoundInnerZ* FindBank(const char* key);
    i32 PlayCreate2(i32 a1, i32 a2, i32 a3);
    i32 PlayCreate3(i32 a1, i32 a2, i32 a3, i32 a4);
    i32 PlayByName(const char* name, i32 a2);
    void StopCurrent();
    i32 Restart(i32 a1);
    i32 StopAll();
    i32 StopBank(i32 a1);
    i32 IsPlaying();

    CGruntzSoundInnerZ* m_pCurrent; // +0x1c
    i32 m_digHandle;                // +0x20
    i32 m_mdiHandle;                // +0x24
    i32 m_enabled;                  // +0x28
};
SIZE(CGruntzSoundZ, 0x2c); // 0x1c CMapStringToOb base + 4 dwords

#endif // GRUNTZ_DSNDMGR_CGRUNTZSOUNDZ_H
