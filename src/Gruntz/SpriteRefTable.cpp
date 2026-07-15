// SpriteRefTable.cpp - the game-registry sprite/animation reference table (trace
// placeholder tomalla-10). Methods in ascending retail-RVA order. The table
// shape (two 17-slot CSpriteRef buckets at +0x08 / +0x4c) and the CSpriteRef /
// CSpriteRefHashTable helpers come from <Gruntz/SpriteRefTable.h>; every cross-
// cluster callee (the node ctor/dtor, the hash Lookup, the engine alloc/free) is
// modeled NO-body so its `call` reloc-masks.
#include <Bute/SymParser.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Win32.h>

#include <Gruntz/SpriteRefTable.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>     // m_spriteMgrHolder's canonical CDDrawSurfaceMgr
#include <DDrawMgr/DDrawWorkerMapSmall.h> // its +0x18 m_workerMap (the sprite/palette registry)

#include <rva.h>

// The sprite-ref / palette hash tables are the real MFC CMapStringToPtr
// (Lookup @0x1b8008, brought in via <Mfc.h>); no local view.
#include <stdio.h> // engine sprintf (reloc-masked) - LoadGruntzPalette's name format

// The engine heap reached by Add()/Clear() is the NAFXCW global operator new/delete
// (??2@YAPAXI@Z @0x1b9b46, ??3@YAXPAX@Z @0x1b9b82); reloc-masked DIR32/REL32.
void* ::operator new(u32); // matches ??2@YAPAXI@Z

// The holder IS the canonical CDDrawSurfaceMgr; its +0x18 m_workerMap
// (CDDrawWorkerMapSmall) carries the name->object registry at +0x10 (m_map1,
// CMapStringToOb). Add()/LoadGruntzPalette() reach it cast-free.

// The object Lookup writes into `out`: +0x10 is the sprite, whose +0xc holds the
// frame data fed to the alpha factory. (@identity-TODO - honest map-value view: the
// registry map (m_map1) is a heterogeneous CMapStringToOb; the palette/worker path
// stores CAniRecordBase2 (dissolved), but this SPRITE value is stored by the sprite
// LOADER elsewhere, so its concrete class needs an xref chase from THAT side - it is
// NOT CAniRecordBase2 (whose +0x10 m_10 is a plain i32 buffer, not a sprite ptr).)
struct CLookupSprite {
    char m_pad00[0xc];
    void* m_frameData; // +0x0c
};
struct CLookupResult {
    char m_pad00[0x10];
    CLookupSprite* m_sprite; // +0x10
};

// ---------------------------------------------------------------------------

RVA(0x000e2250, 0x26)
i32 CSpriteRefTable::Init(i32 p0, i32 p1) {
    if (!p0) {
        return p0;
    }
    m_factory = (CShadeTableCache*)p0;
    m_spriteMgrHolder = (CDDrawSurfaceMgr*)p1;
    m_built = 0;
    return 1;
}

RVA(0x000e2290, 0x2a)
void CSpriteRefTable::Reset() {
    Clear();
    m_factory = 0;
    m_spriteMgrHolder = 0;
    m_built = 0;
    for (i32 i = 0; i < 0x11; i++) {
        m_refA[i] = 0;
        m_refB[i] = 0;
    }
}

RVA(0x000e22d0, 0x6e)
void CSpriteRefTable::Clear() {
    if (m_factory) {
        for (i32 i = 0; i < 0x11; i++) {
            CSpriteRef* a = GetA(i);
            if (a) {
                a->Free();
                ::operator delete(a);
            }
            CSpriteRef* b = GetB(i);
            if (b) {
                b->Free();
                ::operator delete(b);
            }
        }
        for (i32 j = 0; j < 0x11; j++) {
            m_refA[j] = 0;
            m_refB[j] = 0;
        }
        m_built = 0;
    }
}

// GetA (0x0e2360): bucket-A node for slot i, null if out of [0,17). Out-of-line
// (retail emits it standalone; the inline member folded away and never emitted).
RVA(0x000e2360, 0x15)
CSpriteRef* CSpriteRefTable::GetA(i32 i) {
    if ((u32)i >= 0x11) {
        return 0;
    }
    return m_refA[i];
}

