#include <rva.h>

#include <Gruntz/SpriteRefTable.h>

#include <DDrawMgr/DDrawPaletteRegistry.h>
#include <DDrawMgr/DDrawPaletteResource.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Rez/RezArchive.h>

#include <stdio.h>

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
    m_factory = NULL;
    m_spriteMgrHolder = NULL;
    m_built = 0;
    for (i32 i = 0; i < 0x11; i++) {
        m_toolRefs[i] = NULL;
        m_toyRefs[i] = NULL;
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
            m_toolRefs[j] = NULL;
            m_toyRefs[j] = NULL;
        }
        m_built = 0;
    }
}

RVA(0x000e2360, 0x15)
CSpriteRef* CSpriteRefTable::GetTool(i32 colorId) {
    if (static_cast<u32>(colorId) >= TINT_COUNT) {
        return NULL;
    }
    return m_toolRefs[colorId];
}

RVA(0x000e2390, 0x15)
CSpriteRef* CSpriteRefTable::GetToy(i32 colorId) {
    if (static_cast<u32>(colorId) >= TINT_COUNT) {
        return NULL;
    }
    return m_toyRefs[colorId];
}

RVA(0x000e23c0, 0x2d)
CShadeTable* CSpriteRefTable::GetSel(i32 i, i32 bAlt) {
    if (static_cast<u32>(i) >= 0x11) {
        return NULL;
    }
    CSpriteRef* node = bAlt ? m_toyRefs[i] : m_toolRefs[i];
    if (!node) {
        return NULL;
    }
    return node->m_alphaKey;
}

RVA(0x000e2400, 0x39e)
i32 CSpriteRefTable::BuildToolToyColorTable(CRezArchive* src) {
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
    r = Add("BLACKTOOL", TINT_BLACK);
    if (!r) {
        return 0;
    }
    m_toolRefs[7] = r;
    r = Add("BLACKTOY", TINT_BLACK);
    if (!r) {
        return 0;
    }
    m_toyRefs[7] = r;
    r = Add("DKBLUETOOL", TINT_DKBLUE);
    if (!r) {
        return 0;
    }
    m_toolRefs[8] = r;
    r = Add("DKBLUETOY", TINT_DKBLUE);
    if (!r) {
        return 0;
    }
    m_toyRefs[8] = r;
    r = Add("DKGREENTOOL", TINT_DKGREEN);
    if (!r) {
        return 0;
    }
    m_toolRefs[9] = r;
    r = Add("DKGREENTOY", TINT_DKGREEN);
    if (!r) {
        return 0;
    }
    m_toyRefs[9] = r;
    r = Add("TURQTOOL", TINT_TURQ);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xa] = r;
    r = Add("TURQTOY", TINT_TURQ);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xa] = r;
    r = Add("DKREDTOOL", TINT_DKRED);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xb] = r;
    r = Add("DKREDTOY", TINT_DKRED);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xb] = r;
    r = Add("PURPLETOOL", TINT_PURPLE);
    if (!r) {
        return 0;
    }
    m_toolRefs[4] = r;
    r = Add("PURPLETOY", TINT_PURPLE);
    if (!r) {
        return 0;
    }
    m_toyRefs[4] = r;
    r = Add("DKYELLOWTOOL", TINT_DKYELLOW);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xd] = r;
    r = Add("DKYELLOWTOY", TINT_DKYELLOW);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xd] = r;
    r = Add("GREYTOOL", TINT_GREY);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xe] = r;
    r = Add("GREYTOY", TINT_GREY);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xe] = r;
    r = Add("BLUETOOL", TINT_BLUE);
    if (!r) {
        return 0;
    }
    m_toolRefs[2] = r;
    r = Add("BLUETOY", TINT_BLUE);
    if (!r) {
        return 0;
    }
    m_toyRefs[2] = r;
    r = Add("GREENTOOL", TINT_GREEN);
    if (!r) {
        return 0;
    }
    m_toolRefs[1] = r;
    r = Add("GREENTOY", TINT_GREEN);
    if (!r) {
        return 0;
    }
    m_toyRefs[1] = r;
    r = Add("CYANTOOL", TINT_CYAN);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xf] = r;
    r = Add("CYANTOY", TINT_CYAN);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xf] = r;
    r = Add("REDTOOL", TINT_RED);
    if (!r) {
        return 0;
    }
    m_toolRefs[3] = r;
    r = Add("REDTOY", TINT_RED);
    if (!r) {
        return 0;
    }
    m_toyRefs[3] = r;
    r = Add("PINKTOOL", TINT_PINK);
    if (!r) {
        return 0;
    }
    m_toolRefs[0xc] = r;
    r = Add("PINKTOY", TINT_PINK);
    if (!r) {
        return 0;
    }
    m_toyRefs[0xc] = r;
    r = Add("YELLOWTOOL", TINT_YELLOW);
    if (!r) {
        return 0;
    }
    m_toolRefs[5] = r;
    r = Add("YELLOWTOY", TINT_YELLOW);
    if (!r) {
        return 0;
    }
    m_toyRefs[5] = r;
    r = Add("WHITETOOL", TINT_WHITE);
    if (!r) {
        return 0;
    }
    m_toolRefs[0x10] = r;
    r = Add("WHITETOY", TINT_WHITE);
    if (!r) {
        return 0;
    }
    m_toyRefs[0x10] = r;
    r = Add("ORANGETOOL", TINT_ORANGE);
    if (!r) {
        return 0;
    }
    m_toolRefs[0] = r;
    r = Add("ORANGETOY", TINT_ORANGE);
    if (!r) {
        return 0;
    }
    m_toyRefs[0] = r;
    r = Add("HOTPINKTOOL", TINT_HOTPINK);
    if (!r) {
        return 0;
    }
    m_toolRefs[6] = r;
    r = Add("HOTPINKTOY", TINT_HOTPINK);
    if (!r) {
        return 0;
    }
    m_toyRefs[6] = r;
    m_built = 1;
    return 1;
}

static inline CDDrawPaletteResource* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawPaletteResource*>(found);
}

RVA(0x000e2890, 0xb6)
CSpriteRef* CSpriteRefTable::Add(char* szName, ColorTint kind) {
    CDDrawPaletteResource* rec =
        LookupWorker(m_spriteMgrHolder->m_paletteRegistry->m_palettesByName, szName);
    if (!rec) {
        return NULL;
    }

    PALETTEENTRY* entries = rec->m_palette->m_cacheA;
    if (!entries) {
        return NULL;
    }
    CShadeTable* alpha = m_factory->AlphaTable(entries);
    if (!alpha) {
        return NULL;
    }
    CSpriteRef* node = new CSpriteRef;
    if (node->Build(m_factory, alpha, kind) == 0) {
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return NULL;
    }
    return node;
}

RVA(0x000e2980, 0x2cd)
i32 CSpriteRefTable::LoadToolToyPalettes(CRezArchive* src) {

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
i32 CSpriteRefTable::LoadGruntzPalette(CRezArchive* src, const char* name) {
    if (!src) {
        return 0;
    }

    if (LookupWorker(m_spriteMgrHolder->m_paletteRegistry->m_palettesByName, name)) {
        return 1;
    }

    char buf[0x40];
    sprintf(buf, "GRUNTZ_PALETTEZ_%s", name);
    CRezArchiveEntry* pal = (src)->FindEntryByPath(buf, REZ_TAG_PAL);
    if (!pal) {
        return 0;
    }
    return m_spriteMgrHolder->m_paletteRegistry->LoadPaletteFromSource(pal, NULL, 0) != NULL;
}
