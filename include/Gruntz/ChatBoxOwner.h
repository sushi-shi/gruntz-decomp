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
        m_world = 0;
        m_fontConfig = 0;
        m_attached = 0;
        m_inputActive = 0;
        m_originX = 0;
        m_originY = 0;
        m_mode = 1;
    }

    i32 Attach(CDDrawSurfaceMgr* world, CFontConfig* host);

    void Deactivate();

    void Configure(i32 mode);

    i32 HitTest(i32 x, i32 y);

    void ProcessCheatInput(i32 a, i32 b);

    i32 LoadChatBoxSprite(CDDrawSurfacePair* target);

    i32 m_originX;
    i32 m_originY;
    i32 m_mode;
    i32 m_attached;
    i32 m_inputActive;
    CFontConfig* m_fontConfig;

    CDDrawSurfaceMgr* m_world;
};
SIZE(0x1c);

#endif // GRUNTZ_GRUNTZ_CHATBOXOWNER_H
