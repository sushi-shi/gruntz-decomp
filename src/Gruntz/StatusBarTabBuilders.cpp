#define SBI_DTOR_CHAIN // enable the inline base-dtor body (see StatusBarItem.h)
#include <Mfc.h>       // afx-first (TU pulls MFC via unified CObject; superset of Win32.h)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr (m_cmdGrid) + CGrunt (the placed grid grunt)
#include <Gruntz/Grunt.h> // complete CGrunt (CGrunt == CGrunt; the stat fields)
#include <rva.h>
#include <Ints.h>

#include <Gruntz/StatusBarMgr.h>      // CStatusBarMgr (ClearStat, the side-tab owner slot)
#include <Gruntz/GameRegistry.h>      // canonical CGameRegistry (the builders' singleton view)
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical CDDrawSurfaceMgr + CDDrawSubMgrPages + CDDrawWorkerRegistry
#include <DDrawMgr/DDrawSubMgrPages.h>    // the m_drawTarget pages (full def)
#include <Gruntz/Sprite.h>                // CDDrawWorker (fold: ex via ResMgr.h)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <DDrawMgr/DDrawSubMgrPages.h>    // the m_drawTarget pages (full def)
#include <Gruntz/StatusBarItem.h>    // canonical frameless CStatusBarItem base (real RTTI base)
#include <Image/CImage.h>            // the frame handles ARE CImage (RenderFrame @0x153790)
#include <Gruntz/SBI_GruntMachine.h> // canonical CSBI_GruntMachine (vtable @0x5eadbc)
#include <Gruntz/SBI_SideTab.h>      // canonical CSBI_SideTab (vtable @0x5eae3c) + referent views
#include <Gruntz/SbiSideTabBuildViews.h> // (the builder IS CStatusBarMgr now)
#include <Gruntz/SbiConfig.h>       // canonical CDDrawSurfaceMgr (the builders' arg2 config host)
#include <Gruntz/SBI_ImageSetAni.h> // canonical CSBI_StatzTabArrow (SetDirection/SetDirectionAlt)
#include <Gruntz/SBI_StatzTabGruntBar.h> // canonical CSBI_StatzTabGruntBar (BuildMultiplayerTab..)

#include <Gruntz/StatusBarTabBuildersViews.h> // (tombstone - the views are dissolved)
#include <Image/ImageSet.h> // canonical CDDrawWorker (SetAllTypes/SetAllFormats; the config record)
#include <Io/FileMem.h>     // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/SerialCounter.h> // g_serialCounter (bumped once per string field)
#include <string.h> // inline strlen/strcpy/memset over the serialize scratch buffer

#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>

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
    // (g_curPlayer's decl lives in <Gruntz/StatusBarItem.h> now)

} // namespace StatusBarTabBuilders

