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
    // (Next @0xcd00 + the ex-"RangeBox::Roll" @0xcd70 moved to their owner TU,
    // WorldSoundSet.cpp - the 0xcb30..0xce55 band is worldsoundset's tail; 0xcd70
    // IS CRandomAmbientSound::Init2. The ex-"CoinFlip::Flip" @0xda200 moved to
    // Play.cpp - it IS CPlay::GetAmbientId, interleaved in play's band.)

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
