// CRandomAmbientSound.h - the RTTI-named CRandomAmbientSound (vtable 0x5e713c,
// RTTI chain CRandomAmbientSound : CAmbientSound : CUserBase, sizeof 0x58, in
// C:\Proj\Gruntz). A "play a sound at random intervals while the listener is in a
// region" eyecandy object: it owns a DirectSoundMgr handle (+0x04), two visibility
// boxes (+0x18 / +0x28, sentinel 0x80000000 == "no box"), a couple of play params
// (+0x08/+0x0c/+0x10/+0x38), an "is playing" flag (+0x14), and an embedded interval
// roller (+0x40..+0x54: [lo,hi]/[lo2,hi2], the rolled countdown at +0x50, a phase
// flag at +0x54). The roller LCG is the global rand (winapi_00cd00_timeGetTime).
//
// NOTE: the trace-discovered class formerly squatting this name (offset 0 = world
// back-pointer, no vptr) was renamed CWorldSoundSet (include/Gruntz/CWorldSoundSet.h);
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

#include <Ints.h>
#include <Gruntz/CGameRegistry.h>
#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>

// The big game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @ 0x64556c).
// Update gates the play on m_activeLevel and m_world->m_objectCount (the active
// world's object count). The interval roller in Step also loads g_gameReg as a dead
// receiver before the rand call (the binary proves the load even though rand ignores it).
struct WwdActiveLevel {
    char m_pad0[0x24];
    i32 m_objectCount; // +0x24  object count (non-zero == playable)
};
DATA(0x0064556c)
extern CGameRegistry* g_gameReg;

// The global frame-delta clock in ms (DAT_00645584, canonical ?g_tickDelta@@3HA);
// the countdown at +0x50 is drained by it each Step.
DATA(0x00645584)
extern i32 g_tickDelta;

// The MS-CRT LCG rand() at 0x0000cd00 (lazily seeded from timeGetTime). Reached
// through the 0x39ae ILT thunk; called as a free function from Step.
extern "C" i32 winapi_00cd00_timeGetTime();

// The play/reseed drive goes straight through the real DirectSoundMgr methods on
// m_mgr: ApplyAndPlay (0x136300 frame-reseed), SetVolumeByIndex (0x1355c0) and
// SetPanByIndex (0x1357a0) - all declared in <Dsndmgr/DirectSoundMgr.h>.

// The CRT float->int truncation helper (__ftol @ 0x11f570).
extern "C" i32 __ftol(double v);

// An axis-aligned region the listener must be inside for the sound to play. The
// sentinel left==0x80000000 means "no box / always pass".
struct AmbientBox {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};

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
    i32 Lookup(void* key, void** out); // 0x1b8438
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

class CRandomAmbientSound {
public:
    // Declared-only virtual anchor: makes the class polymorphic so cl emits
    // ??_7CRandomAmbientSound and auto-stamps the vptr in the ctor (slot reloc-masks).
    virtual void Vf0();
    CRandomAmbientSound(); // 0x00bb40  base init (cl auto-stamps the vptr)
    // Setup(world, a2, a3, box, a5): seed the mgr handle + play params, copy/clear
    // the primary box, clear the secondary box. Returns 1 (0 on a null world).
    i32 Setup(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientBox* box, i32 a5); // 0x00be50
    // Update(playFlag, pos, kind): start or stop the sound this frame. 0x00c2a0.
    void Update(i32 playFlag, i32 pos, i32 kind); // 0x00c2a0
    // The positional variant's entry points (same CAmbientSound base layout, but
    // m_40/m_44 hold an anchor position instead of the interval roller):
    void
    SetupFromMap(AmbSoundMapHolder* holder, void* key, i32 a3, i32 a4, AmbientPoint* pos, i32 a5);
    i32 SetupPos(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientPoint* pos, i32 a5);
    void UpdateAt(i32 x, i32 y, i32 force); // 0x00c5b0  position-driven volume/pan
    void StopPos(i32 obj);                  // 0x00c9d0  request stop via slot 2
    i32 TickObj(i32 obj);                   // 0x00ca00  per-object placement tick

    // Step(x, y, force): the per-frame tick (vtable slot 3). 0x00cb30.
    void Step(i32 x, i32 y, i32 force); // 0x00cb30
    ~CRandomAmbientSound();             // 0x00bb60  scalar-deleting / full dtor (defined elsewhere)

    // --- layout (sizeof 0x58) -------------------------------------------------
    // +0x00  vptr (compiler ??_7CRandomAmbientSound; was the manual m_vptr slot)
    DirectSoundMgr* m_mgr; // +0x04  the sound-mgr handle (mgr->...)
    i32 m_lastPosition;    // +0x08  last play position / pan base
    i32 m_scaleA;          // +0x0c  scale A (compared to 5; -0xf above)
    i32 m_scaleB;          // +0x10  scale B (>0 gate)
    i32 m_isPlaying;       // +0x14  "is playing" flag
    AmbientBox m_box1;     // +0x18  primary visibility box
    AmbientBox m_box2;     // +0x28  secondary visibility box
    i32 m_panIndex;        // +0x38  pan/reseed parameter
    i32 m_3c;              // +0x3c  zero-init in ctor
    i32 m_40;              // +0x40  interval roller lo
    i32 m_44;              // +0x44  interval roller hi
    i32 m_48;              // +0x48  interval roller lo2 (phase B)
    i32 m_4c;              // +0x4c  interval roller hi2 (phase B)
    i32 m_countdownMs;     // +0x50  rolled countdown (ms, drained by g_645584)
    i32 m_phase;           // +0x54  roller phase flag (toggled each reroll)
};
SIZE(CRandomAmbientSound, 0x58);

#endif // GRUNTZ_CRANDOMAMBIENTSOUND_H
