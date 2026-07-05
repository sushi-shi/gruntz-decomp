// StatusBarTabBuilders.cpp - the three status-bar tab "Build*" methods carved out
// of the ApiCaller backlog (RVAs 0xe8a70 / 0xe9600 / 0xea1f0). Each populates a
// CSBI_SideTab-style status-bar item from the level's GAME_STATUSBAR_TABZ_*
// namespace (a CMapStringToOb lookup) plus the caller-supplied geometry. They are
// __thiscall, share one item layout, and are homed here (a NEW TU; splitting the
// class across TUs is matching-neutral) so the lane-owned SBI TUs are untouched.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes are
// load-bearing. The engine callees / globals are reloc-masked (modeled with NO
// body / by-address DATA externs).
#include <Win32.h>

#include <Gruntz/GameRegistry.h> // g_gameReg singleton (0x24556c) canonical view
#include <Ints.h>
#include <rva.h>
#include <Gruntz/StatusBarTabBuildersViews.h> // CSbGeom/CSbOwner/.../CSbTab (namespace views)

namespace StatusBarTabBuilders {

    // The CSbGeom/CSbNamespaceMap/CSbMapHost/CSbOwner/CSbImageSet/CSbParent/
    // CSpriteRefTable/CSbWorldSlot/CSbTab views moved to
    // <Gruntz/StatusBarTabBuildersViews.h>.

    // The game registry / settings singleton (*0x24556c) - the canonical
    // CGameRegistry view. The namespace owner (+0x30 -> CSbOwner), sprite-ref table
    // (+0x74 -> CSpriteRefTable) and per-world slot array (+0x138, stride 0x238 ->
    // CSbWorldSlot) are cast locally at the deref sites.
    DATA(0x0024556c)
    extern CGameRegistry* g_gameReg;
    DATA(0x00644c54)
    extern i32 g_644c54;

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
    // lands m_imageSet/m_40 in the opposite regs (eax<->ecx free-list coin-flip). ~80.4%; both
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
        owner->m_mapHost->m_map.Lookup(
            "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND",
            (void**)&statusbar
        );
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
        m_owner->m_mapHost->m_map.Lookup(key, (void**)&statusbar);
        m_imageSet = (CSbImageSet*)statusbar;
        if (statusbar == 0) {
            return 0;
        }
        m_38 = idxA;
        m_40 = idxB;
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
        m_imageSet->SetAllTypes(10);
        m_imageSet->SetAllFormats(sel);
        i32 val;
        if (m_40 < m_imageSet->m_idxLo || m_40 > m_imageSet->m_idxHi) {
            val = 0;
        } else {
            val = m_imageSet->m_formats[m_40];
        }
        m_3c = val;
        return val != 0;
    }

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
        m_40 = p11;
        m_54 = onLeft;
        if (onLeft == 0) {
            void* out = 0;
            ((CSbOwner*)g_gameReg->m_world)
                ->m_mapHost->m_map.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_TABONRIGHT", &out);
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
            ((CSbOwner*)g_gameReg->m_world)
                ->m_mapHost->m_map.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT", &out);
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
        owner->m_mapHost->m_map.Lookup(key, (void**)&statusbar);
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
            m_owner->m_mapHost->m_map.Lookup(
                "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_SELECTEDBAR",
                (void**)&statusbar
            );
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
            m_owner->m_mapHost->m_map.Lookup(
                "GAME_STATUSBAR_TABZ_STATZTAB_SELECTEDBAR",
                (void**)&statusbar
            );
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
