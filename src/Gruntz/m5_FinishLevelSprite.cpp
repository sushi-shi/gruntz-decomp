// m5_FinishLevelSprite.cpp - the finish-level state-sprite driver (RVA 0x7c3d0).
//
// A 6-way state transition for the "finish level" overlay: entering state 1 looks
// up the GAME\FINISHLEVEL sprite, latches its completion timer and (when the live
// surface is free) plays its sound cue; the other states just reseat the timer or
// run the grunt death animation. Field names are placeholders; only offsets + code
// bytes are load-bearing.
#include <rva.h>

#include <Ints.h>

DATA(0x00645588)
extern i32 g_645588; // free-running clock
extern "C" {
    DATA(0x0061ab20)
    extern i32 g_sndEnabled;
    DATA(0x0061ab24)
    extern i32 g_sndCueTag;
    DATA(0x006bf3c0)
    extern u32 g_killCueClock;
}

class CGrunt {
public:
    i32 ResolveDeathAnimation(); // 0x0455f0
};

// CStatusBarMgr - ConfigureItem pushes a cue; +0x28 carries the cue duration.
class CStatusBarMgr {
public:
    i32 ConfigureItem(i32 a0, i32 a1, i32 a2, i32 a3); // 0x1360d0
    char m_pad00[0x28];
    i32 m_28; // +0x28 cue duration
};

// The named sprite/cue the GAME\FINISHLEVEL lookup resolves.
struct CueObj {
    char m_pad00[0x10];
    CStatusBarMgr* m_10; // +0x10
    i32 m_14;            // +0x14 last-played clock
    i32 m_18;            // +0x18 cue interval
};

class CSpriteHashTable {
public:
    i32 Lookup(const char* szName, CueObj** ppOut);
};

// The status-bar holder: embedded name->cue hash table at +0x10, live-surface gate
// at +0x30.
struct CStatusBarHolder {
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10
    char m_pad14[0x30 - 0x14];
    i32 m_30; // +0x30
};

struct FinishLevelMgr {
    char m_pad00[0x28];
    CStatusBarHolder* m_28; // +0x28
};

class CFinishLevelState {
public:
    void LoadFinishLevelSprite(i32 state);
    char m_pad000[0x22c];
    FinishLevelMgr* m_22c; // +0x22c
    char m_pad230[0x288 - 0x230];
    i32 m_288; // +0x288 phase
    char m_pad28c[0x290 - 0x28c];
    i32 m_290; // +0x290 timer base
    i32 m_294; // +0x294
    i32 m_298; // +0x298 timer duration
    i32 m_29c; // +0x29c
    CGrunt* m_2a0; // +0x2a0
    char m_pad2a4[0x3ec - 0x2a4];
    i32 m_3ec; // +0x3ec last state
    char m_pad3f0[0x400 - 0x3f0];
    i32 m_400; // +0x400
};

// @early-stop
// Dense 6-case switch: code is the right shape but MSVC emits the jump table as a
// separate $L COMDAT while the delinker inlines it into the fn (the jmp table-reloc
// + table data don't pair) - documented ~79% ceiling, docs/patterns/
// switch-jumptable-separate-comdat.md. The case-1 block also carries a /O2
// regalloc/CSE residual (g_sndEnabled/g_sndCueTag hoist, m_22c re-read vs cache).
// Logic + externs match retail. Final sweep.
RVA(0x0007c3d0, 0x1ae)
void CFinishLevelState::LoadFinishLevelSprite(i32 state) {
    switch (state) {
    case 1:
        if (m_288 != 2) {
            CueObj* p = 0;
            m_22c->m_28->m_10map.Lookup("GAME\\FINISHLEVEL", &p);
            m_298 = p->m_10->m_28 + 500;
            m_29c = 0;
            m_290 = g_645588;
            m_294 = 0;
            if (m_22c->m_28->m_30 == 0) {
                p = 0;
                m_22c->m_28->m_10map.Lookup("GAME\\FINISHLEVEL", &p);
                if (p != 0 && g_sndEnabled != 0 &&
                    (u32)(g_killCueClock - p->m_14) >= (u32)p->m_18) {
                    p->m_14 = g_killCueClock;
                    p->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                }
            }
            m_288 = 1;
            m_400 = 0;
            m_3ec = state;
            return;
        }
        goto Lab_56b;
    case 2:
        m_288 = 1;
        break;
    case 3:
        if (m_288 == 0) {
            m_288 = 2;
            if (m_2a0 != 0) {
                m_2a0->ResolveDeathAnimation();
            }
        }
        goto Lab_522;
    case 4:
        m_288 = 2;
        m_298 = 3000;
        m_29c = 0;
        m_290 = g_645588;
        goto Lab_565;
    case 5:
        m_288 = 2;
        break;
    case 6:
        m_288 = 2;
    Lab_522:
        m_298 = 3000;
        m_29c = 0;
        m_290 = g_645588;
        goto Lab_565;
    default:
        return;
    }
    m_298 = 3000;
    m_29c = 0;
    m_290 = g_645588;
Lab_565:
    m_294 = 0;
Lab_56b:
    m_400 = 0;
    m_3ec = state;
}
