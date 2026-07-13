// WorldSoundSet.cpp - the world-sound TU (C:\Proj\Gruntz), interval
// 0x00b5e0-0x00cc98 (+ four out-of-band strays homed here). ONE original TU per
// docs/exe-map/interval-dossiers.md #10d: our worldsoundset + randomambientsound
// + ambientsound units were slices of this single file - CWorldSoundSet's
// CreateAmbient*/CreatePos*/CreateRandom* factories manufacture exactly the
// channel classes (CAmbientSound / CAmbientPosSound / CRandomAmbientSound)
// defined around them; the head is woven, the tail class-grouped.
//
// CWorldSoundSet manages the list of live sound channels: Init seeds the
// world/level back-pointers and activates the object; Restart/Stop/Resume/Retune
// drive the channels (each walks the embedded CPtrList raw - node->next at +0x00,
// the channel payload at node+0x08) and poke the world's DirectSound sub-object;
// Teardown / the destructor scalar-delete every channel and RemoveAll the list.
//
// ALL-VTABLES mandate: the three channel classes are REAL polymorphic classes
// (canonical headers <Gruntz/AmbientSound.h> / <Gruntz/RandomAmbientSound.h>).
// RTTI derivation CAmbientPosSound / CRandomAmbientSound : CAmbientSound
// (: CUserBase) is modeled by C++ inheritance; each is built via placement
// `new (raw) CXxx`, so cl auto-emits ??_7CAmbientSound / ??_7CAmbientPosSound /
// ??_7CRandomAmbientSound (0x1e710c / 0x1e7124 / 0x1e713c) and inlines the vptr
// stamp at each Create* site.
//
// Field names are placeholders; the OFFSETS + emitted code bytes are load-bearing.
#include <Mfc.h> // MFC superset (afx-first); also pulled by WorldSoundSet.h
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/BoundaryLeafLogicViews.h> // L_8860 (== ~CUserLogic; fold blocked, see below)
#include <Gruntz/AmbientSound.h>           // canonical CAmbientSound / CAmbientPosSound
#include <Gruntz/RandomAmbientSound.h>     // canonical CRandomAmbientSound
#include <Rez/RezMgr.h>                    // RezAlloc - the engine heap allocator (reloc-masked)
#include <rva.h>
#include <Gruntz/UserLogic.h>         // CUserBase (real base of CAmbientSound)
#include <Gruntz/BoundaryTailViews.h> // Arg1_bdd0/Entry_bdd0 (0xbdd0 Dispatch arg views)
#include <Globals.h>                  // g_posSoundReq

#include <math.h> // sqrt intrinsic (UpdateAt's positional falloff) - inline fsqrt

// The positional-sound request flag (owner-TU def; .bss, VA 0x62990c). Set to 2.
DATA(0x0022990c)
i32 g_posSoundReq; // 0x62990c

inline void* operator new(u32, void* p) {
    return p;
}

