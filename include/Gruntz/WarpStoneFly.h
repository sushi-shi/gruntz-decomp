// WarpStoneFly.h - Gruntz CWarpStoneFly (C:\Proj\Gruntz), the flying-warpstone
// status-bar overlay. Allocated (new 0x40) and owned by a CStatusBarMgr at its
// +0x54c (the back-pointer m_owner points to that owner); the owner's
// UpdateWarpStoneStatusBar/m_warpStoneFly teardown drive it. No vtable
// (m_arrivalMode is a mode int, not a vptr), so no RTTI for this leaf class.
//
// The overlay interpolates a sprite (m_sprite) from a current float position
// (m_currentX/m_currentY) toward an integer target (m_targetX/m_targetY) along a
// per-frame velocity (m_velocityScale/m_xDirection/m_yDirection), snapping to the
// target on overshoot. On arrival it pokes the owner's mode byte into the registry
// tab array and frees itself off the owner.
//
// Fields are placeholders; the offsets + code bytes are the load-bearing fact, the
// mangled (?<method>@CWarpStoneFly@@...) name is layout-independent.
#ifndef GRUNTZ_GRUNTZ_WARPSTONEFLY_H
#define GRUNTZ_GRUNTZ_WARPSTONEFLY_H

#include <Ints.h>
#include <rva.h>

// The owner reached through m_owner IS the real <Gruntz/StatusBarMgr.h>
// CStatusBarMgr (the 0x630 host); its m_position @+0, m_10 @+0x10, m_rect14.m_0
// @+0x14, m_activeTab @+0x10c, m_hlBusy @+0x548 and m_retabNotify (this overlay)
// @+0x54c are exactly the fields the three frameless methods touch (the ex empty
// CWsfOwner shell modeled them by offset). Only a pointer member is needed here.
class CStatusBarMgr;

// The sprite (m_sprite) drawn by the overlay IS a CImage: Draw is CImage::RenderFrame
// (0x153790, ctx/x/y/flag, ret 0x10) - see CWarpStoneFly::Draw. (The former empty
// CWsfSprite placeholder view is dissolved.)
class CImage; // <Image/CImage.h> - the drawn sprite frame

SIZE_UNKNOWN(CWarpStoneFly);
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

// The flying-warpstone overlay's registry views (ex WarpStoneFly.cpp): m_cmdGrid+0x260
// is a CByteArray (the registry tab-state array; SetAtGrow @0x1b5485 == the real MFC
// CByteArray::SetAtGrow, cast at the call); m_world->m_drawable->m_context is the
// draw surface context.
struct CWsfTabArray {
    char m_pad0[0x8];
    i32 m_index; // +0x08  array index
};
struct CWsfDrawable {
    char m_pad0[0x14];
    i32 m_context; // +0x14  surface context
};
struct CWsfGameMgr {
    char m_pad0[0x4];
    CWsfDrawable* m_drawable; // +0x04  active drawable
};
SIZE_UNKNOWN(CWsfTabArray);
SIZE_UNKNOWN(CWsfDrawable);
SIZE_UNKNOWN(CWsfGameMgr);

#endif // GRUNTZ_GRUNTZ_WARPSTONEFLY_H
