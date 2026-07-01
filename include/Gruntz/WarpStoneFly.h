// WarpStoneFly.h - Gruntz CWarpStoneFly (C:\Proj\Gruntz), the flying-warpstone
// status-bar overlay. Allocated (new 0x40) and owned by a CSBI_RectOnly at its
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

// The CSBI_RectOnly owner reached through m_owner. Only the members the three
// frameless methods touch are modeled; every call through it goes via the ILT and
// is reloc-masked regardless of name.
SIZE_UNKNOWN(CWsfOwner);
struct CWsfOwner {
    // mode-3 tab-switch helpers, both __thiscall (ILT-reloc-masked):
    void TabReset(i32 on); // 0x100930  owner->Method(0)
    void TabApply();       // 0x104d60
    i32 m_mode;            // +0x000  mode discriminator (!=2 gates the tab switch)
    char m_pad4[0x10c - 0x4];
    i32 m_activeTabId; // +0x10c  active-tab id (==5 arms the tab switch)
    char m_pad110[0x548 - 0x110];
    i32 m_busy;           // +0x548  busy flag, cleared on arrival
    void* m_warpStoneFly; // +0x54c  this overlay (freed + nulled on arrival)
};

// The sprite (m_sprite) drawn by the overlay: a __thiscall Draw(ctx, x, y, flag) with
// callee cleanup (ret 0x10). External/no-body (reloc-masked).
SIZE_UNKNOWN(CWsfSprite);
struct CWsfSprite {
    void Draw(i32 ctx, i32 x, i32 y, i32 flag); // 0x153790
};

SIZE_UNKNOWN(CWarpStoneFly);
class CWarpStoneFly {
public:
    CWarpStoneFly();  // 0x109bb0  clears m_sprite/m_owner, returns this
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
    CWsfSprite* m_sprite;   // +0x38  the drawn sprite
    CWsfOwner* m_owner;     // +0x3c  back-pointer to the owning CSBI_RectOnly
};

#endif // GRUNTZ_GRUNTZ_WARPSTONEFLY_H