// ---------------------------------------------------------------------------
// CSBI_GruntMachine::BuildResourceTabStatusBar (0xe8a70) - the machine widget's own
// configure. Re-homed off the `CSbTab` view, which CONFLATED this class with
// CSBI_StatzTabGruntBar (one 0x88 struct standing in for a 0x48 and an 0x88 class) and
// whose mangled name matched NO call site: LoadTabSprites called it on the fabricated
// CSbConfigItem base, so the reference resolved to nothing at link. The `this` is proven
// by the call site (`new CSBI_GruntMachine` immediately before) and the field map is
// exact: m_parent/m_owner/m_geom ARE the CStatusBarItem base slots m_2c/m_24/m_rect14,
// and the view's CSbImageSet is the canonical CDDrawWorker (frame table + index gates).
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
    CDDrawSurfaceMgr* host,
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
    CDDrawSurfaceMgr* h = host;
    m_2c = owner;
    m_tab = p4;
    m_24 = h;
    m_28 = 0;
    m_enabled = 1;
    m_rect14.m_0 = g.left;
    m_rect14.m_4 = g.top;
    m_rect14.m_8 = g.right;
    m_rect14.m_c = g.bottom;
    CDDrawWorker* rec = 0;
    CObject* recOb = 0;
    m_cmd = p3;
    h->m_imageRegistry->m_10map.Lookup(
        "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND",
        recOb
    );
    rec = static_cast<CDDrawWorker*>(recOb);
    CImage* spr;
    if (rec == 0 || rec->m_minIndex > 1 || rec->m_maxIndex < 1) {
        spr = 0;
    } else {
        spr = static_cast<CImage*>(rec->m_items.GetAt(1));
    }
    m_standaloneFrame = spr;
    if (spr == 0) {
        return 0;
    }
    CDDrawWorker* cfg = 0;
    CObject* cfgOb = 0;
    m_24->m_imageRegistry->m_10map.Lookup(key, cfgOb);
    cfg = static_cast<CDDrawWorker*>(cfgOb);
    m_config = cfg;
    if (cfg == 0) {
        return 0;
    }
    m_frameIdxA = idxA;
    m_frameIdxB = idxB;
    CImage* s;
    if (idxA < m_config->m_minIndex || idxA > m_config->m_maxIndex) {
        s = 0;
    } else {
        s = static_cast<CImage*>(m_config->m_items.GetAt(idxA));
    }
    m_frameA = s;
    if (s == 0) {
        return 0;
    }
    i32 sel =
        g_gameReg->m_spriteFactory
            ->GetSel(
                g_gameReg->m_options[g_curPlayer].m_008, // ex the +0x138 rebased world-slot view (+0x138+0x20 == m_options+0x08)
                0
            );
    if (sel == 0) {
        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }
    m_config->SetAllTypes(10);
    m_config->SetAllFormats(sel);
    CImage* val;
    if (m_frameIdxB < m_config->m_minIndex || m_frameIdxB > m_config->m_maxIndex) {
        val = 0;
    } else {
        val = static_cast<CImage*>(m_config->m_items.GetAt(m_frameIdxB));
    }
    m_frameB = val;
    return val != 0;
}

