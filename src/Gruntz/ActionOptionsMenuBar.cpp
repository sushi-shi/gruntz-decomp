#include <DDrawMgr/DDrawSubMgrPages.h>    // the m_drawTarget pages (full def)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Image/CImage.h> // CImage::RenderFrameClipped (0x153810) - m_frame's clipped blit
#include <Io/FileMem.h>   // the serialize stream (CSerialArchive == the real CFileMemBase)

#include <Gruntz/Grunt.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/GameLevel.h> // canonical CGameLevel (m_world->m_level: planeCtx bar rect + main plane)
#include <string.h> // inlined memset / strcpy in Serialize (rep stos / repne scas + rep movs)

#include <Gruntz/GameRegistry.h>  // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/SerialArchive.h> // the shared archive stream (Serialize's Write @+0x30)
#include <Wwd/WwdFile.h>          // CPlaneRender - the canonical plane (world->screen transform)
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/Sprite.h>

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
    CObject* spr_ob = 0;

    m_active = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup("GAME_ACTIONOPTIONZMENUBAR", spr_ob);
    CSprite* spr = static_cast<CSprite*>(spr_ob);
    m_frame = (spr && spr->m_minIndex <= 1 && spr->m_maxIndex >= 1)
                  ? static_cast<CImage*>(spr->m_items.GetAt(1))
                  : 0;
    if (!m_frame) {
        return 0;
    }

    spr_ob = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
        "GAME_INGAMEICONZ_NORMCHIPZ",
        spr_ob
    );
    spr = static_cast<CSprite*>(spr_ob);
    m_normChipSprite = spr;
    if (!spr) {
        return 0;
    }

    spr_ob = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
        "GAME_INGAMEICONZ_HIGHCHIPZ",
        spr_ob
    );
    spr = static_cast<CSprite*>(spr_ob);
    m_highChipSprite = spr;
    if (!spr) {
        return 0;
    }

    spr_ob = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
        "GAME_INGAMEICONZ_GREYCHIPZ",
        spr_ob
    );
    spr = static_cast<CSprite*>(spr_ob);
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
        i32 limit = (g_gameReg->m_world->m_level->m_mainPlane)->m_wrapW;
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
    CGrunt* grunt = (reinterpret_cast<CGrunt**>(g_gameReg->m_cmdGrid))[m_gridX * 15 + m_gridY];
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
                frame = (*p < s->m_minIndex || *p > s->m_maxIndex)
                            ? 0
                            : reinterpret_cast<i32>(static_cast<CImage*>(s->m_items.GetAt(*p)));
                break;
            }
            case 2: {
                CSprite* s = m_highChipSprite;
                frame = (*p < s->m_minIndex || *p > s->m_maxIndex)
                            ? 0
                            : reinterpret_cast<i32>(static_cast<CImage*>(s->m_items.GetAt(*p)));
                break;
            }
            case 3: {
                CSprite* s = m_greyChipSprite;
                frame = (*p < s->m_minIndex || *p > s->m_maxIndex)
                            ? 0
                            : reinterpret_cast<i32>(static_cast<CImage*>(s->m_items.GetAt(*p)));
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
    (g_gameReg->m_world->m_level->m_mainPlane)->WrapCoord(&sx, &sy);

    i32 r[4];
    i32* src = reinterpret_cast<i32*>(&g_gameReg->m_world->m_level->m_planeCtx);
    i32 ctx = reinterpret_cast<i32>(g_gameReg->m_world->m_drawTarget->m_backPair);
    r[0] = src[0];
    r[1] = src[1];
    r[2] = src[2];
    r[3] = src[3];
    m_frame->RenderFrameClipped(reinterpret_cast<void*>(ctx), reinterpret_cast<void*>(sy), reinterpret_cast<void*>(sx), r, 0);

    if (m_button0Frame) {
        i32* src2 = reinterpret_cast<i32*>(&g_gameReg->m_world->m_level->m_planeCtx);
        r[0] = src2[0];
        r[1] = src2[1];
        r[2] = src2[2];
        r[3] = src2[3];
        m_frame->RenderFrameClipped(reinterpret_cast<void*>(ctx), reinterpret_cast<void*>((sy - 0xc)), reinterpret_cast<void*>((sx + 2)), r, 0);
    }
    if (m_button1Frame) {
        i32* src3 = reinterpret_cast<i32*>(&g_gameReg->m_world->m_level->m_planeCtx);
        r[0] = src3[0];
        r[1] = src3[1];
        r[2] = src3[2];
        r[3] = src3[3];
        m_frame->RenderFrameClipped(reinterpret_cast<void*>(ctx), reinterpret_cast<void*>((sy + 0x10)), reinterpret_cast<void*>((sx + 2)), r, 0);
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
    if ((reinterpret_cast<CGrunt**>(g_gameReg->m_cmdGrid))[m_gridX * 15 + m_gridY] == 0) {
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
    CGruntzMgr* reg = g_gameReg;
    if (reg == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = reg->m_world;
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
        strcpy(tmp, m_normChipSprite->m_name);
    }
    ar->Write(tmp, 0x80);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_highChipSprite) {
        strcpy(tmp, m_highChipSprite->m_name);
    }
    ar->Write(tmp, 0x80);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_greyChipSprite) {
        strcpy(tmp, m_greyChipSprite->m_name);
    }
    ar->Write(tmp, 0x80);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frame) {
            mgr->m_imageRegistry->AnyValueMatches(m_frame, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_button0Frame) {
            mgr->m_imageRegistry->AnyValueMatches(m_button0Frame, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    {
        i32 zero = 0;
        CImage* v20 = m_button1Frame;
        memset(tmp, 0, sizeof(tmp));
        if (v20) {
            mgr->m_imageRegistry->AnyValueMatches(v20, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CActionOptionsMenuBar::Deserialize (0x00009bb0) - the Serialize mirror: read the
// raw field run back, then re-resolve the three chip sprites by name and the
// frame / two button-frame refs by name+bounds-checked frame index through the
// registry name map. __thiscall, ret 4; returns 1, or 0 if the stream / registry
// is absent. [Re-homed from the ex TriggerLoadRec.cpp: "CTriggerLoadRec" was a
// fake view of THIS class - its m_0..m_3c field IO is exactly this layout, and
// the retail body 0x9bb0 sits directly after Serialize 0x9810 in this obj.]
// ---------------------------------------------------------------------------
// @early-stop
// outparam-zeroinit-scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md),
// 92.2%: logic + offsets byte-exact. The indexed-block regalloc (idx pinned in a
// callee-saved reg across the Lookup call) WAS cracked here by the `i32 i = idx;`
// copy (88.9 -> 92.2). Sole residual: the 6 `out = 0` stores - retail SINKS
// `mov [&out],eax` (reusing strlen's `xor eax,eax` zero) past the arg pushes, cl
// HOISTS it after `lea &out`. Tried comma-injecting the store into the call's
// this-expression and a map-receiver temp (regressed to 89%); the store position
// is the MSVC5 scheduler coin-flip, source-invariant.
RVA(0x00009bb0, 0x367)
i32 CActionOptionsMenuBar::Deserialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    CGruntzMgr* gr = g_gameReg;
    if (gr == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = gr->m_world;
    if (mgr == 0) {
        return 0;
    }

    char buf[0x80];
    CObject* out;
    i32 idx;

    s->Read(this, 8); // m_gridX + m_gridY
    s->Read(&m_screenX, 4);
    s->Read(&m_screenY, 4);
    s->Read(&m_loaded, 4);
    s->Read(&m_active, 4);
    s->Read(&m_button0State, 8);
    s->Read(&m_button0Icon, 8);

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        m_normChipSprite = static_cast<CSprite*>(out);
    } else {
        m_normChipSprite = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        m_highChipSprite = static_cast<CSprite*>(out);
    } else {
        m_highChipSprite = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        m_greyChipSprite = static_cast<CSprite*>(out);
    } else {
        m_greyChipSprite = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        CSprite* tt = static_cast<CSprite*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_frame = r;
    } else {
        m_frame = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        CSprite* tt = static_cast<CSprite*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_button0Frame = r;
    } else {
        m_button0Frame = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        mgr->m_imageRegistry->m_10map.Lookup(buf, out);
        CSprite* tt = static_cast<CSprite*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_button1Frame = r;
    } else {
        m_button1Frame = 0;
    }

    return 1;
}

SIZE_UNKNOWN(CActionOptionsMenuBar);
SIZE_UNKNOWN(CDDrawSubMgrPages);
SIZE_UNKNOWN(CSpriteMgr);
SIZE_UNKNOWN(CPlaneRender);
