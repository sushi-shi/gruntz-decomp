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

#include <Gruntz/StatusBarMgr.h>     // CStatusBarMgr (ClearStat, the side-tab owner slot)
#include <Gruntz/GameRegistry.h>     // canonical CGameRegistry (the builders' singleton view)
#include <Gruntz/ResMgr.h>           // canonical CResMgr + CDrawTarget + CImageRegistry
#include <Gruntz/StatusBarItem.h>    // canonical frameless CStatusBarItem base (real RTTI base)
#include <Image/CImage.h>            // the frame handles ARE CImage (RenderFrame @0x153790)
#include <Gruntz/SBI_GruntMachine.h> // canonical CSBI_GruntMachine (vtable @0x5eadbc)
#include <Gruntz/SBI_SideTab.h>      // canonical CSBI_SideTab (vtable @0x5eae3c) + referent views
#include <Gruntz/SbiSideTabBuildViews.h> // CStatzTabBuilder (the side tab's `parent`)
#include <Gruntz/SbiConfig.h> // canonical CSpriteFactoryHolder (the builders' arg2 config host)
#include <Gruntz/SBI_ImageSetAni.h> // canonical CSBI_StatzTabArrow (SetDirection/SetDirectionAlt)
#include <Gruntz/SBI_StatzTabGruntBar.h> // canonical CSBI_StatzTabGruntBar (BuildMultiplayerTab..)

// The name maps are the real MFC CMapStringToOb (Lookup x1b8008 - mfc_class names that
// band CMapStringToOb; the ex-note here had the Ob/Ptr pairing inverted). No local view.
#include <Gruntz/StatusBarTabBuildersViews.h> // CSbGeom/CSbOwner/.../CSbTab (namespace views)
#include <Image/ImageSet.h>                   // canonical CImageSet (SetAllTypes/SetAllFormats)

// The g_gameReg singleton (VA 0x64556c), shared by the CSBI_GruntMachine /
// CSBI_SideTab methods below: the render chain m_world (+0x30, CResMgr) ->
// m_drawTarget (+0x04) -> m_14 surface context, and the side-tab unit table
// m_68. One TU-wide decl (the CSideTabGameReg view from <Gruntz/SBI_SideTab.h>;
// the builders' namespace-scoped CGameRegistry decl below is its own symbol).
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
    extern "C" i32
        g_curPlayer; // the current world index (canonical DATA(0x00244c54) in sbi_rectonly)

} // namespace StatusBarTabBuilders

