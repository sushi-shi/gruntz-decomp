#ifndef GRUNTZ_DDRAWMGR_DDRAWPALETTERESOURCE_H
#define GRUNTZ_DDRAWMGR_DDRAWPALETTERESOURCE_H

#include <Ints.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawSurfaceMgr;

struct CDDPalette;

struct CDDrawPaletteResource : public CWapObj {
    CDDPalette* m_palette;

    CDDrawPaletteResource() {}

    CDDrawPaletteResource(i32 id, class CDDrawSurfaceMgr* owner)
        : CWapObj(owner, id, 0, CWapObj::NO_SEED) {
        m_palette = NULL;
    }

    virtual ~CDDrawPaletteResource()   {
        Unload();
    }

    virtual i32 IsLoaded()   {
        return m_palette != NULL;
    }

    virtual void Unload()  ;

    virtual LoadableClassId GetClassId()   {
        return CLASSID_PALETTE_RESOURCE;
    }

    virtual i32 CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flag);
    virtual i32 CreatePaletteFromRgb(u8* data, i32 flag);
    virtual i32 LoadPaletteFromFile(char* path, i32 flag);
    virtual i32 CreatePaletteFromTrailingData(void* data, i32 size, i32 flag);
    virtual i32 ApplyToFrontSurface();
};

#endif
