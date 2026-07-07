#include <rva.h>
#include <Gruntz/Grunt.h>
#include <Wwd/WwdFile.h>
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
// CSprite (frame-data) + CSpriteHashTable come from <Gruntz/Sprite.h>; the
// resource manager + its image registry (m_10) from <Gruntz/ResMgr.h>.
// ---------------------------------------------------------------------------
#include <Gruntz/GameRegistry.h>  // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/SerialArchive.h> // the shared archive stream (Serialize's Write @+0x30)
#include <Gruntz/Viewport.h>      // shared world->screen transform
#include <Gruntz/ResMgr.h>
#include <Gruntz/Sprite.h>

// CViewport (world->screen transform, g_gameReg->m_world->m_24->m_5c) is the shared
// <Gruntz/Viewport.h> class: m_worldWidth (+0x30) clamps the bar position;
// WrapCoord is NO-body so its __thiscall `call 0xa000` reloc-masks
// (WwdFile::WwdFile_00a000).

// The level/view object (g_gameReg->m_world->m_24) is the canonical CGameViewport
// (<Gruntz/GameRegistry.h>): +0x10 the on-screen bar RECT, +0x5c the viewport.
// CDrawTarget + CImageRegistry (the m_10 image registry) come from <Gruntz/ResMgr.h>.

// The grunt/logic record stored in the level grid object table (g_gameReg->m_gridObjects);
// the bar polls a handful of its fields to pick which mode-chip to light.
// The canonical CGameRegistry view of the singleton (*0x24556c): the resource mgr
// (+0x30, typed CSpriteFactoryHolder) is reached without a cast; the grid object
// table (+0x68) is a genuinely reused slot cast locally (see below).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The menu-bar frame (this->m_frame) doubles as the engine drawable that paints the
// bar/chips; +0x10 is its draw entry. NO-body so `call 0x153810` reloc-masks
// (tomalla-31::Ctor_153810).
struct CMenuBarFrame {
    void Draw(i32 ctx, i32 x, i32 y, i32* rect, i32 flag);
};

// Per-serialize round counter the CString archive helpers bump (g_serialCounter).
DATA(0x00229ad0)
extern i32 g_serialCounter;

// The frame-name reverse-lookup is CImageRegistry::ReadField (0x155630, mgr->m_10,
// <Gruntz/ResMgr.h>); the former CStrReader view is gone (wave 3).

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar
// ---------------------------------------------------------------------------
class CActionOptionsMenuBar {
public:
    CActionOptionsMenuBar();
    void Init(i32 a, i32 b, i32 x, i32 y, i32 gx, i32 gy);
    void Clear();
    i32 Activate(i32 a);
    i32 Refresh();
    i32 Render();
    i32 HitClick(i32 mx, i32 my);
    i32 HitHover(i32 mx, i32 my);
    void Deactivate();
    i32 Serialize(CSerialArchive* ar);
    i32 LoadAssets();

    i32 m_gridX;               // +0x00  grid X
    i32 m_gridY;               // +0x04  grid Y
    i32 m_screenX;             // +0x08  screen X (clamped)
    i32 m_screenY;             // +0x0c  screen Y (adjusted)
    CMenuBarFrame* m_frame;    // +0x10  menu-bar frame 1 (the drawable)
    i32 m_button0State;        // +0x14  button[0] state
    i32 m_button1State;        // +0x18  button[1] state
    i32 m_button0Frame;        // +0x1c  button[0] resolved frame
    i32 m_button1Frame;        // +0x20  button[1] resolved frame
    i32 m_button0Icon;         // +0x24  button[0] icon
    i32 m_button1Icon;         // +0x28  button[1] icon
    i32 m_active;              // +0x2c  active flag
    CSprite* m_normChipSprite; // +0x30  norm-chip sprite
    CSprite* m_highChipSprite; // +0x34  high-chip sprite
    CSprite* m_greyChipSprite; // +0x38  grey-chip sprite
    i32 m_loaded;              // +0x3c  loaded flag
};

// ===========================================================================
// Definitions in ascending retail-RVA order.
// ===========================================================================

