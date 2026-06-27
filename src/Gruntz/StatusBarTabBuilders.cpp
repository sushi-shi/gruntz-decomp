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

#include <Ints.h>
#include <rva.h>

namespace StatusBarTabBuilders {

// The four consecutive geometry anchors (this+0x14..0x20) the builders copy in as
// a block. Passed by value (4 stack dwords, ABI-identical to four i32 args) so the
// `m_14 = g` struct-copy lowers to retail's `lea base,[esi+0x14]; mov [base+k]`
// (docs/patterns/struct-copy-block-store-base-reg.md).
struct CSbGeom {
    i32 a, b, c, d;
};

// The level namespace map (CMapStringToOb) the tab keys are resolved through. Its
// Lookup (0x1b8008, __thiscall, ret 8) fills `out` and returns a BOOL we discard.
struct CSbNamespaceMap {
    void Lookup(char* key, void** out); // 0x1b8008
};

// The object that embeds the namespace map at +0x10 (reached via owner->m_10).
struct CSbMapHost {
    char m_00[0x10];
    CSbNamespaceMap m_10; // +0x10  embedded map
};

// The status-bar owner (param "statusbar"): m_10 -> the map host.
struct CSbOwner {
    char m_00[0x10];
    CSbMapHost* m_10; // +0x10
};

// The namespace-resolved image set (the CMapStringToOb value). m_14 is the
// per-index format/sprite table; [m_64..m_68] is its valid index range.
struct CSbImageSet {
    void SetAllTypes(i32 type);     // 0x152480
    void SetAllFormats(i32 format); // 0x152520
    char m_00[0x14];
    i32* m_14; // +0x14  index -> format table
    char m_18[0x64 - 0x18];
    i32 m_64; // +0x64  index lo
    i32 m_68; // +0x68  index hi
};

// The grunt-machine parent (param "parent"): only its m_10/m_18 anchors are read.
struct CSbParent {
    char m_00[0x10];
    i32 m_10; // +0x10
    char m_14[0x18 - 0x14];
    i32 m_18; // +0x18
};

// The sprite-ref table (g_gameReg->m_74) whose GetSel maps a tool/toy id to a
// palette/format selector.
struct CSpriteRefTable {
    i32 GetSel(i32 id, i32 flag); // 0x4e3a0 (reached via ILT 0x4165)
};

// One 0x238-byte per-world settings record (g_gameReg->m_138[g_644c54]).
struct CSbWorldSlot {
    char m_00[0x20];
    i32 m_20; // +0x20  the tool/toy id GetSel consumes
    char m_24[0x238 - 0x24];
};

// The game registry / settings singleton (?g_gameReg, DATA 0x64556c). Only the
// fields these builders touch are modeled.
struct CSbSettings {
    char m_00[0x30];
    CSbOwner* m_30; // +0x30  the level's namespace owner (StatzTab path)
    char m_34[0x74 - 0x34];
    CSpriteRefTable* m_74; // +0x74
    char m_78[0x138 - 0x78];
    CSbWorldSlot m_138[1]; // +0x138  per-world slot array (size 0x238)
};

DATA(0x0064556c)
extern CSbSettings* g_gameReg;
DATA(0x00644c54)
extern i32 g_644c54;

// The status-bar item being built (a CSBI_SideTab-style value bag). BuildHandle
// (0xe7c70, reached via ILT 0x21c6) and Update (0xea660, via ILT 0x4403) are this
// class's own helpers, modeled no-body so their calls reloc-mask.
struct CSbTab {
    i32 BuildResourceTabStatusBar(
        CSbParent* parent,
        CSbOwner* statusbar,
        i32 p3,
        i32 p4,
        CSbGeom g,
        char* key,
        i32 idxA,
        i32 idxB
    );
    i32 BuildStatzTabStatusBar(
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
    );
    i32 BuildMultiplayerTabStatusBar(
        CSbParent* parent,
        CSbOwner* statusbar,
        i32 p3,
        i32 p4,
        CSbGeom g,
        char* key,
        i32 p10,
        i32 p11,
        i32 selMode
    );
    i32 BuildHandle();
    void Update();

