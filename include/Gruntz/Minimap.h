#ifndef GRUNTZ_GRUNTZ_MINIMAP_H
#define GRUNTZ_GRUNTZ_MINIMAP_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/LevelArea.h>
#include <Ints.h>

class CGruntzMgr;
class CTriggerMgr;
class CGruntzMapMgr;
class CDDrawSurfaceMgr;
class CDDSurface;
class CDDrawSurfacePair;

GZ_ENUM_CONST_BEGIN(MinimapTileColor)
    MINIMAP_TILE_COLOR_COUNT = 0x1f4
GZ_ENUM_CONST_END(MinimapTileColor)

GZ_ENUM_CONST_BEGIN(MinimapLayout)
    MINIMAP_COMBAT_BLINK_PHASE_MS = 50,
    MINIMAP_MAX_CELL_SCALE = 3,
    MINIMAP_SNAP_MARGIN_PX = 32,
    MINIMAP_BORDER_COLOR_16 = 0xffff
GZ_ENUM_CONST_END(MinimapLayout)

class CMinimap {
public:
    CMinimap();

    i32 Init(CGruntzMgr* gameMgr, i32 refreshIntervalMs);

    void Reset();

    void FreeSurface();

    i32 AllocSurface();

    i32 Refresh(i32 elapsedMs, b32 forceRefresh);

    i32 Draw(CDDrawSurfacePair* target, RECT* bounds);

    void DrawBorderRaw(RECT* rect, char* pixels, i32 color);

    void DrawBorder(RECT* rect, CDDrawSurfacePair* target, i32 color);

    i32 SetAreaPalette(LevelArea area);

    i32 BuildRockyRoadzPalette();
    i32 BuildGruntziclezPalette();
    i32 BuildTropiczPalette();
    i32 BuildHighOnSweetzPalette();
    i32 BuildHighRollerzPalette();
    i32 BuildHoneyPalette();
    i32 BuildMiniatureMasterzPalette();
    i32 BuildSpacePalette();

    void FillSpan(u32 x1, u32 x2, u16 color);

    i32 BeginMinimapPan(i32 unusedFlags, i32 cursorX, i32 cursorY);

    i32 EndMinimapPan(i32 unusedFlags, i32 unusedX, i32 unusedY);

    i32 IgnoreMinimapEvent(i32 unusedFlags, i32 unusedX, i32 unusedY);

    i32 IssueMinimapCommand(i32 unusedFlags, i32 cursorX, i32 cursorY);

    i32 ContinueMinimapPan(i32 unusedFlags, i32 cursorX, i32 cursorY);

    i32 ScreenPointToCell(i32 cursorX, i32 cursorY, i32* outCell, i32 snapMargin);

    CGruntzMgr* m_gameMgr;
    CTriggerMgr* m_triggerMgr;
    CGruntzMapMgr* m_mapMgr;
    CDDrawSurfaceMgr* m_world;
    CDDSurface* m_surface;
    char m_pad14[0x10];
    RECT m_boundsRect;
    RECT m_drawRect;
    i32 m_cellScale;
    b32 m_panActive;
    u16 m_tileColors[MINIMAP_TILE_COLOR_COUNT];
    i32 m_refreshInterval;
    i32 m_refreshRemaining;
};

inline CMinimap::CMinimap() {
    m_gameMgr = NULL;
    m_triggerMgr = NULL;
    m_mapMgr = NULL;
    m_world = NULL;
    m_surface = NULL;
    m_panActive = false;
    m_refreshInterval = 0;
    m_refreshRemaining = 0;
}

#endif
