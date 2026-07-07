// WorldSoundSet.cpp - a trace-discovered Gruntz ambient-sound game object
// (C:\Proj\Gruntz). It manages a list of live sound channels: Init seeds the
// world/level back-pointers and activates the object; Restart/Stop/Resume/Retune
// drive the channels (each walks the embedded CPtrList raw - node->next at +0x00,
// the channel payload at node+0x08) and poke the world's DirectSound sub-object;
// Teardown / the destructor scalar-delete every channel and RemoveAll the list.
//
// All engine callees are reloc-masked __thiscall externs:
//   SoundDevice::FreeSamples (0x136ed0) / PurgeVoiceList (0x136e20),
//   DirectSoundMgr::StopAndRewind (0x135380) / SetVolumeByIndex (0x1355c0),
//   CSoundChannel scalar-dtor (vtbl slot 0) / Retune (vtbl slot 3) / Recompute
//   (0x00bf10), CSoundChannelList::RemoveAll (0x1b48a6) / ~ (0x1b48c6), and the
//   sibling Deactivate (0x00b620, via the ILT thunk).
//
// Field names are placeholders; the OFFSETS + emitted code bytes are load-bearing.
#include <Gruntz/WorldSoundSet.h>
#include <Rez/RezMgr.h> // RezAlloc - the engine heap allocator (reloc-masked)
#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserBase (real base of CAmbientSound)

// ALL-VTABLES mandate: the three channel classes are REAL polymorphic classes.
// RTTI derivation CAmbientPosSound / CRandomAmbientSound : CAmbientSound (: CUserBase)
// is modeled by C++ inheritance so the redundant slot-1/2 re-decls fold into the
// base; each is built via placement `new (raw) CXxx`, so cl auto-emits
// ??_7CAmbientSound / ??_7CAmbientPosSound / ??_7CRandomAmbientSound (mapped in
// config/vtable_names.csv at 0x1e710c / 0x1e7124 / 0x1e713c) and inlines the vptr
// stamp in the ctor. Slot 0 is retail's compiler-gen scalar deleting destructor,
// slot 3 is Update; see the ScalarDtor note below for why slot 0 is hand-named.
inline void* operator new(u32, void* p) {
    return p;
}

struct CAmbientSound : public CUserBase {
    // slot 0 is retail's COMPILER-GENERATED scalar deleting destructor; hand-named
    // here (not a real `~CAmbientSound()`) on purpose: a user-declared virtual dtor
    // makes MSVC5 emit the placement-`new` ctor OUT-OF-LINE (a `call ??0CAmbient*`)
    // instead of the inlined vptr-stamp retail schedules at each CreateXxx site,
    // regressing CreatePos6/CreatePos5/CreateRandom ~96%->~87%. So the direct slot-0
    // call (delete-obj shape) stays `delete obj`, an exact match.
    virtual ~CAmbientSound() OVERRIDE;                                 // slot 0 (compiler-gen ??_G)
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual void Update(i32 x, i32 y, i32 force);                      // slot 3 (retail 0xc090)
    CAmbientSound() {}
    // +0x00 vptr (??_7CAmbientSound)
    i32 m_voice; // +0x04
    i32 m_level; // +0x08
    char m_pad0c[0x14 - 0x0c];
    i32 m_14; // +0x14
    char m_pad18[0x3c - 0x18];
    i32 m_listNode; // +0x3c
    i32 Init6(void* world, i32 a1, i32 a2, void* a3, i32 a4, i32 a5);
    i32 Init5(i32 a0, i32 a1, void* a2, i32 a3, i32 a4);
};

// RTTI: CAmbientPosSound : CAmbientSound (vftable 0x1e7124). Overrides slot 0
// (scalar dtor) + slot 3; slots 1/2 inherit CUserBase's.
struct CAmbientPosSound : CAmbientSound {
    virtual ~CAmbientPosSound() OVERRIDE;                              // slot 0
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual void Update(i32 x, i32 y, i32 force) OVERRIDE;             // slot 3 override
    CAmbientPosSound() {}
    // m_voice/m_level/m_14/m_listNode inherited from CAmbientSound
    i32 Init6(void* world, i32 a1, i32 a2, void* a3, i32 a4, i32 a5);
    i32 Init5(i32 a0, i32 a1, void* a2, i32 a3, i32 a4);
};

