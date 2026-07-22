#ifndef GRUNTZ_GRUNTZ_WARPSTONEFLY_H
#define GRUNTZ_GRUNTZ_WARPSTONEFLY_H

#include <Ints.h>
#include <rva.h>

class CStatusBarMgr;

class CImage; // <Image/CImage.h> - the drawn sprite frame

class CWarpStoneFly {
public:
    CWarpStoneFly(); // 0x109bb0  clears m_sprite/m_owner, returns this
    // 0x109bd0 (body in SBI_RectOnly.cpp, its own RVA band): record the owner, resolve
    // the warp-tab fly frame + per-phase screen target, seed the fly velocity. Ex the
    // EngineLabelBacklog::UpdateWarpStoneStatusBar placeholder def (was CSbiMode54c::Init).
    i32 Init(void* owner, i32 phase, i32 srcX, i32 srcY);
    i32 Tick(i32 dt); // 0x10a0f0  integrate toward target, snap, notify on arrival
    i32 Draw();       // 0x10a2f0  blit the sprite at the rounded position

    // ----- layout (placeholders; offsets are the load-bearing fact) -----
    i32 m_arrivalMode; // +0x00  mode value poked into the registry tab array on arrival
    i32 m_targetX;     // +0x04  target x (integer)
    i32 m_targetY;     // +0x08  target y (integer)
    char m_padc[0x10 - 0xc];
    double m_currentX;      // +0x10  current x (float)
    double m_currentY;      // +0x18  current y (float)
    double m_velocityScale; // +0x20  velocity scale
    double m_xDirection;    // +0x28  x direction/sign gate
    double m_yDirection;    // +0x30  y direction/sign gate
    CImage* m_sprite;       // +0x38  the drawn sprite frame (CImage::RenderFrame)
    CStatusBarMgr* m_owner; // +0x3c  back-pointer to the owning CStatusBarMgr
};
SIZE(0x40);
SIZE_UNKNOWN();

struct CWsfDrawable {
    char m_pad0[0x14];
    i32 m_context; // +0x14  surface context
};
SIZE_UNKNOWN();
struct CWsfGameMgr {
    char m_pad0[0x4];
    CWsfDrawable* m_drawable; // +0x04  active drawable
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_WARPSTONEFLY_H
