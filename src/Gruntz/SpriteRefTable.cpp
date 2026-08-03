#include <rva.h>

#include <Gruntz/SpriteRefTable.h>

#include <Bute/SymParser.h>
#include <DDrawMgr/AniRecordBase2.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Enums.h>

#include <stdio.h>

void* ::operator new(u32);

RVA(0x000e2250, 0x26)
i32 CSpriteRefTable::Init(CShadeTableCache* cache, CDDrawSurfaceMgr* holder) {
    if (!cache) {
        return 0;
    }
    m_factory = cache;
    m_spriteMgrHolder = holder;
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
        m_toolRefs[i] = 0;
        m_toyRefs[i] = 0;
    }
}

RVA(0x000e22d0, 0x6e)
void CSpriteRefTable::Clear() {
    if (m_factory) {
        for (i32 i = 0; i < 0x11; i++) {
            CSpriteRef* a = GetTool(i);
            if (a) {
                a->Free();
                ::operator delete(a);
            }
            CSpriteRef* b = GetToy(i);
            if (b) {
                b->Free();
                ::operator delete(b);
            }
        }
        for (i32 j = 0; j < 0x11; j++) {
            m_toolRefs[j] = 0;
            m_toyRefs[j] = 0;
        }
        m_built = 0;
    }
}

RVA(0x000e2360, 0x15)
CSpriteRef* CSpriteRefTable::GetTool(i32 colorId) {
    if (static_cast<u32>(colorId) >= 0x11) {
        return 0;
    }
    return m_toolRefs[colorId];
}

RVA(0x000e2390, 0x15)
CSpriteRef* CSpriteRefTable::GetToy(i32 colorId) {
    if (static_cast<u32>(colorId) >= 0x11) {
        return 0;
    }
    return m_toyRefs[colorId];
}

RVA(0x000e23c0, 0x2d)
CShadeTable* CSpriteRefTable::GetSel(i32 i, i32 bAlt) {
    if (static_cast<u32>(i) >= 0x11) {
        return 0;
    }
    CSpriteRef* node = bAlt ? m_toyRefs[i] : m_toolRefs[i];
    if (!node) {
        return 0;
    }
    return node->m_alphaKey;
}

