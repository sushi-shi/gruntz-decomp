#include <Mfc.h>              // MFC superset (afx-first); also pulled by WorldSoundSet.h
#include <Gruntz/GruntzMgr.h> // complete CGruntzMgr
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/BoundaryLeafLogicViews.h> // the boundary leaf-dtor views (L_8860 dissolved)
#include <Gruntz/AmbientSound.h>           // canonical CAmbientSound / CAmbientPosSound
#include <Gruntz/RandomAmbientSound.h>     // canonical CRandomAmbientSound
#include <Gruntz/PosSound.h>               // PosSoundObj / PosSoundAux spawn-path types
#include <Rez/RezMgr.h>                    // RezAlloc - the engine heap allocator (reloc-masked)
#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserBase (real base of CAmbientSound)

#include <math.h>          // sqrt intrinsic (UpdateAt's positional falloff) - inline fsqrt
#include <Gruntz/Random.h> // the g_randSeed* primary LCG state

VTBL(CAmbientSound, 0x001e710c);
VTBL(CAmbientPosSound, 0x001e7124);
VTBL(CRandomAmbientSound, 0x001e713c);
DATA(0x0022990c)
i32 g_posSoundReq; // 0x62990c

inline void* operator new(u32, void* p) {
    return p;
}

void SpawnPosSound(PosSoundObj* obj);

// ---------------------------------------------------------------------------
// 0x87b0 IS ??1CUserBase@@UAE@XZ - the out-of-line COMDAT copy of the INLINE
// ~CUserBase (<Gruntz/UserLogic.h>), now bound by RVA_COMPGEN in ActionArea.cpp
// (RVA-adjacent; its obj emits the COMDAT). The former placeholder here
// (`CUserBase87b0`, VTBL'd at 0x1e70fc) was a CONFLATION built on a broken thunk
// chase: 0x1e70fc's slot-0 sdd (0x8780) calls thunk 0x2ea5 -> 0x8750 (the _zdvec
// dtor), NOT 0x87b0 - and 0x1e70fc's RTTI COL names
// .?AV?$_zdvec@P8CUserLogic@@AEHXZ@@ (the PMF _zdvec instantiation), so binding
// it to any plain-identifier class was wrong by construction. 0x87b0's real
// identity is proven by its ~150 EH-unwind-funclet callers (every CUserBase-family
// ctor's partial-unwind calls it via thunk 0x1343) + its body: stamp ??_7CUserBase
// (0x5e70b4, RTTI .?AVCUserBase@@) and return.

// (The L_8860 placeholder dtor is DISSOLVED, 2026-07-17: 0x8860 IS ??1CUserLogic -
// ??_7CUserLogic @0x1e705c slot 0 -> ILT thunk 0x3cfb -> sdd 0x8a10 -> 0x8860; it is
// ALSO ~CWarlord unwind action(0) target. The old emitter-blocker died with the CWapX
// conversion (leaf ctor/dtor funclets now odr-use the out-of-line COMDAT); the body is
// pinned by RVA_COMPGEN in src/Gruntz/ActionArea.cpp beside ??1CUserBase.)

RVA(0x0000b5e0, 0x29)
i32 CWorldSoundSet::Init(void* world, i32 a2) {
    if (world == 0) {
        return 0;
    }
    m_world = static_cast<CRandomAmbientWorld*>(world);
    m_volume = a2;
    m_active = 1;
    m_listenerX = 0;
    m_listenerY = 0;
    return 1;
}

RVA(0x0000b620, 0x26)
void CWorldSoundSet::Deactivate() {
    if (m_world != 0 && m_world->m_soundDev != 0) {
        m_world->m_soundDev->FreeSamples();
    }
    Teardown();
    m_world = 0;
}

RVA(0x0000b660, 0x2b)
void CWorldSoundSet::Teardown() {
    CSoundNode* node = reinterpret_cast<CSoundNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CAmbientSound* ch = cur->m_data;
        if (ch != 0) {
            delete ch;
        }
    }
    m_list.RemoveAll();
}