    char m_00[0x4];
    i32 m_04; // +0x04
    char m_08[0x4];
    i32 m_0c; // +0x0c
    i32 m_10;     // +0x10
    CSbGeom m_14; // +0x14..0x20  (4 geometry anchors, block-copied)
    CSbOwner* m_24; // +0x24
    i32 m_28;       // +0x28
    CSbParent* m_2c; // +0x2c
    CSbImageSet* m_30; // +0x30
    i32 m_34; // +0x34
    i32 m_38; // +0x38
    i32 m_3c; // +0x3c
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    i32 m_4c; // +0x4c
    i32 m_50; // +0x50
    i32 m_54; // +0x54
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    i32 m_64; // +0x64
    i32 m_68; // +0x68
    char m_6c[0x70 - 0x6c];
    i32 m_70; // +0x70
    CSbImageSet* m_74; // +0x74
    i32 m_78; // +0x78
    i32 m_7c; // +0x7c
    i32 m_80; // +0x80
    i32 m_84; // +0x84
};

// ===========================================================================
// CSbTab::BuildResourceTabStatusBar  (0xe8a70)
// ===========================================================================
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns/identical-return-epilogue-tailmerge.md,
// topic:wall): prologue + body are byte-exact (the geometry block now groups via the
// struct-copy idiom, struct-copy-block-store-base-reg.md), but the 3 fail-path `return 0`
// sites tail-merge to one shared pop;pop;ret where retail duplicates the epilogue inline,
// and the final range-check lands m_30/m_40 in the opposite regs (a regalloc coin-flip).
// ~79%; both residuals are documented non-steerable walls. Logic complete.
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
    m_2c = parent;
    m_10 = p4;
    m_24 = owner;
    m_28 = 0;
    m_04 = 1;
    m_14 = g;
    statusbar = 0;
    m_0c = p3;
    owner->m_10->m_10.Lookup(
        "GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND", (void**)&statusbar
    );
    CSbImageSet* n = (CSbImageSet*)statusbar;
    i32 spr;
    if (n == 0 || n->m_64 > 1 || n->m_68 < 1) {
        spr = 0;
    } else {
        spr = n->m_14[1];
    }
    m_44 = spr;
    if (spr == 0) {
        return 0;
    }
    statusbar = 0;
    m_24->m_10->m_10.Lookup(key, (void**)&statusbar);
    m_30 = (CSbImageSet*)statusbar;
    if (statusbar == 0) {
        return 0;
    }
    m_38 = idxA;
    m_40 = idxB;
    i32 s;
    if (idxA < m_30->m_64 || m_30->m_68 < idxA) {
        s = 0;
    } else {
        s = m_30->m_14[idxA];
    }
    m_34 = s;
    if (s == 0) {
        return 0;
    }
    i32 sel = g_gameReg->m_74->GetSel(g_gameReg->m_138[g_644c54].m_20, 0);
    if (sel == 0) {
        sel = g_gameReg->m_74->GetSel(1, 0);
    }
    m_30->SetAllTypes(10);
    m_30->SetAllFormats(sel);
    i32 val;
    if (m_40 < m_30->m_64 || m_30->m_68 < m_40) {
        val = 0;
    } else {
        val = m_30->m_14[m_40];
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
    m_24 = statusbar;
    m_10 = p4;
    m_2c = parent;
    m_14.a = p5;
    m_28 = 0;
    m_14.b = p6;
    m_14.c = p7;
    m_14.d = p8;
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
        g_gameReg->m_30->m_10->m_10.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_TABONRIGHT", &out);
        CSbImageSet* n = (CSbImageSet*)out;
        i32 v;
        if (n == 0 || n->m_64 > 1 || n->m_68 < 1) {
            v = 0;
        } else {
            v = n->m_14[1];
        }
        m_30 = (CSbImageSet*)v;
        m_50 = -1;
        m_48 = (p7 - p5) / 2 + parent->m_18;
    } else {
        void* out = 0;
        g_gameReg->m_30->m_10->m_10.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT", &out);
        CSbImageSet* n = (CSbImageSet*)out;
        i32 v;
        if (n == 0 || n->m_64 > 1 || n->m_68 < 1) {
            v = 0;
        } else {
            v = n->m_14[1];
        }
        m_30 = (CSbImageSet*)v;
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
    m_2c = parent;
    m_10 = p4;
    m_24 = owner;
    m_28 = 0;
    m_04 = 1;
    m_14 = g;
    statusbar = 0;
    m_0c = p3;
    owner->m_10->m_10.Lookup(key, (void**)&statusbar);
    CSbImageSet* head = (CSbImageSet*)statusbar;
    m_74 = head;
    if (statusbar == 0) {
        return 0;
    }
    i32 v;
    if (head->m_64 > 0x21 || head->m_68 < 0x21) {
        v = 0;
    } else {
        v = head->m_14[0x21];
    }
    m_30 = (CSbImageSet*)v;
    if (v == 0) {
        return 0;
    }
    i32 w;
    if (head->m_64 > 0x22 || head->m_68 < 0x22) {
        w = 0;
    } else {
        w = head->m_14[0x22];
    }
    m_3c = w;
    if (w == 0) {
        return 0;
    }
    i32 val;
    if (selMode == 0) {
        statusbar = 0;
        m_24->m_10->m_10.Lookup(
            "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_SELECTEDBAR", (void**)&statusbar
        );
        m_68 = (i32)statusbar;
        if (statusbar == 0) {
            return 0;
        }
        if (m_74->m_64 > 0x23 || m_74->m_68 < 0x23) {
            val = 0;
        } else {
            val = m_74->m_14[0x23];
        }
    } else {
        statusbar = 0;
        m_24->m_10->m_10.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_SELECTEDBAR", (void**)&statusbar);
        m_68 = (i32)statusbar;
        if (statusbar == 0) {
            return 0;
        }
        i32 x;
        if (m_74->m_64 > 0x23 || m_74->m_68 < 0x23) {
            x = 0;
        } else {
            x = m_74->m_14[0x23];
        }
        m_54 = x;
        if (x == 0) {
            return 0;
        }
        if (m_74->m_64 > 0x22 || m_74->m_68 < 0x22) {
            val = 0;
        } else {
            val = m_74->m_14[0x22];
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
