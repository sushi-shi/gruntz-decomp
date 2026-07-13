// RandomAmbientSound.h - the RTTI-named CRandomAmbientSound (vtable 0x5e713c,
// RTTI chain CRandomAmbientSound : CAmbientSound : CUserBase, sizeof 0x58, in
// C:\Proj\Gruntz). A "play a sound at random intervals while the listener is in a
// region" eyecandy object: it owns a DirectSoundMgr handle (+0x04), two visibility
// boxes (+0x18 / +0x28, sentinel 0x80000000 == "no box"), a couple of play params
// (+0x08/+0x0c/+0x10/+0x38), an "is playing" flag (+0x14), and an embedded interval
// roller (+0x40..+0x54: [lo,hi]/[lo2,hi2], the rolled countdown at +0x50, a phase
// flag at +0x54). The roller LCG is the global rand (winapi_00cd00_timeGetTime).
//
// NOTE: the trace-discovered class formerly squatting this name (offset 0 = world
// back-pointer, no vptr) was renamed CWorldSoundSet (include/Gruntz/WorldSoundSet.h);
// it is a different, non-polymorphic class.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes
// are load-bearing (campaign doctrine). Real polymorphic: a declared-only virtual
// anchor makes cl emit ??_7CRandomAmbientSound@@6B@ (this class's own vtable at
// 0x5e713c, named in config/vtable_names.csv) and auto-stamp the vptr in the ctor -
// so the base init no longer needs a manual vptr store (the base-vtable stamp
// target 0x5e70b4 becomes this class's own vtable, an accepted codegen shift).
#ifndef GRUNTZ_CRANDOMAMBIENTSOUND_H
#define GRUNTZ_CRANDOMAMBIENTSOUND_H

#include <Gruntz/AmbientSound.h>
#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>
extern "C" CGameRegistry* g_gameReg; // *0x24556c canonical singleton

// The big game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A, RVA 0x24556c /
// VA 0x64556c). Update gates the play on the +0x54 active-level object's armed/
// playable gate (m_inputState->m_active; CWorldSoundSet in <Gruntz/WorldSoundSet.h>). The
// interval roller in Step also loads g_gameReg as a dead receiver before the rand
// call (the binary proves the load even though rand ignores it).

// The global frame-delta clock in ms (DAT_00645584, canonical ?g_frameDelta@@3HA);
// the countdown at +0x50 is drained by it each Step.
extern "C" i32 g_frameDelta;

// The MS-CRT LCG rand() at 0x0000cd00 (lazily seeded from timeGetTime). Reached
// through the 0x39ae ILT thunk; called as a free function from Step.
extern "C" i32 winapi_00cd00_timeGetTime();

// The play/reseed drive goes straight through the real DirectSoundMgr methods on
// m_mgr: ApplyAndPlay (0x136300 frame-reseed), SetVolumeByIndex (0x1355c0) and
// SetPanByIndex (0x1357a0) - all declared in <Dsndmgr/DirectSoundMgr.h>.

// The CRT float->int truncation helper (__ftol @ 0x11f570).
extern "C" i32 __ftol(double v);

// A 2-int (x,y) point copied wholesale into the +0x40/+0x44 field pair by the
// positional Setup path (m_40/m_44 serve as the sound's anchor position).
struct AmbientPoint {
    i32 x; // +0x00
    i32 y; // +0x04
};

// A CMapPtrToPtr-style table (the lookup at 0x1b8438, __thiscall ret 8). The
// SetupFromMap path resolves the mgr record by key out of the holder's table
// embedded at +0x10, so `add ecx,0x10; call Lookup` falls out.
struct AmbSoundMap {
    i32 Lookup(void* key, void** out); // 0x1b8438 (real map method; devs shape)
};
struct AmbSoundMapHolder {
    char m_pad00[0x10];
    AmbSoundMap m_map; // +0x10  the lookup table
};
// The mgr record the map resolves to; its +0x10 is the DirectSoundMgr handle.
struct AmbSoundRecord {
    char m_pad00[0x10];
    DirectSoundMgr* m_mgr; // +0x10
};

