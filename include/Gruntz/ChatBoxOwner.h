// ChatBoxOwner.h - the on-screen chat/text-box owner (C:\Proj\Gruntz). The page
// object that positions the chat box from the active viewport, latches the source
// registry + text host, and hit-tests the cursor against the box rectangle. Its
// LoadChatBoxSprite renderer (RVA 0x20f40) still lives in src/Stub/ApiCallers.cpp
// (parked at the scheduling wall); these four are the place/clear/configure/hit-test
// helpers. Only offsets / code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_GRUNTZ_CHATBOXOWNER_H
#define GRUNTZ_GRUNTZ_CHATBOXOWNER_H

#include <Ints.h>

#include <Mfc.h> // CString

// The text-stamp host reached through m_14 (its +0x34 is a dirty/redraw flag the
// configure path raises). External - opaque view.
struct CChatBoxTextHost {
    char m_pad0[0x34];
    i32 m_34; // +0x34  dirty/redraw flag
};

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

    i32 m_0;                // +0x00  box origin X (or 0/0xa0 by mode)
    i32 m_4;                // +0x04  box origin Y (viewport-relative)
    i32 m_8;                // +0x08  mode (1/2/3)
    i32 m_c;                // +0x0c  active flag
    i32 m_10;               // +0x10  enabled flag (hit-test gate)
    CChatBoxTextHost* m_14; // +0x14  text-stamp host
    void* m_18;             // +0x18  source registry root
    CString m_1c;           // +0x1c  caption/key string
};

#endif // GRUNTZ_GRUNTZ_CHATBOXOWNER_H
