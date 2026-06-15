// SpriteLoaders.cpp - two sibling HUD/UI sprite loaders that pull a named sprite
// out of the engine's string-keyed sprite-set hash table and cache individual
// animation frames off it (C:\Proj\Gruntz). Both share the same idiom:
//   1. look the sprite up by class-name string through the matched sprite-set
//      hash table (CSpriteHashTable::Lookup @0x1b8008, __thiscall, external);
//   2. for each wanted frame number N, extract the frame pointer from the
//      sprite's frame table ONLY when N lies inside the sprite's valid frame
//      range [m_firstFrame(+0x64) .. m_lastFrame(+0x68)], else cache 0.
//
//   LoadTimerSprite      @0x9bb00 (281 B, __thiscall ret 8) - the in-game TIMER
//       sprite ("GAME_TIMER"); looked up through the game-manager singleton
//       g_gameReg @0x64556c (->+0x30 ->+0x10). Caches frames 10/11 and bails to
//       0 if any required frame is missing.
//   LoadLoadingBarSprite @0xd7440 (173 B, __thiscall void) - the loading-screen
//       progress-bar sprite ("GAME_LOADINGBAR"); looked up through this->m_c
//       (->+0x10). Caches frames 1/2/3 and a loaded flag.
//
// Only offsets / code bytes are load-bearing; names are placeholders.
//
// BYTE-EXACT bodies modulo one MSVC5 scheduling coin-flip: the target SINKS the
// lookup out-param's zero-init store (`mov [&spr],0`) to just after the two arg
// pushes; our cl HOISTS it before the lea - the same 2-3 instructions, permuted
// (byte-content identical). See config/units.toml. Kept wip, not strict-exact.

// ---------------------------------------------------------------------------
// The engine string-keyed sprite-set hash table. Lookup() hashes the class-name
// key, finds the entry, and writes the found sprite pointer through *ppOut
// (returning a found-flag). Modeled minimally so the `ecx=<map>; call 0x1b8008`
// shape reloc-masks against the matched lookup helper.
// ---------------------------------------------------------------------------
struct CSprite;
class CSpriteHashTable {
public:
    int Lookup(char *szName, CSprite **ppOut);   // @0x1b8008 (__thiscall ret 8)
};

// The engine sprite (animation) object. Only the load-bearing members are
// reconstructed: a frame-pointer table at +0x14 and the inclusive valid frame
// range [m_64 .. m_68].
struct CSprite {
    char   m_pad00[0x14];
    int  **m_14;         // +0x14  frame-pointer table
    char   m_pad18[0x64 - 0x18];
    int    m_64;         // +0x64  first valid frame number
    int    m_68;         // +0x68  last valid frame number
};

// The sprite manager: its name->sprite hash table is embedded at +0x10 (the
// `add ecx,0x10` before the lookup call addresses it).
struct CSpriteMgr {
    char             m_pad00[0x10];
    CSpriteHashTable m_10map;       // +0x10  the name->sprite hash table
};

// The intermediate resource object both loaders reach the sprite manager through:
// its +0x10 slot points at the CSpriteMgr.
struct CResMgr {
    char        m_pad00[0x10];
    CSpriteMgr *m_10;               // +0x10
};

// The loading bar reaches the resource object through this->m_c.
// The game-manager singleton (g_gameReg) reaches it through g_gameReg->m_30.
struct CGameReg {
    char     m_pad00[0x30];
    CResMgr *m_30;                  // +0x30
};
// @data: 0x24556c
extern CGameReg *g_gameReg;

// ---------------------------------------------------------------------------
// CLoadingBar::LoadLoadingBarSprite  @0xd7440  (__thiscall void)
// ---------------------------------------------------------------------------
class CLoadingBar {
public:
    int LoadLoadingBarSprite();

    char           m_pad00[0xc];
    CResMgr       *m_c;             // +0xc
    char           m_pad10[0x4bc - 0x10];
    int            m_4bc;           // +0x4bc loaded flag (set to 1)
    int           *m_4c0;           // +0x4c0 frame 2
    int           *m_4c4;           // +0x4c4 frame 3
    int           *m_4c8;           // +0x4c8 frame 1
};

// @address: 0xd7440
// @size:    0xad
int CLoadingBar::LoadLoadingBarSprite()
{
    CSprite *spr = 0;
    m_c->m_10->m_10map.Lookup("GAME_LOADINGBAR", &spr);
    if (!spr)
        return 0;

    m_4c8 = (spr->m_64 <= 1 && spr->m_68 >= 1) ? spr->m_14[1] : 0;
    m_4c0 = (spr->m_64 <= 2 && spr->m_68 >= 2) ? spr->m_14[2] : 0;
    m_4c4 = (spr->m_64 <= 3 && spr->m_68 >= 3) ? spr->m_14[3] : 0;
    m_4bc = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CTimer::LoadTimerSprite  @0x9bb00  (__thiscall ret 8)
// ---------------------------------------------------------------------------
class CTimer {
public:
    int LoadTimerSprite(int a, int b);

    int  *m_0;       // +0x00 frame
    int  *m_4;       // +0x04 frame
    int  *m_8;       // +0x08 the looked-up sprite
    int   m_c;       // +0x0c
    int  *m_10;      // +0x10 frame 10
    int  *m_14;      // +0x14 frame 10
    int  *m_18;      // +0x18 frame 10
    int  *m_1c;      // +0x1c frame 10
    int  *m_20;      // +0x20 frame 11
    char  m_pad24[0x48 - 0x24];
    int   m_48;      // +0x48
};

// @address: 0x9bb00
// @size:    0x119
int CTimer::LoadTimerSprite(int a, int b)
{
    CSprite *spr = 0;
    g_gameReg->m_30->m_10->m_10map.Lookup("GAME_TIMER", &spr);
    m_8 = (int *)spr;
    if (!spr)
        return 0;

    m_10 = (spr->m_64 <= 10 && spr->m_68 >= 10) ? spr->m_14[10] : 0;
    if (!m_10)
        return 0;
    m_14 = (spr->m_64 <= 10 && spr->m_68 >= 10) ? spr->m_14[10] : 0;
    if (!m_14)
        return 0;
    m_20 = (spr->m_64 <= 11 && spr->m_68 >= 11) ? spr->m_14[11] : 0;
    if (!m_20)
        return 0;
    m_18 = (spr->m_64 <= 10 && spr->m_68 >= 10) ? spr->m_14[10] : 0;
    if (!m_18)
        return 0;
    m_1c = (spr->m_64 <= 10 && spr->m_68 >= 10) ? spr->m_14[10] : 0;
    if (!m_1c)
        return 0;

    m_0 = (int *)a;          /* the two args captured at the tail */
    m_4 = (int *)b;
    m_c = 1;
    m_48 = 0;
    return 1;
}
