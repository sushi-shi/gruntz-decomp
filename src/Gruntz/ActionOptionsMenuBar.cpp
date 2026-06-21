#include <rva.h>
// ActionOptionsMenuBar.cpp - CActionOptionsMenuBar::LoadAssets, the in-game
// action/option menu bar (C:\Proj\Gruntz). It pulls four named sprites out of
// the engine's string-keyed sprite-set hash table (the same idiom as
// SpriteLoaders.cpp's CTimer::LoadTimerSprite: g_gameReg->m_30->m_10->m_10map.
// Lookup(key, &spr)) and caches them off `this`:
//   GAME_ACTIONOPTIONZMENUBAR -> frame[1]   into this+0x10 (range-guarded; on a
//                                miss/out-of-range it caches 0 and returns 0)
//   GAME_INGAMEICONZ_NORMCHIPZ -> sprite ptr into this+0x30 (return 0 if missing)
//   GAME_INGAMEICONZ_HIGHCHIPZ -> sprite ptr into this+0x34 (return 0 if missing)
//   GAME_INGAMEICONZ_GREYCHIPZ -> sprite ptr into this+0x38 (return 0 if missing)
// then sets this+0x3c=1 and returns 1. this+0x2c is cleared up front.
//
// Only offsets / code bytes are load-bearing; names are placeholders. The engine
// sprite-set types mirror SpriteLoaders.cpp (the shared C:\Proj\incs sprite-set
// header in the original); reconstructed minimally here so the
// `mov edx,[g_gameReg+0x30]; mov ecx,[edx+0x10]; add ecx,0x10; call Lookup`
// shape reloc-masks against the matched lookup helper.
//
// Byte-content IDENTICAL to retail modulo the same MSVC5 scheduling coin-flip
// SpriteLoaders.cpp documents: per lookup the target SINKS the out-param's
// zero-init store (`mov [&spr],0`) past the `lea &spr`/arg-pushes; our cl emits
// it before. Same instruction multiset, permuted by the scheduler (reloc-masked
// byte-diff: only those 2-3 instructions per block reorder, ~85% fuzzy). This is
// the entropy tail (.claude/agents/orchestrator.md §2a) - source-invariant under /O2 (verified:
// block-scoped locals and an explicit `&spr` temp both reproduce the identical
// schedule). Kept as the canonical developer shape, not strict-exact.

// ---------------------------------------------------------------------------
// The engine string-keyed sprite-set hash table and the objects reached through
// the game-manager singleton (g_gameReg->m_30->m_10->m_10map). Mirrors
// SpriteLoaders.cpp.
// ---------------------------------------------------------------------------
struct CSprite;
class CSpriteHashTable {
public:
    int Lookup(char* szName, CSprite** ppOut);
};

struct CSprite {
    char m_pad00[0x14];
    int** m_14; // +0x14  frame-pointer table
    char m_pad18[0x64 - 0x18];
    int m_64; // +0x64  first valid frame number
    int m_68; // +0x68  last valid frame number
};

struct CSpriteMgr {
    char m_pad00[0x10];
    CSpriteHashTable m_10map; // +0x10  the name->sprite hash table
};

struct CResMgr {
    char m_pad00[0x10];
    CSpriteMgr* m_10; // +0x10
};

struct CGameReg {
    char m_pad00[0x30];
    CResMgr* m_30; // +0x30
};
DATA(0x24556c)
extern CGameReg* g_gameReg;

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::LoadAssets
// ---------------------------------------------------------------------------
class CActionOptionsMenuBar {
public:
    int LoadAssets();

    char m_pad00[0x10];
    int* m_10; // +0x10  menu-bar frame 1
    char m_pad14[0x2c - 0x14];
    int m_2c;      // +0x2c  cleared up front
    CSprite* m_30; // +0x30  norm-chip sprite
    CSprite* m_34; // +0x34  high-chip sprite
    CSprite* m_38; // +0x38  grey-chip sprite
    int m_3c;      // +0x3c  loaded flag (set to 1)
};

RVA(0x90e0, 0x100)
int CActionOptionsMenuBar::LoadAssets() {
    CSprite* spr = 0;

    m_2c = 0;
    g_gameReg->m_30->m_10->m_10map.Lookup("GAME_ACTIONOPTIONZMENUBAR", &spr);
    m_10 = (spr && spr->m_64 <= 1 && spr->m_68 >= 1) ? spr->m_14[1] : 0;
    if (!m_10) {
        return 0;
    }

    spr = 0;
    g_gameReg->m_30->m_10->m_10map.Lookup("GAME_INGAMEICONZ_NORMCHIPZ", &spr);
    m_30 = spr;
    if (!spr) {
        return 0;
    }

    spr = 0;
    g_gameReg->m_30->m_10->m_10map.Lookup("GAME_INGAMEICONZ_HIGHCHIPZ", &spr);
    m_34 = spr;
    if (!spr) {
        return 0;
    }

    spr = 0;
    g_gameReg->m_30->m_10->m_10map.Lookup("GAME_INGAMEICONZ_GREYCHIPZ", &spr);
    m_38 = spr;
    if (!spr) {
        return 0;
    }

    m_3c = 1;
    return 1;
}
