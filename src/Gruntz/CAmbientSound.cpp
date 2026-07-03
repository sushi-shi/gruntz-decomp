// CAmbientSound.cpp - the positional ambient-sound game object (see
// include/Gruntz/CAmbientSound.h). Reconstructs three of its __thiscall methods
// in ascending retail-RVA order:
//   0x00b790  ~CAmbientSound  (zero the voice handle / last field; restamp the
//             CUserBase base vptr)
//   0x00c090  Update          (per-frame: (re)start / fade the voice by whether
//             the listener is inside either audible box)
//   0x00c200  SetLevel        (scale + clamp 0..100, then drive the voice)
//
// CAmbientSound : CUserBase comes from the header; the DirectSoundMgr voice +
// g_gameReg view are modeled minimally and every cross-class callee is NO-body so
// its `call`/`mov ds:` reloc-masks. The factory/other ctors (0xb6a0/0xb7b0) stay
// stubbed in src/Stub/CAmbientSound.cpp.
#include <Gruntz/CAmbientSound.h>

#include <rva.h>

// ===========================================================================
// CAmbientSound::~CAmbientSound  (0x00b790)
// ===========================================================================
// Clear the voice handle (+0x04) and +0x3c; cl auto-emits the CUserBase base
// vptr restamp (0x5e70b4) as the sub-object unwinds. Real-polymorphic now, so no
// manual vtable store; the derived-vptr store at entry stays DCE'd (no virtual
// dispatch in the body), matching retail's 15-byte base-restamp-only shape.
RVA(0x0000b790, 0xf)
CAmbientSound::~CAmbientSound() {
    m_04 = 0;
    m_3c = 0;
}

// ===========================================================================
// CAmbientSound::Restart  (0x00bfb0)
// ===========================================================================
// Re-arm the voice at its current level. Gated on the voice handle, the not-yet-
// playing flag (m_14==0) and the active level/world (g_gameReg->m_10 and
// ->m_54->m_24). Reseed the channel, then inline SetLevel(m_08, 0, 0)'s scale+
// clamp through SetVolumeByIndex; the level read (m_08) is re-stored unchanged on
// both sides of the voice call (the reseeded channel may have touched it).
RVA(0x0000bfb0, 0xa9)
void CAmbientSound::Restart() {
    DirectSoundMgr* voice = m_04;
    i32 pos = m_08;
    if (voice == 0) {
        return;
    }
    if (m_14 != 0) {
        return;
    }
    if (g_gameReg->m_10 == 0) {
        return;
    }
    if (((WwdActiveLevel*)g_gameReg->m_54)->m_24 == 0) {
        return;
    }
    m_04->ApplyAndPlay(1, m_38, 0, 1);
    m_08 = pos;
    i32 scale = m_0c;
    if (scale > 5) {
        scale -= 0xf;
    }
    i32 v = (scale * pos) / 100;
    if (m_10 > 0) {
        v = (v * m_10) / 100;
    }
    if (v < 0) {
        v = 0;
    } else if (v > 0x64) {
        v = 0x64;
    }
    m_04->SetVolumeByIndex(v);
    m_08 = pos;
    m_14 = 1;
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
// unconditional `jmp`, and the merge drags a dead `((WwdActiveLevel*)g_gameReg->m_54)->m_24` probe
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
        if (m_14 != 0) {
            return;
        }
        DirectSoundMgr* voice = m_04;
        i32 lvl = m_08;
        if (voice == 0) {
            return;
        }
        if (lvl == 0) {
            return;
        }
        if (g_gameReg->m_10 == 0) {
            return;
        }
        // Retail also probes ((WwdActiveLevel*)g_gameReg->m_54)->m_24 here, then (re)starts
        // regardless; our cl DCEs that unused load (tail-merge wall, see below).
        if (m_04 == 0) {
            return;
        }
        m_04->ApplyAndPlay(1, m_38, 0, 1);
        SetLevel(0x64, 0, 0);
        m_08 = 0x64;
        m_14 = 1;
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

    if (m_14 != 0) {
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
    if (g_gameReg->m_10 == 0 || ((WwdActiveLevel*)g_gameReg->m_54)->m_24 == 0) {
        return;
    }
    if (force != 0) {
        if (m_04 == 0) {
            return;
        }
        m_04->ApplyAndPlay(1, m_38, 0, 1);
        SetLevel(0x64, 0, 0);
        m_08 = 0x64;
        m_14 = 1;
    } else {
        Fade(1, 0x64, 0x3e8);
    }
}

// ===========================================================================
// CAmbientSound::SetLevel  (0x00c200)
// ===========================================================================
// Scale `value` through the level scale-A (m_0c) and the secondary multiplier
// (m_10), clamp to 0..100, then drive the voice: mode 0 -> SetVolumeByIndex, else
// CloneAndPlay carrying the `extra` arg.
RVA(0x0000c200, 0x7e)
i32 CAmbientSound::SetLevel(i32 value, i32 mode, i32 extra) {
    m_08 = value;
    i32 scale = m_0c;
    if (scale > 5) {
        scale -= 0xf;
    }
    i32 v = (scale * value) / 100;
    if (m_10 > 0) {
        v = (v * m_10) / 100;
    }
    if (v < 0) {
        v = 0;
    } else if (v > 0x64) {
        v = 0x64;
    }
    if (mode == 0) {
        return m_04->SetVolumeByIndex(v);
    }
    return m_04->CloneAndPlay(v, mode, extra);
}
