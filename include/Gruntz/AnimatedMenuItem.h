#ifndef GRUNTZ_ANIMATED_MENU_ITEM_H
#define GRUNTZ_ANIMATED_MENU_ITEM_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/MenuItem.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>

GZ_ENUM_FORWARD(MenuItemState);

class CMenuPage;

#define SET_ANIMATED_MENU_ITEM_FRAME_PERIOD_INLINE(value) m_framePeriodMs = value

class CAnimatedMenuItem : public CMenuItem {
public:
    CAnimatedMenuItem();
    virtual ~CAnimatedMenuItem() OVERRIDE {
        Cleanup();
    }
    virtual i32 Init(
        CMenuPage* page,
        const char* name,
        const char* animationKey,
        i32 commandId,
        const char* targetPageKey,
        GZ_ENUM_PARAM(MenuItemFlags, i32) flags
    ) OVERRIDE;
    RVA(0x00184b70, 0x1a)
    virtual void Reset() OVERRIDE {
        m_framePeriodMs = 0x64;
        m_normalAnimation = NULL;
        m_selectedAnimation = NULL;
        m_disabledAnimation = NULL;
        m_frameIndex = 0;
        m_frameTimerMs = 0;
    }
    virtual i32 GetFrameHeight() OVERRIDE;
    virtual i32 GetFrameWidth() OVERRIDE;
    RVA(0x00184a60, 0x17)
    virtual void SetState(MenuItemState state) OVERRIDE {
        i32 framePeriodMs = m_framePeriodMs;
        m_state = state;
        m_frameIndex = 0;
        m_frameTimerMs = framePeriodMs;
    }
    virtual i32 Update(u32 deltaMs) OVERRIDE;
    virtual i32 DrawAt(CDDrawSurfacePair* target, i32 centerX, i32 centerY) OVERRIDE;
    RVA(0x00184a90, 0x6)
    virtual i32 UsesStateAnimations() OVERRIDE {
        return 1;
    }
    RVA(0x00184a80, 0xa)
    virtual void SetFramePeriod(i32 framePeriodMs);

    CDDrawWorker* GetStateAnimation();
    CImage* GetCurrentFrame();
    i32 AdvanceFrame();

    CDDrawWorker* m_normalAnimation;
    CDDrawWorker* m_selectedAnimation;
    CDDrawWorker* m_disabledAnimation;
    i32 m_frameIndex;
    i32 m_frameTimerMs;
    i32 m_framePeriodMs;
};

inline void CAnimatedMenuItem::SetFramePeriod(i32 framePeriodMs) {
    m_framePeriodMs = framePeriodMs;
}

inline CAnimatedMenuItem::CAnimatedMenuItem() {
    m_normalAnimation = NULL;
    m_selectedAnimation = NULL;
    m_disabledAnimation = NULL;
    m_frameIndex = 0;
    m_frameTimerMs = 0;
    SetFramePeriod(0x64);
}

#endif // GRUNTZ_ANIMATED_MENU_ITEM_H
