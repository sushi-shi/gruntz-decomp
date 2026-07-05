// FinishLevelSprite.cpp - the finish-level state-sprite driver (RVA 0x7c3d0).
//
// A 6-way state transition for the "finish level" overlay: entering state 1 looks
// up the GAME\FINISHLEVEL sprite, latches its completion timer and (when the live
// surface is free) plays its sound cue; the other states just reseat the timer or
// run the grunt death animation. Field names are placeholders; only offsets + code
// bytes are load-bearing.
#include <rva.h>

#include <Gruntz/StatusBarCueHolder.h> // the ONE cue-holder shape (CueObj/CCueHashTable/CStatusBarHolder)
#include <Gruntz/Grunt.h>              // the ONE CGrunt definition (dedup; ResolveDeathAnimation)
#include <Gruntz/SoundCueMgr.h>        // the ONE CSoundCueMgr shape (ConfigureItem @0x1360d0)
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

// CSoundCueMgr - ConfigureItem pushes a cue; +0x28 carries the cue duration (both
// modeled in <Gruntz/SoundCueMgr.h>). CueObj / CCueHashTable / CStatusBarHolder
// (the cue-holder shape: name->cue hash @+0x10, live-surface gate @+0x30) are the
// shared shape in <Gruntz/StatusBarCueHolder.h>.

SIZE_UNKNOWN(FinishLevelMgr);
struct FinishLevelMgr {
    char m_pad00[0x28];
    CStatusBarHolder* m_28; // +0x28
};

SIZE_UNKNOWN(CFinishLevelState);
class CFinishLevelState {
public:
    void LoadFinishLevelSprite(i32 state);
    char m_pad000[0x22c];
    FinishLevelMgr* m_22c; // +0x22c
    char m_pad230[0x288 - 0x230];
    i32 m_288; // +0x288 phase
    char m_pad28c[0x290 - 0x28c];
    i32 m_290;     // +0x290 timer base
    i32 m_294;     // +0x294
    i32 m_298;     // +0x298 timer duration
    i32 m_29c;     // +0x29c
    CGrunt* m_2a0; // +0x2a0
    char m_pad2a4[0x3ec - 0x2a4];
    i32 m_3ec; // +0x3ec last state
    char m_pad3f0[0x400 - 0x3f0];
    i32 m_400; // +0x400
};

// @early-stop
// Dense 6-case switch (~61%). Two stacked residuals, both confirmed by llvm-objdump
// -dr base vs target: (1) the jump-table artifact - MSVC emits the table as a
// separate $L COMDAT (jmp reloc -> $L19166), the delinker inlines it at fn+0x1b0
// (jmp reloc -> fn) - documented ~79% ceiling, docs/patterns/
// switch-jumptable-separate-comdat.md. (2) case-1 /O2 scheduling below that ceiling:
// caching CStatusBarHolder* h28 = m_22c->m_28 recovered the m_28 share between the
// m_30 check and the 2nd Lookup (58->61), but MSVC still (a) re-materializes edi=0
// with a redundant `xor edi,edi` (the header zero doesn't propagate through the
// indirect switch jmp in this build) and (b) HOISTS the m_22c reload for h28 up into
// the m_298 computation, which reorders the independent m_298/m_29c stores. Cases
// 2-6 are byte-identical to retail; only case-1 instruction scheduling diverges,
// not source-steerable.
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
                CStatusBarHolder* h28 = m_22c->m_28;
                if (h28->m_30 == 0) {
                    p = 0;
                    h28->m_10map.Lookup("GAME\\FINISHLEVEL", &p);
                    if (p != 0 && g_sndEnabled != 0
                        && (u32)(g_killCueClock - p->m_14) >= (u32)p->m_18) {
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
