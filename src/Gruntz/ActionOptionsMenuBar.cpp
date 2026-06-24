#include <rva.h>
#include <string.h> // inlined memset / strcpy in Serialize (rep stos / repne scas + rep movs)
// ActionOptionsMenuBar.cpp - CActionOptionsMenuBar, the in-game action/option
// menu bar (C:\Proj\Gruntz). It caches four named sprites at LoadAssets and then
// positions / refreshes / hit-tests / serializes a two-button bar (plus three
// option chips) keyed off the grunt under the cursor.
//
// Only offsets / code bytes are load-bearing; field names are placeholders.
// The engine objects reached through g_gameReg are modelled minimally (the same
// WwdGameReg singleton SpriteLoaders.cpp / KitchenSlime.cpp tap); every
// cross-class callee is declared NO-body so its `call`/`mov ds:` reloc-masks.

// ---------------------------------------------------------------------------
// Engine objects reached through the game-manager singleton (g_gameReg).
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

// The world->screen transform object (g_gameReg->m_30->m_24->m_5c). m_30 is the
// world width used to clamp the bar position. WrapCoord is NO-body so its
// __thiscall `call 0xa000` reloc-masks (WwdFile::WwdFile_00a000).
struct CViewport {
    void WrapCoord(int* px, int* py);
    char m_pad00[0x30];
    int m_30; // +0x30  world width
};

// The level/view object (g_gameReg->m_30->m_24): +0x10 is the on-screen bar RECT,
// +0x5c the viewport transform.
struct CMenuViewObj {
    char m_pad00[0x10];
    int m_10rect[4]; // +0x10  on-screen bar RECT (left,top,right,bottom)
    char m_pad20[0x5c - 0x20];
    CViewport* m_5c; // +0x5c
};

// The active draw surface (g_gameReg->m_30->m_04): +0x14 is the draw context.
struct CDrawTarget {
    char m_pad00[0x14];
    int m_14; // +0x14  draw context
};

// The game's resource/level manager (g_gameReg->m_30).
struct CResMgr {
    char m_pad00[0x4];
    CDrawTarget* m_04; // +0x04  active draw surface
    char m_pad08[0x10 - 0x8];
    CSpriteMgr* m_10; // +0x10  sprite manager (LoadAssets lookups)
    char m_pad14[0x24 - 0x14];
    CMenuViewObj* m_24; // +0x24  level/view object
};

// The grunt/logic record stored in the level grid object table (g_gameReg->m_68);
// the bar polls a handful of its fields to pick which mode-chip to light.
struct CGruntRec {
    int IsBusy(); // NO-body -> reloc-masks (CUserLogic::CUserLogic_0514a0)
    char m_pad000[0x170];
    int m_170; // +0x170  grunt kind
    char m_pad174[0x194 - 0x174];
    int m_194; // +0x194  alt mode value
    int m_198; // +0x198  secondary icon
    int m_19c; // +0x19c  primary mode value
};

struct CGameReg {
    char m_pad00[0x30];
    CResMgr* m_30; // +0x30
    char m_pad34[0x68 - 0x34];
    CGruntRec** m_68; // +0x68  grid object table (grid-linear index)
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// The menu-bar frame (this->m_10) doubles as the engine drawable that paints the
// bar/chips; +0x10 is its draw entry. NO-body so `call 0x153810` reloc-masks
// (ClassUnknown_31::ClassUnknown_31_153810).
struct CMenuBarFrame {
    void Draw(int ctx, int x, int y, int* rect, int flag);
};

// Per-serialize round counter the CString archive helpers bump (g_serialCount).
DATA(0x00229ad0)
extern int g_serialCount;

// The CString-read helper (0x155630): receiver = g_gameReg->m_30->m_10.
// NO-body -> reloc-masks (CDDrawWorkerRegistry::CDDrawWorkerRegistry_155630).
struct CStrReader {
    void ReadField(int dst, char* tmp, int* outZero);
};

// The archive (CArchive) passed to Serialize; the read/write entry is the virtual
// at vtable byte-offset 0x30 (slot 12). Modeled polymorphic (slot decls only, never
// defined -> no ??_7 here) so `ar->Transfer(buf,n)` lowers to the exact
// `mov eax,[ar]; push n; push buf; mov ecx,ar; call [eax+0x30]` __thiscall dispatch.
struct CMenuArchive {
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Transfer(void* buf, int n); // +0x30
};

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar
// ---------------------------------------------------------------------------
class CActionOptionsMenuBar {
public:
    CActionOptionsMenuBar();
    void Init(int a, int b, int x, int y, int gx, int gy);
    void Clear();
    int Activate(int a);
    int Refresh();
    int Render();
    int HitClick(int mx, int my);
    int HitHover(int mx, int my);
    void Deactivate();
    int Serialize(CMenuArchive* ar);
    int LoadAssets();

