// StatusBarTabBuilders.cpp - the status-bar tab file: the CSbTab "Build*" builders
// + the tab item classes they build (C:\Proj\Gruntz) - ONE original /GX TU
// (interval dossier 0x0e8a70-0x0ea3ea: the builders weave with CSBI_GruntMachine /
// CSBI_SideTab / CSbConfigItem fn-by-fn; single 9-frag sbi_sidetab init run
// @0xe9e20 inside the interval). Merged wave1-E (absorbed SBI_GruntMachine.cpp +
// SBI_SideTab.cpp + the SetDirection/SetDirectionAlt seam pair from
// StatusBarMgr.cpp), strict retail-RVA order.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes are
// load-bearing. The engine callees / globals are reloc-masked (modeled with NO
// body / by-address DATA externs).
#define SBI_DTOR_CHAIN // enable the inline base-dtor body (see StatusBarItem.h)
#include <Mfc.h>       // afx-first (TU pulls MFC via unified CObject; superset of Win32.h)
#include <rva.h>
#include <Ints.h>

#include <Gruntz/SBI_RectOnly.h>     // CSBI_RectOnly (ClearStat, the side-tab owner slot)
#include <Gruntz/GameRegistry.h>     // canonical CGameRegistry (the builders' singleton view)
#include <Gruntz/ResMgr.h>           // canonical CResMgr + CDrawTarget + CImageRegistry
#include <Gruntz/StatusBarItem.h>    // canonical frameless CStatusBarItem base (real RTTI base)
#include <Image/CImage.h>            // the frame handles ARE CImage (RenderFrame @0x153790)
#include <Gruntz/SBI_GruntMachine.h> // canonical CSBI_GruntMachine (vtable @0x5eadbc)
#include <Gruntz/SBI_SideTab.h>      // canonical CSBI_SideTab (vtable @0x5eae3c) + referent views
#include <Gruntz/SbConfigItem.h>     // the builder-facet base (SetDirection/SetDirectionAlt sink)

// The name maps are the real MFC CMapStringToPtr (Lookup @0x1b8008, from <Mfc.h>); no local view.
#include <Gruntz/StatusBarTabBuildersViews.h> // CSbGeom/CSbOwner/.../CSbTab (namespace views)
#include <Image/ImageSet.h>                   // canonical CImageSet (SetAllTypes/SetAllFormats)

// The g_gameReg singleton (VA 0x64556c), shared by the CSBI_GruntMachine /
// CSBI_SideTab methods below: the render chain m_world (+0x30, CResMgr) ->
// m_drawTarget (+0x04) -> m_14 surface context, and the side-tab unit table
// m_68. One TU-wide decl (the CSideTabGameReg view from <Gruntz/SBI_SideTab.h>;
// the builders' namespace-scoped CGameRegistry decl below is its own symbol).
DATA(0x0024556c)
extern "C" CSideTabGameReg* g_gameReg;

namespace StatusBarTabBuilders {

    // The CSbGeom/CSbNamespaceMap/CSbMapHost/CSbOwner/CSbImageSet/CSbParent/
    // CSpriteRefTable/CSbWorldSlot/CSbTab views live in
    // <Gruntz/StatusBarTabBuildersViews.h>.

    // The game registry / settings singleton (*0x24556c) - the canonical
    // CGameRegistry view. The namespace owner (+0x30 -> CSbOwner), sprite-ref table
    // (+0x74 -> CSpriteRefTable) and per-world slot array (+0x138, stride 0x238 ->
    // CSbWorldSlot) are cast locally at the deref sites. C++-namespaced (its OWN symbol,
    // distinct from the file-scope extern "C" _g_gameReg above) so the two typed views of
    // *0x24556c coexist in one TU without an extern "C" type clash (clang -emit-llvm).
    extern CGameRegistry* g_gameReg;
    extern i32 g_644c54; // the current world index (canonical DATA(0x00244c54) in sbi_rectonly)

