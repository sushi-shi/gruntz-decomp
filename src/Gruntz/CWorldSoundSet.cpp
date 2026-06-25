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
#include <rva.h>

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
    m_04->SetVolByIdx(v);
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
