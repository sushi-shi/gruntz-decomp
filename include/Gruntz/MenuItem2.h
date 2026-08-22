#ifndef GRUNTZ_MENUITEM2_H
#define GRUNTZ_MENUITEM2_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/MenuItem.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>

GZ_ENUM_FORWARD(MenuItemState);

class CMenuPage;

#define SET_MENU_ITEM2_FRAME_DELAY_INLINE(value) m_frameDelay = value

class CMenuItem2 : public CMenuItem {
public:
    CMenuItem2();
    // 0x1847e0 (RVA_COMPGEN pin at the keeper, MenuPage.cpp - an RVA() here
    // would annotate BOTH cl dtor variants and collide with ??_GCMenuItem2@0x1847c0).
    virtual ~CMenuItem2() OVERRIDE {
        Cleanup();
    }
    virtual i32 Init(CMenuPage*, const char*, const char*, i32, const char*, i32) OVERRIDE;
    RVA(0x00184890, 0x1a)
    virtual void Reset() OVERRIDE {
        m_frameDelay = 0x64;
        m_spriteNormal = NULL;
        m_spriteSelected = NULL;
        m_spriteDisabled = NULL;
        m_frameIdx = 0;
        m_frameCountdown = 0;
    }
    virtual i32 GetWidth() OVERRIDE;
    virtual i32 GetFrameWidth() OVERRIDE;
    RVA(0x00184780, 0x17)
    virtual void Disable(MenuItemState mode) OVERRIDE {
        i32 frameLimit = m_frameDelay;
        m_state = mode;
        m_frameIdx = 0;
        m_frameCountdown = frameLimit;
    }
    virtual i32 Notify(u32 dt) OVERRIDE;
    virtual i32 Place(CDDrawSurfacePair* target, i32 x, i32 y) OVERRIDE;
    RVA(0x001847b0, 0x6)
    virtual i32 OnInit() OVERRIDE {
        return 1;
    }
    RVA(0x001847a0, 0xa)
    virtual void SetFrame(i32 v);

    CDDrawWorker* GetCurrentSprite();
    CImage* GetCurrentFrame();
    i32 NextFrame();

    CDDrawWorker* m_spriteNormal;
    CDDrawWorker* m_spriteSelected;
    CDDrawWorker* m_spriteDisabled;
    i32 m_frameIdx;
    i32 m_frameCountdown;
    i32 m_frameDelay;
};

// The constructor calls this header-visible body; the vtable also retains the
// standalone COMDAT emitted from MenuItem.cpp.
inline void CMenuItem2::SetFrame(i32 v) {
    m_frameDelay = v;
}

inline CMenuItem2::CMenuItem2() {
    m_spriteNormal = NULL;
    m_spriteSelected = NULL;
    m_spriteDisabled = NULL;
    m_frameIdx = 0;
    m_frameCountdown = 0;
    SetFrame(0x64);
}

#endif // GRUNTZ_MENUITEM2_H