// @early-stop
// MSVC5 emits a two-zero-register (ecx+edx) esi-base paired-store form for the
// adjacent (m_button0Frame,m_button1Frame)/(m_button0Icon,m_button1Icon)/(m_button0State,m_button1State) zero-inits; our cl emits single
// `mov [this+off],0` stores. Same member-init set/order, regalloc/addressing-mode
// wall - no source spelling reproduces the base-register pairing. Logic exact.
RVA(0x00009090, 0x32)
CActionOptionsMenuBar::CActionOptionsMenuBar() {
    m_frame = 0;
    m_normChipSprite = 0;
    m_highChipSprite = 0;
    m_greyChipSprite = 0;
    m_button0Frame = 0;
    m_button1Frame = 0;
    m_button0Icon = 0;
    m_button1Icon = 0;
    m_button0State = 0;
    m_button1State = 0;
    m_loaded = 0;
}

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::LoadAssets - cache the four named sprites.
// ---------------------------------------------------------------------------
// @early-stop
// Lookup out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-
// scheduling.md): per Lookup the target sinks the `mov [&spr],0` past the arg
// pushes; our cl emits it before. Identical multiset, permuted. Logic exact.
RVA(0x000090e0, 0x100)
i32 CActionOptionsMenuBar::LoadAssets() {
    CSprite* spr = 0;

    m_active = 0;
    ((CMapStringToOb*)&g_gameReg->m_world->m_10->m_10map)
        ->Lookup("GAME_ACTIONOPTIONZMENUBAR", (CObject*&)spr);
    m_frame = (spr && spr->m_firstFrame <= 1 && spr->m_lastFrame >= 1)
                  ? (CMenuBarFrame*)spr->m_frames.m_pData[1]
                  : 0;
    if (!m_frame) {
        return 0;
    }

    spr = 0;
    ((CMapStringToOb*)&g_gameReg->m_world->m_10->m_10map)
        ->Lookup("GAME_INGAMEICONZ_NORMCHIPZ", (CObject*&)spr);
    m_normChipSprite = spr;
    if (!spr) {
        return 0;
    }

    spr = 0;
    ((CMapStringToOb*)&g_gameReg->m_world->m_10->m_10map)
        ->Lookup("GAME_INGAMEICONZ_HIGHCHIPZ", (CObject*&)spr);
    m_highChipSprite = spr;
    if (!spr) {
        return 0;
    }

    spr = 0;
    ((CMapStringToOb*)&g_gameReg->m_world->m_10->m_10map)
        ->Lookup("GAME_INGAMEICONZ_GREYCHIPZ", (CObject*&)spr);
    m_greyChipSprite = spr;
    if (!spr) {
        return 0;
    }

    m_loaded = 1;
    return 1;
}

// @early-stop
// Store-block scheduling wall: the six member stores (precomputed clamped-x /
// adjusted-y + four arg loads) are the same instruction multiset as retail but
// our cl sinks the precomputed `m_screenY=eax` store one slot early. Logic exact.
RVA(0x00009220, 0x8f)
void CActionOptionsMenuBar::Init(i32 gx, i32 a, i32 x, i32 y, i32 b, i32 gy) {
    if (m_active) {
        return;
    }
    if (x - 0x25 < 0) {
        x = 0x25;
    } else {
        i32 limit = ((CViewport*)g_gameReg->m_world->m_24->m_5c)->m_worldWidth;
        if (x + 0x25 >= limit) {
            x = limit - 0x26;
        }
    }
    i32 ym = y - 0x34;
    i32 yy;
    if (ym - 0x19 >= 0) {
        yy = ym;
    } else {
        yy = y + 0x34;
    }
    m_screenX = x;
    m_gridX = b;
    m_screenY = yy;
    m_button1State = a;
    m_gridY = gy;
    m_button0State = gx;
    if (Refresh()) {
        m_active = 1;
    }
}

RVA(0x000092e0, 0x8)
void CActionOptionsMenuBar::Clear() {
    m_loaded = 0;
}

