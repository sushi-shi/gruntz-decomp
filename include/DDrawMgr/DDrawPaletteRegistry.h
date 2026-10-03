#ifndef GRUNTZ_DDRAWMGR_DDRAWPALETTEREGISTRY_H
#define GRUNTZ_DDRAWMGR_DDRAWPALETTEREGISTRY_H

#include <map>
#include <string>

#include <Ints.h>

#include <DDrawMgr/DDrawPaletteResource.h>
#include <Gruntz/MapStringToOb.h>
#include <Ints.h>
#include <Rez/RezArchiveEntry.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawPaletteRegistry : public CWapObj {
public:
    CDDrawPaletteRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0) {
        m_activePalette = NULL;
    }
    virtual i32 IsLoaded()  ;
    virtual i32 IsReady()  ;
    virtual void Unload()  ;

    virtual LoadableClassId GetClassId()  ;
    virtual CDDrawPaletteResource* LoadPaletteFromSource(CRezItm* src, const char* key, i32 flags);

    virtual CDDrawPaletteResource* CreatePaletteFromRgb(u8* data, const char* key, i32 flags);
    virtual CDDrawPaletteResource* LoadPaletteFromFile(char* path, const char* key, i32 flags);

    virtual CDDrawPaletteResource* LoadPaletteFromTrailingData(CRezItm* src, i32 key, i32 flags);
    virtual ~CDDrawPaletteRegistry()  ;

    std::map<std::string, CDDrawPaletteResource*> m_palettesByName;

    std::map<std::string, CObject*> m_reservedMap2;
    std::map<std::string, CObject*> m_reservedMap3;

    CDDrawPaletteResource* m_activePalette;

    void ClearPalettes();
    i32 RemovePalette(CObject* obj);
    i32 RemovePaletteByName(const char* key);
};

#endif
