#ifndef GRUNTZ_DDRAWMGR_WORLDINITERROR_H
#define GRUNTZ_DDRAWMGR_WORLDINITERROR_H

#include <Enums.h>

// Failures exposed by CDDrawSurfaceMgr while constructing the game world.
// Each value is written immediately after the named operation fails. The
// 0x80e9 bank is the same five DirectDraw-device stages re-banked by
// CDDrawFrontSurface::SetGeometry for the outer manager.
// Retail's dispatch switch is UNSIGNED: CGruntzMgr::ReportWorldStatus lowers its
// binary search with `ja`, not `jg` (`sema disasm 0x00090ac0 --branches --diff`),
// so the field and the switch key are u32.
GZ_ENUM_BEGIN_SPLIT(WorldInitError, u32)
    WORLDERR_NONE = 0,
    WORLDERR_CHILD_GROUP = 0x3e9,
    WORLDERR_WORKER_LIST = 0x3ea,
    WORLDERR_IMAGE_REGISTRY = 0x3eb,
    WORLDERR_WORKER_CACHE = 0x3ec,
    WORLDERR_WORKER_MAP = 0x3ed,
    WORLDERR_ANIM_REGISTRY = 0x3ee,
    WORLDERR_LEVEL_EXTENTS = 0x3ef,
    WORLDERR_CREATE_PAGES = 0x3f0,
    WORLDERR_SOUND_OUTPUT = 0x3f1,
    WORLDERR_SOUND_REGISTRY = 0x3f2,
    WORLDERR_FRONT_SURFACE = 0x7d1,
    WORLDERR_BACK_SURFACE = 0x7d2,
    WORLDERR_OVERLAY_SURFACE = 0x7d3,
    WORLDERR_CREATE_DEVICE = 0xbb9,
    WORLDERR_CREATE_PALETTE_SURFACE = 0xbba,
    WORLDERR_FRONT_DIMENSIONS = 0xfa1,
    WORLDERR_BACK_DIMENSIONS = 0xfa2,
    WORLDERR_FRONT_SURFACE_COPY = 0xfa3,
    WORLDERR_BACK_SURFACE_CREATE = 0xfa4,
    WORLDERR_DDRAW_CREATE = 0x80e9,
    WORLDERR_DDRAW_COOPERATIVE_LEVEL = 0x80ea,
    WORLDERR_DDRAW_CAPABILITIES = 0x80eb,
    WORLDERR_DDRAW_DISPLAY_MODE = 0x80ec,
    WORLDERR_DDRAW_COLOR_MASKS = 0x80ed
GZ_ENUM_END_SPLIT(WorldInitError, u32)

// Identifies the outer operation whose world initialization failed. Used only
// when CDDrawSurfaceMgr has no more specific WorldInitError to report.
GZ_ENUM_BEGIN(WorldInitReportTag)
    WORLD_REPORT_STARTUP_INIT = 0x407,
    WORLD_REPORT_COLOR_DEPTH_REINIT = 0x43f
GZ_ENUM_END(WorldInitReportTag)

// The inner DirectDraw wrapper's error bank before SetGeometry translates it
// into WorldInitError. DDRAWERR_CAPABILITIES is retained because the outer
// translation has a retail arm for that stage, even though the current GetCaps
// reconstruction only logs its HRESULT.
GZ_ENUM_BEGIN(DDrawDeviceError)
    DDRAWERR_NONE = 0,
    DDRAWERR_CREATE = 0x3e9,
    DDRAWERR_COOPERATIVE_LEVEL = 0x3ea,
    DDRAWERR_CAPABILITIES = 0x3eb,
    DDRAWERR_DISPLAY_MODE = 0x3ec,
    DDRAWERR_COLOR_MASKS = 0x3ed,
    DDRAWERR_QUERY_INTERFACE = 0x3ef
GZ_ENUM_END(DDrawDeviceError)

#endif // GRUNTZ_DDRAWMGR_WORLDINITERROR_H