RVA(0x00009300, 0x14)
i32 CActionOptionsMenuBar::Activate(i32 a) {
    if (m_active) {
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
i32 CActionOptionsMenuBar::Refresh() {
    CGrunt* grunt = ((CGrunt**)g_gameReg->m_cmdGrid)[m_gridX * 15 + m_gridY];
    if (grunt != 0) {
        m_button1Icon = grunt->m_198;
        if (grunt->m_entranceReason >= 0x17) {
            m_button1State = 3;
        } else if (m_button1State == 3) {
            m_button1State = 1;
        }
        i32 prim = (grunt->m_entranceReason > 0x16) ? grunt->m_19c : grunt->m_entranceReason;
        m_button0Icon = prim;
        if (prim == 0) {
            m_button0Icon = 0x21;
        } else if (prim == 3) {
            m_button0Icon = grunt->m_194;
        }
        if (!grunt->CanShowStamina()) {
            m_button0State = 3;
        } else if (m_button0State == 3) {
            m_button0State = 1;
        }
    } else {
        m_button1Icon = 0;
        m_button0Icon = 0;
    }
    // Refresh both buttons: icon in m_button0Icon/m_button1Icon, state in
    // m_button0State/m_button1State, resolved frame into m_button0Frame/m_button1Frame.
    i32* p = &m_button0Icon;
    i32 n = 2;
    do {
        if (*p == 0) {
            p[-4] = 0;
        } else if (p[-4] == 0) {
            p[-4] = 1;
        }
        i32 frame;
        switch (p[-4]) {
            case 1: {
                CSprite* s = m_normChipSprite;
                frame = (*p < s->m_firstFrame || *p > s->m_lastFrame)
                            ? 0
                            : (i32)s->m_frames.m_pData[*p];
                break;
            }
            case 2: {
                CSprite* s = m_highChipSprite;
                frame = (*p < s->m_firstFrame || *p > s->m_lastFrame)
                            ? 0
                            : (i32)s->m_frames.m_pData[*p];
                break;
            }
            case 3: {
                CSprite* s = m_greyChipSprite;
                frame = (*p < s->m_firstFrame || *p > s->m_lastFrame)
                            ? 0
                            : (i32)s->m_frames.m_pData[*p];
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
// `add eax,0x10` then `(%eax)`, and reads m_screenX/m_screenY in the opposite order; the
// register/addressing-mode choices cascade. Logic exact (size matches at 305).
RVA(0x000094c0, 0x131)
i32 CActionOptionsMenuBar::Render() {
    if (!m_active) {
        return 1;
    }
    i32 sx = m_screenX;
    i32 sy = m_screenY;
    ((CPlaneRender*)g_gameReg->m_world->m_24->m_5c)->WrapCoord(&sx, &sy);

    i32 r[4];
    i32* src = g_gameReg->m_world->m_24->m_barRect;
    i32 ctx = (i32)g_gameReg->m_world->m_drawTarget->m_14;
    r[0] = src[0];
    r[1] = src[1];
    r[2] = src[2];
    r[3] = src[3];
    m_frame->Draw(ctx, sy, sx, r, 0);

    if (m_button0Frame) {
        i32* src2 = g_gameReg->m_world->m_24->m_barRect;
        r[0] = src2[0];
        r[1] = src2[1];
        r[2] = src2[2];
        r[3] = src2[3];
        m_frame->Draw(ctx, sy - 0xc, sx + 2, r, 0);
    }
    if (m_button1Frame) {
        i32* src3 = g_gameReg->m_world->m_24->m_barRect;
        r[0] = src3[0];
        r[1] = src3[1];
        r[2] = src3[2];
        r[3] = src3[3];
        m_frame->Draw(ctx, sy + 0x10, sx + 2, r, 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::HitClick - hit-test a click against the two buttons.
// ---------------------------------------------------------------------------
// @early-stop
// Regalloc wall: structure (spilled &m_button0State, shared y/x bounds) matches retail, but
// our cl assigns `my`->ebx and the bounds to edi/esi where retail uses ebp and
// ebx/edi; the naming cascade is the residual. Logic exact.
RVA(0x00009650, 0xcf)
i32 CActionOptionsMenuBar::HitClick(i32 mx, i32 my) {
    if (!m_active) {
        return 1;
    }
    if (((CGrunt**)g_gameReg->m_cmdGrid)[m_gridX * 15 + m_gridY] == 0) {
        return 1;
    }
    // Demote any held (==2) button back to armed (==1).
    i32* btn = &m_button0State;
    i32* p = btn;
    i32 k = 2;
    do {
        if (*p == 2) {
            *p = 1;
        }
        ++p;
    } while (--k != 0);

    i32 y0 = m_screenY;
    i32 ylo = y0 - 0xa;
    i32 yhi = y0 + 0xe;
    i32 x0 = m_screenX;
    // Button[0] box.
    if (mx < x0 && mx >= x0 - 0x18 && my < yhi && my >= ylo) {
        if (*btn == 1) {
            *btn = 2;
        }
        return 1;
    }
    // Button[1] box.
    if (mx < x0 + 0x1c && mx >= x0 + 0x4 && my < yhi && my >= ylo) {
        if (m_button1State == 1) {
            m_button1State = 2;
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
i32 CActionOptionsMenuBar::HitHover(i32 mx, i32 my) {
    if (!m_active) {
        return 0;
    }
    i32 y0 = m_screenY;
    i32 x0 = m_screenX;
    i32 ylo = y0 - 0xc;
    i32 yhi = y0 + 0xc;
    if (mx < x0 && mx >= x0 - 0x18 && my < yhi && my >= ylo && m_button0State != 3) {
        return 2;
    }
    if (mx < x0 + 0x18 && mx >= x0 && my < yhi && my >= ylo && m_button1State == 3) {
        return 3;
    }
    return 0;
}

RVA(0x000097f0, 0x8)
void CActionOptionsMenuBar::Deactivate() {
    m_active = 0;
}

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::Serialize - read this bar's state from an archive.
// ---------------------------------------------------------------------------
// @early-stop
// Stack-packing wall (~96%): retail reuses the dead g_gameReg->m_world spill slot
// ([esp+0x10]) for the per-block `zero` int, giving a 0x84 frame; our cl gives
// `zero` its own slot -> 0x88 frame, which shifts every frame-size immediate and
// arg offset by 4. Body (vtable Write dispatch @+0x30 + inlined memset/strcpy) exact.
RVA(0x00009810, 0x2df)
i32 CActionOptionsMenuBar::Serialize(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    CGameRegistry* reg = g_gameReg;
    if (reg == 0) {
        return 0;
    }
    CSpriteFactoryHolder* mgr = reg->m_world;
    if (mgr == 0) {
        return 0;
    }

    ar->Write(this, 8);
    ar->Write(&m_screenX, 4);
    ar->Write(&m_screenY, 4);
    ar->Write(&m_loaded, 4);
    ar->Write(&m_active, 4);
    ar->Write(&m_button0State, 8);
    ar->Write(&m_button0Icon, 8);

    char tmp[0x80];

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_normChipSprite) {
        strcpy(tmp, (char*)m_normChipSprite + 0x24);
    }
    ar->Write(tmp, 0x80);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_highChipSprite) {
        strcpy(tmp, (char*)m_highChipSprite + 0x24);
    }
    ar->Write(tmp, 0x80);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_greyChipSprite) {
        strcpy(tmp, (char*)m_greyChipSprite + 0x24);
    }
    ar->Write(tmp, 0x80);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frame) {
            mgr->m_10->ReadField((i32)m_frame, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_button0Frame) {
            mgr->m_10->ReadField((i32)m_button0Frame, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    {
        i32 zero = 0;
        i32 v20 = m_button1Frame;
        memset(tmp, 0, sizeof(tmp));
        if (v20) {
            mgr->m_10->ReadField(v20, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }
    return 1;
}

SIZE_UNKNOWN(CActionOptionsMenuBar);
SIZE_UNKNOWN(CDrawTarget);
SIZE_UNKNOWN(CMenuBarFrame);
SIZE_UNKNOWN(CSpriteHashTable);
SIZE_UNKNOWN(CSpriteMgr);
SIZE_UNKNOWN(CViewport);