// @early-stop
RVA(0x000e2400, 0x39e)
i32 CSpriteRefTable::BuildToolToyColorTable(CSymParser* src) {
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
    m_toolRefs[7] = r;
    r = Add("BLACKTOY", 7);
    if (!r) {
        return 0;
    }
    m_toyRefs[7] = r;
    r = Add("DKBLUETOOL", 8);
    if (!r) {
        return 0;
    }
    m_toolRefs[8] = r;
    r = Add("DKBLUETOY", 8);
    if (!r) {
        return 0;
    }
    m_toyRefs[8] = r;
    r = Add("DKGREENTOOL", 9);
    if (!r) {
        return 0;
    }
    m_toolRefs[9] = r;
    r = Add("DKGREENTOY", 9);
    if (!r) {
        return 0;
    }
    m_toyRefs[9] = r;
    r = Add("TURQTOOL", 0xa);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xa] = r;
    r = Add("TURQTOY", 0xa);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xa] = r;
    r = Add("DKREDTOOL", 0xb);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xb] = r;
    r = Add("DKREDTOY", 0xb);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xb] = r;
    r = Add("PURPLETOOL", 4);
    if (!r) {
        return 0;
    }
    m_toolRefs[4] = r;
    r = Add("PURPLETOY", 4);
    if (!r) {
        return 0;
    }
    m_toyRefs[4] = r;
    r = Add("DKYELLOWTOOL", 0xd);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xd] = r;
    r = Add("DKYELLOWTOY", 0xd);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xd] = r;
    r = Add("GREYTOOL", 0xe);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xe] = r;
    r = Add("GREYTOY", 0xe);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xe] = r;
    r = Add("BLUETOOL", 2);
    if (!r) {
        return 0;
    }
    m_toolRefs[2] = r;
    r = Add("BLUETOY", 2);
    if (!r) {
        return 0;
    }
    m_toyRefs[2] = r;
    r = Add("GREENTOOL", 1);
    if (!r) {
        return 0;
    }
    m_toolRefs[1] = r;
    r = Add("GREENTOY", 1);
    if (!r) {
        return 0;
    }
    m_toyRefs[1] = r;
    r = Add("CYANTOOL", 0xf);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xf] = r;
    r = Add("CYANTOY", 0xf);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xf] = r;
    r = Add("REDTOOL", 3);
    if (!r) {
        return 0;
    }
    m_toolRefs[3] = r;
    r = Add("REDTOY", 3);
    if (!r) {
        return 0;
    }
    m_toyRefs[3] = r;
    r = Add("PINKTOOL", 0xc);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xc] = r;
    r = Add("PINKTOY", 0xc);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xc] = r;
    r = Add("YELLOWTOOL", 5);
    if (!r) {
        return 0;
    }
    m_toolRefs[5] = r;
    r = Add("YELLOWTOY", 5);
    if (!r) {
        return 0;
    }
    m_toyRefs[5] = r;
    r = Add("WHITETOOL", 0x10);
    if (!r) {
        return 0;
    }
    m_toolRefs[0x10] = r;
    r = Add("WHITETOY", 0x10);
    if (!r) {
        return 0;
    }
    m_toyRefs[0x10] = r;
    r = Add("ORANGETOOL", 0);
    if (!r) {
        return 0;
    }
    m_toolRefs[0] = r;
    r = Add("ORANGETOY", 0);
    if (!r) {
        return 0;
    }
    m_toyRefs[0] = r;
    r = Add("HOTPINKTOOL", 6);
    if (!r) {
        return 0;
    }
    m_toolRefs[6] = r;
    r = Add("HOTPINKTOY", 6);
    if (!r) {
        return 0;
    }
    m_toyRefs[6] = r;
    m_built = 1;
    return 1;
}
RVA(0x000e2890, 0xb6)
CSpriteRef* CSpriteRefTable::Add(char* szName, i32 kind) {
    CObject* out = 0;
    m_spriteMgrHolder->m_workerMap->m_map1.Lookup(szName, out);
    if (!out) {
        return 0;
    }

    PALETTEENTRY* entries = (static_cast<CAniRecordBase2*>(out))->m_buf->m_cacheA;
    if (!entries) {
        return 0;
    }
    void* alpha = m_factory->AlphaTable(entries);
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
    if (node->Build(m_factory, alpha, kind) == 0) {
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    return node;
}

// @early-stop
RVA(0x000e2980, 0x2cd)
i32 CSpriteRefTable::LoadToolToyPalettes(CSymParser* src) {

    if (src && LoadGruntzPalette(src, "BLACKTOOL") && LoadGruntzPalette(src, "BLACKTOY")
        && LoadGruntzPalette(src, "DKBLUETOOL") && LoadGruntzPalette(src, "DKBLUETOY")
        && LoadGruntzPalette(src, "DKGREENTOOL") && LoadGruntzPalette(src, "DKGREENTOY")
        && LoadGruntzPalette(src, "TURQTOOL") && LoadGruntzPalette(src, "TURQTOY")
        && LoadGruntzPalette(src, "DKREDTOOL") && LoadGruntzPalette(src, "DKREDTOY")
        && LoadGruntzPalette(src, "PURPLETOOL") && LoadGruntzPalette(src, "PURPLETOY")
        && LoadGruntzPalette(src, "DKYELLOWTOOL") && LoadGruntzPalette(src, "DKYELLOWTOY")
        && LoadGruntzPalette(src, "GREYTOOL") && LoadGruntzPalette(src, "GREYTOY")
        && LoadGruntzPalette(src, "BLUETOOL") && LoadGruntzPalette(src, "BLUETOY")
        && LoadGruntzPalette(src, "GREENTOOL") && LoadGruntzPalette(src, "GREENTOY")
        && LoadGruntzPalette(src, "CYANTOOL") && LoadGruntzPalette(src, "CYANTOY")
        && LoadGruntzPalette(src, "REDTOOL") && LoadGruntzPalette(src, "REDTOY")
        && LoadGruntzPalette(src, "PINKTOOL") && LoadGruntzPalette(src, "PINKTOY")
        && LoadGruntzPalette(src, "YELLOWTOOL") && LoadGruntzPalette(src, "YELLOWTOY")
        && LoadGruntzPalette(src, "WHITETOOL") && LoadGruntzPalette(src, "WHITETOY")
        && LoadGruntzPalette(src, "ORANGETOOL") && LoadGruntzPalette(src, "ORANGETOY")
        && LoadGruntzPalette(src, "HOTPINKTOOL") && LoadGruntzPalette(src, "HOTPINKTOY")) {
        return 1;
    }
    return 0;
}

RVA(0x000e2d10, 0xa1)
i32 CSpriteRefTable::LoadGruntzPalette(CSymParser* src, const char* name) {
    if (!src) {
        return 0;
    }

    CObject* found = 0;
    m_spriteMgrHolder->m_workerMap->m_map1.Lookup(name, found);
    if (found) {
        return 1;
    }

    char buf[0x40];
    sprintf(buf, "GRUNTZ_PALETTEZ_%s", name);
    CParseSource* pal = (src)->ResolveQualified(buf, static_cast<RezTypeTag>(0x50414c));
    if (!pal) {
        return 0;
    }
    return m_spriteMgrHolder->m_workerMap->LoadPaletteFromSource(pal, 0, 0) != 0;
}
