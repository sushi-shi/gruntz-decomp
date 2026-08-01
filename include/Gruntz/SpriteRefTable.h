#ifndef GRUNTZ_SPRITEREFTABLE_H
#define GRUNTZ_SPRITEREFTABLE_H

#include <Ints.h>
#include <DDrawMgr/ShadeTableCache.h>
#include <rva.h>

class CSpriteRef {
public:
    i32 Build(CShadeTableCache* cache, void* shade, i32 kind);
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

    CSpriteRef* GetA(i32 i);

    CSpriteRef* GetB(i32 i);

    CShadeTable* GetSel(i32 i, i32 bAlt);

    CSpriteRef* Add(char* szName, i32 kind);

    i32 LoadGruntzPalette(CSymParser* src, const char* name);

    i32 LoadToolToyPalettes(CSymParser* src);

    i32 BuildToolToyColorTable(CSymParser* src);

    CShadeTableCache* m_factory;
    CDDrawSurfaceMgr* m_spriteMgrHolder;
    CSpriteRef* m_refA[0x11];
    CSpriteRef* m_refB[0x11];
    i32 m_built;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_SPRITEREFTABLE_H