// RTTI: CRandomAmbientSound : CAmbientSound (vftable 0x1e713c). Overrides slot 0
// (scalar dtor) + slot 3; slots 1/2 inherit CUserBase's.
struct CRandomAmbientSound : CAmbientSound {
    virtual ~CRandomAmbientSound() OVERRIDE;                           // slot 0
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual void Update(i32 x, i32 y, i32 force) OVERRIDE;             // slot 3 override
    CRandomAmbientSound() {}
    // m_voice/m_level/m_14/m_listNode inherited from CAmbientSound
    i32 Init5(i32 a0, i32 a1, void* a2, i32 a3, i32 a4);
    void Init2(i32 a0, i32 a1, i32 a2, i32 a3);
};

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
// Teardown: scalar-delete every channel in the list (vtbl slot 0, flag 1), then
// RemoveAll the now-dangling list.
// ---------------------------------------------------------------------------
RVA(0x0000b660, 0x2b)
void CWorldSoundSet::Teardown() {
    CSoundNode* node = m_list.m_head;
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
    CSoundNode* node = m_list.m_head;
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
    CSoundNode* node = m_list.m_head;
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
    CSoundNode* node = m_list.m_head;
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
    CSoundNode* node = m_list.m_head;
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

// ---------------------------------------------------------------------------
// ~CWorldSoundSet: deactivate (sibling 0x00b620), then the embedded list's
// destructor fires from the epilogue. The destructible list member forces the /GX
// EH frame (state 0 across Deactivate, -1 across ~list).
// ---------------------------------------------------------------------------
RVA(0x00085ed0, 0x4a)
CWorldSoundSet::~CWorldSoundSet() {
    Deactivate();
}

// ===========================================================================
// Create* factories (rtti-vptr mislabeled them onto the channel classes whose
// vtable they stamp; the `this` is this owner). Each allocates a fresh channel,
// seeds its level fields (m_level = 100 default), stamps its retail vtable, runs the
// one-time Init; on failure scalar-deletes it and returns 0, on success appends
// it to m_list (storing the list node in m_listNode) and returns it.
// ===========================================================================

// CAmbientSound (0x40), 6-arg Init (m_world + the owner's m_04 threaded in).
RVA(0x0000b6a0, 0x83)
SoundChannelNew* CWorldSoundSet::CreateAmbient6_b6a0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    void* raw = RezAlloc(0x40);
    CAmbientSound* obj;
    if (raw != 0) {
        obj = new (raw) CAmbientSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_14 = 0;
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
    obj->m_listNode = (i32)m_list.AddTail(obj);
    return (SoundChannelNew*)obj;
}

// CAmbientSound (0x40), 5-arg Init (no m_world).
RVA(0x0000b7b0, 0x80)
SoundChannelNew* CWorldSoundSet::CreateAmbient5_b7b0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    void* raw = RezAlloc(0x40);
    CAmbientSound* obj;
    if (raw != 0) {
        obj = new (raw) CAmbientSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_14 = 0;
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
    obj->m_listNode = (i32)m_list.AddTail(obj);
    return (SoundChannelNew*)obj;
}

// CAmbientPosSound (0x48), 6-arg Init (vtable stamped last).
RVA(0x0000b850, 0x83)
SoundChannelNew* CWorldSoundSet::CreatePos6_b850(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    void* raw = RezAlloc(0x48);
    CAmbientPosSound* obj;
    if (raw != 0) {
        obj = new (raw) CAmbientPosSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_14 = 0;
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
    obj->m_listNode = (i32)m_list.AddTail(obj);
    return (SoundChannelNew*)obj;
}

// CAmbientPosSound (0x48), 5-arg Init.
RVA(0x0000b960, 0x80)
SoundChannelNew* CWorldSoundSet::CreatePos5_b960(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    void* raw = RezAlloc(0x48);
    CAmbientPosSound* obj;
    if (raw != 0) {
        obj = new (raw) CAmbientPosSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_14 = 0;
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
    obj->m_listNode = (i32)m_list.AddTail(obj);
    return (SoundChannelNew*)obj;
}

// CRandomAmbientSound (0x58): 5-arg Init then an ungated 4-arg Init2.
RVA(0x0000bb60, 0x9b)
SoundChannelNew* CWorldSoundSet::
    CreateRandom_bb60(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8) {
    void* raw = RezAlloc(0x58);
    CRandomAmbientSound* obj;
    if (raw != 0) {
        obj = new (raw) CRandomAmbientSound;
        obj->m_voice = 0;
        obj->m_level = 0x64;
        obj->m_14 = 0;
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
    obj->m_listNode = (i32)m_list.AddTail(obj);
    return (SoundChannelNew*)obj;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CAmbientSound);
SIZE_UNKNOWN(CAmbientPosSound);
VTBL(CAmbientPosSound, 0x001e7124); // vtable_names -> code (RTTI game class)
SIZE_UNKNOWN(CRandomAmbientSound);
SIZE_UNKNOWN(CRandomAmbientWorld);
SIZE_UNKNOWN(CSoundChannel);
SIZE_UNKNOWN(CSoundChannelList);
SIZE_UNKNOWN(CSoundNode);
SIZE_UNKNOWN(CWorldSoundSet);
SIZE_UNKNOWN(SoundChannelNew);