    // ===========================================================================
    // CSbTab::BuildResourceTabStatusBar  (0xe8a70)
    // ===========================================================================
    // @early-stop
    // identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md,
    // topic:wall): prologue + body are byte-exact (the geometry block groups via the struct-copy
    // idiom, struct-copy-block-store-base-reg.md; the variable-index range checks now emit retail's
    // `cmp idx,[hi]; jg` after spelling them `idx > m_idxHi` instead of `m_idxHi < idx`). Residual:
    // (1) the 3 mid `return 0` guards inline `jne;pop;pop;ret` in retail (eax already 0) but my
    // compile tail-merges all 5 guards into one shared `pop;xor;pop;ret`; (2) the final range-check
    // lands m_imageSet/m_frameIdx in the opposite regs (eax<->ecx free-list coin-flip). ~80.4%; both
    // residuals are documented non-steerable walls. Logic complete.
    RVA(0x000e8a70, 0x18c)
    i32 CSbTab::BuildResourceTabStatusBar(
        CSbParent* parent,
        CSbOwner* statusbar,
        i32 p3,
        i32 p4,
        CSbGeom g,
        char* key,
        i32 idxA,
        i32 idxB
    ) {
        if (statusbar == 0 || parent == 0) {
            return 0;
        }
        CSbOwner* owner = statusbar;
        m_parent = parent;
        m_10 = p4;
        m_owner = owner;
        m_28 = 0;
        m_04 = 1;
        m_geom = g;
        statusbar = 0;
        m_0c = p3;
        ((CMapStringToPtr*)&owner->m_mapHost->m_map)
            ->Lookup("GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND", (void*&)statusbar);
        CSbImageSet* n = (CSbImageSet*)statusbar;
        i32 spr;
        if (n == 0 || n->m_idxLo > 1 || n->m_idxHi < 1) {
            spr = 0;
        } else {
            spr = n->m_formats[1];
        }
        m_44 = spr;
        if (spr == 0) {
            return 0;
        }
        statusbar = 0;
        ((CMapStringToPtr*)&m_owner->m_mapHost->m_map)->Lookup(key, (void*&)statusbar);
        m_imageSet = (CSbImageSet*)statusbar;
        if (statusbar == 0) {
            return 0;
        }
        m_38 = idxA;
        m_frameIdx = idxB;
        i32 s;
        if (idxA < m_imageSet->m_idxLo || idxA > m_imageSet->m_idxHi) {
            s = 0;
        } else {
            s = m_imageSet->m_formats[idxA];
        }
        m_34 = s;
        if (s == 0) {
            return 0;
        }
        i32 sel = ((CSpriteRefTable*)g_gameReg->m_spriteFactory)
                      ->GetSel(((CSbWorldSlot*)((char*)g_gameReg + 0x138))[g_644c54].m_toolId, 0);
        if (sel == 0) {
            sel = ((CSpriteRefTable*)g_gameReg->m_spriteFactory)->GetSel(1, 0);
        }
        ((CImageSet*)m_imageSet)->SetAllTypes(10);
        ((CImageSet*)m_imageSet)->SetAllFormats(sel);
        i32 val;
        if (m_frameIdx < m_imageSet->m_idxLo || m_frameIdx > m_imageSet->m_idxHi) {
            val = 0;
        } else {
            val = m_imageSet->m_formats[m_frameIdx];
        }
        m_3c = val;
        return val != 0;
    }

} // namespace StatusBarTabBuilders

// ---------------------------------------------------------------------------
// vtable slot 3 (0xe8c70): drop the standalone frame handle + the two resolved frame
// records (also the destructor's member teardown). Homed out-of-line (matcher-5).
RVA(0x000e8c70, 0xc)
void CSBI_GruntMachine::Reset() {
    m_34 = 0;
    m_3c = 0;
    m_30 = 0;
}