// ---------------------------------------------------------------------------
// CSBI_GruntMachine::BuildResourceTabStatusBar (0xe8a70) - the machine widget's own
// configure. Re-homed off the `CSbTab` view, which CONFLATED this class with
// CSBI_StatzTabGruntBar (one 0x88 struct standing in for a 0x48 and an 0x88 class) and
// whose mangled name matched NO call site: LoadTabSprites called it on the fabricated
// CSbConfigItem base, so the reference resolved to nothing at link. The `this` is proven
// by the call site (`new CSBI_GruntMachine` immediately before) and the field map is
// exact: m_parent/m_owner/m_geom ARE the CStatusBarItem base slots m_2c/m_24/m_rect14,
// and the view's CSbImageSet is the canonical CGmConfig (m_14 table + m_64/m_68 gates).
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md,
// topic:wall): prologue + body are byte-exact (the geometry block groups via the struct-copy
// idiom, struct-copy-block-store-base-reg.md; the variable-index range checks emit retail's
// `cmp idx,[hi]; jg` from spelling them `idx > m_68`). Residual: (1) the 3 mid `return 0`
// guards inline `jne;pop;pop;ret` in retail (eax already 0) but my compile tail-merges all 5
// guards into one shared `pop;xor;pop;ret`; (2) the final range-check lands m_30/m_40 in the
// opposite regs (eax<->ecx free-list coin-flip). Both are non-steerable walls; logic complete.
RVA(0x000e8a70, 0x18c)
i32 CSBI_GruntMachine::BuildResourceTabStatusBar(
    CStatusBarMgr* owner,
    CSpriteFactoryHolder* host,
    i32 p3,
    i32 p4,
    SbRect g,
    const char* key,
    i32 idxA,
    i32 idxB
) {
    if (host == 0 || owner == 0) {
        return 0;
    }
    CSpriteFactoryHolder* h = host;
    m_2c = (i32)owner;
    m_10 = p4;
    m_24 = (i32)h;
    m_28 = 0;
    m_4 = 1;
    m_rect14.m_0 = g.left;
    m_rect14.m_4 = g.top;
    m_rect14.m_8 = g.right;
    m_rect14.m_c = g.bottom;
    CGmConfig* rec = 0;
    m_c = p3;
    h->m_10->m_10map.Lookup("GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND", (CObject*&)rec);
    CImage* spr;
    if (rec == 0 || rec->m_64 > 1 || rec->m_68 < 1) {
        spr = 0;
    } else {
        spr = rec->m_14[1];
    }
    m_44 = spr;
    if (spr == 0) {
        return 0;
    }
    CGmConfig* cfg = 0;
    ((CSpriteFactoryHolder*)m_24)->m_10->m_10map.Lookup(key, (CObject*&)cfg);
    m_30 = cfg;
    if (cfg == 0) {
        return 0;
    }
    m_38 = idxA;
    m_40 = idxB;
    CImage* s;
    if (idxA < m_30->m_64 || idxA > m_30->m_68) {
        s = 0;
    } else {
        s = m_30->m_14[idxA];
    }
    m_34 = s;
    if (s == 0) {
        return 0;
    }
    i32 sel =
        ((CSpriteRefTable*)StatusBarTabBuilders::g_gameReg->m_spriteFactory)
            ->GetSel(
                ((StatusBarTabBuilders::CSbWorldSlot*)((char*)StatusBarTabBuilders::g_gameReg
                                                       + 0x138))[StatusBarTabBuilders::g_curPlayer]
                    .m_toolId,
                0
            );
    if (sel == 0) {
        sel = ((CSpriteRefTable*)StatusBarTabBuilders::g_gameReg->m_spriteFactory)->GetSel(1, 0);
    }
    ((CImageSet*)m_30)->SetAllTypes(10);
    ((CImageSet*)m_30)->SetAllFormats(sel);
    CImage* val;
    if (m_40 < m_30->m_64 || m_40 > m_30->m_68) {
        val = 0;
    } else {
        val = m_30->m_14[m_40];
    }
    m_3c = val;
    return val != 0;
}

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

namespace StatusBarTabBuilders {} // namespace StatusBarTabBuilders