RVA(0x000e8c70, 0xc)
void CSBI_GruntMachine::Reset() {
    m_frameA = 0;
    m_frameB = 0;
    m_config = 0;
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
i32 CSBI_GruntMachine::Render() {
    if (m_28 <= 0) {
        return 1;
    }
    i32 idx = m_frameIdxA;
    m_28--;
    CDDrawWorker* cfg = m_config;

    m_frameA = (idx < cfg->m_minIndex || idx > cfg->m_maxIndex) ? 0 : static_cast<CImage*>(cfg->m_items.GetAt(idx));
    idx = m_frameIdxB;
    m_frameB = (idx < cfg->m_minIndex || idx > cfg->m_maxIndex) ? 0 : static_cast<CImage*>(cfg->m_items.GetAt(idx));

    i32 ctx = reinterpret_cast<i32>(g_gameReg->m_world->m_drawTarget->m_backPair);

    CImage* f = m_standaloneFrame;
    if (f) {
        f->RenderFrame(
            reinterpret_cast<void*>(ctx),
            reinterpret_cast<void*>((m_rect14.m_0 + f->m_anchorX)),
            reinterpret_cast<void*>((m_rect14.m_4 + f->m_anchorY)),
            0
        );
    }
    f = m_frameB;
    if (f) {
        f->RenderFrame(
            reinterpret_cast<void*>(ctx),
            reinterpret_cast<void*>((m_rect14.m_0 + f->m_anchorX + 0x2c)),
            reinterpret_cast<void*>((m_rect14.m_4 + f->m_anchorY)),
            0
        );
    }
    f = m_frameA;
    if (f) {
        f->RenderFrame(
            reinterpret_cast<void*>(ctx),
            reinterpret_cast<void*>((m_rect14.m_0 + f->m_anchorX)),
            reinterpret_cast<void*>((m_rect14.m_4 + f->m_anchorY)),
            0
        );
    }
    return 1;
}

RVA(0x000e8dc0, 0x22)
void CSBI_GruntMachine::SetFrames(i32 idxA, i32 idxB) {
    if (idxA != -1) {
        m_frameIdxA = idxA;
    }
    if (idxB != -1) {
        m_frameIdxB = idxB;
    }
    m_28 = 2;
}

// ===========================================================================
// CSBI_GruntMachine::SerializeFields (0x0e8e00) - ??_7CSBI_GruntMachine (0x1eadbc)
// slot 1 (thunk 0x381e); the grunt-machine dual-mode serialize leg. __thiscall
// (stream, mode, a2, a3), ret 0x10; bails 0 when the stream or g_gameReg->m_world
// is absent.
//
// Mode 7 (read): the config record m_30 by NAME (registry Lookup, ungated), the
// raw m_38, then the three frames m_34/m_3c/m_44 as name+index registry refs
// gated to rec->m_frames[idx] (the family's [m_minIndex..m_maxIndex] resolve),
// with the raw m_40 between the first two. Mode 4 (write): m_30 round-trips by
// name only (strcpy of its +0x24 m_name); each frame reverse-looks-up its
// name+index through AnyValueMatches. Both arms tail-chain the QUALIFIED
// CStatusBarItem::SerializeFields (retail `call 0x1848`) and 0/1-normalise.
// ===========================================================================
// @early-stop
// stack-slot recoloring wall (the GruntStateRec/CTimer scratch-slot family), 99.89%:
// every instruction byte-identical except the frame size (mine 0x8c vs retail 0x88)
// and the dependent esp-relative offsets. Retail's compiler reuses the dead `reg`
// spill slot ([esp+0x14]) for `idx` once reg is cached in esi after the first
// (index-less) name block; cl gives idx its own slot. The sibling CSBI_SideTab leg
// (whose FIRST block already uses idx, keeping the slot hot) hit 100.00 EXACT with
// the same spelling; block-scoping v/out/idx recovered out/v sharing but the
// reg/idx recoloring is not source-steerable (permuter: no change).
RVA(0x000e8e00, 0x41a)
i32 CSBI_GruntMachine::SerializeFields(CFileMemBase* s, i32 mode, i32 a2, i32 a3) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];

    switch (mode) {
        case 4: {
            i32 v;
            // --- mode 4 (store): the config record by name, each frame by
            // reverse name+index lookup ---
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_config != 0) {
                strcpy(buf, m_config->m_name);
            }
            s->Write(buf, 0x80);
            s->Write(&m_frameIdxA, 4);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_frameA != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_frameA, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);
            s->Write(&m_frameIdxB, 4);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_frameB != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_frameB, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_standaloneFrame != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_standaloneFrame, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);
            break;
        }

        case 7: {
            CObject* out;
            i32 idx;
            // --- mode 7 (load): the config record by name, each frame by
            // name + gated index ---
            g_serialCounter++;
            s->Read(buf, 0x80);
            if (strlen(buf) != 0) {
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                m_config = static_cast<CDDrawWorker*>(out);
            } else {
                m_config = 0;
            }
            s->Read(&m_frameIdxA, 4);

            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_frameA = r;
            } else {
                m_frameA = 0;
            }
            s->Read(&m_frameIdxB, 4);

            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_frameB = r;
            } else {
                m_frameB = 0;
            }

            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_standaloneFrame = r;
            } else {
                m_standaloneFrame = 0;
            }
            break;
        }
    }

    // QUALIFIED = the direct base leg (retail `call 0x1848`); unqualified would be
    // recursion on this override.
    return CStatusBarItem::SerializeFields(s, mode, a2, a3) != 0 ? 1 : 0;
}

namespace StatusBarTabBuilders {} // namespace StatusBarTabBuilders

