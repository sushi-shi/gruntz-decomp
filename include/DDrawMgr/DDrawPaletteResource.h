#ifndef GRUNTZ_DDRAWMGR_DDRAWPALETTERESOURCE_H
#define GRUNTZ_DDRAWMGR_DDRAWPALETTERESOURCE_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawSurfaceMgr;

struct CDDPalette; // The class key is ABI-significant in MSVC mangling.

struct CDDrawPaletteResource : public CWapObj {
    CDDPalette* m_palette;

    CDDrawPaletteResource() {}

    CDDrawPaletteResource(i32 id, class CDDrawSurfaceMgr* owner)
        : CWapObj(owner, id, 0, CWapObj::NO_SEED) {
        m_palette = NULL;
    }

    // 0x165dd0 (RVA_COMPGEN pin at the keeper, DDrawSurfacePair.cpp - an RVA()
    // here would annotate BOTH cl dtor variants and collide with
    // ??_GCDDrawPaletteResource@0x165db0).
    virtual ~CDDrawPaletteResource() OVERRIDE {
        Unload();
    }
    RVA(0x00165d90, 0xb)
    virtual i32 IsLoaded() OVERRIDE {
        return m_palette != NULL;
    }

    virtual void Unload() OVERRIDE;
    RVA(0x00165da0, 0x6)
    virtual LoadableClassId GetClassId() OVERRIDE {
        return CLASSID_PALETTE_RESOURCE;
    }

    virtual i32 CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flag);
    virtual i32 CreatePaletteFromRgb(u8* data, i32 flag);
    virtual i32 LoadPaletteFromFile(char* path, i32 flag);
    virtual i32 CreatePaletteFromTrailingData(void* data, i32 size, i32 flag);
    virtual i32 ApplyToFrontSurface();
};

#endif // GRUNTZ_DDRAWMGR_DDRAWPALETTERESOURCE_H
