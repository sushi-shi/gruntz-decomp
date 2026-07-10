// MenuItem2.h - the animated three-state sprite menu item (C:\Proj\Gruntz).
//
// CMenuItem2 derives from CMenuItem (the 0x5c-byte leaf in MenuItem.h) and adds a
// per-state sprite trio plus a current-frame cursor. Its vtable (0x5f08f8,
// ??_7CMenuItem2@@6B@, 15 slots) overrides the visual slots and adds one new slot:
// Init (0x185750) resolves three CImageSet sprites by name
// ("<key>_NORMAL/_SELECTED/_DISABLED"); the Place override (0x1858d0) draws the
// current animation frame and caches its hit rect; the slot-14 setter (0x1847a0,
// SetFrame) is reached by AddItem2. Three non-virtual helpers walk the frame
// cursor. Recovered from the 0x185750..0x185a10 cluster.
//
// CMenuItem2 is a REAL polymorphic derived class: MSVC emits ??_7CMenuItem2@@6B@
// (VTBL() catalogs the 0x1f08f8 datum, was vtbl-placeholders vtbl-cluster-75
// / g_menuItem2Vtbl) and auto-stamps the derived vptr after the base ctor/dtor.
// The overrides without a reconstructed body (Reset/GetWidth/Vf5/Vf6/Notify/OnInit)
// and the new SetFrame slot are declared-only -> reloc-masked external references.
//
// Layout (CMenuItem base is 0x5c; offsets + code bytes load-bearing):
//   +0x5c m_spriteNormal   - CImageSet* NORMAL-state sprite
//   +0x60 m_spriteSelected - CImageSet* SELECTED-state sprite
//   +0x64 m_spriteDisabled - CImageSet* DISABLED-state sprite
//   +0x68 m_frameIdx       - i32 current frame cursor
//   +0x6c m_6c             - i32 (zeroed by Init; role unproven)
//   +0x70 m_70             - i32 (Init writes 0x64; role unproven)
#ifndef GRUNTZ_MENUITEM2_H
#define GRUNTZ_MENUITEM2_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

#include <Gruntz/MenuItem.h>
#include <Image/CImage.h> // the canonical frame-record class (CImage::RenderFrame @0x153790)

// The per-frame record (a CImageSet frame) is the RTTI-confirmed CImage: a
// draw-offset pair at m_18/m_1c, blitted by CImage::RenderFrame (0x153790,
// __thiscall). Modeled by the shared <Image/CImage.h> definition.

// The per-state sprite (a CImageSet): a frame table at m_14 indexed by a signed
// frame index gated to [m_64, m_68]. GetAt is the bounds-checked accessor the cursor
// helpers inline (same shape as CImageSet::GetAt).
struct CMenuSprite {
    char m_pad0[0x14];
    CImage** m_frames; // +0x14  frame table
    char m_pad18[0x64 - 0x18];
    i32 m_firstFrame; // +0x64  frame-index range lo
    i32 m_lastFrame;  // +0x68  frame-index range hi

    CImage* GetAt(i32 idx) {
        if (idx < m_firstFrame || idx > m_lastFrame) {
            return 0;
        }
        return m_frames[idx];
    }
};
SIZE_UNKNOWN(CMenuSprite);

class CMenuItem2 : public CMenuItem {
public:
    CMenuItem2();                   // inlined derived ctor (base ctor + derived vptr + fields)
    virtual ~CMenuItem2() OVERRIDE; // 0x1847e0  slot 0 (scalar-deleting-dtor thunk @0x1847c0)
    virtual i32 Init(i32, i32, i32, i32, i32, i32) OVERRIDE; // 0x185750  slot 1
    virtual void Reset() OVERRIDE;                           // 0x184890  slot 3  (decl-only)
    virtual i32 GetWidth() OVERRIDE;                         // 0x185890  slot 4  (decl-only)
    virtual i32 Vf5() OVERRIDE;                              // 0x185880  slot 5  (decl-only)
    virtual void Vf6(i32) OVERRIDE;                          // 0x184780  slot 6  (decl-only)
    virtual i32 Notify(void* arg) OVERRIDE;                  // 0x1858a0  slot 8  (frame cursor)
    virtual i32 Place(i32 ctx, i32 x, i32 y) OVERRIDE;       // 0x1858d0  slot 9  (draws frame)
    virtual i32 OnInit() OVERRIDE;                           // 0x1847b0  slot 13 (decl-only)
    virtual void SetFrame(i32 v); // 0x1847a0  slot 14 (new; body in BoundaryUpper)

    // Non-virtual __thiscall frame-cursor helpers (bodies in MenuItem2.cpp):
    RVA(0x00185950, 0x1b)
    CMenuSprite* GetCurrentSprite() {
        switch (m_state) {
        case 1:
        return m_spriteNormal;
        case 2:
        return m_spriteSelected;
        case 3:
        return m_spriteDisabled;
        }
        return 0;
    }
    CImage* GetCurrentFrame();       // 0x185970
    i32 NextFrame();                 // 0x1859c0

    CMenuSprite* m_spriteNormal;   // +0x5c  NORMAL-state sprite (m_state 1)
    CMenuSprite* m_spriteSelected; // +0x60  SELECTED-state sprite (m_state 2)
    CMenuSprite* m_spriteDisabled; // +0x64  DISABLED-state sprite (m_state 3)
    i32 m_frameIdx;                // +0x68  current frame cursor
    i32 m_6c;                      // +0x6c  (zeroed by Init; role unproven)
    i32 m_70;                      // +0x70  seeded to 0x64 (role unproven)
};

// The derived ctor MSVC inlines into the page's AddItem2/AddSubItem2: the base
// CMenuItem() ctor (base vptr + CStrings + sentinels) runs first, then the implicit
// derived vptr stamp, then this body seeds the sprite/cursor fields.
inline CMenuItem2::CMenuItem2() {
    m_spriteNormal = 0;
    m_spriteSelected = 0;
    m_spriteDisabled = 0;
    m_frameIdx = 0;
    m_6c = 0;
    m_70 = 0x64;
}

#endif // GRUNTZ_MENUITEM2_H