    int m_00;      // +0x00  grid X
    int m_04;      // +0x04  grid Y
    int m_08;      // +0x08  screen X (clamped)
    int m_0c;      // +0x0c  screen Y (adjusted)
    int* m_10;     // +0x10  menu-bar frame 1 (the drawable)
    int m_14;      // +0x14  button[0] state
    int m_18;      // +0x18  button[1] state
    int m_1c;      // +0x1c  button[0] resolved frame
    int m_20;      // +0x20  button[1] resolved frame
    int m_24;      // +0x24  button[0] icon
    int m_28;      // +0x28  button[1] icon
    int m_2c;      // +0x2c  active flag
    CSprite* m_30; // +0x30  norm-chip sprite
    CSprite* m_34; // +0x34  high-chip sprite
    CSprite* m_38; // +0x38  grey-chip sprite
    int m_3c;      // +0x3c  loaded flag
};

// ===========================================================================
// Definitions in ascending retail-RVA order.
// ===========================================================================

// @early-stop
// MSVC5 emits a two-zero-register (ecx+edx) esi-base paired-store form for the
// adjacent (m_1c,m_20)/(m_24,m_28)/(m_14,m_18) zero-inits; our cl emits single
// `mov [this+off],0` stores. Same member-init set/order, regalloc/addressing-mode
// wall - no source spelling reproduces the base-register pairing. Logic exact.
RVA(0x00009090, 0x32)
CActionOptionsMenuBar::CActionOptionsMenuBar() {
    m_10 = 0;
    m_30 = 0;
    m_34 = 0;
    m_38 = 0;
    m_1c = 0;
    m_20 = 0;
    m_24 = 0;
    m_28 = 0;
    m_14 = 0;
    m_18 = 0;
    m_3c = 0;
}

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::LoadAssets - cache the four named sprites.
// ---------------------------------------------------------------------------
// @early-stop
// Lookup out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-
// scheduling.md): per Lookup the target sinks the `mov [&spr],0` past the arg
// pushes; our cl emits it before. Identical multiset, permuted. Logic exact.
RVA(0x000090e0, 0x100)
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

// @early-stop
// Store-block scheduling wall: the six member stores (precomputed clamped-x /
// adjusted-y + four arg loads) are the same instruction multiset as retail but
// our cl sinks the precomputed `m_0c=eax` store one slot early. Logic exact.
RVA(0x00009220, 0x8f)
void CActionOptionsMenuBar::Init(int gx, int a, int x, int y, int b, int gy) {
    if (m_2c) {
        return;
    }
    if (x - 0x25 < 0) {
        x = 0x25;
    } else {
        int limit = g_gameReg->m_30->m_24->m_5c->m_30;
        if (x + 0x25 >= limit) {
            x = limit - 0x26;
        }
    }
    int ym = y - 0x34;
    int yy;
    if (ym - 0x19 >= 0) {
        yy = ym;
    } else {
        yy = y + 0x34;
    }
    m_08 = x;
    m_00 = b;
    m_0c = yy;
    m_18 = a;
    m_04 = gy;
    m_14 = gx;
    if (Refresh()) {
        m_2c = 1;
    }
}

RVA(0x000092e0, 0x8)
void CActionOptionsMenuBar::Clear() {
    m_3c = 0;
}

RVA(0x00009300, 0x14)
int CActionOptionsMenuBar::Activate(int a) {
    if (m_2c) {
        Refresh();
    }
    return 1;
}

// @early-stop
// Regalloc wall: retail pins `this` in ebx and the loop walk-pointer in eax;
// our cl picks edi for `this` and ecx for the walk-pointer, plus an earlier
// g_gameReg load - the choice cascades through every [this+off] encoding and the
// frame is 6 bytes short of 310 (size mismatch -> no per-fn %). Logic exact.
RVA(0x00009330, 0x136)
int CActionOptionsMenuBar::Refresh() {
    CGruntRec* grunt = g_gameReg->m_68[m_00 * 15 + m_04];
    if (grunt != 0) {
        m_28 = grunt->m_198;
        if (grunt->m_170 >= 0x17) {
            m_18 = 3;
        } else if (m_18 == 3) {
            m_18 = 1;
        }
        int prim = (grunt->m_170 > 0x16) ? grunt->m_19c : grunt->m_170;
        m_24 = prim;
        if (prim == 0) {
            m_24 = 0x21;
        } else if (prim == 3) {
            m_24 = grunt->m_194;
        }
        if (!grunt->IsBusy()) {
            m_14 = 3;
        } else if (m_14 == 3) {
            m_14 = 1;
        }
    } else {
        m_28 = 0;
        m_24 = 0;
    }
    // Refresh both buttons: icon in m_24/m_28, state in m_14/m_18, resolved frame
    // into m_1c/m_20.
    int* p = &m_24;
    int n = 2;
    do {
        if (*p == 0) {
            p[-4] = 0;
        } else if (p[-4] == 0) {
            p[-4] = 1;
        }
        int frame;
        switch (p[-4]) {
            case 1: {
                CSprite* s = m_30;
                frame = (*p < s->m_64 || *p > s->m_68) ? 0 : (int)s->m_14[*p];
                break;
            }
            case 2: {
                CSprite* s = m_34;
                frame = (*p < s->m_64 || *p > s->m_68) ? 0 : (int)s->m_14[*p];
                break;
            }
            case 3: {
                CSprite* s = m_38;
                frame = (*p < s->m_64 || *p > s->m_68) ? 0 : (int)s->m_14[*p];
                break;
            }
            default:
                frame = 0;
                break;
        }
        p[-2] = frame;
        p += 1;
    } while (--n != 0);
    return 1;
}

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::Render - paint the bar + chip indicators.
// ---------------------------------------------------------------------------
// @early-stop
// Scheduling/regalloc wall: the draw block is near byte-identical, but our cl
// folds the bar-RECT base into `0x10(%eax)` addressing where retail materializes
// `add eax,0x10` then `(%eax)`, and reads m_08/m_0c in the opposite order; the
// register/addressing-mode choices cascade. Logic exact (size matches at 305).
RVA(0x000094c0, 0x131)
int CActionOptionsMenuBar::Render() {
    if (!m_2c) {
        return 1;
    }
    int sx = m_08;
    int sy = m_0c;
    g_gameReg->m_30->m_24->m_5c->WrapCoord(&sx, &sy);

    int r[4];
    int* src = g_gameReg->m_30->m_24->m_10rect;
    int ctx = g_gameReg->m_30->m_04->m_14;
    r[0] = src[0];
    r[1] = src[1];
    r[2] = src[2];
    r[3] = src[3];
    ((CMenuBarFrame*)m_10)->Draw(ctx, sy, sx, r, 0);

    if (m_1c) {
        int* src2 = g_gameReg->m_30->m_24->m_10rect;
        r[0] = src2[0];
        r[1] = src2[1];
        r[2] = src2[2];
        r[3] = src2[3];
        ((CMenuBarFrame*)m_10)->Draw(ctx, sy - 0xc, sx + 2, r, 0);
    }
    if (m_20) {
        int* src3 = g_gameReg->m_30->m_24->m_10rect;
        r[0] = src3[0];
        r[1] = src3[1];
        r[2] = src3[2];
        r[3] = src3[3];
        ((CMenuBarFrame*)m_10)->Draw(ctx, sy + 0x10, sx + 2, r, 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::HitClick - hit-test a click against the two buttons.
// ---------------------------------------------------------------------------
// @early-stop
// Regalloc wall: structure (spilled &m_14, shared y/x bounds) matches retail, but
// our cl assigns `my`->ebx and the bounds to edi/esi where retail uses ebp and
// ebx/edi; the naming cascade is the residual. Logic exact.
RVA(0x00009650, 0xcf)
int CActionOptionsMenuBar::HitClick(int mx, int my) {
    if (!m_2c) {
        return 1;
    }
    if (g_gameReg->m_68[m_00 * 15 + m_04] == 0) {
        return 1;
    }
    // Demote any held (==2) button back to armed (==1).
    int* btn = &m_14;
    int* p = btn;
    int k = 2;
    do {
        if (*p == 2) {
            *p = 1;
        }
        ++p;
    } while (--k != 0);

    int y0 = m_0c;
    int ylo = y0 - 0xa;
    int yhi = y0 + 0xe;
    int x0 = m_08;
    // Button[0] box.
    if (mx < x0 && mx >= x0 - 0x18 && my < yhi && my >= ylo) {
        if (*btn == 1) {
            *btn = 2;
        }
        return 1;
    }
    // Button[1] box.
    if (mx < x0 + 0x1c && mx >= x0 + 0x4 && my < yhi && my >= ylo) {
        if (m_18 == 1) {
            m_18 = 2;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::HitHover - hover hit-test (returns a button id or 0).
// ---------------------------------------------------------------------------
// @early-stop
// Regalloc wall (~89%): retail keeps y0 in eax to derive both bounds, THEN reuses
// eax for `my`; our cl reads `my` early into ebp and puts the bounds in ebx/edi.
// Same shape, register/scheduling residual. Logic exact.
RVA(0x00009760, 0x6c)
int CActionOptionsMenuBar::HitHover(int mx, int my) {
    if (!m_2c) {
        return 0;
    }
    int y0 = m_0c;
    int x0 = m_08;
    int ylo = y0 - 0xc;
    int yhi = y0 + 0xc;
    if (mx < x0 && mx >= x0 - 0x18 && my < yhi && my >= ylo && m_14 != 3) {
        return 2;
    }
    if (mx < x0 + 0x18 && mx >= x0 && my < yhi && my >= ylo && m_18 == 3) {
        return 3;
    }
    return 0;
}

RVA(0x000097f0, 0x8)
void CActionOptionsMenuBar::Deactivate() {
    m_2c = 0;
}

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::Serialize - read this bar's state from an archive.
// ---------------------------------------------------------------------------
// @early-stop
// Stack-packing wall (~96%): retail reuses the dead g_gameReg->m_30 spill slot
// ([esp+0x10]) for the per-block `zero` int, giving a 0x84 frame; our cl gives
// `zero` its own slot -> 0x88 frame, which shifts every frame-size immediate and
// arg offset by 4. Body (vtable Transfer dispatch + inlined memset/strcpy) exact.
RVA(0x00009810, 0x2df)
int CActionOptionsMenuBar::Serialize(CMenuArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    CGameReg* reg = g_gameReg;
    if (reg == 0) {
        return 0;
    }
    CResMgr* mgr = reg->m_30;
    if (mgr == 0) {
        return 0;
    }

    ar->Transfer(this, 8);
    ar->Transfer(&m_08, 4);
    ar->Transfer(&m_0c, 4);
    ar->Transfer(&m_3c, 4);
    ar->Transfer(&m_2c, 4);
    ar->Transfer(&m_14, 8);
    ar->Transfer(&m_24, 8);

    char tmp[0x80];

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    if (m_30) {
        strcpy(tmp, (char*)m_30 + 0x24);
    }
    ar->Transfer(tmp, 0x80);

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    if (m_34) {
        strcpy(tmp, (char*)m_34 + 0x24);
    }
    ar->Transfer(tmp, 0x80);

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    if (m_38) {
        strcpy(tmp, (char*)m_38 + 0x24);
    }
    ar->Transfer(tmp, 0x80);

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    {
        int zero = 0;
        if (m_10) {
            ((CStrReader*)mgr->m_10)->ReadField((int)m_10, tmp, &zero);
        }
        ar->Transfer(tmp, 0x80);
        ar->Transfer(&zero, 4);
    }

    g_serialCount++;
    memset(tmp, 0, sizeof(tmp));
    {
        int zero = 0;
        if (m_1c) {
            ((CStrReader*)mgr->m_10)->ReadField((int)m_1c, tmp, &zero);
        }
        ar->Transfer(tmp, 0x80);
        ar->Transfer(&zero, 4);
    }

    g_serialCount++;
    {
        int zero = 0;
        int v20 = m_20;
        memset(tmp, 0, sizeof(tmp));
        if (v20) {
            ((CStrReader*)mgr->m_10)->ReadField(v20, tmp, &zero);
        }
        ar->Transfer(tmp, 0x80);
        ar->Transfer(&zero, 4);
    }
    return 1;
}