RVA(0x0000b6a0, 0x83)
CAmbientSound* CWorldSoundSet::CreateAmbient6(const char* key, i32 a1, RECT* box, i32 a3, i32 a4) {
    CAmbientSound* obj = new CAmbientSound;
    if (obj == 0) {
        return 0;
    }
    if (obj->Init6(m_world, key, a1, m_volume, box, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

// 0xb790 - ??1CAmbientSound@@UAE@XZ: the out-of-line COMDAT copy of the inline
// ~CAmbientSound (<Gruntz/AmbientSound.h>). Clears m_voice/m_listNode, folds the
// inline ~CUserBase (stamp ??_7CUserBase). RVA_COMPGEN NAMES the retail copy.
RVA_COMPGEN(0x0000b790, 0xf, ??1CAmbientSound@@UAE@XZ)

RVA(0x0000b7b0, 0x80)
CAmbientSound*
CWorldSoundSet::CreateAmbient5(DirectSoundMgr* mgr, i32 a1, RECT* box, i32 a3, i32 a4) {
    CAmbientSound* obj = new CAmbientSound;
    if (obj == 0) {
        return 0;
    }
    if (obj->Init5(mgr, a1, m_volume, box, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

RVA(0x0000b850, 0x83)
CAmbientPosSound*
CWorldSoundSet::CreatePos6(const char* key, i32 a1, AmbientPoint* pos, i32 a3, i32 a4) {
    CAmbientPosSound* obj = new CAmbientPosSound;
    if (obj == 0) {
        return 0;
    }
    if (obj->Init6(m_world, key, a1, m_volume, pos, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

// 0xb940 - ??1CAmbientPosSound@@UAE@XZ: the out-of-line COMDAT copy of the inline
// ~CAmbientPosSound (<Gruntz/AmbientSound.h>). Inlines the base ~CAmbientSound so it
// collapses to the same bytes as 0xb790 (stamp ??_7CUserBase, clear m_voice/m_listNode).
RVA_COMPGEN(0x0000b940, 0xf, ??1CAmbientPosSound@@UAE@XZ)

RVA(0x0000b960, 0x80)
CAmbientPosSound*
CWorldSoundSet::CreatePos5(DirectSoundMgr* mgr, i32 a1, AmbientPoint* pos, i32 a3, i32 a4) {
    CAmbientPosSound* obj = new CAmbientPosSound;
    if (obj == 0) {
        return 0;
    }
    if (obj->Init5(mgr, a1, m_volume, pos, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

// CRandomAmbientSound (0x58) with a validated bounding box: reject an inverted x
// (a5<a4) or y (a7<a6) range, then `new CRandomAmbientSound` (operator new == RezAlloc,
// which inlines the ctor's vptr stamp + seed stores), 6-arg Init, the Init2 box roll,
// append, return. (a8 unused.)
RVA(0x0000ba00, 0xc6)
CRandomAmbientSound* CWorldSoundSet::CreateRandomBox(
    const char* key,
    i32 a1,
    RECT* box,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
) {
    if (static_cast<u32>(a5) < static_cast<u32>(a4)) {
        return 0;
    }
    if (static_cast<u32>(a7) < static_cast<u32>(a6)) {
        return 0;
    }
    CRandomAmbientSound* obj = new CRandomAmbientSound;
    if (obj == 0) {
        return 0;
    }
    if (obj->Init6(m_world, key, a1, m_volume, box, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->Init2(a4, a5, a6, a7);
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

// 0xbb40 - ??1CRandomAmbientSound@@UAE@XZ: the out-of-line COMDAT copy of the inline
// ~CRandomAmbientSound (<Gruntz/RandomAmbientSound.h>). Inlines the base ~CAmbientSound so
// it collapses to the same bytes as 0xb790 (stamp ??_7CUserBase, clear m_voice/m_listNode).
// Ghidra mislabeled it ??0 (ctor) from the byte-shape overlap, but its `xor eax,eax` (no
// this-return) + its sole caller being the scalar-deleting-dtor 0xbb10 (vtable slot 0)
// prove it is the dtor.
RVA_COMPGEN(0x0000bb40, 0xf, ??1CRandomAmbientSound@@UAE@XZ)

RVA(0x0000bb60, 0x9b)
CRandomAmbientSound* CWorldSoundSet::CreateRandom(
    DirectSoundMgr* mgr,
    i32 a1,
    RECT* box,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
) {
    CRandomAmbientSound* obj = new CRandomAmbientSound;
    if (obj == 0) {
        return 0;
    }
    if (obj->Init5(mgr, a1, m_volume, box, a3) == 0) {
        delete obj;
        return 0;
    }
    obj->Init2(a4, a5, a6, a7);
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

RVA(0x0000bc30, 0x3a)
void CWorldSoundSet::Restart(i32 a1) {
    m_volume = a1;
    if (m_world->m_soundDev != 0) {
        m_world->m_soundDev->FreeSamples();
    }
    CSoundNode* node = reinterpret_cast<CSoundNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CAmbientSound* ch = cur->m_data;
        if (ch != 0) {
            ch->Recompute(static_cast<i32>(a1));
        }
    }
}

RVA(0x0000bc80, 0x44)
void CWorldSoundSet::Stop() {
    if (m_world != 0 && m_world->m_soundDev != 0) {
        m_world->m_soundDev->FreeSamples();
    }
    CSoundNode* node = reinterpret_cast<CSoundNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CAmbientSound* ch = cur->m_data;
        if (ch != 0 && ch->m_voice != 0) {
            ch->m_voice->StopAndRewind();
            ch->m_isPlaying = 0;
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
    CSoundNode* node = reinterpret_cast<CSoundNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CAmbientSound* ch = cur->m_data;
        if (ch != 0) {
            ch->m_isPlaying = 0;
            ch->Update(m_listenerX, m_listenerY, 1);
        }
    }
    if (m_world->m_soundDev != 0) {
        m_world->m_soundDev->PurgeVoiceList(-1);
    }
}

// ---------------------------------------------------------------------------
// Retune: record the new listener position, push it to every live channel
// (vtbl slot 3 = Update(x,y,force), force 0), then rewind the world handle.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc/schedule-coinflip wall (~86.3%) - logic complete, all relocs paired.
// Structurally identical to Resume (matched body) but the two up-front member
// stores (m_listenerX=x, m_listenerY=y) let cl hoist the loop-head load + null-test above
// the stores and pin `mov ebp,ecx` early, whereas retail keeps `this` in ecx
// until the stores and reads the head after - a pure register/schedule permutation
// (same bytes, reordered) plus the same dead-this tail load as Resume. The
// for-loop / store-order levers did not flip it. See zero-register-pinning.md.
RVA(0x0000bd60, 0x4b)
void CWorldSoundSet::Retune(i32 x, i32 y) {
    m_listenerX = x;
    m_listenerY = y;
    CSoundNode* node = reinterpret_cast<CSoundNode*>(m_list.GetHeadPosition());
    while (node != 0) {
        CSoundNode* cur = node;
        node = node->m_next;
        CAmbientSound* ch = cur->m_data;
        if (ch != 0) {
            ch->Update(x, y, 0);
        }
    }
    if (m_world->m_soundDev != 0) {
        m_world->m_soundDev->PurgeVoiceList(-1);
    }
}

RVA(0x0000bdd0, 0x53)
void* CAmbientSound::Init6(
    CRandomAmbientWorld* world,
    const char* key,
    i32 a3,
    i32 a4,
    RECT* box,
    i32 a6
) {
    void* out_ob = 0; // CMapStringToPtr's value slot (Lookup 0x1b8438 takes void*&)
    world->m_map.Lookup(key, out_ob);
    AmbSoundRecord* out = static_cast<AmbSoundRecord*>(out_ob);
    if (out == 0) {
        return static_cast<void*>(out);
    }
    return reinterpret_cast<void*>(Init5(out->m_mgr, a3, a4, box, a6));
}

RVA(0x0000be50, 0x8f)
i32 CAmbientSound::Init5(DirectSoundMgr* mgr, i32 a2, i32 a3, RECT* box, i32 a5) {
    if (mgr == 0) {
        return 0;
    }
    m_voice = mgr;
    m_level = a2;
    m_scaleA = a3;
    m_scaleB = a5;
    m_panIndex = 0;
    m_isPlaying = 0;
    RECT* p = &m_box1;
    if (box != 0) {
        *p = *box;
    } else {
        p->left = static_cast<i32>(0x80000000);
    }
    if (p->left == 0 && m_box1.top == 0 && m_box1.right == 0 && m_box1.bottom == 0) {
        p->left = static_cast<i32>(0x80000000);
    }
    m_box2.left = static_cast<i32>(0x80000000);
    return 1;
}

// ---------------------------------------------------------------------------
// CAmbientSound::Recompute (0x00bf10): per-channel volume recompute, invoked by
// CWorldSoundSet::Restart for each live channel (was the CSoundChannel view's
// method - the channels ARE this family). Skip when the pushed master level is
// unchanged from the cached m_scaleA; otherwise cache it, apply the >5 -> -0xf
// curve, scale by m_level then m_scaleB (signed /100 by the 0x51eb851f reciprocal
// each step), clamp to 0..100 and push it to the voice via SetVolByIdx. This is
// the SetLevel scale math with the master (m_scaleA) as the LIVE operand.
//
// @early-stop
// regalloc-coinflip wall (~97.9%) - logic complete, all relocs paired. retail pins
// the `master` arg in eax (dead m_scaleA -> edx); our cl does the reverse (arg in edx),
// which permutes the first ~5 instrs (cmp modrm, the m_scaleA store reg, add eax,-0xf vs
// sub edx,0xf). The compare-operand-order lever did not flip the pin. See
// docs/patterns/zero-register-pinning.md.
RVA(0x0000bf10, 0x72)
void CAmbientSound::Recompute(i32 master) {
    if (master == m_scaleA) {
        return;
    }
    i32 mult = m_level;
    m_scaleA = master;
    if (master > 5) {
        master -= 0xf;
    }
    i32 v = (master * mult) / 100;
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
// CAmbientSound::Fade (0x00c2a0, __thiscall, 3 args playFlag/level/mode):
// the play/stop driver. Gated on the mgr handle, the playing flag, and the active
// level (g_gameReg->m_soundEnabled and g_gameReg->m_inputState->m_active). On play it
// reseeds the voice (ApplyAndPlay(1,m_panIndex,0,1)), scales level by (m_scaleA
// clamped)/100 then m_scaleB/100 (both signed magic-/100), clamps the result to
// [0,100], and dispatches SetVolumeByIndex (mode==0) or CloneAndPlay (mode!=0);
// on stop it StopAndRewind's (mode==0) or CloneAndPlay-stops (mode!=0).
// ---------------------------------------------------------------------------
// @early-stop
// ~89% register-materialization wall (was 35% - the play/stop branch polarity and both
// mode branches are now retail-correct: playFlag!=0 play path is the fall-through, mode==0
// is the fall-through in BOTH the play and stop arms). Residual: retail pushes the shared
// ApplyAndPlay args (push 1 / push 1 immediates) once before the kind branch and stores
// m_isPlaying=1 as an immediate; cl instead pins the constant 1 in ebp (mov ebp,1; push
// ebp; ...; mov [esi+0x14],ebp), which blocks the arg-push hoist. Pure regalloc coin-flip
// (the /100 magic-division family); no source spelling flips the ebp pin. See
// zero-register-pinning.md.
RVA(0x0000c2a0, 0x19e)
void CAmbientSound::Fade(i32 playFlag, i32 level, i32 mode) {
    if (m_voice == 0) {
        return;
    }
    if (playFlag != 0) {
        // Play path (fall-through; retail's `je stop` branch polarity puts the
        // shorter stop path last).
        if (m_isPlaying != 0) {
            return;
        }
        if (g_gameReg->m_soundEnabled == 0) {
            return;
        }
        if (g_gameReg->m_inputState->m_active == 0) {
            return;
        }
        if (mode == 0) {
            m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
            i32 t = m_scaleA;
            m_level = level;
            if (t > 5) {
                t -= 0xf;
            }
            i32 v = (t * level) / 100;
            if (m_scaleB > 0) {
                v = (v * m_scaleB) / 100;
            }
            if (v < 0) {
                m_voice->SetVolumeByIndex(0);
                m_level = level;
                m_isPlaying = 1;
                return;
            }
            if (v > 0x64) {
                v = 0x64;
            }
            m_voice->SetVolumeByIndex(v);
            m_level = level;
            m_isPlaying = 1;
            return;
        }

        m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
        i32 t = m_scaleA;
        m_level = level;
        if (t > 5) {
            t -= 0xf;
        }
        i32 v = (t * level) / 100;
        if (m_scaleB > 0) {
            v = (v * m_scaleB) / 100;
        }
        if (v < 0) {
            v = 0;
        } else if (v > 0x64) {
            v = 0x64;
        }
        m_voice->CloneAndPlay(v, mode, 0);
        m_level = level;
        m_isPlaying = 1;
        return;
    }

    // Stop path (playFlag == 0). Retail's `jne` puts StopAndRewind (mode==0) as the
    // fall-through and the CloneAndPlay-stop (mode!=0) as the jumped-to arm.
    if (m_isPlaying == 0) {
        return;
    }
    if (mode == 0) {
        m_voice->StopAndRewind();
        m_isPlaying = 0;
        return;
    }
    m_level = 0;
    m_voice->CloneAndPlay(0, mode, 1);
    m_isPlaying = 0;
}

// ---------------------------------------------------------------------------
// CAmbientPosSound::Init6 (0x00c4b0, __thiscall, 6 args): resolve the mgr record
// for `key` out of world->m_map; when found, seed this object via
// Init5(record->m_mgr, a3, a4, pos, a5). The miss path returns Lookup's 0
// (CreatePos6 tests the return - byte-proven).
// ---------------------------------------------------------------------------
// @early-stop
// CODE BYTE-EXACT - residual is the reloc-naming scoring artifact: retail's two
// calls go through ILT thunks (Lookup / thunk_FUN_0040c530) while our base names
// the callee directly, so objdiff scores the REL32 operands against
// differently-named symbols (~88.75%). Every instruction byte matches the
// delinked target (verified by base-vs-target objdump). Effectively matched.
RVA(0x0000c4b0, 0x53)
i32 CAmbientPosSound::Init6(
    CRandomAmbientWorld* world,
    const char* key,
    i32 a3,
    i32 a4,
    AmbientPoint* pos,
    i32 a5
) {
    void* found = 0;
    i32 r = world->m_map.Lookup(key, found);
    if (found != 0) {
        return Init5((static_cast<AmbSoundRecord*>(found))->m_mgr, a3, a4, pos, a5);
    }
    return r;
}

RVA(0x0000c530, 0x51)
i32 CAmbientPosSound::Init5(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientPoint* pos, i32 a5) {
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
// CAmbientPosSound::Update (0x00c5b0, slot 3; ex UpdateAt): the positional play driver.
// Compute the listener->anchor distance (|m_40-x|, |m_44-y|); if either axis is
// past 0x280 stop the voice. Otherwise derive a falloff volume (100 - dist/12,
// clamped) and a pan (dx/4, clamped, signed by which side of m_40 the listener
// is), scale the volume by m_scaleA/100 then m_scaleB/100, set volume + pan; and when not
// already playing (and the active level is live) reseed and re-set the volume,
// marking the voice playing.
RVA(0x0000c5b0, 0x1df)
void CAmbientPosSound::Update(i32 x, i32 y, i32 force) {
    i32 dx = abs(m_40 - x); // branchless cdq/xor/sub (MSVC abs intrinsic), not jns/neg
    i32 dy = abs(m_44 - y);
    i32 dist2 = dx * dx + dy * dy;
    if (dx > 0x280 || dy > 0x280) {
        if (m_voice != 0 && m_isPlaying != 0) {
            m_voice->StopAndRewind();
            m_isPlaying = 0;
        }
        return;
    }

    i32 dist = static_cast<i32>(sqrt(static_cast<double>(dist2)));
    i32 vol = 0x64 - dist / 12; // retail magic 0x2aaaaaab + sar edx,1 = signed /12
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
// The create-helper return is a CAmbientSound-family channel; its m_box2 is the
// placement rectangle copied below.
RVA(0x0000c840, 0x13d)
i32 CommitSpriteAction(PosSoundObj* obj) {
    PosSoundAux* aux = obj->m_aux;
    if (aux->m_requestState == 0) {
        obj->m_flags08 |= 1;
        obj->m_flags40 |= 1;
        if (aux->m_handler == DefaultActionHandler_2d15) {
            obj->m_flags08 |= 2;
        } else {
            obj->m_flags08 &= ~2;
        }
        LeafCue* layer = obj->m_layer;
        if (layer && g_gameReg) {
            RECT rc;
            CopyRect(&rc, &obj->m_area);
            if (aux->m_srcL > 0 || aux->m_srcR > 0) {
                SetRect(&rc, aux->m_srcL, aux->m_srcT, aux->m_srcR, aux->m_srcB);
            }
            if (g_gameReg->m_inputState) {
                CAmbientSound* placed;
                if (obj->m_extent.top > 0) {
                    placed = g_gameReg->m_inputState->CreateRandom(
                        layer->m_10,
                        0x64,
                        &rc,
                        obj->m_120,
                        obj->m_extent.left,
                        obj->m_extent.top,
                        obj->m_extent.right,
                        obj->m_extent.bottom,
                        0
                    );
                } else {
                    placed = g_gameReg->m_inputState
                                 ->CreateAmbient5(layer->m_10, 0x64, &rc, obj->m_120, 0);
                }
                if (placed && obj->m_placed.top > 0) {
                    placed->m_box2 = obj->m_placed;
                }
            }
        }
        obj->m_flags08 |= 0x10000;
        aux->m_requestState = 5;
    }
    return 1;
}

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
        CAmbientPosSound* sound = aux->m_voice;
        if (sound == 0) {
            return;
        }
        CPtrList* arr =
            reinterpret_cast<CPtrList*>((reinterpret_cast<char*>(g_gameReg->m_inputState) + 8));
        if (sound->m_voice != 0) {
            sound->m_voice->StopAndRewind();
            sound->m_isPlaying = 0;
        }
        if (sound->m_listNode != 0) {
            arr->RemoveAt(sound->m_listNode);
            delete sound;
        }
        aux->m_voice = 0;
        aux->m_requestState = 0;
        return;
    }

    obj->m_flags08 = (obj->m_flags08 & ~2) | 0x100001;
    obj->m_flags40 |= 1;
    aux->m_voice = 0;
    LeafCue* layer = obj->m_layer;
    if (layer != 0 && g_gameReg != 0 && g_gameReg->m_inputState != 0) {
        i32 pt[2];
        pt[0] = obj->m_x;
        pt[1] = obj->m_y;
        void* v = PosSoundSpawn(layer->m_10, 0x64, &pt, obj->m_120, 0);
        if (v != 0) {
            aux->m_voice = static_cast<CAmbientPosSound*>(v);
        }
    }
    aux->m_requestState = 5;
}

// ---------------------------------------------------------------------------
// CRandomAmbientSound::Update (0x00cb30, slot 3; ex Step): test the
// listener position against the two visibility boxes; if it left both (and we are
// playing) stop the voice. Otherwise drain the rolled countdown by the frame
// delta, and on expiry flip the roller phase, roll a fresh interval over the
// active phase's [lo,hi], halve+clamp it to <=1000, and (re)play via Update.
// ---------------------------------------------------------------------------
// @early-stop
// ~95% idiv-scheduling/reroll-regalloc wall (was 89.7%: qualifying the 3 Update calls
// to direct/non-virtual dispatch, the unsigned (u32) countdown shift, and reordering the
// countdown compare to `frameDelta >= countdownMs` all landed). Residual: (a) the box2
// last-term (y >= m_box2.bottom) inBox=1 block layout (jl vs jge) and (b) the two reroll
// arms computing span=hi-lo+1 via `lea ebx,[eax+1]; test` (retail, span kept in a fresh
// reg for the lo:hi coin-flip) vs cl's `inc edi` - pure register-pressure/scheduling.
RVA(0x0000cb30, 0x168)
void CRandomAmbientSound::Update(i32 x, i32 y, i32 force) {
    i32 inBox = 0;
    i32 b1 = m_box1.left;
    if (b1 == static_cast<i32>(0x80000000)) {
        inBox = 1;
    } else if (x <= b1 || x >= m_box1.right || y <= m_box1.top || y >= m_box1.bottom) {
        i32 b2 = m_box2.left;
        if (b2 == static_cast<i32>(0x80000000) || x <= b2 || x >= m_box2.right || y <= m_box2.top
            || y >= m_box2.bottom) {
            inBox = 1;
        }
    }

    if (inBox == 0) {
        if (m_isPlaying != 0 && m_voice != 0) {
            SetLevel(0, 0x3e8, 1);
            m_isPlaying = 0;
        }
        m_phase = 0;
        return;
    }

    if (force != 0 && m_phase != 0 && m_isPlaying != 0) {
        return;
    }

    // retail: cmp frameDelta, countdownMs; jb subtract (frameDelta as the left operand).
    if (static_cast<u32>(g_frameDelta) >= static_cast<u32>(m_countdownMs)) {
        m_countdownMs = 0;
    } else {
        m_countdownMs = m_countdownMs - g_frameDelta;
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
            r = (g_gameReg->Rand() & 1) ? lo : hi;
        } else {
            r = g_gameReg->Rand() % span + lo;
        }
        m_countdownMs = r;
        i32 half = static_cast<u32>(r) >> 1; // logical shr (retail), not arithmetic sar
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Fade(1, 0x64, half);
    } else {
        i32 lo = m_intervalLoB;
        i32 hi = m_intervalHiB;
        i32 span = hi - lo + 1;
        i32 r;
        if (span == 0) {
            r = (g_gameReg->Rand() & 1) ? lo : hi;
        } else {
            r = g_gameReg->Rand() % span + lo;
        }
        m_countdownMs = r;
        i32 half = static_cast<u32>(r) >> 1; // logical shr (retail), not arithmetic sar
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Fade(0, 0x64, half);
    }
}

// Lazily seed the manager's primary LCG from timeGetTime, advance it, and return
// the top 15 bits (the classic MS rand()). Ambient-sound callers load g_gameReg
// into ecx before reaching this body, matching the other retail call sites.
RVA(0x0000cd00, 0x46)
i32 CGruntzMgr::Rand() {
    i32 seed;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = timeGetTime();
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    return (g_randSeed >> 0x10) & 0x7fff;
}

// Init2(lo, hi, lo2, hi2): seed the interval roller (phase-A bounds into
// m_40/m_44, phase-B into +0x48/+0x4c) and roll the first countdown in [lo,hi]
// (lazily-seeded LCG; coin-flip endpoints when the span is empty).
RVA(0x0000cd70, 0xe5)
void CRandomAmbientSound::Init2(i32 lo, i32 hi, i32 a3, i32 a4) {
    i32 span = hi - lo + 1;
    m_40 = lo;
    m_44 = hi;
    m_intervalLoB = a3;
    m_intervalHiB = a4;
    i32 seed;
    if (span == 0) {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_randSeed;
        }
        g_randSeed = seed * 214013 + 2531011;
        if (g_randSeed & 0x10000) {
            m_phase = 1;
            m_countdownMs = lo;
        } else {
            m_phase = 1;
            m_countdownMs = hi;
        }
        return;
    }
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = timeGetTime();
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    m_phase = 1;
    m_countdownMs = lo + ((g_randSeed >> 0x10) & 0x7fff) % span;
}

RVA(0x00085ed0, 0x4a)
CWorldSoundSet::~CWorldSoundSet() {
    Deactivate();
}
