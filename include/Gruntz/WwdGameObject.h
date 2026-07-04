#ifndef GRUNTZ_WWDGAMEOBJECT_H
#define GRUNTZ_WWDGAMEOBJECT_H

#include <Ints.h>
#include <rva.h>

// CWwdGameObject - a runtime "plane object" deserialized from WWD level data.
// WwdFile::ReadPlaneObjects (0x162af0) constructs one per record via the ctor
// at 0x15b390 (NOT reconstructed here - it lives in the eh-frame ctor TU). The
// object owns a sprite-animation worker at +0x7c (0x17c-byte, the same family
// as CDDrawWorkerCache's AnimWorker, foreign vtable g_*Vtbl), a small
// command-dispatch sub-object at +0x1a0, and a back-pointer to its owning
// manager at +0x0c.
//
// Class identity is a role inference (no RTTI on the vtable @0x5f0020); only the
// this-OFFSETS and emitted code bytes are load-bearing (campaign doctrine), so
// field names are placeholders m_<hexoffset>.

// The owning manager reached through CWwdGameObject+0x0c. Its methods are
// reached as [[+0xc]+slot] - modeled as a typed vtable struct so the dispatch
// lowers to the exact `mov eax,[mgr+slot]; call` with no cast.
struct WwdMgr;

// Forward of the AnimWorker+0x18 sub-object interface (defined in the .cpp); the
// worker's m_18 slot is typed as this so the [m_18][+0x8] dispatch needs no cast.
class WorkerSub;

// The animation/sprite worker at CWwdGameObject+0x7c. Foreign vtable; its
// virtuals are DECLARED only (never defined) so cl emits no ??_7 - the real
// vtable is the engine datum the ctor stamps. Declared as a polymorphic class
// so the virtual dispatch lowers to the exact `mov ecx,worker; call [vtbl+off]`
// __thiscall sequence (virtuals are __thiscall by default in MSVC 5.0).
SIZE_UNKNOWN(AnimWorker);
class AnimWorker {
public:
    virtual void Slot00();             // +0x00
    virtual void Slot04();             // +0x04
    virtual void Slot08();             // +0x08
    virtual void Slot0C();             // +0x0c
    virtual void Advance(void* owner); // +0x10
    virtual void Slot14();             // +0x14
    virtual void Slot18();             // +0x18
    virtual void Slot1C();             // +0x1c
    virtual void Slot20();             // +0x20
    virtual i32 Init(i32 a1, i32 a3);  // +0x24

    // Non-virtual play-step helper (0x164830, __thiscall, 4 args) Play tail-calls.
    i32 Method164830(i32 a1, i32 type, i32 a3, i32 a4);

    i32 m_04;
    i32 m_08; // +0x08  flag bits (bit0/bit1 read by Setup)
    char m_pad0c[0x18 - 0x0c];
    WorkerSub* m_18; // +0x18  sub-object with its own vtable ([m_18][+0x8] called)
    i32 m_1c;        // +0x1c  scratch state id (saved/forced 0x50..0x53/restored)
};

#endif // GRUNTZ_WWDGAMEOBJECT_H
