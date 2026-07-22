#include <Gruntz/Random.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>

#include <Mfc.h> // superset of Win32.h; GameRegistry.h pulls afx via SoundCue.h
#include <rva.h>
#include <Gruntz/GameRegistry.h> // g_gameReg canonical view (0x24556c)

DATA(0x0024c22c)
char g_coinRolled; // bit0 set once this frame's coin was rolled
DATA(0x0024c26c)
i32 g_coinValue; // the cached 0/1 result
DATA(0x002c127d)
u8 g_randSeeded; // 0x6c127d bit0 set once seeded
DATA(0x002c1288)
i32 g_randSeed; // 0x6c1288 32-bit LCG state
DATA(0x002c278c)
char g_rng2Seeded; // bit0 set once seeded
DATA(0x002c2798)
i32 g_rng2State; // 32-bit LCG state

namespace Rng {
    // __cdecl rand(): lazily seed from timeGetTime, then advance the MS-CRT LCG.
    RVA(0x0000cd00, 0x46)
    i32 Next() {
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

    // __thiscall(lo, hi, a3, a4): roll a random value in [lo,hi] (lazily-seeded
    // LCG; coin-flip endpoints when the span is empty) and cache it + the params.
    RVA(0x0000cd70, 0xe5)
    void RangeBox::Roll(i32 lo, i32 hi, i32 a3, i32 a4) {
        i32 span = hi - lo + 1;
        m_40 = lo;
        m_44 = hi;
        m_48 = a3;
        m_4c = a4;
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
                m_54 = 1;
                m_50 = lo;
            } else {
                m_54 = 1;
                m_50 = hi;
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
        m_54 = 1;
        m_50 = lo + ((g_randSeed >> 0x10) & 0x7fff) % span;
    }

    // __stdcall(lo, hi): lazily-seeded LCG random in [lo,hi]. When the span is
    // empty (hi==lo-1) it coin-flips between the endpoints on bit 0x10000.
    // @interleaver Rng - own-namespace LCG helper COMDAT scattered at 0x19f50 (the Rng
    // helpers are each their own tiny COMDAT); RVA-placement artifact, kept together here.
    // @early-stop
    // 14% -> 94% by ordering the span==0 (coin-flip) arm FIRST so it falls through and
    // the modulo arm is the jumped-to block, matching retail's `jne`. Residual is pure
    // /O2 scheduling: cl(g_randSeeded) hoisted early vs retail's late load, and `inc esi`
    // vs retail's `lea esi,[eax+1]` span materialization. Logic byte-for-byte otherwise.
    RVA(0x00019f50, 0xb2)
    i32 __stdcall RangeStd(i32 lo, i32 hi) {
        i32 span = hi - lo + 1;
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
                return lo;
            }
            return hi;
        }
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_randSeed;
        }
        g_randSeed = seed * 214013 + 2531011;
        return lo + ((g_randSeed >> 0x10) & 0x7fff) % span;
    }

    // __thiscall coin-flip: deterministic ((m_1c+1)%2) in replay mode, otherwise a
    // once-per-frame random bit lazily seeded from timeGetTime.
    // @interleaver Rng - own-namespace helper COMDAT scattered at 0xda200; RVA-placement.
    // reloc-fidelity: 0xda200 IS CPlay::GetAmbientId - CPlay::SyncState (0xd7520) and the
    // Monolith AMBIENT cheat (GruntzMgrCmd) both call it __thiscall on a CPlay `this` to
    // pick the "AMBIENT%d" variant index. SYMBOL exports it under the canonical CPlay
    // name so those calls bind; the Rng::CoinFlip view is the recovered-symbol placeholder
    // (m_1c == CPlay+0x1c replay-seed; body-fold onto CPlay deferred).
    // retail identity: ?GetAmbientId@CPlay@@QAEHXZ (byte-matched as CoinFlip::Flip; fold onto CPlay deferred)
    RVA(0x000da200, 0x9b)
    i32 CoinFlip::Flip() {
        CGruntzMgr* gr = g_gameReg;
        if (gr->m_134 == 1 && gr->m_130 == 0) {
            return (m_1c + 1) % 2;
        }
        if (!(g_coinRolled & 1)) {
            i32 seed;
            g_coinRolled |= 1;
            if (!(g_randSeeded & 1)) {
                g_randSeeded |= 1;
                seed = timeGetTime();
            } else {
                seed = g_randSeed;
            }
            g_randSeed = seed * 214013 + 2531011;
            g_coinValue = ((g_randSeed >> 0x10) & 0x7fff) % 2;
        }
        return g_coinValue;
    }

    // __cdecl rand(): lazily seed from timeGetTime, then advance the MS-CRT LCG.
    // @interleaver Rng - own-namespace helper COMDAT scattered at 0x15cbe0; RVA-placement.
    RVA(0x0015cbe0, 0x46)
    i32 Next2() {
        i32 seed;
        if (!(g_rng2Seeded & 1)) {
            g_rng2Seeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_rng2State;
        }
        g_rng2State = seed * 214013 + 2531011;
        return (g_rng2State >> 0x10) & 0x7fff;
    }
} // namespace Rng
