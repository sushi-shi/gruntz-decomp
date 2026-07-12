// Random.cpp - the MS-CRT-style LCG random helpers, re-homed out of the
// src/Stub/ApiCallers.cpp winapi grab-bag. Two 214013/2531011 LCG generators
// reached through ILT jmp-thunks; the primary state lives in src/Globals.cpp
// (g_randSeeded/g_randSeed), the second generator's state (g_rng2*) is here.
#include <Gruntz/Random.h>

#include <Win32.h>
#include <rva.h>

// The primary generator's state (canonical DATA in src/Globals.cpp:
// g_randSeeded @0x2c127d / g_randSeed @0x2c1288 - bind to the tree winners).
extern u8 g_randSeeded; // 0x6c127d bit0 set once seeded
extern i32 g_randSeed;  // 0x6c1288 32-bit LCG state

// Owner-TU definitions of this TU's private generator/coin state (.bss zero),
// RVA-ascending. Per-frame cached coin bit used by the deterministic coin-flip helper:
DATA(0x0024c22c)
char g_coinRolled; // bit0 set once this frame's coin was rolled
DATA(0x0024c26c)
i32 g_coinValue; // the cached 0/1 result
// The second generator's state, seeded lazily from timeGetTime:
DATA(0x002c278c)
char g_rng2Seeded; // bit0 set once seeded
DATA(0x002c2798)
i32 g_rng2State; // 32-bit LCG state

// The game-registry singleton (0x24556c); only its replay-mode flags are read
// here, so a minimal local view (canonical DATA in src/Stub/ApiCallers.cpp).
SIZE_UNKNOWN(CoinGameReg);
struct CoinGameReg {
    char m_pad0[0x130];
    i32 m_130; // +0x130
    i32 m_134; // +0x134 replay-active flag
};
extern "C" CoinGameReg* g_gameReg;

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
    RVA(0x00019f50, 0xb2)
    i32 __stdcall RangeStd(i32 lo, i32 hi) {
        i32 span = hi - lo + 1;
        i32 seed;
        if (span != 0) {
            if (!(g_randSeeded & 1)) {
                g_randSeeded |= 1;
                seed = timeGetTime();
            } else {
                seed = g_randSeed;
            }
            g_randSeed = seed * 214013 + 2531011;
            return lo + ((g_randSeed >> 0x10) & 0x7fff) % span;
        }
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

    // __thiscall coin-flip: deterministic ((m_1c+1)%2) in replay mode, otherwise a
    // once-per-frame random bit lazily seeded from timeGetTime.
    // @interleaver Rng - own-namespace helper COMDAT scattered at 0xda200; RVA-placement.
    RVA(0x000da200, 0x9b)
    i32 CoinFlip::Flip() {
        CoinGameReg* gr = g_gameReg;
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