// vtable slot 5 (0xe8cb0): the per-frame render. Idle (return 1) while the frame
// countdown is non-positive; otherwise tick it down, resolve the two indexed frame
// records (m_38 -> m_34, m_40 -> m_3c) through the config record's gated frame
// table, pull the surface context from the active drawable, then blit up to three
// frames: the standalone handle (m_44), the second resolved record (m_3c, drawn
// shifted +0x2c in x), and the first resolved record (m_34). Each draws at the base
// origin plus the frame record's own m_rect14.m_4/m_1c offset.
// @early-stop
// reloc-residual plateau + TU-merge ripple (~87%, was 92% in the standalone
// sbi_gruntmachine TU with identical source): the RenderFrame rel32s + g_gameReg
// DIR32 are reloc-masked against differently-named symbols
// (docs/patterns/reloc-typing-vptr-global.md), and the merged (retail-shaped) TU
// additionally flips the m_28 early-out layout (retail jle-to-end vs inline
// return-1) plus the commutative anchor adds - operand flips don't steer it.
RVA(0x000e8cb0, 0xc4)
i32 CSBI_GruntMachine::Render(i32 z) {
    if (m_28 <= 0) {
        return 1;
    }
    i32 idx = m_38;
    m_28--;
    CGmConfig* cfg = m_30;

    m_34 = (idx < cfg->m_64 || idx > cfg->m_68) ? 0 : cfg->m_14[idx];
    idx = m_40;
    m_3c = (idx < cfg->m_64 || idx > cfg->m_68) ? 0 : cfg->m_14[idx];

    i32 ctx = (i32)g_gameReg->m_world->m_drawTarget->m_14;

    CImage* f = m_44;
    if (f) {
        f->RenderFrame(
            (void*)ctx,
            (void*)(m_rect14.m_0 + f->m_anchorX),
            (void*)(m_rect14.m_4 + f->m_anchorY),
            0
        );
    }
    f = m_3c;
    if (f) {
        f->RenderFrame(
            (void*)ctx,
            (void*)(m_rect14.m_0 + f->m_anchorX + 0x2c),
            (void*)(m_rect14.m_4 + f->m_anchorY),
            0
        );
    }
    f = m_34;
    if (f) {
        f->RenderFrame(
            (void*)ctx,
            (void*)(m_rect14.m_0 + f->m_anchorX),
            (void*)(m_rect14.m_4 + f->m_anchorY),
            0
        );
    }
    return 1;
}

// 0xe8dc0 (__thiscall, ret 8): prime the two frame indices (each gated by != -1) and
// arm the countdown (m_28 = 2). Homed out-of-line (matcher-5).
RVA(0x000e8dc0, 0x22)
void CSBI_GruntMachine::SetFrames(i32 idxA, i32 idxB) {
    if (idxA != -1) {
        m_38 = idxA;
    }
    if (idxB != -1) {
        m_40 = idxB;
    }
    m_28 = 2;
}

// @early-stop
// 0x0e8e00 (1.0 KB) - homed from src/Stub/GapFunctions.cpp (matcher-5) by RVA
// neighbourhood (this TU owns the 0xe8cb0 SBI_GruntMachine block). __thiscall(4 args,
// 0x88-byte frame) status-bar grunt-machine tab worker over g_gameReg->m_30. Homed
// pending leaf-first reconstruction (>512 B).
RVA(0x000e8e00, 0x41a)
i32 Gap_0e8e00(void) {
    return 0;
}

namespace StatusBarTabBuilders {

    // ===========================================================================
    // CSbTab::BuildStatzTabStatusBar  (0xe9600)
    // ===========================================================================
    // @early-stop
    // identical-return-epilogue tail-merge wall (topic:wall) + the p5/p7 callee-saved
    // register reuse (they stay in ebx/ebp for the (p7-p5)/2 arithmetic, so the geometry
    // block can't use the struct-copy idiom). Body logic byte-faithful; ~65%. Deferred.
    RVA(0x000e9600, 0x18c)
    i32 CSbTab::BuildStatzTabStatusBar(
        CSbParent* parent,
        CSbOwner* statusbar,
        i32 p3,
        i32 p4,
        i32 p5,
        i32 p6,
        i32 p7,
        i32 p8,
        i32 p9,
        i32 p10,
        i32 p11,
        i32 p12,
        i32 onLeft
    ) {
        (void)p9;
        if (statusbar == 0 || parent == 0) {
            return 0;
        }
        m_owner = statusbar;
        m_10 = p4;
        m_parent = parent;
        m_geom.a = p5;
        m_28 = 0;
        m_geom.b = p6;
        m_geom.c = p7;
        m_geom.d = p8;
        m_0c = p3;
        if (p12 == 0) {
            m_04 = 0;
        } else {
            m_04 = 1;
        }
        m_3c = p10;
        m_frameIdx = p11;
        m_54 = onLeft;
        if (onLeft == 0) {
            void* out = 0;
            ((CMapStringToPtr*)&((CSbOwner*)g_gameReg->m_world)->m_mapHost->m_map)
                ->Lookup("GAME_STATUSBAR_TABZ_STATZTAB_TABONRIGHT", (void*&)out);
            CSbImageSet* n = (CSbImageSet*)out;
            i32 v;
            if (n == 0 || n->m_idxLo > 1 || n->m_idxHi < 1) {
                v = 0;
            } else {
                v = n->m_formats[1];
            }
            m_imageSet = (CSbImageSet*)v;
            m_50 = -1;
            m_48 = (p7 - p5) / 2 + parent->m_18;
        } else {
            void* out = 0;
            ((CMapStringToPtr*)&((CSbOwner*)g_gameReg->m_world)->m_mapHost->m_map)
                ->Lookup("GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT", (void*&)out);
            CSbImageSet* n = (CSbImageSet*)out;
            i32 v;
            if (n == 0 || n->m_idxLo > 1 || n->m_idxHi < 1) {
                v = 0;
            } else {
                v = n->m_formats[1];
            }
            m_imageSet = (CSbImageSet*)v;
            m_50 = 1;
            m_48 = parent->m_10 - (p7 - p5) / 2;
        }
        m_4c = p11 * 0x12 + 0xd1;
        if (m_imageSet == 0) {
            return 0;
        }
        m_44 = p12;
        m_38 = -1;
        m_58 = BuildHandle();
        return 1;
    }

} // namespace StatusBarTabBuilders