// ---------------------------------------------------------------------------
// CSBI_SideTab::BuildStatzTabStatusBar (0xe9600) - the side tab's own configure, run on
// the freshly-`new`ed child by CStatzTabBuilder::Build. Re-homed off `CSbTab` (the same
// conflation view that held the other two Build*). `this` is proven by the call site
// (`newobj->BuildStatzTabStatusBar` straight after `new CSBI_SideTab`); `parent` is the
// BUILDER, not another side tab - the body reads parent->m_10 / parent->m_rect14.m_4, which are
// CStatzTabBuilder's geometry anchors. The caller-side view typed that param CSBI_SideTab*
// purely to compile, forcing a cross-cast of a CStatzTabBuilder. The view's CSbImageSet is
// the canonical CSbiConfigRecord.
// @early-stop
// identical-return-epilogue tail-merge wall (topic:wall) + the p5/p7 callee-saved
// register reuse (they stay in ebx/ebp for the (p7-p5)/2 arithmetic, so the geometry
// block can't use the struct-copy idiom). Body logic byte-faithful; ~65%. Deferred.
RVA(0x000e9600, 0x18c)
i32 CSBI_SideTab::BuildStatzTabStatusBar(
    CStatusBarMgr* parent,
    CDDrawSurfaceMgr* host,
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
    static_cast<void>(p9);
    if (host == 0 || parent == 0) {
        return 0;
    }
    m_24 = host;
    m_tab = p4;
    m_2c = parent;
    m_rect14.m_0 = p5;
    m_28 = 0;
    m_rect14.m_4 = p6;
    m_rect14.m_8 = p7;
    m_rect14.m_c = p8;
    m_cmd = p3;
    if (p12 == 0) {
        m_enabled = 0;
    } else {
        m_enabled = 1;
    }
    m_rowIndex = p10;
    m_colIndex = p11;
    m_onLeft = onLeft;
    if (onLeft == 0) {
        CDDrawWorker* n = 0;
        CObject* nOb = 0;
        g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
            "GAME_STATUSBAR_TABZ_STATZTAB_TABONRIGHT",
            nOb
        );
        n = static_cast<CDDrawWorker*>(nOb);
        CImage* v;
        if (n == 0 || n->m_minIndex > 1 || n->m_maxIndex < 1) {
            v = 0;
        } else {
            v = static_cast<CImage*>(n->m_items.GetAt(1));
        }
        m_topFrame = v;
        m_bottomFrameDy = -1;
        m_drawX = (p7 - p5) / 2 + parent->m_rect14.m_4;
    } else {
        CDDrawWorker* n = 0;
        CObject* nOb = 0;
        g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
            "GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT",
            nOb
        );
        n = static_cast<CDDrawWorker*>(nOb);
        CImage* v;
        if (n == 0 || n->m_minIndex > 1 || n->m_maxIndex < 1) {
            v = 0;
        } else {
            v = static_cast<CImage*>(n->m_items.GetAt(1));
        }
        m_topFrame = v;
        m_bottomFrameDy = 1;
        m_drawX = parent->m_10 - (p7 - p5) / 2;
    }
    m_drawY = p11 * 0x12 + 0xd1;
    if (m_topFrame == 0) {
        return 0;
    }
    m_sampleMode = p12;
    m_sampledValue = -1;
    m_drawGate = BuildHandle();
    return 1;
}

RVA(0x000e9800, 0x9)
void CSBI_SideTab::Reset() {
    m_topFrame = 0;
    m_bottomFrame = 0;
}

