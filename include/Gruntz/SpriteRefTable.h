#ifndef GRUNTZ_SPRITEREFTABLE_H
#define GRUNTZ_SPRITEREFTABLE_H

#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/SpriteTeamColorVariant.h>
#include <Ints.h>

class CGruntPalette {
public:
    CGruntPalette();

    i32 Build(CShadeTableCache* cache, CShadeTable* shade, ColorTint kind);
    void Free();
    CShadeTableCache* m_cache;
    CShadeTable* m_shadeTable;
    u16 m_teamColor1;
    u16 m_teamColor3;
    u16 m_teamColor2;
};

inline CGruntPalette::CGruntPalette() {
    m_cache = NULL;
    m_shadeTable = NULL;
}

class CShadeTableCache;

class CDDrawSurfaceMgr;

class CRezMgr;
class CGruntPaletteTable {
public:
    CGruntPaletteTable();
    ~CGruntPaletteTable();

    i32 Init(CShadeTableCache* cache, CDDrawSurfaceMgr* holder);

    void Reset();

    void Clear();

    CGruntPalette* GetTool(i32 colorId);

    void GetToolColor(i32 colorId, SpriteTeamColorVariant variant, u16& color) {
        CGruntPalette* sprite = GetTool(colorId);
        if (sprite == NULL) {
            color = 0;
            return;
        }
        switch (variant) {
            case SPRITE_TEAM_COLOR_PRIMARY:
                color = sprite->m_teamColor1;
                break;
            case SPRITE_TEAM_COLOR_SECONDARY:
                color = sprite->m_teamColor2;
                break;
            case SPRITE_TEAM_COLOR_TERTIARY:
                color = sprite->m_teamColor3;
                break;
            default:
                color = sprite->m_teamColor1;
                break;
        }
    }

    CGruntPalette* GetToy(i32 colorId);

    CShadeTable* GetShadeTable(i32 colorIndex, i32 usingToy);

    CGruntPalette* Add(char* szName, ColorTint kind);

    i32 LoadGruntzPalette(CRezMgr* src, const char* name);

    i32 LoadToolToyPalettes(CRezMgr* src);

    i32 BuildToolToyColorTable(CRezMgr* src);

    CShadeTableCache* m_shadeCache;
    CDDrawSurfaceMgr* m_spriteMgrHolder;
    CGruntPalette* m_toolPalettes[TINT_COUNT];
    CGruntPalette* m_toyPalettes[TINT_COUNT];
    b32 m_built;
};

inline CGruntPaletteTable::CGruntPaletteTable() {
    m_shadeCache = NULL;
    m_spriteMgrHolder = NULL;
    m_built = false;
    for (i32 i = 0; i < TINT_COUNT; ++i) {
        m_toolPalettes[i] = NULL;
        m_toyPalettes[i] = NULL;
    }
}

inline CGruntPaletteTable::~CGruntPaletteTable() {
    Reset();
}

#endif // GRUNTZ_SPRITEREFTABLE_H
