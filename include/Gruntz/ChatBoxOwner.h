#ifndef GRUNTZ_GRUNTZ_CHATBOXOWNER_H
#define GRUNTZ_GRUNTZ_CHATBOXOWNER_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_BEGIN(ChatBoxLayout)
    CHATBOX_WITH_RIGHT_STATUSBAR = 1,
    CHATBOX_WITH_LEFT_STATUSBAR = 2,
    CHATBOX_WITH_HIDDEN_STATUSBAR = 3
GZ_ENUM_END(ChatBoxLayout)

class CDDrawSurfaceMgr;
class CDDrawSurfacePair;

class CFontConfig;

class CChatBoxOwner {
public:
    CChatBoxOwner() {
        m_world = NULL;
        m_fontConfig = NULL;
        m_attached = false;
        m_inputActive = false;
        m_originX = 0;
        m_originY = 0;
        m_mode = CHATBOX_WITH_RIGHT_STATUSBAR;
    }

    i32 Attach(CDDrawSurfaceMgr* world, CFontConfig* host);

    void Deactivate();

    void Configure(ChatBoxLayout mode);

    i32 HitTest(i32 x, i32 y);

    void HandleTextInputKey(i32 charCode, i32 keyData);

    i32 LoadChatBoxSprite(CDDrawSurfacePair* target);

    i32 m_originX;
    i32 m_originY;
    ChatBoxLayout m_mode;
    b32 m_attached;
    b32 m_inputActive;
    CFontConfig* m_fontConfig;

    CDDrawSurfaceMgr* m_world;
};

#endif // GRUNTZ_GRUNTZ_CHATBOXOWNER_H
