#ifndef GRUNTZ_MENUITEM2_H
#define GRUNTZ_MENUITEM2_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

#include <Gruntz/MenuItem.h>
#include <Image/CImage.h> // the canonical frame-record class (CImage::RenderFrame @0x153790)

#include <Image/ImageSet.h> // CDDrawWorker == CDDrawWorker (the ONE real class)

class CMenuItem2 : public CMenuItem {
public:
    CMenuItem2();                   // inlined derived ctor (base ctor + derived vptr + fields)
    virtual ~CMenuItem2() OVERRIDE; // 0x1847e0  slot 0 (scalar-deleting-dtor thunk @0x1847c0)
    virtual i32 Init(i32, i32, i32, i32, i32, i32) OVERRIDE; // 0x185750  slot 1
    virtual void Reset() OVERRIDE;                           // 0x184890  slot 3  (decl-only)
    virtual i32 GetWidth() OVERRIDE;                         // 0x185890  slot 4  (decl-only)
    virtual i32 GetFrameWidth() OVERRIDE;                    // 0x185880  slot 5  (decl-only)
    virtual void Disable(i32 mode) OVERRIDE;                 // 0x184780  slot 6  (decl-only)
    virtual i32 Notify(void* arg) OVERRIDE;                  // 0x1858a0  slot 8  (frame cursor)
    virtual i32 Place(i32 ctx, i32 x, i32 y) OVERRIDE;       // 0x1858d0  slot 9  (draws frame)
    virtual i32 OnInit() OVERRIDE;                           // 0x1847b0  slot 13 (return 1)
    virtual void SetFrame(i32 v); // 0x1847a0  slot 14 (new; body in BoundaryUpper)

    // Non-virtual __thiscall frame-cursor helpers (bodies in MenuItem2.cpp):
    CDDrawWorker* GetCurrentSprite(); // 0x185950
    CImage* GetCurrentFrame();        // 0x185970
    i32 NextFrame();                  // 0x1859c0

    CDDrawWorker* m_spriteNormal;   // +0x5c  NORMAL-state sprite (m_state 1)
    CDDrawWorker* m_spriteSelected; // +0x60  SELECTED-state sprite (m_state 2)
    CDDrawWorker* m_spriteDisabled; // +0x64  DISABLED-state sprite (m_state 3)
    i32 m_frameIdx;                 // +0x68  current frame cursor
    i32 m_6c;                       // +0x6c  (zeroed by Init; role unproven)
    i32 m_70;                       // +0x70  seeded to 0x64 (role unproven)
};
SIZE(0x74);

inline CMenuItem2::CMenuItem2() {
    m_spriteNormal = 0;
    m_spriteSelected = 0;
    m_spriteDisabled = 0;
    m_frameIdx = 0;
    m_6c = 0;
    m_70 = 0x64;
}

#endif // GRUNTZ_MENUITEM2_H
