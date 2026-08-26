#ifndef GRUNTZ_SPRITEREFTABLE_H
#define GRUNTZ_SPRITEREFTABLE_H

#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Ints.h>

class CSpriteRef {
public:
    CSpriteRef();

    i32 Build(CShadeTableCache* cache, CShadeTable* shade, ColorTint kind);
    void Free();
    CShadeTableCache* m_cache;
    CShadeTable* m_alphaKey;
    u16 m_teamColor1;
    u16 m_teamColor3;
    u16 m_teamColor2;
    u16 m_pad0e;
};

inline CSpriteRef::CSpriteRef() {
    m_cache = NULL;
    m_alphaKey = NULL;
}

class CShadeTableCache;

class CDDrawSurfaceMgr;

class CRezArchive;
class CSpriteRefTable {
public:
    CSpriteRefTable();
    ~CSpriteRefTable();

    i32 Init(CShadeTableCache* cache, CDDrawSurfaceMgr* holder);

    void Reset();

    void Clear();

    CSpriteRef* GetTool(i32 colorId);

    CSpriteRef* GetToy(i32 colorId);

    CShadeTable* GetSel(i32 i, i32 bAlt);

    CSpriteRef* Add(char* szName, ColorTint kind);

    i32 LoadGruntzPalette(CRezArchive* src, const char* name);

    i32 LoadToolToyPalettes(CRezArchive* src);

    i32 BuildToolToyColorTable(CRezArchive* src);

    CShadeTableCache* m_factory;
    CDDrawSurfaceMgr* m_spriteMgrHolder;
    CSpriteRef* m_toolRefs[TINT_COUNT];
    CSpriteRef* m_toyRefs[TINT_COUNT];
    b32 m_built;
};

inline CSpriteRefTable::CSpriteRefTable() {
    m_factory = NULL;
    m_spriteMgrHolder = NULL;
    m_built = false;
    for (i32 i = 0; i < TINT_COUNT; ++i) {
        m_toolRefs[i] = NULL;
        m_toyRefs[i] = NULL;
    }
}

inline CSpriteRefTable::~CSpriteRefTable() {
    Reset();
}

#endif // GRUNTZ_SPRITEREFTABLE_H
