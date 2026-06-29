// CWorldSoundSet.cpp - a trace-discovered Gruntz ambient-sound game object
// (C:\Proj\Gruntz). It manages a list of live sound channels: Init seeds the
// world/level back-pointers and activates the object; Restart/Stop/Resume/Retune
// drive the channels (each walks the embedded CPtrList raw - node->next at +0x00,
// the channel payload at node+0x08) and poke the world's DirectSound sub-object;
// Teardown / the destructor scalar-delete every channel and RemoveAll the list.
//
// All engine callees are reloc-masked __thiscall externs:
//   MinervaInner::MinervaInner_136ed0 (0x136ed0), DirectSoundMgr::StopAndRewind
//   (0x135380) / winapi_136e20_timeGetTime (0x136e20),
//   CSoundChannel scalar-dtor (vtbl slot 0) / Retune (vtbl slot 3) / Recompute
//   (0x00bf10), CSoundChannelList::RemoveAll (0x1b48a6) / ~ (0x1b48c6), and the
//   sibling Deactivate (0x00b620, via the ILT thunk).
//
// Field names are placeholders; the OFFSETS + emitted code bytes are load-bearing.
#include <Gruntz/CWorldSoundSet.h>
#include <Rez/RezMgr.h> // RezAlloc - the engine heap allocator (reloc-masked)
#include <rva.h>

// The created channels' retail vtables, stamped by address into each fresh
// allocation (reloc-masked DIR32 store) - the transitional vtable workaround,
// since each class's virtuals live in another TU and are not all matched.
DATA(0x001e710c)
extern void* g_ambientSoundVtbl; // CAmbientSound        0x5e710c
DATA(0x001e7124)
extern void* g_posSoundVtbl; // CAmbientPosSound    0x5e7124
DATA(0x001e713c)
extern void* g_randomSoundVtbl; // CRandomAmbientSound 0x5e713c

// ---------------------------------------------------------------------------
// Init: refuse a null world, otherwise stash both back-pointers, mark active and
// clear the pending pan/volume. Returns 1 on success, 0 on the null guard.
// ---------------------------------------------------------------------------
RVA(0x0000b5e0, 0x29)
i32 CWorldSoundSet::Init(void* world, void* a2) {
    if (world == 0) {
        return 0;
    }
    m_world = (CRandomAmbientWorld*)world;
    m_04 = a2;
    m_24 = 1;
    m_28 = 0;
    m_2c = 0;
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
            ch->ScalarDtor(1);
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
    if (m_world->m_2c != 0) {
        ((MinervaInner*)m_world->m_2c)->MinervaInner_136ed0();
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
    if (m_world != 0 && m_world->m_2c != 0) {
        ((MinervaInner*)m_world->m_2c)->MinervaInner_136ed0();
    }
    CSoundNode* node = m_list.m_head;
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CSoundChannel* ch = cur->m_data;
        if (ch != 0 && ch->m_04 != 0) {
            ch->m_04->StopAndRewind();
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
            ch->Retune(m_28, m_2c, 1);
        }
    }
    if (m_world->m_2c != 0) {
        m_world->m_2c->winapi_136e20_timeGetTime(-1);
    }
}

// ---------------------------------------------------------------------------
// Retune: record the new pan/vol, push them to every live channel (vtbl slot 3,
// flag 0), then rewind the world handle.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc/schedule-coinflip wall (~86.3%) - logic complete, all relocs paired.
// Structurally identical to Resume (matched body) but the two up-front member
// stores (m_28=pan, m_2c=vol) let cl hoist the loop-head load + null-test above
// the stores and pin `mov ebp,ecx` early, whereas retail keeps `this` in ecx
// until the stores and reads the head after - a pure register/schedule permutation
// (same bytes, reordered) plus the same dead-this tail load as Resume. The
// for-loop / store-order levers did not flip it. See zero-register-pinning.md.
RVA(0x0000bd60, 0x4b)
void CWorldSoundSet::Retune(i32 pan, i32 vol) {
    m_28 = pan;
    m_2c = vol;
    CSoundNode* node = m_list.m_head;
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CSoundChannel* ch = cur->m_data;
        if (ch != 0) {
            ch->Retune(pan, vol, 0);
        }
    }
    if (m_world->m_2c != 0) {
        m_world->m_2c->winapi_136e20_timeGetTime(-1);
    }
}