// GetB (0x0e2390): bucket-B node for slot i, null if out of [0,17). Out-of-line.
RVA(0x000e2390, 0x15)
CSpriteRef* CSpriteRefTable::GetB(i32 i) {
    if ((u32)i >= 0x11) {
        return 0;
    }
    return m_refB[i];
}

// GetSel (0x0e23c0): resolve slot i (bucket B when bAlt else A), return the node's
// m_alphaKey, or 0. Out-of-line (retail emits it standalone).
RVA(0x000e23c0, 0x2d)
i32 CSpriteRefTable::GetSel(i32 i, i32 bAlt) {
    if ((u32)i >= 0x11) {
        return 0;
    }
    CSpriteRef* node = bAlt ? m_refB[i] : m_refA[i];
    if (!node) {
        return 0;
    }
    return node->m_alphaKey;
}

// @early-stop
// out-param zero-init scheduling wall: retail SINKS `mov [&out],0` past both arg
// pushes, cl HOISTS it between them (identical multiset, 1 instr permuted); the
// rest of the 182-byte body is byte-exact. docs/patterns/outparam-zeroinit-scheduling.md
RVA(0x000e2890, 0xb6)
CSpriteRef* CSpriteRefTable::Add(char* szName, i32 kind) {
    CObject* out = 0;
    m_spriteMgrHolder->m_workerMap->m_map1.Lookup(szName, out);
    if (!out) {
        return 0;
    }
    void* sprite = ((CLookupResult*)out)->m_sprite->m_frameData;
    if (!sprite) {
        return 0;
    }
    void* alpha = m_factory->AlphaTable((unsigned char*)sprite);
    if (!alpha) {
        return 0;
    }
    CSpriteRef* node;
    CSpriteRef* tmp = (CSpriteRef*)::operator new(0x10);
    if (tmp) {
        tmp->m_cache = 0;
        tmp->m_alphaKey = 0;
        node = tmp;
    } else {
        node = 0;
    }
    if (node->Build((i32)m_factory, alpha, kind) == 0) {
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    return node;
}

// ---------------------------------------------------------------------------
// CSpriteRefTable::LoadGruntzPalette (0xe2d10) - register a level's
// "GRUNTZ_PALETTEZ_<name>" palette into the sprite registry reached through
// this->m_spriteMgrHolder->m_workerMap (CDDrawWorkerMapSmall). m_map1.Lookup() (the
// +0x10 CMapStringToOb) probes whether it is already present; Factory_1658c0
// (vtable slot 9, the fabricated "Install") installs the resolved palette. src is
// the source resolver (FUN_0053bff0 resolves a packed-tag 'PAL'=0x50414c resource
// by namespaced name); name is the level/name string. Helpers are reloc-masked
// externals; the registry is reached cast-free through the real polymorphic member.
//
// int (BOOL) return: the `!src` and already-present guards return literal 0/1
// (reusing the zeroed eax / `mov eax,1`); the success path normalizes the
// Factory_1658c0() return through neg/sbb/neg (`!!x`). A void return would tail-merge
// the bare epilogues and drop the eax=1 tail.
//
// The fabricated CPaletteDestRegistry (v0-v8 + Install@9) / CPaletteHashTable /
// CPaletteDestRoot / CSpriteMgrHolder views ARE the real CDDrawSurfaceMgr +
// CDDrawWorkerMapSmall (Install@slot9 == Factory_1658c0 @0x1658c0, Lookup @0x1b8008
// == CMapStringToOb::Lookup). Dissolved onto them 2026-07-14.

// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// the ONLY residual is retail SINKING `mov [&found],0` past the `lea &found` (lea
// then store) while cl HOISTS it (store then lea) - identical instruction multiset,
// one 2-instr permutation, source-invariant under /O2. Logic + all bytes otherwise
// exact (frame 0x40, epilogues, !!x normalize all match).
RVA(0x000e2d10, 0xa1)
i32 CSpriteRefTable::LoadGruntzPalette(i32 src, i32 name) {
    if (!src) {
        return 0;
    }

    CObject* found = 0;
    m_spriteMgrHolder->m_workerMap->m_map1.Lookup((char*)name, found);
    if (found) {
        return 1;
    }

    char buf[0x40];
    sprintf(buf, "GRUNTZ_PALETTEZ_%s", (char*)name);
    void* pal = (void*)((CSymParser*)src)->ResolveQualified(buf, (void*)0x50414c);
    if (!pal) {
        return 0;
    }
    return m_spriteMgrHolder->m_workerMap->Factory_1658c0((CDDrawSurfaceSource*)pal, 0, 0) != 0;
}

// ---------------------------------------------------------------------------
// CSpriteRefTable::LoadToolToyPalettes (0xe2980) - register the full tool/toy color
// palette set by calling LoadGruntzPalette for each of the 34 fixed color names.
// Short-circuits to 0 on the first failure (a null src bails immediately); returns 1
// only when every palette resolved. __thiscall(src), ret 4.
RVA(0x000e2980, 0x2cd)
i32 CSpriteRefTable::LoadToolToyPalettes(i32 src) {
    // One short-circuit && chain so MSVC shares a single return-0 tail (each rung
    // `test;je fail`), matching retail's layout (an if/return-0 per rung inlines 35
    // epilogues and bloats the body).
    if (src && LoadGruntzPalette(src, (i32) "BLACKTOOL") && LoadGruntzPalette(src, (i32) "BLACKTOY")
        && LoadGruntzPalette(src, (i32) "DKBLUETOOL") && LoadGruntzPalette(src, (i32) "DKBLUETOY")
        && LoadGruntzPalette(src, (i32) "DKGREENTOOL") && LoadGruntzPalette(src, (i32) "DKGREENTOY")
        && LoadGruntzPalette(src, (i32) "TURQTOOL") && LoadGruntzPalette(src, (i32) "TURQTOY")
        && LoadGruntzPalette(src, (i32) "DKREDTOOL") && LoadGruntzPalette(src, (i32) "DKREDTOY")
        && LoadGruntzPalette(src, (i32) "PURPLETOOL") && LoadGruntzPalette(src, (i32) "PURPLETOY")
        && LoadGruntzPalette(src, (i32) "DKYELLOWTOOL")
        && LoadGruntzPalette(src, (i32) "DKYELLOWTOY") && LoadGruntzPalette(src, (i32) "GREYTOOL")
        && LoadGruntzPalette(src, (i32) "GREYTOY") && LoadGruntzPalette(src, (i32) "BLUETOOL")
        && LoadGruntzPalette(src, (i32) "BLUETOY") && LoadGruntzPalette(src, (i32) "GREENTOOL")
        && LoadGruntzPalette(src, (i32) "GREENTOY") && LoadGruntzPalette(src, (i32) "CYANTOOL")
        && LoadGruntzPalette(src, (i32) "CYANTOY") && LoadGruntzPalette(src, (i32) "REDTOOL")
        && LoadGruntzPalette(src, (i32) "REDTOY") && LoadGruntzPalette(src, (i32) "PINKTOOL")
        && LoadGruntzPalette(src, (i32) "PINKTOY") && LoadGruntzPalette(src, (i32) "YELLOWTOOL")
        && LoadGruntzPalette(src, (i32) "YELLOWTOY") && LoadGruntzPalette(src, (i32) "WHITETOOL")
        && LoadGruntzPalette(src, (i32) "WHITETOY") && LoadGruntzPalette(src, (i32) "ORANGETOOL")
        && LoadGruntzPalette(src, (i32) "ORANGETOY") && LoadGruntzPalette(src, (i32) "HOTPINKTOOL")
        && LoadGruntzPalette(src, (i32) "HOTPINKTOY")) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CSpriteRefTable::BuildToolToyColorTable (0xe2400) - build the per-color tool/toy
// sprite-ref table. Bails on a null src; if already built (m_built) returns success.
// Otherwise registers the color palettes (LoadToolToyPalettes), then Add()s each
// color's "<COLOR>TOOL"/"<COLOR>TOY" sprite into bucket A/B at the color's fixed
// kind slot (any Add miss aborts with 0), and latches m_built. __thiscall(src), ret 4.
RVA(0x000e2400, 0x39e)
i32 CSpriteRefTable::BuildToolToyColorTable(i32 src) {
    if (!src) {
        return 0;
    }
    if (m_built != 0) {
        return 1;
    }
    if (!LoadToolToyPalettes(src)) {
        return 0;
    }
    CSpriteRef* r;
    r = Add("BLACKTOOL", 7);
    if (!r) {
        return 0;
    }
    m_refA[7] = r;
    r = Add("BLACKTOY", 7);
    if (!r) {
        return 0;
    }
    m_refB[7] = r;
    r = Add("DKBLUETOOL", 8);
    if (!r) {
        return 0;
    }
    m_refA[8] = r;
    r = Add("DKBLUETOY", 8);
    if (!r) {
        return 0;
    }
    m_refB[8] = r;
    r = Add("DKGREENTOOL", 9);
    if (!r) {
        return 0;
    }
    m_refA[9] = r;
    r = Add("DKGREENTOY", 9);
    if (!r) {
        return 0;
    }
    m_refB[9] = r;
    r = Add("TURQTOOL", 0xa);
    if (!r) {
        return 0;
    }
    m_refA[0xa] = r;
    r = Add("TURQTOY", 0xa);
    if (!r) {
        return 0;
    }
    m_refB[0xa] = r;
    r = Add("DKREDTOOL", 0xb);
    if (!r) {
        return 0;
    }
    m_refA[0xb] = r;
    r = Add("DKREDTOY", 0xb);
    if (!r) {
        return 0;
    }
    m_refB[0xb] = r;
    r = Add("PURPLETOOL", 4);
    if (!r) {
        return 0;
    }
    m_refA[4] = r;
    r = Add("PURPLETOY", 4);
    if (!r) {
        return 0;
    }
    m_refB[4] = r;
    r = Add("DKYELLOWTOOL", 0xd);
    if (!r) {
        return 0;
    }
    m_refA[0xd] = r;
    r = Add("DKYELLOWTOY", 0xd);
    if (!r) {
        return 0;
    }
    m_refB[0xd] = r;
    r = Add("GREYTOOL", 0xe);
    if (!r) {
        return 0;
    }
    m_refA[0xe] = r;
    r = Add("GREYTOY", 0xe);
    if (!r) {
        return 0;
    }
    m_refB[0xe] = r;
    r = Add("BLUETOOL", 2);
    if (!r) {
        return 0;
    }
    m_refA[2] = r;
    r = Add("BLUETOY", 2);
    if (!r) {
        return 0;
    }
    m_refB[2] = r;
    r = Add("GREENTOOL", 1);
    if (!r) {
        return 0;
    }
    m_refA[1] = r;
    r = Add("GREENTOY", 1);
    if (!r) {
        return 0;
    }
    m_refB[1] = r;
    r = Add("CYANTOOL", 0xf);
    if (!r) {
        return 0;
    }
    m_refA[0xf] = r;
    r = Add("CYANTOY", 0xf);
    if (!r) {
        return 0;
    }
    m_refB[0xf] = r;
    r = Add("REDTOOL", 3);
    if (!r) {
        return 0;
    }
    m_refA[3] = r;
    r = Add("REDTOY", 3);
    if (!r) {
        return 0;
    }
    m_refB[3] = r;
    r = Add("PINKTOOL", 0xc);
    if (!r) {
        return 0;
    }
    m_refA[0xc] = r;
    r = Add("PINKTOY", 0xc);
    if (!r) {
        return 0;
    }
    m_refB[0xc] = r;
    r = Add("YELLOWTOOL", 5);
    if (!r) {
        return 0;
    }
    m_refA[5] = r;
    r = Add("YELLOWTOY", 5);
    if (!r) {
        return 0;
    }
    m_refB[5] = r;
    r = Add("WHITETOOL", 0x10);
    if (!r) {
        return 0;
    }
    m_refA[0x10] = r;
    r = Add("WHITETOY", 0x10);
    if (!r) {
        return 0;
    }
    m_refB[0x10] = r;
    r = Add("ORANGETOOL", 0);
    if (!r) {
        return 0;
    }
    m_refA[0] = r;
    r = Add("ORANGETOY", 0);
    if (!r) {
        return 0;
    }
    m_refB[0] = r;
    r = Add("HOTPINKTOOL", 6);
    if (!r) {
        return 0;
    }
    m_refA[6] = r;
    r = Add("HOTPINKTOY", 6);
    if (!r) {
        return 0;
    }
    m_refB[6] = r;
    m_built = 1;
    return 1;
}
SIZE_UNKNOWN(CLookupResult);
SIZE_UNKNOWN(CLookupSprite);

// --- vtable catalog ---
