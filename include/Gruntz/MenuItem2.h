// MenuItem2.h - the animated three-state sprite menu item (C:\Proj\Gruntz).
//
// CMenuItem2 derives from CMenuItem (the 0x5c-byte leaf in MenuItem.h) and adds a
// per-state sprite trio plus a current-frame cursor. Its vtable (0x5f08f8,
// g_menuItem2Vtbl) overrides the visual slots: Init (0x185750) resolves three
// CImageSet sprites by name ("<key>_NORMAL/_SELECTED/_DISABLED") out of the same
// catalog CMenuItem::Init uses, and the Place override (0x1858d0) draws the current
// animation frame and caches its hit rect. Three non-virtual helpers walk the frame
// cursor. Recovered from the 0x185750..0x185a10 cluster.
//
// Layout (CMenuItem base is 0x5c; offsets + code bytes load-bearing):
//   +0x5c m_5c  - CImageSet* NORMAL-state sprite
//   +0x60 m_60  - CImageSet* SELECTED-state sprite
//   +0x64 m_64  - CImageSet* DISABLED-state sprite
//   +0x68 m_68  - i32 current frame index
//   +0x6c m_6c  - i32 (zeroed by Init)
//   +0x70 m_70  - i32 (Init writes 0x64)
#ifndef GRUNTZ_MENUITEM2_H
#define GRUNTZ_MENUITEM2_H

#include <Ints.h>

#include <Mfc.h>

#include <Gruntz/MenuItem.h>

// A frame record (a CImageSet frame, used as a CImage): a draw-offset pair at
// m_18/m_1c. RenderFrame (0x153790, __thiscall) blits it at a screen position. Same
// shape the rest of the engine's sprite chain uses (cf. SBI_GruntMachine CGmFrame).
struct CMenuFrame {
    char m_pad0[0x18];
    i32 m_18;                                              // +0x18  x draw offset
    i32 m_1c;                                              // +0x1c  y draw offset
    void RenderFrame(i32 surfaceCtx, i32 x, i32 y, i32 z); // 0x153790
};

// The per-state sprite (a CImageSet): a frame table at m_14 indexed by a signed
// frame index gated to [m_64, m_68]. GetAt is the bounds-checked accessor the cursor
// helpers inline (same shape as CImageSet::GetAt).
struct CMenuSprite {
    char m_pad0[0x14];
    CMenuFrame** m_14; // +0x14  frame table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  frame-index range lo
    i32 m_68; // +0x68  frame-index range hi

    CMenuFrame* GetAt(i32 idx) {
        if (idx < m_64 || idx > m_68) {
            return 0;
        }
        return m_14[idx];
    }
};

struct CMenuItem2 : CMenuItem {
    CMenuSprite* m_5c; // +0x5c  NORMAL sprite
    CMenuSprite* m_60; // +0x60  SELECTED sprite
    CMenuSprite* m_64; // +0x64  DISABLED sprite
    i32 m_68;          // +0x68  current frame index
    i32 m_6c;          // +0x6c
    i32 m_70;          // +0x70

    ~CMenuItem2();                                            // 0x1847e0 (in menuitem TU)
    i32 Init(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5); // 0x185750
    i32 Draw(i32 ctx, i32 x, i32 y);                          // 0x1858d0
    CMenuSprite* GetCurrentSprite();                          // 0x185950
    CMenuFrame* GetCurrentFrame();                            // 0x185970
    i32 NextFrame();                                          // 0x1859c0
};

#endif // GRUNTZ_MENUITEM2_H
