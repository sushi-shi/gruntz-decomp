// StatusBarTabBuildersViews.h - the shapes the three status-bar tab "Build*" methods
// (StatusBarTabBuilders.cpp, RVAs 0xe8a70 / 0xe9600 / 0xea1f0) drive. Moved here from
// the per-TU inline defs (inside namespace StatusBarTabBuilders) so each carries a
// single shared definition (matching-neutral; only the OFFSETS + code bytes are
// load-bearing, engine callees are reloc-masked).
#ifndef GRUNTZ_STATUSBAR_TAB_BUILDERS_VIEWS_H
#define GRUNTZ_STATUSBAR_TAB_BUILDERS_VIEWS_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/SpriteRefTable.h> // the shared ::CSpriteRefTable (g_gameReg->m_74->GetSel)

namespace StatusBarTabBuilders {

    // The four consecutive geometry anchors (this+0x14..0x20) the builders copy in as
    // a block. Passed by value (4 stack dwords, ABI-identical to four i32 args) so the
    // `m_geom = g` struct-copy lowers to retail's `lea base,[esi+0x14]; mov [base+k]`
    // (docs/patterns/struct-copy-block-store-base-reg.md).
    struct CSbGeom {
        i32 a, b, c, d;
    };

    // The level namespace map (CMapStringToOb) the tab keys are resolved through. Its
    // Lookup (0x1b8008, __thiscall, ret 8) fills `out` and returns a BOOL we discard.
    struct CSbNamespaceMap {
        void Lookup(char* key, void** out); // 0x1b8008
    };

    // The object that embeds the namespace map at +0x10 (reached via owner->m_mapHost).
    struct CSbMapHost {
        char m_00[0x10];
        CSbNamespaceMap m_map; // +0x10  embedded map
    };

    // The status-bar owner (param "statusbar"): m_mapHost -> the map host.
    struct CSbOwner {
        char m_00[0x10];
        CSbMapHost* m_mapHost; // +0x10
    };

    // The namespace-resolved image set (the CMapStringToOb value). m_formats is the
    // per-index format/sprite table; [m_idxLo..m_idxHi] is its valid index range.
    struct CSbImageSet {
        void SetAllTypes(i32 type);     // 0x152480
        void SetAllFormats(i32 format); // 0x152520
        char m_00[0x14];
        i32* m_formats; // +0x14  index -> format table
        char m_18[0x64 - 0x18];
        i32 m_idxLo; // +0x64  index lo
        i32 m_idxHi; // +0x68  index hi
    };

    // The grunt-machine parent (param "parent"): only its m_10/m_18 anchors are read.
    struct CSbParent {
        char m_00[0x10];
        i32 m_10; // +0x10
        char m_14[0x18 - 0x14];
        i32 m_18; // +0x18
    };

    // The sprite-ref table (g_gameReg->m_74) whose GetSel (0xe23c0, reached via ILT
    // 0x4165) maps a tool/toy id to a palette/format selector is the shared
    // ::CSpriteRefTable (unqualified CSpriteRefTable below resolves to it).

    // One 0x238-byte per-world settings record (g_gameReg->m_138[g_644c54]).
    struct CSbWorldSlot {
        char m_00[0x20];
        i32 m_toolId; // +0x20  the tool/toy id GetSel consumes
        char m_24[0x238 - 0x24];
    };

    // The status-bar item being built (a CSBI_SideTab-style value bag). BuildHandle
    // (0xe7c70, reached via ILT 0x21c6) and Update (0xea660, via ILT 0x4403) are this
    // class's own helpers, modeled no-body so their calls reloc-mask. Only the
    // structural fields (parent/owner/image sets/geometry) carry a recovered name;
    // the rest are opaque per-tab config slots left as m_<hexoffset>.
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
        i32 m_0c;                // +0x0c
        i32 m_10;                // +0x10
        CSbGeom m_geom;          // +0x14..0x20  (4 geometry anchors, block-copied)
        CSbOwner* m_owner;       // +0x24
        i32 m_28;                // +0x28
        CSbParent* m_parent;     // +0x2c
        CSbImageSet* m_imageSet; // +0x30
        i32 m_34;                // +0x34
        i32 m_38;                // +0x38
        i32 m_3c;                // +0x3c
        i32 m_40;                // +0x40
        i32 m_44;                // +0x44
        i32 m_48;                // +0x48
        i32 m_4c;                // +0x4c
        i32 m_50;                // +0x50
        i32 m_54;                // +0x54
        i32 m_58;                // +0x58
        i32 m_5c;                // +0x5c
        i32 m_60;                // +0x60
        i32 m_64;                // +0x64
        i32 m_68;                // +0x68
        char m_6c[0x70 - 0x6c];
        i32 m_70;                 // +0x70
        CSbImageSet* m_headImage; // +0x74
        i32 m_78;                 // +0x78
        i32 m_7c;                 // +0x7c
        i32 m_80;                 // +0x80
        i32 m_84;                 // +0x84
    };

} // namespace StatusBarTabBuilders

#endif // GRUNTZ_STATUSBAR_TAB_BUILDERS_VIEWS_H