// ---------------------------------------------------------------------------
// CSBI_SideTab::BuildStatzTabStatusBar (0xe9600) - the side tab's own configure, run on
// the freshly-`new`ed child by CStatzTabBuilder::Build. Re-homed off `CSbTab` (the same
// conflation view that held the other two Build*). `this` is proven by the call site
// (`newobj->BuildStatzTabStatusBar` straight after `new CSBI_SideTab`); `parent` is the
// BUILDER, not another side tab - the body reads parent->m_10 / parent->m_18, which are
// CStatzTabBuilder's geometry anchors. The caller-side view typed that param CSBI_SideTab*
// purely to compile, forcing a cross-cast of a CStatzTabBuilder. The view's CSbImageSet is
// the canonical CSbiConfigRecord.
// @early-stop
// identical-return-epilogue tail-merge wall (topic:wall) + the p5/p7 callee-saved
// register reuse (they stay in ebx/ebp for the (p7-p5)/2 arithmetic, so the geometry
// block can't use the struct-copy idiom). Body logic byte-faithful; ~65%. Deferred.
RVA(0x000e9600, 0x18c)
i32 CSBI_SideTab::BuildStatzTabStatusBar(
    CStatzTabBuilder* parent,
    CSpriteFactoryHolder* host,
    i32 p3,
    i32 p4,
    i32 p5,
    i32 p6,
    i32 p7,
    i32 p8,
    const char* p9,
    i32 p10,
    i32 p11,
    i32 p12,
    i32 onLeft
) {
    (void)p9;
    if (host == 0 || parent == 0) {
        return 0;
    }
    m_24 = (i32)host;
    m_10 = p4;
    m_2c = (i32)parent;
    m_rect14.m_0 = p5;
    m_28 = 0;
    m_rect14.m_4 = p6;
    m_rect14.m_8 = p7;
    m_rect14.m_c = p8;
    m_c = p3;
    if (p12 == 0) {
        m_4 = 0;
    } else {
        m_4 = 1;
    }
    m_3c = p10;
    m_40 = p11;
    m_54 = onLeft;
    if (onLeft == 0) {
        CSbiConfigRecord* n = 0;
        ((CSpriteFactoryHolder*)g_gameReg->m_world)
            ->m_10->m_10map.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_TABONRIGHT", (CObject*&)n);
        CImage* v;
        if (n == 0 || n->m_64 > 1 || n->m_68 < 1) {
            v = 0;
        } else {
            v = (CImage*)n->m_14[1];
        }
        m_30 = v;
        m_50 = -1;
        m_48 = (p7 - p5) / 2 + parent->m_18;
    } else {
        CSbiConfigRecord* n = 0;
        ((CSpriteFactoryHolder*)g_gameReg->m_world)
            ->m_10->m_10map.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT", (CObject*&)n);
        CImage* v;
        if (n == 0 || n->m_64 > 1 || n->m_68 < 1) {
            v = 0;
        } else {
            v = (CImage*)n->m_14[1];
        }
        m_30 = v;
        m_50 = 1;
        m_48 = parent->m_10 - (p7 - p5) / 2;
    }
    m_4c = p11 * 0x12 + 0xd1;
    if (m_30 == 0) {
        return 0;
    }
    m_44 = p12;
    m_38 = -1;
    m_58 = BuildHandle();
    return 1;
}

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
    CSideTabGruntRec* unit = g_gameReg->m_unitTable->m_units[m_40 + 15 * m_3c];
    if (unit == 0) {
        ((CStatusBarMgr*)m_2c)->ClearStat(m_40);
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
    CObject* gm_ob = 0;
    g_gameReg->m_world->m_10->m_10map.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", gm_ob);
    CSprite* gm = (CSprite*)gm_ob;
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
// CSBI_StatzTabArrow::SetDirection  (0x0ea0f0) - re-homed from StatusBarMgr.cpp
// (interval dossier seam: positioned between the side-tab block and
// BuildMultiplayerTabStatusBar; the config-item setters used by the builders).
// ===========================================================================
// Two boolean selectors (a,b) pick one of four frame-window tuples, forwarded to the
// slot-14 virtual. That slot is SetRange (thunk 0x3bde -> jmp 0xe7c30), introduced by
// CSBI_ImageSetAni; the owner is the ARROW because LoadTabSprites calls both setters on
// a freshly `new CSBI_StatzTabArrow` (Statz per-grunt row). They used to be members of
// the fabricated 15-slot CSbConfigItem, forwarding to a body-less `ApplyDir` placeholder.
// Reached via thunk 0x1573 from LoadTabSprites + FUN_00504f90.
RVA(0x000ea0f0, 0x5c)
void CSBI_StatzTabArrow::SetDirection(i32 a, i32 b) {
    if (a == 0) {
        if (b == 0) {
            SetRange_0e7c30(4, -1, 0, 0, -1);
        } else {
            SetRange_0e7c30(-1, -1, 1, 0, -1);
        }
    } else {
        if (b == 0) {
            SetRange_0e7c30(1, -1, 0, 0, -1);
        } else {
            SetRange_0e7c30(-1, -1, -1, 0, -1);
        }
    }
}

// ---------------------------------------------------------------------------
// 0x0ea170: the mirror sibling of SetDirection: the same four SetRange tuples, re-keyed
// on (a1,a2).
RVA(0x000ea170, 0x5c)
void CSBI_StatzTabArrow::SetDirectionAlt(i32 a1, i32 a2) {
    if (a1 == 0) {
        if (a2 == 0) {
            SetRange_0e7c30(1, -1, 0, 0, -1);
        } else {
            SetRange_0e7c30(-1, -1, -1, 0, -1);
        }
    } else {
        if (a2 == 0) {
            SetRange_0e7c30(4, -1, 0, 0, -1);
        } else {
            SetRange_0e7c30(-1, -1, 1, 0, -1);
        }
    }
}

// ---------------------------------------------------------------------------
// CSBI_StatzTabGruntBar::BuildMultiplayerTabStatusBar (0xea1f0) - the stat bar's own
// configure. Re-homed off the `CSbTab` view (the same conflation that held
// BuildResourceTabStatusBar; `this` is proven by the call site's `new
// CSBI_StatzTabGruntBar`). The view's CSbImageSet is the canonical CStatzGlyphMap.
// @early-stop
// identical-return-epilogue tail-merge wall (topic:wall): prologue + body byte-exact
// (geometry block grouped via the struct-copy idiom), residual is the many fail-path
// `return 0` sites tail-merging to one shared epilogue. ~78%. Logic complete.
RVA(0x000ea1f0, 0x1fa)
i32 CSBI_StatzTabGruntBar::BuildMultiplayerTabStatusBar(
    CStatusBarMgr* owner,
    CSpriteFactoryHolder* host,
    i32 p3,
    i32 p4,
    SbRect g,
    const char* key,
    i32 p10,
    i32 p11,
    i32 selMode
) {
    if (host == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    CSpriteFactoryHolder* h = host;
    m_2c = (i32)owner;
    m_10 = p4;
    m_24 = (i32)h;
    m_28 = 0;
    m_4 = 1;
    m_rect14.m_0 = g.left;
    m_rect14.m_4 = g.top;
    m_rect14.m_8 = g.right;
    m_rect14.m_c = g.bottom;
    CStatzGlyphMap* head = 0;
    m_c = p3;
    h->m_10->m_10map.Lookup(key, (CObject*&)head);
    m_glyphMap = head;
    if (head == 0) {
        return 0;
    }
    CImage* v;
    if (head->m_minIndex > 0x21 || head->m_maxIndex < 0x21) {
        v = 0;
    } else {
        v = head->m_glyphs[0x21];
    }
    m_statusGlyph = v;
    if (v == 0) {
        return 0;
    }
    CImage* w;
    if (head->m_minIndex > 0x22 || head->m_maxIndex < 0x22) {
        w = 0;
    } else {
        w = head->m_glyphs[0x22];
    }
    m_abilityGlyph = w;
    if (w == 0) {
        return 0;
    }
    CImage* val;
    if (selMode == 0) {
        CStatzGlyphMap* sel = 0;
        ((CSpriteFactoryHolder*)m_24)
            ->m_10->m_10map
            .Lookup("GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_SELECTEDBAR", (CObject*&)sel);
        m_timerGlyphMap = sel;
        if (sel == 0) {
            return 0;
        }
        if (m_glyphMap->m_minIndex > 0x23 || m_glyphMap->m_maxIndex < 0x23) {
            val = 0;
        } else {
            val = m_glyphMap->m_glyphs[0x23];
        }
    } else {
        CStatzGlyphMap* sel = 0;
        ((CSpriteFactoryHolder*)m_24)
            ->m_10->m_10map.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_SELECTEDBAR", (CObject*&)sel);
        m_timerGlyphMap = sel;
        if (sel == 0) {
            return 0;
        }
        CImage* x;
        if (m_glyphMap->m_minIndex > 0x23 || m_glyphMap->m_maxIndex < 0x23) {
            x = 0;
        } else {
            x = m_glyphMap->m_glyphs[0x23];
        }
        m_selectKey = x;
        if (x == 0) {
            return 0;
        }
        if (m_glyphMap->m_minIndex > 0x22 || m_glyphMap->m_maxIndex < 0x22) {
            val = 0;
        } else {
            val = m_glyphMap->m_glyphs[0x22];
        }
    }
    m_overrideGlyph = val;
    if (val == 0) {
        return 0;
    }
    m_unitRow = p10;
    m_unitCol = p11;
    m_timerValue = -1;
    m_overrideValue = -1;
    m_abilityValue = -1;
    m_statusValue = -1;
    m_selectValue = 0;
    m_timerAnchorLo = 0;
    m_timerWindowLo = 0;
    m_timerAnchorHi = 0;
    m_timerWindowHi = 0;
    Update();
    return 1;
}

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
