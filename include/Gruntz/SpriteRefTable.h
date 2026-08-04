#ifndef GRUNTZ_SPRITEREFTABLE_H
#define GRUNTZ_SPRITEREFTABLE_H

#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Ints.h>

GZ_ENUM_FORWARD_SPLIT(ColorTint, u8);

class CSpriteRef {
public:
    i32 Build(CShadeTableCache* cache, void* shade, ColorTint kind);
    void Free();
    CShadeTableCache* m_cache;
    CShadeTable* m_alphaKey;
    u16 m_teamColor1;
    u16 m_teamColor3;
    u16 m_teamColor2;
    u16 m_pad0e;
};
SIZE_UNKNOWN();

class CShadeTableCache;

class CDDrawSurfaceMgr;

class CSymParser;
class CSpriteRefTable {
public:
    i32 Init(CShadeTableCache* cache, CDDrawSurfaceMgr* holder);

    void Reset();

    void Clear();

    CSpriteRef* GetTool(i32 colorId);

    CSpriteRef* GetToy(i32 colorId);

    CShadeTable* GetSel(i32 i, i32 bAlt);

    CSpriteRef* Add(char* szName, ColorTint kind);

    i32 LoadGruntzPalette(CSymParser* src, const char* name);

    i32 LoadToolToyPalettes(CSymParser* src);

    i32 BuildToolToyColorTable(CSymParser* src);

    CShadeTableCache* m_factory;
    CDDrawSurfaceMgr* m_spriteMgrHolder;
    CSpriteRef* m_toolRefs[TINT_COUNT];
    CSpriteRef* m_toyRefs[TINT_COUNT];
    i32 m_built;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_SPRITEREFTABLE_H
