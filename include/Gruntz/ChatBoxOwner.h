// ChatBoxOwner.h - the on-screen chat/text-box owner (C:\Proj\Gruntz). The page
// object that positions the chat box from the active viewport, latches the source
// registry + text host, and hit-tests the cursor against the box rectangle. Only
// offsets / code bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_GRUNTZ_CHATBOXOWNER_H
#define GRUNTZ_GRUNTZ_CHATBOXOWNER_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h> // CString, HDC

struct CChatBoxRegRoot; // registry root reached via m_18 (defined in ChatBoxOwner.cpp)

// The text-stamp host reached through m_14 (its +0x34 is a dirty/redraw flag the
// configure path raises). External - opaque view. StampText (0x1cd0, __thiscall)
// is the sprite renderer's text overlay.
struct CChatBoxTextHost {
    char m_pad0[0x34];
    i32 m_34; // +0x34  dirty/redraw flag
    // StampText @0x1cd0 IS m4::PwdHost::Render22160; cast at the call.
};
SIZE_UNKNOWN(CChatBoxTextHost);

class CChatBoxOwner {
public:
    // Inline ctor, recovered from its retail inline expansion at CMulti::
    // SetupMultiplayerSession 0xb583d (`push 0x1c; call RezAlloc` then the seven
    // field stores in this exact order, m_8 = 1 through the pinned-1 register):
    // clear everything, mode = 1. No out-of-line ??0 exists (header-inline).
    CChatBoxOwner() {
        m_18 = 0;
        m_14 = 0;
        m_c = 0;
        m_10 = 0;
        m_0 = 0;
        m_4 = 0;
        m_8 = 1;
    }

    // Latch the source registry root + text host and raise the active flag.
    void Attach(void* reg, CChatBoxTextHost* host);
    // Lower the active flag.
    void Deactivate(); // 0x00020510
    // Configure the box origin from the current viewport for the given mode and
    // mark the text host dirty.
    void Configure(i32 mode);
    // Hit-test a screen point against the box rectangle for the current mode.
    i32 HitTest(i32 x, i32 y);
    // (The ex-`GetField1c` @0x20ef0 was a misattribution: 0x20ef0 is
    // CFontConfig::GetInputText, the +0x14 text host's inline CString accessor whose COMDAT
    // this obj emitted. It is defined in ChatBoxOwner.cpp - its rva-order home - but
    // declared on CFontConfig, and the invented +0x1c CString member is gone with it.)
    // The chat-box cheat-code processor ("Enable Cheatzfile" command).
    void ProcessCheatInput(i32 a, i32 b);
    // Render the chat-box sprite + stamp its text for the current mode.
    i32 LoadChatBoxSprite(i32 arg1);
    // OnMouseUp (CPlay 0x0cdb10) click-consume probe (thiscall(x,y); reloc-masked).
    i32 HitTest43e0(i32 x, i32 y); // 0x43e0

    i32 m_0;                // +0x00  box origin X (or 0/0xa0 by mode)
    i32 m_4;                // +0x04  box origin Y (viewport-relative)
    i32 m_8;                // +0x08  mode (1/2/3)
    i32 m_c;                // +0x0c  active flag
    i32 m_10;               // +0x10  enabled flag (hit-test gate)
    CChatBoxTextHost* m_14; // +0x14  text-stamp host (IS a CFontConfig - TypeChar/GetInputText/
                            //        ClearInput run on it; fold deferred, see FontConfig.h)
    CChatBoxRegRoot* m_18;  // +0x18  source registry root
};
SIZE_UNKNOWN(CChatBoxOwner);

#endif // GRUNTZ_GRUNTZ_CHATBOXOWNER_H
