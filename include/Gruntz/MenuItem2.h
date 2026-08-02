#ifndef GRUNTZ_MENUITEM2_H
#define GRUNTZ_MENUITEM2_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/MenuItem.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>

class CMenuPage;
class CMenuItem2 : public CMenuItem {
public:
    CMenuItem2();
    virtual ~CMenuItem2() OVERRIDE;
    virtual i32 Init(CMenuPage*, const char*, const char*, i32, const char*, i32) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 GetWidth() OVERRIDE;
    virtual i32 GetFrameWidth() OVERRIDE;
    virtual void Disable(i32 mode) OVERRIDE;
    virtual i32 Notify(u32 dt) OVERRIDE;
    virtual i32 Place(CDDrawSurfacePair* target, i32 x, i32 y) OVERRIDE;
    virtual i32 OnInit() OVERRIDE;
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
SIZE(0x74);

inline CMenuItem2::CMenuItem2() {
    m_spriteNormal = 0;
    m_spriteSelected = 0;
    m_spriteDisabled = 0;
    m_frameIdx = 0;
    m_frameCountdown = 0;
    m_frameDelay = 0x64;
}

#endif // GRUNTZ_MENUITEM2_H