// ---------------------------------------------------------------------------
// CSBI_SideTab::Reset (0xe9800): drop the resolved config + frame. Out-of-line (matcher-5).
RVA(0x000e9800, 0x9)
void CSBI_SideTab::Reset() {
    m_30 = 0;
    m_34 = 0;
}

// vslot 4: (re)build the +0x58 draw gate from a sibling builder (BuildHandle, now
// co-named in this TU). The single stack arg is unused (the `ret 4` discards it);
// the slot returns int 0 (the trailing `xor eax,eax`).
RVA(0x000e9820, 0x11)
i32 CSBI_SideTab::Refresh(i32 unused) {
    m_58 = BuildHandle();
    return 0;
}

// 0xe9850: resample the selected unit's tracked value (one of: ability cap, override
// badge, or - when the chosen value is 0 - the health band) and, when it changed,
// resolve its glyph through the "SMALLICONZ" sprite set into m_34. Returns the draw
// gate: 0 if the tab is idle (mode 0) or the unit slot is empty, else 1.
// @early-stop
// 86% (zero-register-pinning + constant-materialization wall, docs/patterns/
// zero-register-pinning.md): the whole control flow - mode dispatch, the
// ability/badge/health-band derive, the unit-table index, the changed-value glyph
// Lookup + range-gate - is byte-correct, BUT retail pins this->edi / val->esi and
// uses immediate `1` everywhere, while cl pins this->esi / val->edi and CSE's the
// constant 1 into ebx (an extra callee-save push + ebx-form stores/cmp/returns).
// Same family wall as CSBI_StatzTabGruntBar::Update (91.7%); no source lever flips
// the allocation. Logic complete; deferred to the final sweep (whole-hierarchy model).
RVA(0x000e9850, 0x111)
i32 CSBI_SideTab::BuildHandle() {
    i32 mode = m_44;
    if (mode == 0) {
        return 0;
    }
    CSideTabGruntRec* unit = g_gameReg->m_68->m_units[m_40 + 15 * m_3c];
    if (unit == 0) {
        ((CSBI_RectOnly*)m_2c)->ClearStat(m_40);
        return 0;
    }
    i32 val;
    if (mode == 2) {
        i32 level = unit->m_170;
        if (level > 0x16) {
            val = unit->m_19c;
            if (val == 0) {
                m_44 = 1;
            }
        } else {
            val = level;
            if (val == 0) {
                m_44 = 1;
            }
        }
    } else if (mode == 3) {
        val = unit->m_198;
        if (val == 0) {
            m_44 = 1;
        }
    }
    if (m_44 == 1) {
        i32 hp = unit->m_3ec;
        if (hp >= 0x50) {
            val = 0x24;
        } else if (hp >= 0x28) {
            val = 0x25;
        } else {
            val = (hp <= 0 ? 1 : 0) + 0x26;
        }
    }
    if (m_38 == val) {
        return 1;
    }
    CSprite* gm = 0;
    ((CMapStringToOb*)&g_gameReg->m_world->m_10->m_10map)
        ->Lookup("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", (CObject*&)gm);
    i32 glyph;
    if (gm == 0 || val < gm->m_firstFrame || val > gm->m_lastFrame) {
        glyph = 0;
    } else {
        glyph = (i32)gm->m_frames.m_pData[val];
    }
    m_38 = val;
    m_34 = (CImage*)glyph;
    return 1;
}

