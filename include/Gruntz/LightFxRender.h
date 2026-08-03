#ifndef GRUNTZ_GRUNTZ_CLIGHTFXRENDER_H
#define GRUNTZ_GRUNTZ_CLIGHTFXRENDER_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>

class CGruntzMgr;
class CTriggerMgr;
class CGruntzMapMgr;
class CDDrawSurfaceMgr;
class CDDSurface;
class CDDrawSurfacePair;

// How many tile ids the minimap colour table covers. m_buf is indexed by
// m_tileGrid->m_rows[y][x].m_tileId, and both readers guard with
// `idx >= MINIMAP_TILE_COLOR_COUNT` before indexing - so the array bound and the
// guard are the same fact written twice.
GZ_ENUM_CONST_BEGIN(MinimapTileColor)
    MINIMAP_TILE_COLOR_COUNT = 0x1f4
GZ_ENUM_CONST_END(MinimapTileColor)

class CLightFxRender {
public:
    i32 Init(CGruntzMgr* mgr, i32 refreshInterval);

    void Reset();

    void FreeSurface();

    i32 AllocSurface();

    i32 Resize(i32 delta, i32 rebuild);

    i32 ComputeRect(CDDrawSurfacePair* ctx, RECT* src);

    void DrawBorderRaw(RECT* r, void* base, i32 color);

    void DrawBorder(RECT* r, CDDrawSurfacePair* ctx, i32 color);

    i32 BuildShape(i32 shape);

    i32 BuildRockyRoadzPalette();
    i32 BuildGruntziclezPalette();
    i32 BuildTropiczPalette();
    i32 BuildHighOnSweetzPalette();
    i32 BuildHighRollerzPalette();
    i32 BuildHoneyPalette();
    i32 BuildMiniatureMasterzPalette();
    i32 BuildSpacePalette();

    void FillSpan(u32 x1, u32 x2, u16 color);

    i32 BeginMinimapPan(i32 unusedFlags, i32 x, i32 y);

    i32 EndMinimapPan(i32 unusedFlags, i32 unusedX, i32 unusedY);

    i32 IssueMinimapCommand(i32 unusedFlags, i32 x, i32 y);

    i32 ContinueMinimapPan(i32 unusedFlags, i32 x, i32 y);

    i32 ClampRect(i32 x, i32 y, i32* out, i32 margin);

    CGruntzMgr* m_mgr;
    CTriggerMgr* m_cmdGrid;
    CGruntzMapMgr* m_tileGrid;
    CDDrawSurfaceMgr* m_world;
    CDDSurface* m_surface;
    char m_pad14[0x10];
    RECT m_srcRect;
    RECT m_dstRect;
    i32 m_scale;
    i32 m_handle;
    u16 m_buf[MINIMAP_TILE_COLOR_COUNT];
    i32 m_refreshInterval;
    i32 m_refreshRemaining;
};
SIZE(0x43c);

#endif
