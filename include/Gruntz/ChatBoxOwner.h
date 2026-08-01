#ifndef GRUNTZ_GRUNTZ_CHATBOXOWNER_H
#define GRUNTZ_GRUNTZ_CHATBOXOWNER_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

class CDDrawSurfaceMgr;
class CDDrawSurfacePair;

class CFontConfig;

class CChatBoxOwner {
public:
    CChatBoxOwner() {
        m_18 = 0;
        m_14 = 0;
        m_c = 0;
        m_10 = 0;
        m_0 = 0;
        m_4 = 0;
        m_8 = 1;
    }

    i32 Attach(CDDrawSurfaceMgr* world, CFontConfig* host);

    void Deactivate();

    void Configure(i32 mode);

    i32 HitTest(i32 x, i32 y);

    void ProcessCheatInput(i32 a, i32 b);

    i32 LoadChatBoxSprite(CDDrawSurfacePair* target);

    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 m_10;
    CFontConfig* m_14;

    CDDrawSurfaceMgr* m_18;
};
SIZE(0x1c);

#endif // GRUNTZ_GRUNTZ_CHATBOXOWNER_H