// vslot 5: if the draw gate is set, blit the two side frames through the game
// manager's active drawable surface. Returns 1.
// @early-stop
// ~98.1% reloc-residual plateau: CODE BYTES byte-identical to retail (verified
// llvm-objdump base vs target). Raised from 87.8% by unifying the CSideTabFrame
// view to CImage (the two RenderFrame rel32s now co-name) and fixing the 4th
// RenderFrame arg to the literal 0 (was `z` - passing the arg forced an extra
// `push ebx` to stage it). Residual is only the g_gameReg DIR32 name artifact.
RVA(0x000e99c0, 0x4c)
i32 CSBI_SideTab::Render(i32 z) {
    if (m_58) {
        i32 ctx = (i32)g_gameReg->m_world->m_drawTarget->m_14;
        m_30->RenderFrame((void*)ctx, (void*)m_48, (void*)m_4c, 0);
        m_34->RenderFrame((void*)ctx, (void*)(m_48 + m_50), (void*)m_4c, 0);
    }
    return 1;
}

// @early-stop
// 0x0e9a30 (798 B) - homed from src/Stub/GapFunctions.cpp (matcher-5) by RVA
// neighbourhood (this TU owns the 0xe9820/0xe99c0/0xe9850 SBI_SideTab block).
// A status-bar side-tab worker; homed pending leaf-first reconstruction (>512 B).
RVA(0x000e9a30, 0x31e)
i32 Gap_0e9a30(void) {
    return 0;
}

// ===========================================================================
// CSbConfigItem::SetDirection  (0x0ea0f0) - re-homed from StatusBarMgr.cpp
// (interval dossier seam: positioned between the side-tab block and
// BuildMultiplayerTabStatusBar; the config-item setters used by the builders).
// ===========================================================================
// Two boolean selectors (a,b) pick one of four direction tuples, forwarded to
// the +0x38 virtual. Reached via thunk 0x1573 from LoadTabSprites + FUN_00504f90.
RVA(0x000ea0f0, 0x5c)
void CSbConfigItem::SetDirection(i32 a, i32 b) {
    if (a == 0) {
        if (b == 0) {
            ApplyDir(4, -1, 0, 0, -1);
        } else {
            ApplyDir(-1, -1, 1, 0, -1);
        }
    } else {
        if (b == 0) {
            ApplyDir(1, -1, 0, 0, -1);
        } else {
            ApplyDir(-1, -1, -1, 0, -1);
        }
    }
}

// ---------------------------------------------------------------------------
// 0x0ea170: the mirror sibling of SetDirection: the same four ApplyDir (+0x38
// virtual, slot 14) tuples, re-keyed on (a1,a2). RTTI-confirmed CSbConfigItem
// (slots 0-13 + ApplyDir @+0x38 match).
RVA(0x000ea170, 0x5c)
void CSbConfigItem::SetDirectionAlt(i32 a1, i32 a2) {
    if (a1 == 0) {
        if (a2 == 0) {
            ApplyDir(1, -1, 0, 0, -1);
        } else {
            ApplyDir(-1, -1, -1, 0, -1);
        }
    } else {
        if (a2 == 0) {
            ApplyDir(4, -1, 0, 0, -1);
        } else {
            ApplyDir(-1, -1, 1, 0, -1);
        }
    }
}

namespace StatusBarTabBuilders {

