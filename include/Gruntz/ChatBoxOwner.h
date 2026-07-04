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
    i32 m_34;                                   // +0x34  dirty/redraw flag
    void StampText(HDC dc, i32 id, void* rect); // 0x1cd0
};
SIZE_UNKNOWN(CChatBoxTextHost);

class CChatBoxOwner {
public:
    // Latch the source registry root + text host and raise the active flag.
    void Attach(void* reg, CChatBoxTextHost* host);
    // Lower the active flag.
    void Deactivate();
    // Configure the box origin from the current viewport for the given mode and
    // mark the text host dirty.
    void Configure(i32 mode);
    // Hit-test a screen point against the box rectangle for the current mode.
    i32 HitTest(i32 x, i32 y);
    // Return the box's caption/key CString (m_1c) by value (copy-ctor into sret).
    CString GetField1c();
    // The chat-box cheat-code processor ("Enable Cheatzfile" command).
    void ProcessCheatInput(i32 a, i32 b);
    // Render the chat-box sprite + stamp its text for the current mode.
    i32 LoadChatBoxSprite(i32 arg1);

    i32 m_0;                // +0x00  box origin X (or 0/0xa0 by mode)
    i32 m_4;                // +0x04  box origin Y (viewport-relative)
    i32 m_8;                // +0x08  mode (1/2/3)
    i32 m_c;                // +0x0c  active flag
    i32 m_10;               // +0x10  enabled flag (hit-test gate)
    CChatBoxTextHost* m_14; // +0x14  text-stamp host
    CChatBoxRegRoot* m_18;  // +0x18  source registry root
    CString m_1c;           // +0x1c  caption/key string
};
SIZE_UNKNOWN(CChatBoxOwner);

#endif // GRUNTZ_GRUNTZ_CHATBOXOWNER_H