RVA(0x000e9820, 0x11)
i32 CSBI_SideTab::Refresh(i32 unused) {
    m_drawGate = BuildHandle();
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
    i32 mode = m_sampleMode;
    if (mode == 0) {
        return 0;
    }
    CGrunt* unit = g_gameReg->m_cmdGrid->m_grid[m_colIndex + 15 * m_rowIndex]; // the placed grid grunt (ex CSideTabGruntRec view)
    if (unit == 0) {
        m_2c->ClearStat(m_colIndex);
        return 0;
    }
    i32 val;
    if (mode == 2) {
        i32 level = unit->m_entranceReason; // the multiplexed current-tool kind (>0x16 = melee)
        if (level > 0x16) {
            val = unit->m_19c;
            if (val == 0) {
                m_sampleMode = 1;
            }
        } else {
            val = level;
            if (val == 0) {
                m_sampleMode = 1;
            }
        }
    } else if (mode == 3) {
        val = unit->m_198;
        if (val == 0) {
            m_sampleMode = 1;
        }
    }
    if (m_sampleMode == 1) {
        i32 hp = unit->m_health;
        if (hp >= 0x50) {
            val = 0x24;
        } else if (hp >= 0x28) {
            val = 0x25;
        } else {
            val = (hp <= 0 ? 1 : 0) + 0x26;
        }
    }
    if (m_sampledValue == val) {
        return 1;
    }
    CObject* gm_ob = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup(
        "GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ",
        gm_ob
    );
    CDDrawWorker* gm = static_cast<CDDrawWorker*>(gm_ob);
    i32 glyph;
    if (gm == 0 || val < gm->m_minIndex || val > gm->m_maxIndex) {
        glyph = 0;
    } else {
        glyph = reinterpret_cast<i32>(static_cast<CImage*>(gm->m_items.GetAt(val)));
    }
    m_sampledValue = val;
    m_bottomFrame = reinterpret_cast<CImage*>(glyph);
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
i32 CSBI_SideTab::Render() {
    if (m_drawGate) {
        i32 ctx = reinterpret_cast<i32>(g_gameReg->m_world->m_drawTarget->m_backPair);
        m_topFrame->RenderFrame(reinterpret_cast<void*>(ctx), reinterpret_cast<void*>(m_drawX), reinterpret_cast<void*>(m_drawY), 0);
        m_bottomFrame->RenderFrame(reinterpret_cast<void*>(ctx), reinterpret_cast<void*>((m_drawX + m_bottomFrameDy)), reinterpret_cast<void*>(m_drawY), 0);
    }
    return 1;
}

RVA(0x000e9a30, 0x31e)
i32 CSBI_SideTab::SerializeFields(CFileMemBase* s, i32 mode, i32 a2, i32 a3) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];

    switch (mode) {
        case 4: {
            i32 v;
            // --- mode 4 (store): each frame by reverse name+index lookup, then
            // the raw field run ---
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_topFrame != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_topFrame, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            v = 0;
            if (m_bottomFrame != 0) {
                reg->m_imageRegistry->AnyValueMatches(m_bottomFrame, buf, &v);
            }
            s->Write(buf, 0x80);
            s->Write(&v, 4);

            s->Write(&m_sampledValue, 4);
            s->Write(&m_rowIndex, 4);
            s->Write(&m_colIndex, 4);
            s->Write(&m_sampleMode, 4);
            s->Write(&m_drawX, 8); // the m_48+m_4c draw-origin pair, one 8-byte record
            s->Write(&m_bottomFrameDy, 4);
            s->Write(&m_onLeft, 4);
            s->Write(&m_drawGate, 4);
            break;
        }

        case 7: {
            CObject* out;
            i32 idx;
            // --- mode 7 (load): each frame by name + gated index, then the raw
            // field run ---
            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_topFrame = r;
            } else {
                m_topFrame = 0;
            }

            g_serialCounter++;
            s->Read(buf, 0x80);
            s->Read(&idx, 4);
            if (strlen(buf) != 0) {
                i32 i = idx;
                out = 0;
                reg->m_imageRegistry->m_10map.Lookup(buf, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_bottomFrame = r;
            } else {
                m_bottomFrame = 0;
            }

            s->Read(&m_sampledValue, 4);
            s->Read(&m_rowIndex, 4);
            s->Read(&m_colIndex, 4);
            s->Read(&m_sampleMode, 4);
            s->Read(&m_drawX, 8); // the m_48+m_4c draw-origin pair, one 8-byte record
            s->Read(&m_bottomFrameDy, 4);
            s->Read(&m_onLeft, 4);
            s->Read(&m_drawGate, 4);
            break;
        }
    }

    // QUALIFIED = the direct base leg (retail `call 0x1848`); unqualified would be
    // recursion on this override.
    return CStatusBarItem::SerializeFields(s, mode, a2, a3) != 0 ? 1 : 0;
}

RVA(0x000ea0f0, 0x5c)
void CSBI_StatzTabArrow::SetDirection(i32 a, i32 b) {
    if (a == 0) {
        if (b == 0) {
            SetRange(4, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, 1, 0, -1);
        }
    } else {
        if (b == 0) {
            SetRange(1, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, -1, 0, -1);
        }
    }
}

RVA(0x000ea170, 0x5c)
void CSBI_StatzTabArrow::SetDirectionAlt(i32 a1, i32 a2) {
    if (a1 == 0) {
        if (a2 == 0) {
            SetRange(1, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, -1, 0, -1);
        }
    } else {
        if (a2 == 0) {
            SetRange(4, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, 1, 0, -1);
        }
    }
}