    // ===========================================================================
    // CSbTab::BuildMultiplayerTabStatusBar  (0xea1f0)
    // ===========================================================================
    // @early-stop
    // identical-return-epilogue tail-merge wall (topic:wall): prologue + body byte-exact
    // (geometry block grouped via the struct-copy idiom), residual is the many fail-path
    // `return 0` sites tail-merging to one shared epilogue. ~78%. Logic complete.
    RVA(0x000ea1f0, 0x1fa)
    i32 CSbTab::BuildMultiplayerTabStatusBar(
        CSbParent* parent,
        CSbOwner* statusbar,
        i32 p3,
        i32 p4,
        CSbGeom g,
        char* key,
        i32 p10,
        i32 p11,
        i32 selMode
    ) {
        if (statusbar == 0) {
            return 0;
        }
        if (parent == 0) {
            return 0;
        }
        CSbOwner* owner = statusbar;
        m_parent = parent;
        m_10 = p4;
        m_owner = owner;
        m_28 = 0;
        m_04 = 1;
        m_geom = g;
        statusbar = 0;
        m_0c = p3;
        ((CMapStringToPtr*)&owner->m_mapHost->m_map)->Lookup(key, (void*&)statusbar);
        CSbImageSet* head = (CSbImageSet*)statusbar;
        m_headImage = head;
        if (statusbar == 0) {
            return 0;
        }
        i32 v;
        if (head->m_idxLo > 0x21 || head->m_idxHi < 0x21) {
            v = 0;
        } else {
            v = head->m_formats[0x21];
        }
        m_imageSet = (CSbImageSet*)v;
        if (v == 0) {
            return 0;
        }
        i32 w;
        if (head->m_idxLo > 0x22 || head->m_idxHi < 0x22) {
            w = 0;
        } else {
            w = head->m_formats[0x22];
        }
        m_3c = w;
        if (w == 0) {
            return 0;
        }
        i32 val;
        if (selMode == 0) {
            statusbar = 0;
            ((CMapStringToPtr*)&m_owner->m_mapHost->m_map)
                ->Lookup("GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_SELECTEDBAR", (void*&)statusbar);
            m_68 = (i32)statusbar;
            if (statusbar == 0) {
                return 0;
            }
            if (m_headImage->m_idxLo > 0x23 || m_headImage->m_idxHi < 0x23) {
                val = 0;
            } else {
                val = m_headImage->m_formats[0x23];
            }
        } else {
            statusbar = 0;
            ((CMapStringToPtr*)&m_owner->m_mapHost->m_map)
                ->Lookup("GAME_STATUSBAR_TABZ_STATZTAB_SELECTEDBAR", (void*&)statusbar);
            m_68 = (i32)statusbar;
            if (statusbar == 0) {
                return 0;
            }
            i32 x;
            if (m_headImage->m_idxLo > 0x23 || m_headImage->m_idxHi < 0x23) {
                x = 0;
            } else {
                x = m_headImage->m_formats[0x23];
            }
            m_54 = x;
            if (x == 0) {
                return 0;
            }
            if (m_headImage->m_idxLo > 0x22 || m_headImage->m_idxHi < 0x22) {
                val = 0;
            } else {
                val = m_headImage->m_formats[0x22];
            }
        }
        m_48 = val;
        if (val == 0) {
            return 0;
        }
        m_60 = p10;
        m_64 = p11;
        m_70 = -1;
        m_50 = -1;
        m_44 = -1;
        m_38 = -1;
        m_5c = 0;
        m_78 = 0;
        m_80 = 0;
        m_7c = 0;
        m_84 = 0;
        Update();
        return 1;
    }

} // namespace StatusBarTabBuilders

// ---------------------------------------------------------------------------
// ~CSBI_GruntMachine (0x104ce0): the /GX chain destructor - stamp
// ??_7CSBI_GruntMachine, run Reset (the slot-3 teardown above, 0xe8c70), then
// MSVC folds the inline ~CStatusBarItem in (??_7CStatusBarItem + DtorStatus - the
// SBI_DTOR_CHAIN device) behind the /GX SEH frame. COMDAT-at-usage: retail emits
// it inside the 0x104d60 SBI_RectOnly obj (the deleting user); the class's file
// is this TU, so the definition lives here.
RVA(0x00104ce0, 0x55)
CSBI_GruntMachine::~CSBI_GruntMachine() {
    Reset();
}

// ---------------------------------------------------------------------------
// ~CSBI_SideTab (0x105200): the /GX chain destructor - stamp ??_7CSBI_SideTab,
// run Reset (the slot-3 teardown above, 0xe9800), then MSVC folds the inline
// ~CStatusBarItem in (??_7CStatusBarItem + DtorStatus - the SBI_DTOR_CHAIN
// device) behind the /GX SEH frame. COMDAT-at-usage: retail emits it inside the
// 0x104d60 SBI_RectOnly obj (per the dossier: leave, not a re-home); the class's
// file is this TU, so the definition lives here.
RVA(0x00105200, 0x55)
CSBI_SideTab::~CSBI_SideTab() {
    Reset();
}