struct Arg1_bdd0; // the keyed-map holder Dispatch resolves against (<Gruntz/BoundaryTailViews.h>)

VTBL(CRandomAmbientSound, 0x001e713c);
class CRandomAmbientSound : public CAmbientSound {
public:
    CRandomAmbientSound(); // 0x00bb40  base init (cl auto-stamps the vptr)
    // Setup(world, a2, a3, box, a5): seed the mgr handle + play params, copy/clear
    // the primary box, clear the secondary box. Returns 1 (0 on a null world).
    i32 Setup(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientBox* box, i32 a5); // 0x00be50
    // 0xbdd0: resolve a mgr record for `key` from a1's embedded CMapStringToOb (+0x10),
    // then Setup(record->m_10, a3, a4, box, a6). Null return on a miss. (This is the
    // real home of the old CObj_bdd0::Dispatch placeholder - `this` is a CRandomAmbientSound,
    // proven by the tail-call into Setup @0xbe50.)
    void* Dispatch(Arg1_bdd0* a1, const char* key, i32 a3, i32 a4, AmbientBox* box, i32 a6);
    // Update(playFlag, pos, kind): start or stop the sound this frame. 0x00c2a0.
    virtual void Update(i32 playFlag, i32 pos, i32 kind) OVERRIDE; // slot 3 (0x00c2a0)
    // The positional variant's entry points (same CAmbientSound base layout, but
    // m_40/m_44 hold an anchor position instead of the interval roller):
    void
    SetupFromMap(AmbSoundMapHolder* holder, void* key, i32 a3, i32 a4, AmbientPoint* pos, i32 a5);
    i32 SetupPos(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientPoint* pos, i32 a5);
    void UpdateAt(i32 x, i32 y, i32 force); // 0x00c5b0  position-driven volume/pan
    void StopPos(i32 obj);                  // 0x00c9d0  request stop via slot 2
    i32 TickObj(i32 obj);                   // 0x00ca00  per-object placement tick

    // Step(x, y, force): the per-frame tick (vtable slot 3). 0x00cb30.
    void Step(i32 x, i32 y, i32 force);      // 0x00cb30
    virtual ~CRandomAmbientSound() OVERRIDE; // slot 0  0x00bb60
    // Init2(lo, hi, lo2, hi2): the interval-roller seed run by the box factory
    // (CWorldSoundSet::CreateRandomBox_ba00; unreconstructed, reloc-masked).
    void Init2(i32 a0, i32 a1, i32 a2, i32 a3);

    // --- layout (sizeof 0x58) -------------------------------------------------
    // +0x00..+0x3f come from the CAmbientSound base (vptr, m_voice, m_level,
    // m_scaleA/m_scaleB, m_isPlaying, m_box1/m_box2, m_panIndex, m_listNode).
    // (An earlier revision re-declared the base fields here, shadowing them at
    // +0x40.. - a proven layout bug: retail Setup/Update write +0x04..+0x38.)
    //
    // +0x40/+0x44 are a union: the random path (Setup/Step) uses them as the
    // phase-A interval bounds [lo,hi]; the positional path (SetupPos/UpdateAt) uses
    // them as the (x,y) anchor. Left as raw offsets since the role forks by instance.
    i32 m_40;          // +0x40  interval-roller phase-A lo  / anchor x
    i32 m_44;          // +0x44  interval-roller phase-A hi  / anchor y
    i32 m_intervalLoB; // +0x48  interval-roller phase-B lo (roller-only)
    i32 m_intervalHiB; // +0x4c  interval-roller phase-B hi (roller-only)
    i32 m_countdownMs; // +0x50  rolled countdown (ms, drained by g_frameDelta)
    i32 m_phase;       // +0x54  roller phase flag (toggled each reroll)
};
SIZE(CRandomAmbientSound, 0x58);

#endif // GRUNTZ_CRANDOMAMBIENTSOUND_H
