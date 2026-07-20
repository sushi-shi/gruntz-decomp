#include <Bute/SymParser.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Win32.h>

#include <Gruntz/SpriteRefTable.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>     // m_spriteMgrHolder's canonical CDDrawSurfaceMgr
#include <DDrawMgr/DDrawWorkerMapSmall.h> // its +0x18 m_workerMap (the sprite/palette registry)

#include <rva.h>

#include <stdio.h> // engine sprintf (reloc-masked) - LoadGruntzPalette's name format

void* ::operator new(u32); // matches ??2@YAPAXI@Z

// The object Lookup writes into `out`: +0x10 is the sprite, whose +0xc holds the
// frame data fed to the alpha factory. The reduced reader views CLookupSprite /
// CLookupResult live in <Gruntz/SpriteRefTable.h> (@identity-TODO - the sprite value's
// concrete class needs an xref chase from the sprite LOADER side; it is NOT
// CAniRecordBase2, whose +0x10 m_10 is a plain i32 buffer, not a sprite ptr).

RVA(0x000e2250, 0x26)
i32 CSpriteRefTable::Init(i32 p0, i32 p1) {
    if (!p0) {
        return p0;
    }
    m_factory = reinterpret_cast<CShadeTableCache*>(p0);
    m_spriteMgrHolder = reinterpret_cast<CDDrawSurfaceMgr*>(p1);
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

RVA(0x000e2360, 0x15)
CSpriteRef* CSpriteRefTable::GetA(i32 i) {
    if (static_cast<u32>(i) >= 0x11) {
        return 0;
    }
    return m_refA[i];
}

RVA(0x000e2390, 0x15)
CSpriteRef* CSpriteRefTable::GetB(i32 i) {
    if (static_cast<u32>(i) >= 0x11) {
        return 0;
    }
    return m_refB[i];
}

RVA(0x000e23c0, 0x2d)
i32 CSpriteRefTable::GetSel(i32 i, i32 bAlt) {
    if (static_cast<u32>(i) >= 0x11) {
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
    void* sprite = (reinterpret_cast<CLookupResult*>(out))->m_sprite->m_frameData;
    if (!sprite) {
        return 0;
    }
    void* alpha = m_factory->AlphaTable(static_cast<unsigned char*>(sprite));
    if (!alpha) {
        return 0;
    }
    CSpriteRef* node;
    CSpriteRef* tmp = static_cast<CSpriteRef*>(::operator new(0x10));
    if (tmp) {
        tmp->m_cache = 0;
        tmp->m_alphaKey = 0;
        node = tmp;
    } else {
        node = 0;
    }
    if (node->Build(reinterpret_cast<i32>(m_factory), alpha, kind) == 0) {
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    return node;
}

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
    m_spriteMgrHolder->m_workerMap->m_map1.Lookup(reinterpret_cast<char*>(name), found);
    if (found) {
        return 1;
    }

    char buf[0x40];
    sprintf(buf, "GRUNTZ_PALETTEZ_%s", reinterpret_cast<char*>(name));
    CParseSource* pal = (reinterpret_cast<CSymParser*>(src))->ResolveQualified(buf, reinterpret_cast<void*>(0x50414c));
    if (!pal) {
        return 0;
    }
    return m_spriteMgrHolder->m_workerMap->Factory_1658c0(pal, 0, 0) != 0;
}

RVA(0x000e2980, 0x2cd)
i32 CSpriteRefTable::LoadToolToyPalettes(i32 src) {
    // One short-circuit && chain so MSVC shares a single return-0 tail (each rung
    // `test;je fail`), matching retail's layout (an if/return-0 per rung inlines 35
    // epilogues and bloats the body).
    if (src && LoadGruntzPalette(src, reinterpret_cast<i32>("BLACKTOOL")) && LoadGruntzPalette(src, reinterpret_cast<i32>("BLACKTOY"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("DKBLUETOOL")) && LoadGruntzPalette(src, reinterpret_cast<i32>("DKBLUETOY"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("DKGREENTOOL")) && LoadGruntzPalette(src, reinterpret_cast<i32>("DKGREENTOY"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("TURQTOOL")) && LoadGruntzPalette(src, reinterpret_cast<i32>("TURQTOY"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("DKREDTOOL")) && LoadGruntzPalette(src, reinterpret_cast<i32>("DKREDTOY"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("PURPLETOOL")) && LoadGruntzPalette(src, reinterpret_cast<i32>("PURPLETOY"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("DKYELLOWTOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("DKYELLOWTOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("GREYTOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("GREYTOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("BLUETOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("BLUETOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("GREENTOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("GREENTOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("CYANTOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("CYANTOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("REDTOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("REDTOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("PINKTOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("PINKTOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("YELLOWTOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("YELLOWTOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("WHITETOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("WHITETOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("ORANGETOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("ORANGETOY")) && LoadGruntzPalette(src, reinterpret_cast<i32>("HOTPINKTOOL"))
        && LoadGruntzPalette(src, reinterpret_cast<i32>("HOTPINKTOY"))) {
        return 1;
    }
    return 0;
}

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