// ---------------------------------------------------------------------------
// CSBI_StatzTabGruntBar::BuildMultiplayerTabStatusBar (0xea1f0) - the stat bar's own
// configure. Re-homed off the `CSbTab` view (the same conflation that held
// BuildResourceTabStatusBar; `this` is proven by the call site's `new
// CSBI_StatzTabGruntBar`). The view's CSbImageSet is the canonical CDDrawWorker (the glyph map).
// @early-stop
// identical-return-epilogue tail-merge wall (topic:wall): prologue + body byte-exact
// (geometry block grouped via the struct-copy idiom), residual is the many fail-path
// `return 0` sites tail-merging to one shared epilogue. ~78%. Logic complete.
RVA(0x000ea1f0, 0x1fa)
i32 CSBI_StatzTabGruntBar::BuildMultiplayerTabStatusBar(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
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
    CDDrawSurfaceMgr* h = host;
    m_2c = owner;
    m_tab = p4;
    m_24 = h;
    m_28 = 0;
    m_enabled = 1;
    m_rect14.m_0 = g.left;
    m_rect14.m_4 = g.top;
    m_rect14.m_8 = g.right;
    m_rect14.m_c = g.bottom;
    CDDrawWorker* head = 0;
    CObject* headOb = 0;
    m_cmd = p3;
    h->m_imageRegistry->m_10map.Lookup(key, headOb);
    head = static_cast<CDDrawWorker*>(headOb);
    m_glyphMap = head;
    if (head == 0) {
        return 0;
    }
    CImage* v;
    if (head->m_minIndex > 0x21 || head->m_maxIndex < 0x21) {
        v = 0;
    } else {
        v = static_cast<CImage*>(head->m_items.GetAt(0x21));
    }
    m_statusGlyph = v;
    if (v == 0) {
        return 0;
    }
    CImage* w;
    if (head->m_minIndex > 0x22 || head->m_maxIndex < 0x22) {
        w = 0;
    } else {
        w = static_cast<CImage*>(head->m_items.GetAt(0x22));
    }
    m_abilityGlyph = w;
    if (w == 0) {
        return 0;
    }
    CImage* val;
    if (selMode == 0) {
        CDDrawWorker* sel = 0;
        CObject* selOb = 0;
        m_24
            ->m_imageRegistry->m_10map
            .Lookup("GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_SELECTEDBAR", selOb);
        sel = static_cast<CDDrawWorker*>(selOb);
        m_timerGlyphMap = sel;
        if (sel == 0) {
            return 0;
        }
        if (m_glyphMap->m_minIndex > 0x23 || m_glyphMap->m_maxIndex < 0x23) {
            val = 0;
        } else {
            val = static_cast<CImage*>(m_glyphMap->m_items.GetAt(0x23));
        }
    } else {
        CDDrawWorker* sel = 0;
        CObject* selOb = 0;
        m_24
            ->m_imageRegistry->m_10map
            .Lookup("GAME_STATUSBAR_TABZ_STATZTAB_SELECTEDBAR", selOb);
        sel = static_cast<CDDrawWorker*>(selOb);
        m_timerGlyphMap = sel;
        if (sel == 0) {
            return 0;
        }
        CImage* x;
        if (m_glyphMap->m_minIndex > 0x23 || m_glyphMap->m_maxIndex < 0x23) {
            x = 0;
        } else {
            x = static_cast<CImage*>(m_glyphMap->m_items.GetAt(0x23));
        }
        m_selectKey = x;
        if (x == 0) {
            return 0;
        }
        if (m_glyphMap->m_minIndex > 0x22 || m_glyphMap->m_maxIndex < 0x22) {
            val = 0;
        } else {
            val = static_cast<CImage*>(m_glyphMap->m_items.GetAt(0x22));
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

RVA(0x00104ce0, 0x55)
CSBI_GruntMachine::~CSBI_GruntMachine() {
    Reset();
}

RVA(0x00105200, 0x55)
CSBI_SideTab::~CSBI_SideTab() {
    Reset();
}