// ---------------------------------------------------------------------------
// The free `Spawn`/`Stop` ambient-sound pair (0x00c9d0 / 0x00ca00, __cdecl). They
// drive the ambient voice that hangs off a CGameObject's +0x7c aux: aux->m_requestState is
// the request state (0 = "spawn", 0x1e = "stop"), aux->m_voice the live voice.
// ---------------------------------------------------------------------------
// The sound voice object the aux points at (+0x168): its CObject vptr (slot 0 =
// scalar-deleting dtor), the DirectSoundMgr handle (+0x04), the playing flag
// (+0x14) and the spatial-mgr list node (+0x3c) RemoveAt unlinks.
class PosSoundVoice {
public:
    virtual ~PosSoundVoice(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    DirectSoundMgr* m_mgr;    // +0x04
    char m_pad8[0x14 - 0x8];
    i32 m_isPlaying; // +0x14  playing flag
    char m_pad18[0x3c - 0x18];
    void* m_spatialNode;        // +0x3c  spatial-mgr list node
    virtual void VtSlotFill0(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2(); // vtable-slot filler (real slot; declared-only)
};
// The aux sub-object (CGameObject+0x7c): the init/action handler, the request state,
// the emit src-clip and the live voice slot.
struct PosSoundAux {
    char m_pad00[0x10];
    void* m_handler; // +0x10  the object's init/action handler (vs the default at 0x402d15)
    char m_pad14[0x1c - 0x14];
    i32 m_requestState; // +0x1c  request state (0 spawn / 0x1e stop / 5 spawned)
    char m_pad20[0x2c - 0x20];
    i32 m_srcL; // +0x2c  emit src clip: left
    i32 m_srcR; // +0x30                   right
    i32 m_srcT; // +0x34                   top
    i32 m_srcB; // +0x38                   bottom
    char m_pad3c[0x168 - 0x3c];
    PosSoundVoice* m_voice; // +0x168  the live voice
};
// The CGameObject the request rides on (only the touched offsets).
struct PosSoundObj {
    char m_pad00[0x08];
    i32 m_flags08; // +0x08  flags
    char m_pad0c[0x40 - 0xc];
    i32 m_flags40; // +0x40  flags
    char m_pad44[0x5c - 0x44];
    i32 m_x; // +0x5c  x
    i32 m_y; // +0x60  y
    char m_pad64[0x7c - 0x64];
    PosSoundAux* m_aux; // +0x7c  the aux
    char m_pad80[0x120 - 0x80];
    i32 m_120; // +0x120
    char m_pad124[0x134 - 0x124];
    i32 m_extentL; // +0x134  per-side emit extents (L/T/R/B)
    i32 m_extentT; // +0x138
    i32 m_extentR; // +0x13c
    i32 m_extentB; // +0x140
    RECT m_area;   // +0x144  emit source area (CopyRect base)
    RECT m_placed; // +0x154  placed rect written back on emit
    char m_pad164[0x19c - 0x164];
    void* m_layer; // +0x19c  layer/desc (its +0x10 feeds the factory)
};
// The spatial-sound voice CPtrList lives at g_gameReg->m_inputState + 0x08 (the same
// embedded CPtrList the manager ctors/tears down); RemoveAt unlinks the voice's node.
// g_gameReg->m_inputState is the CWorldSoundSet modeled in this TU's header.

// The factory the spawn path calls (Stub_00b960 via the 0x20e5 thunk). It news a
// 0x48-byte voice; modeled __stdcall (callee-cleaned, no `add esp`).
extern "C" void* __stdcall PosSoundSpawn(void* layer, i32 a2, void* outPt, i32 a4, i32 a5);

void SpawnPosSound(PosSoundObj* obj);

// ---------------------------------------------------------------------------
// 0x87b0 IS ??1CUserBase@@UAE@XZ - the out-of-line COMDAT copy of the INLINE
// ~CUserBase (<Gruntz/UserLogic.h>), now bound by @rva-symbol in ActionArea.cpp
// (RVA-adjacent; its obj emits the COMDAT). The former placeholder here
// (`CUserBase87b0`, VTBL'd at 0x1e70fc) was a CONFLATION built on a broken thunk
// chase: 0x1e70fc's slot-0 sdd (0x8780) calls thunk 0x2ea5 -> 0x8750 (the zDArray
// dtor), NOT 0x87b0 - and 0x1e70fc's RTTI COL names
// .?AV?$zDArray@P8CUserLogic@@AEHXZ@@ (the PMF zDArray instantiation), so binding
// it to any plain-identifier class was wrong by construction. 0x87b0's real
// identity is proven by its ~150 EH-unwind-funclet callers (every CUserBase-family
// ctor's partial-unwind calls it via thunk 0x1343) + its body: stamp ??_7CUserBase
// (0x5e70b4, RTTI .?AVCUserBase@@) and return.

// @early-stop
// 0x8860 IS ??1CUserLogic - PROVEN from the binary: ??_7CUserLogic @0x1e705c slot 0 holds
// an ILT thunk (0x3cfb) to the scalar-deleting dtor 0x8a10, whose body calls 0x8860. (The
// old note claimed a rival ~CUserLogic copy at 0x117f0; that was a MISBINDING - 0x117f0's
// sdd 0x117c0 sits in slot 0 of ??_7CTileTriggerTransition @0x1e7db4, and MSVC5 keeps
// exactly ONE COMDAT per mangled name, so N byte-identical empty leaf dtors can never be
// N copies of one ~CUserLogic: each is its OWN class's dtor. 0x117f0 is now correctly
// bound as ??1CTileTriggerTransition in TileLogicPump.cpp.)
// The fold is BLOCKED on the EMITTER, not on the identity: ~CUserLogic is inline in
// <Gruntz/UserLogic.h> (load-bearing - ~50 leaf dtors fold it), so it can only be pinned
// by @rva-symbol from a TU whose obj emits the COMDAT, and this TU does not odr-use
// CUserLogic yet (retail's did - the whole CUserBase/CUserLogic base-method COMDAT pool
// 0x87d0/0x87f0/0x8810/0x8840/0x8a10/0x8b50 lands in THIS obj's band). Keeping the
// placeholder holds the body at 100% instead of dropping it. @identity-TODO: L_8860 == CUserLogic.
RVA(0x00008860, 0x44)
L_8860::~L_8860() {}

// ---------------------------------------------------------------------------
// Init: refuse a null world, otherwise stash both back-pointers, mark active and
// clear the pending pan/volume. Returns 1 on success, 0 on the null guard.
// ---------------------------------------------------------------------------
RVA(0x0000b5e0, 0x29)
i32 CWorldSoundSet::Init(void* world, i32 a2) {
    if (world == 0) {
        return 0;
    }
    m_world = (CRandomAmbientWorld*)world;
    m_04 = (void*)a2;
    m_active = 1;
    m_pan = 0;
    m_vol = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// Deactivate (0x0000b620): stop the world's sound device (FreeSamples, if live),
// tear down every channel (Teardown), then clear the world back-pointer. 0xb620 is
// CODE; Ghidra mis-typed it as g_typeDesc3 data (that bogus Globals DATA removed).
// ---------------------------------------------------------------------------
RVA(0x0000b620, 0x26)
void CWorldSoundSet::Deactivate() {
    if (m_world != 0 && m_world->m_soundDev != 0) {
        m_world->m_soundDev->FreeSamples();
    }
    Teardown();
    m_world = 0;
}

// ---------------------------------------------------------------------------
// Teardown: scalar-delete every channel in the list (vtbl slot 0, flag 1), then
// RemoveAll the now-dangling list.
// ---------------------------------------------------------------------------
RVA(0x0000b660, 0x2b)
void CWorldSoundSet::Teardown() {
    CSoundNode* node = (CSoundNode*)m_list.GetHeadPosition();
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CSoundChannel* ch = cur->m_data;
        if (ch != 0) {
            delete ch;
        }
    }
    m_list.RemoveAll();
}

// ===========================================================================
// Create* factories (rtti-vptr mislabeled them onto the channel classes whose
// vtable they stamp; the `this` is this owner). Each allocates a fresh channel,
// seeds its level fields (m_level = 100 default), lets the inlined placement-new
// ctor stamp the class's own vtable, runs the one-time Init; on failure
// scalar-deletes it and returns 0, on success appends it to m_list (storing the
// list node in m_listNode) and returns it.
// ===========================================================================

// CAmbientSound (0x40), 6-arg Init (m_world + the owner's m_04 threaded in).
RVA(0x0000b6a0, 0x83)
CAmbientSound* CWorldSoundSet::CreateAmbient6_b6a0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    void* raw = RezAlloc(0x40);
    CAmbientSound* obj;
    if (raw != 0) {
        obj = new (raw) CAmbientSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_isPlaying = 0;
        obj->m_listNode = 0;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init6(m_world, a0, a1, m_04, a2, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = (void*)m_list.AddTail(obj);
    return obj;
}

// ===========================================================================
// CAmbientSound::~CAmbientSound  (0x00b790)
// ===========================================================================
// Clear the voice handle (+0x04) and the list node (+0x3c); cl auto-emits the
// CUserBase base vptr restamp (0x5e70b4) as the sub-object unwinds. Real-
// polymorphic, so no manual vtable store; the derived-vptr store at entry stays
// DCE'd (no virtual dispatch in the body), matching retail's 15-byte shape.
RVA(0x0000b790, 0xf)
CAmbientSound::~CAmbientSound() {
    m_voice = 0;
    m_listNode = 0;
}

// CAmbientSound (0x40), 5-arg Init (no m_world).
RVA(0x0000b7b0, 0x80)
CAmbientSound* CWorldSoundSet::CreateAmbient5_b7b0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    void* raw = RezAlloc(0x40);
    CAmbientSound* obj;
    if (raw != 0) {
        obj = new (raw) CAmbientSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_isPlaying = 0;
        obj->m_listNode = 0;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init5(a0, a1, m_04, a2, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = (void*)m_list.AddTail(obj);
    return obj;
}

// CAmbientPosSound (0x48), 6-arg Init (vtable stamped last).
RVA(0x0000b850, 0x83)
CAmbientPosSound* CWorldSoundSet::CreatePos6_b850(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    void* raw = RezAlloc(0x48);
    CAmbientPosSound* obj;
    if (raw != 0) {
        obj = new (raw) CAmbientPosSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_isPlaying = 0;
        obj->m_listNode = 0;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init6(m_world, a0, a1, m_04, a2, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = (void*)m_list.AddTail(obj);
    return obj;
}

// 0xb940 - CAmbientPosSound::~CAmbientPosSound. IDENTITY PROVEN from the binary (was the
// fake placeholder class CUserBaseSubB940, RELOC_VTBL'd onto CUserBase's vtable): the
// class vtable ??_7CAmbientPosSound @0x1e7124 (bound in <Gruntz/AmbientSound.h>) holds at
// slot 0 an ILT thunk to the scalar-deleting dtor 0xb910, whose body calls THIS dtor. Its
// slots 1/2 are the SHARED CUserBase defaults (0x87d0 SerializeMove / 0x87f0 GetTypeTag
// `xor eax,eax`) - which is why the placeholder looked like a bare "CUserBase" subobject.
// The body zeroes the INHERITED CAmbientSound fields (m_voice @+0x04, m_listNode @+0x3c):
// the empty leaf dtor folds ~CAmbientSound (0xb790) inline, exactly as retail.
RVA(0x0000b940, 0xf)
CAmbientPosSound::~CAmbientPosSound() {}

// CAmbientPosSound (0x48), 5-arg Init.
RVA(0x0000b960, 0x80)
CAmbientPosSound* CWorldSoundSet::CreatePos5_b960(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    void* raw = RezAlloc(0x48);
    CAmbientPosSound* obj;
    if (raw != 0) {
        obj = new (raw) CAmbientPosSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_isPlaying = 0;
        obj->m_listNode = 0;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init5(a0, a1, m_04, a2, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = (void*)m_list.AddTail(obj);
    return obj;
}

// CRandomAmbientSound (0x58) with a validated bounding box: reject an inverted x
// (a5<a4) or y (a7<a6) range, then ::operator new the channel (not RezAlloc), 6-arg
// Init, the Init2 box roll, append, return. (a8 unused.)
// @early-stop
// ~84%: complete correct reconstruction. Residual is the same vtable-stamp-schedule +
// shrink-wrap wall the sibling factories hit (CreateRandom_bb60/CreatePos ~87%): retail
// defers the callee-saved pushes past the two range guards and schedules the ??_7 vptr
// store AFTER the member inits (obj in eax), cl saves eagerly + stamps the vptr in the
// placement-new ctor first (obj in esi). Not source-steerable; permuter no-change.
RVA(0x0000ba00, 0xc6)
CRandomAmbientSound* CWorldSoundSet::
    CreateRandomBox_ba00(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8) {
    if ((u32)a5 < (u32)a4) {
        return 0;
    }
    if ((u32)a7 < (u32)a6) {
        return 0;
    }
    void* raw = ::operator new(0x58);
    CRandomAmbientSound* obj;
    if (raw != 0) {
        obj = new (raw) CRandomAmbientSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_isPlaying = 0;
        obj->m_listNode = 0;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init6(m_world, a0, a1, m_04, a2, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->Init2(a4, a5, a6, a7);
    obj->m_listNode = (void*)m_list.AddTail(obj);
    return obj;
}

// ---------------------------------------------------------------------------
// CRandomAmbientSound base ctor (0x00bb40): cl auto-stamps the vptr, then clear
// the voice handle (+0x04) and the list node (+0x3c). The rest is set up by Setup.
// ---------------------------------------------------------------------------
RVA(0x0000bb40, 0xf)
CRandomAmbientSound::CRandomAmbientSound() {
    m_voice = 0;
    m_listNode = 0;
}

// CRandomAmbientSound (0x58): 5-arg Init then an ungated 4-arg Init2.
RVA(0x0000bb60, 0x9b)
CRandomAmbientSound* CWorldSoundSet::
    CreateRandom_bb60(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8) {
    void* raw = RezAlloc(0x58);
    CRandomAmbientSound* obj;
    if (raw != 0) {
        obj = new (raw) CRandomAmbientSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_isPlaying = 0;
        obj->m_listNode = 0;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init5(a0, a1, m_04, a2, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->Init2(a4, a5, a6, a7);
    obj->m_listNode = (void*)m_list.AddTail(obj);
    return obj;
}

// ---------------------------------------------------------------------------
// Restart: re-seed the secondary back-pointer, poke the world handle, then ask
// every live channel to recompute against the new frame.
// ---------------------------------------------------------------------------
RVA(0x0000bc30, 0x3a)
void CWorldSoundSet::Restart(void* a1) {
    m_04 = a1;
    if (m_world->m_soundDev != 0) {
        m_world->m_soundDev->FreeSamples();
    }
    CSoundNode* node = (CSoundNode*)m_list.GetHeadPosition();
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CSoundChannel* ch = cur->m_data;
        if (ch != 0) {
            ch->Recompute((i32)a1);
        }
    }
}

// ---------------------------------------------------------------------------
// Stop: poke the world handle, then stop+rewind each channel's DirectSound handle
// and clear its +0x14 field.
// ---------------------------------------------------------------------------
RVA(0x0000bc80, 0x44)
void CWorldSoundSet::Stop() {
    if (m_world != 0 && m_world->m_soundDev != 0) {
        m_world->m_soundDev->FreeSamples();
    }
    CSoundNode* node = (CSoundNode*)m_list.GetHeadPosition();
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CSoundChannel* ch = cur->m_data;
        if (ch != 0 && ch->m_voice != 0) {
            ch->m_voice->StopAndRewind();
            ch->m_14 = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// Resume: clear each channel's +0x14, retune it (vtbl slot 3 with the pending
// pan/vol and flag 1), then rewind the world handle to the start (-1).
// ---------------------------------------------------------------------------
// @early-stop
// regalloc-coinflip wall (~99.7%, the entropy tail) - code byte-exact and all
// relocs paired; the only diff is the final world load picking eax vs retail's
// edi (this is dead, so retail reuses it: `mov edi,[edi]` vs our `mov eax,[edi]`),
// one byte. No source lever flips the dead-this reuse. See zero-register-pinning.md.
RVA(0x0000bcf0, 0x43)
void CWorldSoundSet::Resume() {
    CSoundNode* node = (CSoundNode*)m_list.GetHeadPosition();
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CSoundChannel* ch = cur->m_data;
        if (ch != 0) {
            ch->m_14 = 0;
            ch->Retune(m_pan, m_vol, 1);
        }
    }
    if (m_world->m_soundDev != 0) {
        m_world->m_soundDev->PurgeVoiceList(-1);
    }
}

// ---------------------------------------------------------------------------
// Retune: record the new pan/vol, push them to every live channel (vtbl slot 3,
// flag 0), then rewind the world handle.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc/schedule-coinflip wall (~86.3%) - logic complete, all relocs paired.
// Structurally identical to Resume (matched body) but the two up-front member
// stores (m_pan=pan, m_vol=vol) let cl hoist the loop-head load + null-test above
// the stores and pin `mov ebp,ecx` early, whereas retail keeps `this` in ecx
// until the stores and reads the head after - a pure register/schedule permutation
// (same bytes, reordered) plus the same dead-this tail load as Resume. The
// for-loop / store-order levers did not flip it. See zero-register-pinning.md.
RVA(0x0000bd60, 0x4b)
void CWorldSoundSet::Retune(i32 pan, i32 vol) {
    m_pan = pan;
    m_vol = vol;
    CSoundNode* node = (CSoundNode*)m_list.GetHeadPosition();
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CSoundChannel* ch = cur->m_data;
        if (ch != 0) {
            ch->Retune(pan, vol, 0);
        }
    }
    if (m_world->m_soundDev != 0) {
        m_world->m_soundDev->PurgeVoiceList(-1);
    }
}

// 0xbdd0: CRandomAmbientSound::Dispatch - look `key` up in arg1's embedded
// CMapStringToOb (at +0x10) into a zero-initialised out slot; on miss return the
// (null) slot, on hit tail-call this->Setup with the found entry's m_10 (the mgr
// handle) plus the four trailing args. __thiscall, 6 stack args (ret 0x18). The old
// CObj_bdd0 placeholder is dissolved: the tail call binds to Setup @0xbe50, which
// PROVES `this` is a CRandomAmbientSound. 100% EXACT.
RVA(0x0000bdd0, 0x53)
void* CRandomAmbientSound::Dispatch(
    Arg1_bdd0* a1,
    const char* key,
    i32 a3,
    i32 a4,
    AmbientBox* box,
    i32 a6
) {
    void* out_ob = 0; // CMapStringToPtr's value slot (Lookup 0x1b8438 takes void*&)
    a1->m_10.Lookup(key, out_ob);
    Entry_bdd0* out = (Entry_bdd0*)out_ob;
    if (out == 0) {
        return (void*)out;
    }
    return (void*)Setup((DirectSoundMgr*)out->m_10, a3, a4, box, a6);
}

// ---------------------------------------------------------------------------
// CRandomAmbientSound::Setup (0x00be50, __thiscall, 5 args): refuse a null mgr;
// otherwise stash the mgr + the three play params, copy the primary box (or stamp
// the no-box sentinel), and reset the secondary box to the sentinel. Returns 1,
// or 0 on the null guard.
// ---------------------------------------------------------------------------
RVA(0x0000be50, 0x8f)
i32 CRandomAmbientSound::Setup(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientBox* box, i32 a5) {
    if (mgr == 0) {
        return 0;
    }
    m_voice = mgr;
    m_level = a2;
    m_scaleA = a3;
    m_scaleB = a5;
    m_panIndex = 0;
    m_isPlaying = 0;
    AmbientBox* p = &m_box1;
    if (box != 0) {
        *p = *box;
    } else {
        p->left = (i32)0x80000000;
    }
    if (p->left == 0 && m_box1.top == 0 && m_box1.right == 0 && m_box1.bottom == 0) {
        p->left = (i32)0x80000000;
    }
    m_box2.left = (i32)0x80000000;
    return 1;
}

// ---------------------------------------------------------------------------
// CSoundChannel::Recompute (0x00bf10): per-channel volume recompute, invoked by
// CWorldSoundSet::Restart for each live channel. Skip when the frame is unchanged
// from last time; otherwise scale the frame through m_level (with the >5 -> -0xf
// curve), apply the secondary multiplier m_multiplier (signed /100 by the 0x51eb851f
// reciprocal each step), clamp to 0..100 and push it to the voice via SetVolByIdx.
// The level-scale math is the CAmbientSound::SetLevel idiom with frame/m_level
// transposed and an unconditional voice drive.
//
// @early-stop
// regalloc-coinflip wall (~97.9%) - logic complete, all relocs paired. retail pins
// the `frame` arg in eax (dead m_lastFrame -> edx); our cl does the reverse (frame in edx),
// which permutes the first ~5 instrs (cmp modrm, the m_lastFrame store reg, add eax,-0xf vs
// sub edx,0xf). The compare-operand-order lever did not flip the pin. See
// docs/patterns/zero-register-pinning.md.
RVA(0x0000bf10, 0x72)
void CSoundChannel::Recompute(i32 frame) {
    if (frame == m_lastFrame) {
        return;
    }
    i32 mult = m_level;
    m_lastFrame = frame;
    if (frame > 5) {
        frame -= 0xf;
    }
    i32 v = (frame * mult) / 100;
    if (m_multiplier > 0) {
        v = (v * m_multiplier) / 100;
    }
    if (v < 0) {
        v = 0;
    } else if (v > 0x64) {
        v = 0x64;
    }
    m_voice->SetVolumeByIndex(v);
}

// ===========================================================================
// CAmbientSound::Restart  (0x00bfb0)
// ===========================================================================
// Re-arm the voice at its current level. Gated on the voice handle, the not-yet-
// playing flag (m_isPlaying==0) and the active level/world (g_gameReg->m_soundEnabled and
// ->m_54->m_objectCount). Reseed the channel, then inline SetLevel(m_level, 0, 0)'s scale+
// clamp through SetVolumeByIndex; the level read (m_level) is re-stored unchanged on
// both sides of the voice call (the reseeded channel may have touched it).
RVA(0x0000bfb0, 0xa9)
void CAmbientSound::Restart() {
    DirectSoundMgr* voice = m_voice;
    i32 pos = m_level;
    if (voice == 0) {
        return;
    }
    if (m_isPlaying != 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    if (g_gameReg->m_inputState->m_active == 0) {
        return;
    }
    m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
    m_level = pos;
    i32 scale = m_scaleA;
    if (scale > 5) {
        scale -= 0xf;
    }
    i32 v = (scale * pos) / 100;
    if (m_scaleB > 0) {
        v = (v * m_scaleB) / 100;
    }
    if (v < 0) {
        v = 0;
    } else if (v > 0x64) {
        v = 0x64;
    }
    m_voice->SetVolumeByIndex(v);
    m_level = pos;
    m_isPlaying = 1;
}

// ===========================================================================
// CAmbientSound::Update  (0x00c090)
// ===========================================================================
// Per-frame driver (CAmbientSound vtable slot 3). If the source is bounded, test
// whether the listener (x,y) sits inside either audible rectangle; if unbounded
// it is always "in range". When already playing, keep going while in range and
// fade out when it leaves. When silent and in range, (re)start: with `force` set,
// fully arm the channel and push it to full level; otherwise fade it in.
//
// @early-stop
// Tail-merge wall (~77%): retail folds the two identical (re)start tails - the
// unbounded path's and the bounded `force` path's - into ONE block reached by an
// unconditional `jmp`, and the merge drags a dead `g_gameReg->m_inputState->m_active` probe
// into the unbounded path. Our cl emits the tail TWICE (and DCEs the unused m_24
// load), so the back half re-permutes. The bounded hit-test + the shared back
// half are byte-exact; only the duplicate-vs-shared tail + a couple of regalloc
// picks (edx/eax for the m_54 walk) differ. See identical-return-epilogue-
// tailmerge.md. Logic exact.
RVA(0x0000c090, 0x118)
void CAmbientSound::Update(i32 x, i32 y, i32 force) {
    i32 inRange;
    if (m_box1.left == AMBIENT_BOX_UNBOUNDED) {
        // Unbounded source: nothing to do while already playing.
        if (m_isPlaying != 0) {
            return;
        }
        DirectSoundMgr* voice = m_voice;
        i32 lvl = m_level;
        if (voice == 0) {
            return;
        }
        if (lvl == 0) {
            return;
        }
        if (g_gameReg->m_soundEnabled == 0) {
            return;
        }
        // Retail also probes g_gameReg->m_inputState->m_active here, then (re)starts
        // regardless; our cl DCEs that unused load (tail-merge wall, see below).
        if (m_voice == 0) {
            return;
        }
        m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
        SetLevel(0x64, 0, 0);
        m_level = 0x64;
        m_isPlaying = 1;
        return;
    }

    if (x > m_box1.left && x < m_box1.right && y > m_box1.top && y < m_box1.bottom) {
        inRange = 1;
    } else if (m_box2.left != AMBIENT_BOX_UNBOUNDED && x > m_box2.left && x < m_box2.right
               && y > m_box2.top && y < m_box2.bottom) {
        inRange = 1;
    } else {
        inRange = 0;
    }

    if (m_isPlaying != 0) {
        // Currently playing: keep running while in range, fade out otherwise.
        if (inRange != 0) {
            return;
        }
        Fade(0, 0, 0x3e8);
        return;
    }

    // Silent: only start when in range and the audio path is live.
    if (inRange == 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0 || g_gameReg->m_inputState->m_active == 0) {
        return;
    }
    if (force != 0) {
        if (m_voice == 0) {
            return;
        }
        m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
        SetLevel(0x64, 0, 0);
        m_level = 0x64;
        m_isPlaying = 1;
    } else {
        Fade(1, 0x64, 0x3e8);
    }
}

// ===========================================================================
// CAmbientSound::SetLevel  (0x00c200)
// ===========================================================================
// Scale `value` through the level scale-A (m_scaleA) and the secondary multiplier
// (m_scaleB), clamp to 0..100, then drive the voice: mode 0 -> SetVolumeByIndex, else
// CloneAndPlay carrying the `extra` arg.
RVA(0x0000c200, 0x7e)
i32 CAmbientSound::SetLevel(i32 value, i32 mode, i32 extra) {
    m_level = value;
    i32 scale = m_scaleA;
    if (scale > 5) {
        scale -= 0xf;
    }
    i32 v = (scale * value) / 100;
    if (m_scaleB > 0) {
        v = (v * m_scaleB) / 100;
    }
    if (v < 0) {
        v = 0;
    } else if (v > 0x64) {
        v = 0x64;
    }
    if (mode == 0) {
        return m_voice->SetVolumeByIndex(v);
    }
    return m_voice->CloneAndPlay(v, mode, extra);
}

// ---------------------------------------------------------------------------
// CRandomAmbientSound::Update (0x00c2a0, __thiscall, 3 args playFlag/pos/kind):
// the play/stop driver. Gated on the mgr handle, the playing flag, and the active
// level (g_gameReg->m_soundEnabled and g_gameReg->m_inputState->m_active). On play it
// reseeds the voice (ApplyAndPlay(1,m_panIndex,0,1)), scales pos by (m_scaleA
// clamped)/100 then m_scaleB/100 (both signed magic-/100), clamps the result to
// [0,100], and dispatches SetVolumeByIndex (kind==0) or CloneAndPlay (kind!=0);
// on stop it StopAndRewind's (kind==0) or CloneAndPlay-stops (kind!=0).
// ---------------------------------------------------------------------------
// @early-stop
// 3-push frame + twin signed-/100 magic-division scheduling wall (logic complete,
// all relocs paired). cl duplicates the scale-and-clamp block per kind branch (as
// retail does) but permutes the eax/ecx/edx use across the two 0x51eb851f reductions
// and the SetVolumeByIndex/CloneAndPlay tail; no source spelling pins that schedule.
// See zero-register-pinning.md and CGruntSpawnConfig::PickWeighted (the /100 family).
RVA(0x0000c2a0, 0x19e)
void CRandomAmbientSound::Update(i32 playFlag, i32 pos, i32 kind) {
    if (m_voice == 0) {
        return;
    }
    if (playFlag == 0) {
        // Stop path.
        if (m_isPlaying == 0) {
            return;
        }
        if (kind != 0) {
            m_level = 0;
            m_voice->CloneAndPlay(0, kind, 1);
            m_isPlaying = 0;
            return;
        }
        m_voice->StopAndRewind();
        m_isPlaying = 0;
        return;
    }
    if (m_isPlaying != 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    if (g_gameReg->m_inputState->m_active == 0) {
        return;
    }

    if (kind != 0) {
        m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
        i32 t = m_scaleA;
        m_level = pos;
        if (t > 5) {
            t -= 0xf;
        }
        i32 v = (t * pos) / 100;
        if (m_scaleB > 0) {
            v = (v * m_scaleB) / 100;
        }
        if (v < 0) {
            v = 0;
        } else if (v > 0x64) {
            v = 0x64;
        }
        m_voice->CloneAndPlay(v, kind, 0);
        m_level = pos;
        m_isPlaying = 1;
        return;
    }

    m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
    i32 t = m_scaleA;
    m_level = pos;
    if (t > 5) {
        t -= 0xf;
    }
    i32 v = (t * pos) / 100;
    if (m_scaleB > 0) {
        v = (v * m_scaleB) / 100;
    }
    if (v < 0) {
        m_voice->SetVolumeByIndex(0);
        m_level = pos;
        m_isPlaying = 1;
        return;
    }
    if (v > 0x64) {
        v = 0x64;
    }
    m_voice->SetVolumeByIndex(v);
    m_level = pos;
    m_isPlaying = 1;
}

// ---------------------------------------------------------------------------
// SetupFromMap (0x00c4b0, __thiscall, 5 args): resolve the mgr record for `key`
// out of holder->m_map (a CMapPtrToPtr); when found, seed this object via
// SetupPos(record->m_mgr, a3, a4, pos, a5). No-op when the key is absent.
// ---------------------------------------------------------------------------
// @early-stop
// CODE BYTE-EXACT - residual is the reloc-naming scoring artifact: retail's two
// calls go through ILT thunks (Lookup / thunk_FUN_0040c530) while our base names
// the callee directly, so objdiff scores the REL32 operands against
// differently-named symbols (~88.75%). Every instruction byte matches the
// delinked target (verified by base-vs-target objdump). Effectively matched.
RVA(0x0000c4b0, 0x53)
void CRandomAmbientSound::SetupFromMap(
    AmbSoundMapHolder* holder,
    void* key,
    i32 a3,
    i32 a4,
    AmbientPoint* pos,
    i32 a5
) {
    void* found = 0;
    holder->m_map.Lookup(key, &found);
    if (found != 0) {
        SetupPos(((AmbSoundRecord*)found)->m_mgr, a3, a4, pos, a5);
    }
}

// ---------------------------------------------------------------------------
// SetupPos (0x00c530, __thiscall, 5 args): the positional Setup. Refuse a null
// mgr or null position; otherwise stash the mgr + the two play params + a5,
// clear the playing flag (+0x14) and m_panIndex, and copy the (x,y) anchor into
// m_40/m_44. Returns 1, or 0 on either null guard.
// ---------------------------------------------------------------------------
RVA(0x0000c530, 0x51)
i32 CRandomAmbientSound::SetupPos(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientPoint* pos, i32 a5) {
    if (mgr == 0) {
        return 0;
    }
    if (pos == 0) {
        return 0;
    }
    m_voice = mgr;
    m_level = a2;
    m_scaleA = a3;
    m_panIndex = 0;
    m_scaleB = a5;
    m_isPlaying = 0;
    m_40 = pos->x;
    m_44 = pos->y;
    return 1;
}

// ---------------------------------------------------------------------------
// UpdateAt (0x00c5b0, __thiscall, 3 args x/y/force): the positional play driver.
// Compute the listener->anchor distance (|m_40-x|, |m_44-y|); if either axis is
// past 0x280 stop the voice. Otherwise derive a falloff volume (100 - dist/3,
// clamped) and a pan (dx/4, clamped, signed by which side of m_40 the listener
// is), scale the volume by m_scaleA/100 then m_scaleB/100, set volume + pan; and when not
// already playing (and the active level is live) reseed and re-set the volume,
// marking the voice playing.
// ---------------------------------------------------------------------------
// @early-stop
// twin signed-/100 magic-division scheduling wall (logic complete, all relocs
// paired) - the SAME family as the sibling Update (0x00c2a0): cl duplicates the
// scale-and-clamp block (as retail does) but permutes eax/ecx/edx across the two
// 0x51eb851f reductions and the SetVolByIdx/SetPanByIdx tail; no source spelling
// pins that schedule. See zero-register-pinning.md and the /100 family.
RVA(0x0000c5b0, 0x1df)
void CRandomAmbientSound::UpdateAt(i32 x, i32 y, i32 force) {
    i32 ax = m_40 - x;
    i32 dx = ax < 0 ? -ax : ax;
    i32 ay = m_44 - y;
    i32 dy = ay < 0 ? -ay : ay;
    i32 dist2 = dx * dx + dy * dy;
    if (dx > 0x280 || dy > 0x280) {
        if (m_voice != 0 && m_isPlaying != 0) {
            m_voice->StopAndRewind();
            m_isPlaying = 0;
        }
        return;
    }

    i32 dist = __ftol(sqrt((double)dist2));
    i32 vol = 0x64 - dist / 3;
    if (vol > 0x64) {
        vol = 0x64;
    } else if (vol < 0) {
        vol = 0;
    }
    i32 pan = dx / 4;
    if (pan > 0x64) {
        pan = 0x64;
    } else if (pan < 0) {
        pan = 0;
    }
    if (m_40 < x) {
        pan = -pan;
    }

    {
        i32 t = m_scaleA;
        m_level = vol;
        if (t > 5) {
            t -= 0xf;
        }
        i32 v = (t * vol) / 100;
        if (m_scaleB > 0) {
            v = (v * m_scaleB) / 100;
        }
        if (v < 0) {
            v = 0;
        } else if (v > 0x64) {
            v = 0x64;
        }
        m_voice->SetVolumeByIndex(v);
    }
    m_panIndex = pan;
    m_voice->SetPanByIndex(pan);

    if (m_isPlaying != 0) {
        return;
    }
    if (m_voice == 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    if (g_gameReg->m_inputState->m_active == 0) {
        return;
    }
    m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
    {
        i32 t = m_scaleA;
        m_level = vol;
        if (t > 5) {
            t -= 0xf;
        }
        i32 v = (t * vol) / 100;
        if (m_scaleB > 0) {
            v = (v * m_scaleB) / 100;
        }
        if (v < 0) {
            v = 0;
        } else if (v > 0x64) {
            v = 0x64;
        }
        m_voice->SetVolumeByIndex(v);
    }
    m_level = vol;
    m_isPlaying = 1;
}

// ---------------------------------------------------------------------------
// CommitSpriteAction (0x0000c840, __cdecl) - a sibling of SpawnPosSound in the
// positional-sound spawn path (re-homed from src/Stub/ApiCallers.cpp). On a fresh
// spawn request (aux->m_requestState == 0) it stamps the object's placed/spawn flag
// bits, resolves the handler-vs-default flag, and - when the layer and the input mgr
// are live - emits the sound-sprite into the active layer through the world sound-set
// factory (full vs simple by the +0x138 extent), copies the placed rect back, then
// latches the request "spawned" (5). Returns 1.
// @early-stop
// arg-load scheduling wall (~94%): body byte-exact through the flag math and both
// exits; the residual is MSVC's just-in-time vs pre-load interleaving of the factory
// member-arg loads (same push order, same args) + the g_gameReg->m_inputState test
// landing in eax vs retail's ecx. Same instructions, different temp-register rotation.
struct PosSoundPlaced { // the create-helper return record; its placed RECT is at +0x28
    char m_pad0[0x28];
    RECT m_28; // +0x28
};
SIZE_UNKNOWN(PosSoundPlaced);
extern "C" void DefaultActionHandler_2d15(); // LAB_00402d15 (address only)
// The world sound-set factory create calls (CWorldSoundSet::CreateRandom @0xbb60 via
// thunk 0x3c97 / CreateAmbient5 @0xb7b0 via thunk 0x2ad6); modeled __stdcall exactly
// like the sibling PosSoundSpawn - the factory ptr (layer +0x10) leads the arg list.
PosSoundPlaced* __stdcall WorldSoundCreateFull(
    void* factory,
    i32 z,
    RECT* rc,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e,
    i32 f
); // 0xbb60
PosSoundPlaced* __stdcall
WorldSoundCreateSimple(void* factory, i32 z, RECT* rc, i32 a, i32 b); // 0xb7b0
RVA(0x0000c840, 0x13d)
i32 CommitSpriteAction(PosSoundObj* obj) {
    PosSoundAux* aux = obj->m_aux;
    if (aux->m_requestState == 0) {
        obj->m_flags08 |= 1;
        obj->m_flags40 |= 1;
        if (aux->m_handler == (void*)DefaultActionHandler_2d15) {
            obj->m_flags08 |= 2;
        } else {
            obj->m_flags08 &= ~2;
        }
        void* layer = obj->m_layer;
        if (layer && g_gameReg) {
            RECT rc;
            CopyRect(&rc, &obj->m_area);
            if (aux->m_srcL > 0 || aux->m_srcR > 0) {
                SetRect(&rc, aux->m_srcL, aux->m_srcT, aux->m_srcR, aux->m_srcB);
            }
            if (g_gameReg->m_inputState) {
                PosSoundPlaced* placed;
                if (obj->m_extentT > 0) {
                    placed = WorldSoundCreateFull(
                        *(void**)((char*)layer + 0x10),
                        0x64,
                        &rc,
                        obj->m_120,
                        obj->m_extentL,
                        obj->m_extentT,
                        obj->m_extentR,
                        obj->m_extentB,
                        0
                    );
                } else {
                    placed = WorldSoundCreateSimple(
                        *(void**)((char*)layer + 0x10),
                        0x64,
                        &rc,
                        obj->m_120,
                        0
                    );
                }
                if (placed && obj->m_placed.top > 0) {
                    placed->m_28 = obj->m_placed;
                }
            }
        }
        obj->m_flags08 |= 0x10000;
        aux->m_requestState = 5;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// StopPosSound (0x00c9d0): mark the request "stop" (state 2 in the global queue
// at 0x62990c) and run the spawn/stop driver.
// ---------------------------------------------------------------------------
RVA(0x0000c9d0, 0x18)
void StopPosSound(PosSoundObj* obj) {
    g_posSoundReq = 2;
    SpawnPosSound(obj);
}

// ---------------------------------------------------------------------------
// SpawnPosSound (0x00ca00): per-object placement tick. On a "spawn" request
// (aux->m_requestState == 0) stamp the object flags and, if its layer + the active world
// are live, new a voice through the factory; on a "stop" request (0x1e) tear the
// live voice down (StopAndRewind, unlink from the spatial mgr, scalar-dtor it).
// ---------------------------------------------------------------------------
// @early-stop
// out-param stack struct + virtual scalar-dtor dispatch + factory calling-conv:
// logic complete, but the spawn path passes obj->m_x/m_y through a 2-int stack
// out-param (the `sub esp,8` slots, address-escaped to the factory) whose exact
// [esp+N] schedule and the factory's callee-clean shape are not source-steerable.
// The teardown arm (StopAndRewind / RemoveAt / `call [vptr]`) is byte-exact.
RVA(0x0000ca00, 0xf0)
void SpawnPosSound(PosSoundObj* obj) {
    PosSoundAux* aux = obj->m_aux;
    i32 state = aux->m_requestState;
    if (state != 0) {
        if (state != 0x1e) {
            return;
        }
        PosSoundVoice* sound = aux->m_voice;
        if (sound == 0) {
            return;
        }
        CPtrList* arr = (CPtrList*)((char*)g_gameReg->m_inputState + 8);
        if (sound->m_mgr != 0) {
            sound->m_mgr->StopAndRewind();
            sound->m_isPlaying = 0;
        }
        if (sound->m_spatialNode != 0) {
            arr->RemoveAt((POSITION)sound->m_spatialNode);
            delete sound;
        }
        aux->m_voice = 0;
        aux->m_requestState = 0;
        return;
    }

    obj->m_flags08 = (obj->m_flags08 & ~2) | 0x100001;
    obj->m_flags40 |= 1;
    aux->m_voice = 0;
    void* layer = obj->m_layer;
    if (layer != 0 && g_gameReg != 0 && g_gameReg->m_inputState != 0) {
        i32 pt[2];
        pt[0] = obj->m_x;
        pt[1] = obj->m_y;
        void* v = PosSoundSpawn(*(void**)((char*)layer + 0x10), 0x64, &pt, obj->m_120, 0);
        if (v != 0) {
            aux->m_voice = (PosSoundVoice*)v;
        }
    }
    aux->m_requestState = 5;
}

// ---------------------------------------------------------------------------
// CRandomAmbientSound::Step (0x00cb30, __thiscall, 3 args x/y/force): test the
// listener position against the two visibility boxes; if it left both (and we are
// playing) stop the voice. Otherwise drain the rolled countdown by the frame
// delta, and on expiry flip the roller phase, roll a fresh interval over the
// active phase's [lo,hi], halve+clamp it to <=1000, and (re)play via Update.
// ---------------------------------------------------------------------------
// @early-stop
// rand()-call + idiv-scheduling wall (~70%, logic complete, all relocs paired).
// The two structurally-identical reroll arms (phase A over m_40..m_44, phase B
// over m_48..m_4c) each emit the global rand call twice (span==0 coin-flip vs
// idiv-by-span) and cl interleaves the span test / idiv / the +0x50 store with the
// loop-head box compares; no single source spelling pins that interleave. The box
// in/out test + the countdown drain are byte-exact. See zero-register-pinning.md
// and CGruntSpawnConfig::PickWeighted (the same rand-inline/idiv family).
RVA(0x0000cb30, 0x168)
void CRandomAmbientSound::Step(i32 x, i32 y, i32 force) {
    i32 inBox = 0;
    i32 b1 = m_box1.left;
    if (b1 == (i32)0x80000000) {
        inBox = 1;
    } else if (x <= b1 || x >= m_box1.right || y <= m_box1.top || y >= m_box1.bottom) {
        i32 b2 = m_box2.left;
        if (b2 == (i32)0x80000000 || x <= b2 || x >= m_box2.right || y <= m_box2.top
            || y >= m_box2.bottom) {
            inBox = 1;
        }
    }

    if (inBox == 0) {
        if (m_isPlaying != 0 && m_voice != 0) {
            Update(0, 0x3e8, 1);
            m_isPlaying = 0;
        }
        m_phase = 0;
        return;
    }

    if (force != 0 && m_phase != 0 && m_isPlaying != 0) {
        return;
    }

    if ((u32)m_countdownMs <= (u32)g_tickDelta) {
        m_countdownMs = 0;
    } else {
        m_countdownMs = m_countdownMs - g_tickDelta;
    }
    if (m_countdownMs != 0) {
        return;
    }

    m_phase ^= 1;
    if (m_phase != 0) {
        i32 lo = m_40;
        i32 hi = m_44;
        i32 span = hi - lo + 1;
        i32 r;
        if (span == 0) {
            r = (winapi_00cd00_timeGetTime() & 1) ? lo : hi;
        } else {
            r = winapi_00cd00_timeGetTime() % span + lo;
        }
        m_countdownMs = r;
        i32 half = r >> 1;
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Update(1, 0x64, half);
    } else {
        i32 lo = m_intervalLoB;
        i32 hi = m_intervalHiB;
        i32 span = hi - lo + 1;
        i32 r;
        if (span == 0) {
            r = (winapi_00cd00_timeGetTime() & 1) ? lo : hi;
        } else {
            r = winapi_00cd00_timeGetTime() % span + lo;
        }
        m_countdownMs = r;
        i32 half = r >> 1;
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Update(0, 0x64, half);
    }
}

// ---------------------------------------------------------------------------
// ~CWorldSoundSet (0x085ed0, out-of-band stray - RVA-last in this file):
// deactivate (sibling 0x00b620), then the embedded list's destructor fires from
// the epilogue. The destructible list member forces the /GX EH frame (state 0
// across Deactivate, -1 across ~list).
// ---------------------------------------------------------------------------
RVA(0x00085ed0, 0x4a)
CWorldSoundSet::~CWorldSoundSet() {
    Deactivate();
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
// (CAmbientSound/CAmbientPosSound/CRandomAmbientSound carry SIZE/VTBL in their
// canonical headers.)
SIZE_UNKNOWN(AmbSoundMap);
SIZE_UNKNOWN(AmbSoundMapHolder);
SIZE_UNKNOWN(AmbSoundRecord);
SIZE_UNKNOWN(AmbientPoint);
SIZE_UNKNOWN(PosSoundAux);
SIZE_UNKNOWN(PosSoundObj);
SIZE_UNKNOWN(PosSoundVoice);
SIZE_UNKNOWN(CRandomAmbientWorld);
SIZE_UNKNOWN(CSoundChannel);
SIZE_UNKNOWN(CSoundNode);
SIZE_UNKNOWN(CWorldSoundSet);