// ---------------------------------------------------------------------------
// CSoundChannel::Recompute (0x00bf10): per-channel volume recompute, invoked by
// CWorldSoundSet::Restart for each live channel. Skip when the frame is unchanged
// from last time; otherwise scale the frame through m_08 (with the >5 -> -0xf
// curve), apply the secondary multiplier m_10 (signed /100 by the 0x51eb851f
// reciprocal each step), clamp to 0..100 and push it to the voice via SetVolByIdx.
// The level-scale math is the CAmbientSound::SetLevel idiom with frame/m_08
// transposed and an unconditional voice drive.
//
// @early-stop
// regalloc-coinflip wall (~97.9%) - logic complete, all relocs paired. retail pins
// the `frame` arg in eax (dead m_0c -> edx); our cl does the reverse (frame in edx),
// which permutes the first ~5 instrs (cmp modrm, the m_0c store reg, add eax,-0xf vs
// sub edx,0xf). The compare-operand-order lever did not flip the pin. See
// docs/patterns/zero-register-pinning.md.
RVA(0x0000bf10, 0x72)
void CSoundChannel::Recompute(i32 frame) {
    if (frame == m_0c) {
        return;
    }
    i32 mult = m_08;
    m_0c = frame;
    if (frame > 5) {
        frame -= 0xf;
    }
    i32 v = (frame * mult) / 100;
    if (m_10 > 0) {
        v = (v * m_10) / 100;
    }
    if (v < 0) {
        v = 0;
    } else if (v > 0x64) {
        v = 0x64;
    }
    m_04->SetVolumeByIndex(v);
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
// seeds its level fields (m_08 = 100 default), stamps its retail vtable, runs the
// one-time Init; on failure scalar-deletes it and returns 0, on success appends
// it to m_list (storing the list node in m_3c) and returns it.
// ===========================================================================

// CAmbientSound (0x40), 6-arg Init (m_world + the owner's m_04 threaded in).
RVA(0x0000b6a0, 0x83)
SoundChannelNew* CWorldSoundSet::CreateAmbient6_b6a0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    char* raw = (char*)RezAlloc(0x40);
    SoundChannelNew* obj;
    if (raw != 0) {
        *(void**)raw = &g_ambientSoundVtbl;
        ((SoundChannelNew*)raw)->m_04 = 0;
        ((SoundChannelNew*)raw)->m_08 = 0x64;
        ((SoundChannelNew*)raw)->m_14 = 0;
        ((SoundChannelNew*)raw)->m_3c = 0;
        obj = (SoundChannelNew*)raw;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init6(m_world, a0, a1, m_04, a2, a3) == 0) {
        obj->ScalarDtor(1);
        return 0;
    }
    obj->m_3c = (i32)m_list.AddTail(obj);
    return obj;
}

// CAmbientSound (0x40), 5-arg Init (no m_world).
RVA(0x0000b7b0, 0x80)
SoundChannelNew* CWorldSoundSet::CreateAmbient5_b7b0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    char* raw = (char*)RezAlloc(0x40);
    SoundChannelNew* obj;
    if (raw != 0) {
        *(void**)raw = &g_ambientSoundVtbl;
        ((SoundChannelNew*)raw)->m_04 = 0;
        ((SoundChannelNew*)raw)->m_08 = 0x64;
        ((SoundChannelNew*)raw)->m_14 = 0;
        ((SoundChannelNew*)raw)->m_3c = 0;
        obj = (SoundChannelNew*)raw;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init5(a0, a1, m_04, a2, a3) == 0) {
        obj->ScalarDtor(1);
        return 0;
    }
    obj->m_3c = (i32)m_list.AddTail(obj);
    return obj;
}

// CAmbientPosSound (0x48), 6-arg Init (vtable stamped last).
RVA(0x0000b850, 0x83)
SoundChannelNew* CWorldSoundSet::CreatePos6_b850(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    char* raw = (char*)RezAlloc(0x48);
    SoundChannelNew* obj;
    if (raw != 0) {
        ((SoundChannelNew*)raw)->m_04 = 0;
        ((SoundChannelNew*)raw)->m_08 = 0x64;
        ((SoundChannelNew*)raw)->m_14 = 0;
        ((SoundChannelNew*)raw)->m_3c = 0;
        *(void**)raw = &g_posSoundVtbl;
        obj = (SoundChannelNew*)raw;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init6(m_world, a0, a1, m_04, a2, a3) == 0) {
        obj->ScalarDtor(1);
        return 0;
    }
    obj->m_3c = (i32)m_list.AddTail(obj);
    return obj;
}

// CAmbientPosSound (0x48), 5-arg Init.
RVA(0x0000b960, 0x80)
SoundChannelNew* CWorldSoundSet::CreatePos5_b960(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    char* raw = (char*)RezAlloc(0x48);
    SoundChannelNew* obj;
    if (raw != 0) {
        ((SoundChannelNew*)raw)->m_04 = 0;
        ((SoundChannelNew*)raw)->m_08 = 0x64;
        ((SoundChannelNew*)raw)->m_14 = 0;
        ((SoundChannelNew*)raw)->m_3c = 0;
        *(void**)raw = &g_posSoundVtbl;
        obj = (SoundChannelNew*)raw;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init5(a0, a1, m_04, a2, a3) == 0) {
        obj->ScalarDtor(1);
        return 0;
    }
    obj->m_3c = (i32)m_list.AddTail(obj);
    return obj;
}

// CRandomAmbientSound (0x58): 5-arg Init then an ungated 4-arg Init2.
RVA(0x0000bb60, 0x9b)
SoundChannelNew* CWorldSoundSet::
    CreateRandom_bb60(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8) {
    char* raw = (char*)RezAlloc(0x58);
    SoundChannelNew* obj;
    if (raw != 0) {
        ((SoundChannelNew*)raw)->m_04 = 0;
        ((SoundChannelNew*)raw)->m_08 = 0x64;
        ((SoundChannelNew*)raw)->m_14 = 0;
        ((SoundChannelNew*)raw)->m_3c = 0;
        *(void**)raw = &g_randomSoundVtbl;
        obj = (SoundChannelNew*)raw;
    } else {
        obj = 0;
    }
    if (obj == 0) {
        return 0;
    }
    if (obj->Init5(a0, a1, m_04, a2, a3) == 0) {
        obj->ScalarDtor(1);
        return 0;
    }
    obj->Init2(a4, a5, a6, a7);
    obj->m_3c = (i32)m_list.AddTail(obj);
    return obj;
}
