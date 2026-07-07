// GruntzSoundZ.h - a positional/zoned sound bank manager in the Dsndmgr module
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

#include <Mfc.h>          // real MFC CMapStringToOb / CObject / CString / POSITION
#include <Wap32/Object.h> // Wap::CObject grand-base (slots 0/2/3/4 = 0x1bef01/0x28ec/0x106e/0x4034)

// The Miles Sound System XMIDI sequence handle: <mss.h>'s opaque HSEQUENCE. Forward-
// declared here so this ~light header need not pull the 4200-line <mss.h> into every
// includer; GruntzSoundZ.cpp gets the identical typedef from <mss.h> itself.
typedef struct _SEQUENCE* HSEQUENCE;

// The inner per-bank sound object (0x60 bytes, vtable @ 0x5ef700). Derives from the
// shared Wap::CObject grand-base: cl inherits its 5 base slots (0 GetRuntimeClass /
// 1 dtor / 2 Serialize / 3 AssertValid / 4 Dump) and this class overrides the dtor
// (slot 1) and adds 11 new virtuals (slots 5..15). cl auto-emits ??_7CGruntzSoundInnerZ
// @@6B@ (0x5ef700) and stamps the vptr in the inline ctor; `new` in the create helpers
// lowers to operator-new + the null-guarded inline ctor.
class CGruntzSoundInnerZ : public Wap::CObject {
public:
    // Only the dtor overrides a base (Wap::CObject slot 1); slots 5..15 are new virtuals
    // (CObject has just 5 slots, so there is nothing above slot 4 to override).
    // [1] 0x138a30 overrides ~CObject (scalar-deleting dtor, defined externally).
    virtual ~CGruntzSoundInnerZ() OVERRIDE;
    // slot 5 = one-time setup from an in-memory buffer: copies `name` into m_name (or
    // auto-names "MIDI%i"), allocs the sequence handle + m_loadBuffer, seeds defaults.
    virtual i32 DecodeBuf(const void* buf, u32 len, const char* name); // [5] 0x138c20 in-mem setup
    // slot 6 = open `path` as a file, slurp it into m_loadBuffer, run DecodeBuf; a ".."
    // path forwards to LoadSpecial. `name` = the registration name (0 -> auto).
    virtual i32 Load(const char* path, const char* name); // [6] 0x138aa0 load a bank from a file
    virtual void ReleaseHandle();            // [7]  0x138dd0  Stop + free seq handle & buffer
    virtual i32 IsStarted();                 // [8]  0x138a10  m_seqHandle != 0
    virtual i32 Play(i32 hDriver, i32 mode); // [9]  0x138e10  start on the digital driver
    virtual i32 StopAll();                   // [10] 0x138e90  pause (nest via m_pauseDepth)
    virtual i32 StopBank(i32 bank);          // [11] 0x138ed0  resume (unnest via m_pauseDepth)
    virtual i32 Stop();                      // [12] 0x138e60  AIL_end_sequence
    virtual i32 Retrigger(); // [13] 0x138f20  re-Play(m_playDriver,m_playMode) if idle
    // [14] 0x138a20  `mov eax,1; retn` const-true predicate. Declared-only own slot of
    // this class (like the sibling slots, no PDB name survives): named descriptively for
    // the MIDI/XMIDI sequence object it interrogates; exact original identity unrecovered.
    virtual i32 IsMidi();
    virtual i32 LoadSpecial(const char* path, const char* name); // [15] 0x138d50 ".." AIL-file load

    // Inline ctor: cl stamps ??_7 then seeds the fields in retail store order.
    CGruntzSoundInnerZ() {
        m_name[0] = 0;
        m_pauseDepth = 0;
        m_playMode = 0;
        m_playDriver = 0;
        m_tempoPct = 0x64;
        m_volumePct = 0x64;
        m_seqHandle = 0;
        m_loadBuffer = 0;
    }

    i32 IsBusy(); // RVA 0x138f60 - IsStarted() gate + AIL_sequence_status(m_seqHandle)
    // Non-virtual AIL-sequence tuning helpers (gate on IsStarted(); reloc-masked AIL imports).
    i32 SetTempo(i32 tempo, i32 ms);   // RVA 0x138f90 - AIL_set_sequence_tempo, cache m_tempoPct
    i32 SetVolume(i32 volume, i32 ms); // RVA 0x138fd0 - scale 0..100->0..127, cache m_volumePct
    i32 SetLoop(i32 loop);             // RVA 0x139030 - re-arm AIL_set_sequence_loop_count

    char m_name[0x40];     // +0x04  inline map key/name buffer
    i32 m_pauseDepth;      // +0x44  pause nesting counter (StopAll++ / StopBank-- ; 0 = playing)
    i32 m_playMode;        // +0x48  saved Play() mode arg (loop flag; re-used by Retrigger)
    i32 m_playDriver;      // +0x4c  saved Play() digital-driver handle (re-used by Retrigger)
    i32 m_volumePct;       // +0x50  seeded 0x64 (AIL volume percent default 100)
    i32 m_tempoPct;        // +0x54  seeded 0x64 (AIL tempo percent default 100)
    HSEQUENCE m_seqHandle; // +0x58  AIL sequence handle (queried by IsBusy)
    char* m_loadBuffer;    // +0x5c  owned load buffer (operator new; SoundBankLoad::Load)
};
SIZE(CGruntzSoundInnerZ, 0x60);       // allocated 0x60 bytes (inner sound object)
VTBL(CGruntzSoundInnerZ, 0x001ef700); // cl-emitted ??_7CGruntzSoundInnerZ@@6B@ (16-slot)

class CGruntzSoundZ : public CMapStringToOb {
public:
    virtual ~CGruntzSoundZ() OVERRIDE; // body at RVA 0x086040
    // AIL driver bring-up: cache the digital/MIDI handles, optionally open the MIDI-out driver.
    i32 Init(i32 mdiHandle, i32 digHandle, i32 skipInit); // RVA 0x138490
    void Shutdown();     // RVA 0x1384f0 - StopAndFlush + AIL_shutdown teardown
    void StopAndFlush(); // RVA 0x138530 - stop current + destroy every map entry
    // XMIDI master-volume push/read. __thiscall on the m_sound bank (CGruntzMgr +0x48);
    // the body ignores `this` and drives the global AIL MIDI driver.
    i32 SetXMidiVolume(i32 volume); // RVA 0x138950 - scale 0..100 and push to AIL
    i32 GetXMidiVolume();           // RVA 0x1389c0 - read + rescale to 0..100
    CGruntzSoundInnerZ* CreateBank2(const char* path, const char* name);
    CGruntzSoundInnerZ* CreateBank(const void* buf, u32 len, const char* name);
    void Insert(CGruntzSoundInnerZ* inner);
    CGruntzSoundInnerZ* FindBank(const char* key);
    i32 PlayCreate2(const char* path, i32 playMode, const char* name);
    i32 PlayCreate3(const void* buf, u32 len, i32 playMode, const char* name);
    i32 PlayByName(const char* name, i32 playMode);
    void StopCurrent();
    i32 Restart(i32 playMode);
    i32 StopAll();
    i32 StopBank(i32 bank);
    i32 IsPlaying();

    CGruntzSoundInnerZ* m_pCurrent; // +0x1c
    i32 m_digHandle;                // +0x20
    i32 m_mdiHandle;                // +0x24
    i32 m_enabled;                  // +0x28
};
SIZE(CGruntzSoundZ, 0x2c); // 0x1c CMapStringToOb base + 4 dwords

#endif // GRUNTZ_DSNDMGR_CGRUNTZSOUNDZ_H
