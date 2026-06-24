// CRandomAmbientSound.cpp - the RTTI-named CRandomAmbientSound (vtable 0x5e713c,
// chain CRandomAmbientSound : CAmbientSound : CUserBase, sizeof 0x58). A
// random-interval ambient sound bound to one or two visibility boxes: Setup seeds
// the mgr handle / play params / boxes; Step (vtable slot 3) ticks the listener
// position against the boxes, drains the rolled countdown by the frame delta, and
// when it expires rerolls a fresh interval (global LCG rand) and (re)plays via
// Update; Update starts/stops the DirectSoundMgr voice.
//
// The base CUserBase vptr (0x5e70b4) is stamped manually (transitional workaround:
// the full 0x5e713c vtable / its virtuals are not reconstructed here, so letting
// the compiler emit a polymorphic ??_7 would diverge).
//
// Field names are placeholders; OFFSETS + emitted code bytes are load-bearing.
#include <Gruntz/CRandomAmbientSound.h>
#include <rva.h>

// The base CUserBase vftable (VA 0x5e70b4) the base ctor stamps. Manual store -
// see the header note. Referenced by address so the DIR32 operand reloc-masks.
DATA(0x001e70b4)
extern void* g_vtbl_CUserBase[]; // 0x5e70b4

// ---------------------------------------------------------------------------
// Base ctor (0x00bb40): stamp the CUserBase vptr, clear the mgr handle (+0x04)
// and +0x3c. The rest is set up by Setup.
// ---------------------------------------------------------------------------
RVA(0x0000bb40, 0xf)
void CRandomAmbientSound::BaseInit() {
    m_vptr = g_vtbl_CUserBase;
    m_4 = 0;
    m_3c = 0;
}

// ---------------------------------------------------------------------------
// Setup (0x00be50, __thiscall, 5 args): refuse a null mgr; otherwise stash the
// mgr + the three play params, copy the primary box (or stamp the no-box
// sentinel), and reset the secondary box to the sentinel. Returns 1, or 0 on the
// null guard.
// ---------------------------------------------------------------------------
RVA(0x0000be50, 0x8f)
i32 CRandomAmbientSound::Setup(DirectSoundMgr* mgr, i32 a2, i32 a3, AmbientBox* box, i32 a5) {
    if (mgr == 0) {
        return 0;
    }
    m_4 = mgr;
    m_8 = a2;
    m_c = a3;
    m_10 = a5;
    m_38 = 0;
    m_14 = 0;
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
// Update (0x00c2a0, __thiscall, 3 args playFlag/pos/kind): the play/stop driver.
// Gated on the mgr handle, the playing flag, and the active level (g_gameReg->m_10
// and g_gameReg->m_54->m_24). On play it reseeds the voice (mgr->Reseed(1,m_38,0,1)),
// scales pos by (m_c clamped)/100 then m_10/100 (both signed magic-/100), clamps the
// result to [0,100], and dispatches SetVolumeByIndex (kind==0) or CloneAndPlay
// (kind!=0); on stop it StopAndRewind's (kind==0) or CloneAndPlay-stops (kind!=0).
// ---------------------------------------------------------------------------
// @early-stop
// 3-push frame + twin signed-/100 magic-division scheduling wall (logic complete,
// all relocs paired). cl duplicates the scale-and-clamp block per kind branch (as
// retail does) but permutes the eax/ecx/edx use across the two 0x51eb851f reductions
// and the SetVolumeByIndex/CloneAndPlay tail; no source spelling pins that schedule.
// See zero-register-pinning.md and CGruntSpawnConfig::PickWeighted (the /100 family).
RVA(0x0000c2a0, 0x19e)
void CRandomAmbientSound::Update(i32 playFlag, i32 pos, i32 kind) {
    if (m_4 == 0) {
        return;
    }
    if (playFlag == 0) {
        // Stop path.
        if (m_14 == 0) {
            return;
        }
        if (kind != 0) {
            m_8 = 0;
            m_4->CloneAndPlay(0, kind, 1);
            m_14 = 0;
            return;
        }
        m_4->StopAndRewind();
        m_14 = 0;
        return;
    }
    if (m_14 != 0) {
        return;
    }
    if (g_gameReg->m_10 == 0) {
        return;
    }
    if (g_gameReg->m_54->m_24 == 0) {
        return;
    }

    if (kind != 0) {
        ((DsndReseed*)m_4)->Reseed(1, m_38, 0, 1);
        i32 t = m_c;
        m_8 = pos;
        if (t > 5) {
            t -= 0xf;
        }
        i32 v = (t * pos) / 100;
        if (m_10 > 0) {
            v = (v * m_10) / 100;
        }
        if (v < 0) {
            v = 0;
        } else if (v > 0x64) {
            v = 0x64;
        }
        m_4->CloneAndPlay(v, kind, 0);
        m_8 = pos;
        m_14 = 1;
        return;
    }

    ((DsndReseed*)m_4)->Reseed(1, m_38, 0, 1);
    i32 t = m_c;
    m_8 = pos;
    if (t > 5) {
        t -= 0xf;
    }
    i32 v = (t * pos) / 100;
    if (m_10 > 0) {
        v = (v * m_10) / 100;
    }
    if (v < 0) {
        m_4->SetVolumeByIndex(0);
        m_8 = pos;
        m_14 = 1;
        return;
    }
    if (v > 0x64) {
        v = 0x64;
    }
    m_4->SetVolumeByIndex(v);
    m_8 = pos;
    m_14 = 1;
}

// ---------------------------------------------------------------------------
// Step (0x00cb30, vtable slot 3, __thiscall, 3 args x/y/force): test the listener
// position against the two visibility boxes; if it left both (and we are playing)
// stop the voice. Otherwise drain the rolled countdown by the frame delta, and on
// expiry flip the roller phase, roll a fresh interval over the active phase's
// [lo,hi], halve+clamp it to <=1000, and (re)play via Update.
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
        if (m_14 != 0 && m_4 != 0) {
            Update(0, 0x3e8, 1);
            m_14 = 0;
        }
        m_54 = 0;
        return;
    }

    if (force != 0 && m_54 != 0 && m_14 != 0) {
        return;
    }

    if ((u32)m_50 <= (u32)g_tickDelta) {
        m_50 = 0;
    } else {
        m_50 = m_50 - g_tickDelta;
    }
    if (m_50 != 0) {
        return;
    }

    m_54 ^= 1;
    if (m_54 != 0) {
        i32 lo = m_40;
        i32 hi = m_44;
        i32 span = hi - lo + 1;
        i32 r;
        if (span == 0) {
            r = (winapi_00cd00_timeGetTime() & 1) ? lo : hi;
        } else {
            r = winapi_00cd00_timeGetTime() % span + lo;
        }
        m_50 = r;
        i32 half = r >> 1;
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Update(1, 0x64, half);
    } else {
        i32 lo = m_48;
        i32 hi = m_4c;
        i32 span = hi - lo + 1;
        i32 r;
        if (span == 0) {
            r = (winapi_00cd00_timeGetTime() & 1) ? lo : hi;
        } else {
            r = winapi_00cd00_timeGetTime() % span + lo;
        }
        m_50 = r;
        i32 half = r >> 1;
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Update(0, 0x64, half);
    }
}
