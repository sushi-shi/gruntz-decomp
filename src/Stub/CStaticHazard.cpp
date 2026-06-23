#include <rva.h>
// CStaticHazard.cpp - engine-label stubs for CStaticHazard.
//
// CStaticHazard::LoadAttributes2 is the static-hazard's periodic tick/pulse
// (Ghidra mislabeled it "LoadAttributes2" off the RTTI vptr - the body is a
// time-gated animation pulse, the same idiom as the CGrunt::Resolve*Animation
// cluster in Grunt.cpp). Per call:
//   1. Bail (return 0) when the registry is in the gated state
//      (g_gameReg->m_118 != 0 && g_gameReg->m_134 == 1).
//   2. Period gate: with the running game clock g_645588, compute the phase
//      `(g_645588 - m_54) - m_10->m_118` and bail unless that phase, taken
//      modulo `m_5c + m_58`, lands within `m_58` (the active window). Both the
//      subtract/compare and the divide are UNSIGNED (jbe / div / ja).
//   3. On a hit: latch m_60 = 1, re-arm the animation player geometry off the
//      "LEVEL_STATICHAZARDGO" lookup (caching m_38->m_1b4 into m_40 first), set
//      the active animation to "LEVEL_STATICHAZARD" using the descriptor's first
//      element's m_14 as the 2nd lookup arg (the SetAnimEx idiom), then re-resolve
//      the "B" anim-set node through the global g_buteTree (caching the old m_1c
//      into m_30 first).
//
// Returns int 0 on every path (Ghidra mislabeled the return void; the trailing
// `xor eax,eax` recovers the int-returning shape). /O2 /MT leaf, no /GX frame.
//
// CGameReg/g_gameReg, CButeTree/g_buteTree are the shared aggregate definitions
// (ApiCallers.cpp / CButeTree.cpp, both #included before this file in All.cpp).
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing.

// The running game clock (DAT_00645588; low 32 bits of the engine counter).
extern "C" unsigned int g_645588;

// The "LEVEL_STATICHAZARDGO" / "LEVEL_STATICHAZARD" lookup keys + the "B" anim-set
// key (original .data string literals; objdiff matches these by value).
#define s_LEVEL_STATICHAZARDGO "LEVEL_STATICHAZARDGO"
#define s_LEVEL_STATICHAZARD "LEVEL_STATICHAZARD"
#define s_keyB "B"

// ---------------------------------------------------------------------------
// The animation player @hazard+0x38 (a CGruntAnimState-like). ApplyLookupGeometry
// re-arms its geometry off a named lookup (engine 0x1505b0, __thiscall ret 8);
// SetAnimEx sets the active animation with a frame seed (engine 0x1504d0,
// __thiscall ret 8). m_1b4 is the active-anim descriptor pointer.
// ---------------------------------------------------------------------------
struct CHazAnimElem {
    char m_pad0[0x14];
    int m_14; // +0x14
};

struct CHazAnimDescColl {
    char m_pad0[0xc];
    CHazAnimElem** m_c; // +0x0c  element vector (first elem = *m_c)
    int m_10;           // +0x10  element count (>0 gate)
};

class CHazAnimPlayer {
public:
    int ApplyLookupGeometry(const char* name, int hint); // 0x1505b0 (ret 8)
    void SetAnimEx(const char* key, int frame);          // 0x1504d0 (ret 8)

    char m_pad0[0x1b4];
    CHazAnimDescColl* m_1b4; // +0x1b4  active-anim descriptor
};

// The host @hazard+0x10 (the level/geometry source); m_118 is the period base.
struct CHazHost {
    char m_pad0[0x118];
    unsigned m_118; // +0x118  period base
};

// The anim-set node holder @hazard+0x14; m_1c caches the resolved anim-set node.
struct CHazAnimNode {
    char m_pad0[0x1c];
    void* m_1c; // +0x1c
};

class CStaticHazard {
public:
    CStaticHazard(int);
    int LoadAttributes2();
    void LoadAttributes();

    char m_pad0[0x10];
    CHazHost* m_10;     // +0x10  level/geometry host
    CHazAnimNode* m_14; // +0x14  anim-set node holder
    char m_pad18[0x30 - 0x18];
    void* m_30;            // +0x30  previous anim-set node cache
    char m_pad34[0x38 - 0x34];
    CHazAnimPlayer* m_38; // +0x38  animation player
    char m_pad3c[0x40 - 0x3c];
    void* m_40; // +0x40  previous descriptor cache
    char m_pad44[0x54 - 0x44];
    unsigned m_54; // +0x54  pulse epoch
    unsigned m_58; // +0x58  active window
    unsigned m_5c; // +0x5c  idle window
    int m_60;      // +0x60  fired flag (set to 1)
};

// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x000fb7a0, 0x2d4)
CStaticHazard::CStaticHazard(int) {}

// @early-stop: 97.86% - logic 100% correct, all reloc operands named. Residual is
// a 1-instr scheduling swap: retail emits `mov [m_60],1` BEFORE reloading `m_38`
// (mov ecx,[esi+0x38]); MSVC5 here hoists the load above the store. The m_38 reload
// is load-bearing (retail re-reads it per call, so caching it in a local diverges),
// so the store/load pair can't be pinned - the documented store-vs-load scheduling
// wall (docs/patterns/statement-schedule-faithful.md). Deferred to the final sweep.
RVA(0x000fc0b0, 0xb2)
int CStaticHazard::LoadAttributes2() {
    CGameReg* reg = g_gameReg;
    if (reg->m_118 != 0 && reg->m_134 == 1) {
        return 0;
    }

    unsigned phase = g_645588 - m_54;
    unsigned base = m_10->m_118;
    if (phase <= base) {
        return 0;
    }

    phase -= base;
    unsigned span = m_5c + m_58;
    if (phase % span > m_58) {
        return 0;
    }

    m_60 = 1;

    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry(s_LEVEL_STATICHAZARDGO, 0);

    CHazAnimDescColl* desc = m_38->m_1b4;
    CHazAnimElem* elem = desc->m_10 > 0 ? *desc->m_c : 0;
    m_38->SetAnimEx(s_LEVEL_STATICHAZARD, elem->m_14);

    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_keyB);
    return 0;
}

// @confidence: high
// @source: decomp-xref
// @stub
RVA(0x000fc1a0, 0x33b)
void CStaticHazard::LoadAttributes() {}
